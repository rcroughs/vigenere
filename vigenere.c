#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ALPHASIZE       26

static void
usage(void)
{
	(void)fprintf(stderr, "usage: vegenere [-d|-e] key\n");
	exit(EXIT_FAILURE);
}

static int
alphaxor(int ch, int add, int *res)
{
	int base;

	/*
	 * Perform modular addition (XOR) of `ch` + `add` modulo ALPHASIZE.
	 * `ch` is interpreted as an ASCII letter (A=41, B=42, ...).
	 * `add` is interpreted as a letter position (A=0, B=1, ...).
	 *
	 * If `ch` is a letter:
	 * - Set `*res` to the ASCII representation of the resulting character.
	 * - Return the position in the alphabet (A=0, B=1, ...) of the result.
	 *
	 * If `ch` is not a letter:
	 * - Set `*res` to `ch`.
	 * - Return -1.
	 */
	if (res != NULL)
		*res = ch;
	if (ch >= 'A' && ch <= 'Z')
		ch -= base = 'A';
	else if (ch >= 'a' && ch <= 'z')
		ch -= base = 'a';
	else
		return -1;
	ch += add;
	if ((ch %= ALPHASIZE) < 0)
		ch += ALPHASIZE;
	if (res != NULL)
		*res = base + ch;
	return ch;
}

static void
encrypt(char *key, int prod)
{
	int keylen, ch, i;

	/*
	 * Replace `key` from an array of ASCII letters to an array of
	 * letter positions in the alphabet (A=0, B=1, ...); non-letters
	 * are ignored.
	 */
	for (keylen = i = 0; key[i] != '\0'; i++)
		if ((ch = alphaxor(key[i], 0, NULL)) != -1)
			key[keylen++] = ch;

	/*
	 * Consider the empty key as the trivial key "A".
	 */
	if (keylen == 0)
		key[keylen++] = 0;

	/*
	 * Perform alphabetic xor between each read character (in ASCII
	 * form) and the i-th element of key (in position form).  Rotate
	 * `i` whenever a xor occurr.
	 */
	i = 0;
	while ((ch = getchar()) != EOF) {
		if (alphaxor(ch, prod * key[i], &ch) != -1)
			i = (i + 1) % keylen;
		putchar(ch);
	}
}

int
main(int argc, char *argv[])
{
	int ch, n;

	n = 1;
	while ((ch = getopt(argc, argv, "de")) != -1) {
		switch (ch) {
		case 'd':
			n = -1;
			break;
		case 'e':
			n = 1;
			break;
		default:
			usage();
			return 1;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();
	encrypt(*argv, n);
	if (ferror(stdin) || ferror(stdout)) {
		(void)fprintf(stderr, "vegenere: could not perform I/O\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
