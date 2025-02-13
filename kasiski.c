#include "primes.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* a factor must occur at least in this much of the trigrams occurrences */
#define THRESHOLD 0.5

#define ALPHASIZE 26
#define DEFSIZE 1024
#define HASHSIZE (ALPHASIZE * ALPHASIZE * ALPHASIZE)
#define TRIGRAPHSIZE 3
#define CTOI(c) (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' : (c) - 'a')

struct Factor {
  struct Factor *next;
  uint16_t val; /* the value of the prime factor */
  size_t nrep;  /* number of repetitions the factor happens */
  size_t nall;  /* number of times the factor happens */
};

struct Repeat {
  struct Repeat *next;
  size_t pos;
};

struct Trigram {
  /*
   * We maintain both a list of trigrams (so we can traverse
   * each trigram at a time) and a hashtable of trigrams (so
   * we can access a trigram at constant time).
   *
   * .next points to the next trigram in the list.
   *
   * .pos is a list of relative positions between occurrences.
   *
   * .trig is the trigram itself.  Although it is an array of
   * chars, it is not a nul-terminating string, neither each char
   * is encoded in ASCII.  It is an triplet of values from 0 to 25
   * (0 for 'A', 1 for 'B', etc).  Except that the third value
   * (.trig[2]) can be `26`, in which case this is a digram
   * rather than a trigram.
   *
   * .lastpos is the position of the last occurrence of the
   * trigraph.
   */

  struct Trigram *next;
  struct Repeat *rep;
  char trig[TRIGRAPHSIZE];
  size_t lastpos;
};

static double freqen[ALPHASIZE] = {
    [CTOI('A')] = 0.08200, [CTOI('B')] = 0.01500, [CTOI('C')] = 0.02800,
    [CTOI('D')] = 0.04300, [CTOI('E')] = 0.13000, [CTOI('F')] = 0.02200,
    [CTOI('G')] = 0.02000, [CTOI('H')] = 0.06100, [CTOI('I')] = 0.07000,
    [CTOI('J')] = 0.00150, [CTOI('K')] = 0.00770, [CTOI('L')] = 0.04000,
    [CTOI('M')] = 0.02400, [CTOI('N')] = 0.06700, [CTOI('O')] = 0.07500,
    [CTOI('P')] = 0.01900, [CTOI('Q')] = 0.00095, [CTOI('R')] = 0.06000,
    [CTOI('S')] = 0.06300, [CTOI('T')] = 0.09100, [CTOI('U')] = 0.02800,
    [CTOI('V')] = 0.00980, [CTOI('W')] = 0.02400, [CTOI('X')] = 0.00150,
    [CTOI('Y')] = 0.02000, [CTOI('Z')] = 0.00074,
};

static double freqpt[ALPHASIZE] = {
    [CTOI('A')] = 0.1463, [CTOI('B')] = 0.0104, [CTOI('C')] = 0.0388,
    [CTOI('D')] = 0.0499, [CTOI('E')] = 0.1257, [CTOI('F')] = 0.0102,
    [CTOI('G')] = 0.0130, [CTOI('H')] = 0.0128, [CTOI('I')] = 0.0618,
    [CTOI('J')] = 0.0040, [CTOI('K')] = 0.0002, [CTOI('L')] = 0.0278,
    [CTOI('M')] = 0.0474, [CTOI('N')] = 0.0505, [CTOI('O')] = 0.1073,
    [CTOI('P')] = 0.0252, [CTOI('Q')] = 0.0120, [CTOI('R')] = 0.0653,
    [CTOI('S')] = 0.0781, [CTOI('T')] = 0.0434, [CTOI('U')] = 0.0463,
    [CTOI('V')] = 0.0167, [CTOI('W')] = 0.0001, [CTOI('X')] = 0.0021,
    [CTOI('Y')] = 0.0001, [CTOI('Z')] = 0.0047,
};

static void usage(void) {
  (void)fprintf(stderr, "usage: kasiski [-e|-p]\n");
  exit(EXIT_FAILURE);
}

static void err(const char *str) {
  (void)fprintf(stderr, "kasiski: %s\n", str);
  exit(EXIT_FAILURE);
}

static void *emalloc(size_t size) {
  void *p;

  if ((p = malloc(size)) == NULL)
    err("alloc error");
  return p;
}

static void *erealloc(void *ptr, size_t size) {
  void *p;

  if ((p = realloc(ptr, size)) == NULL)
    err("alloc error");
  return p;
}

static int isalpha(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static double *parseargs(int argc, char *argv[]) {
  double *freq;
  int ch;

  freq = freqen;
  while ((ch = getopt(argc, argv, "ep")) != -1) {
    switch (ch) {
    case 'e':
      freq = freqen;
      break;
    case 'p':
      freq = freqpt;
      break;
    default:
      usage();
      break;
    }
  }
  argc -= optind;
  argv += optind;
  if (argc != 0)
    usage();
  return freq;
}

static char *readstdin(void) {
  /*
   * Read standard input; put into *ret the size of the read input.
   */

  size_t i, j, buflen, msglen, msgsize;
  char *msg;
  char buf[DEFSIZE];

  msgsize = msglen = 0;
  msg = NULL;
  while (fgets(buf, sizeof(buf), stdin) != NULL) {
    for (i = j = 0; buf[i] != '\0'; i++)
      if (isalpha(buf[i]))
        buf[j++] = buf[i];
    buf[j] = '\0';
    buflen = strlen(buf);
    if (msglen + buflen >= msgsize) {
      msgsize += DEFSIZE;
      msg = erealloc(msg, msgsize);
    }
    memcpy(msg + msglen, buf, buflen + 1);
    msglen += buflen;
  }
  if (msg == NULL)
    err("could not read from standard input\n");
  return msg;
}

static unsigned int trigtohash(char *cp) {
  return CTOI(cp[0]) * ALPHASIZE * ALPHASIZE + CTOI(cp[1]) * ALPHASIZE +
         CTOI(cp[2]);
}

static void inccount(struct Trigram **tab, struct Trigram **last, size_t *nreps,
                     char *trig, size_t n) {
  struct Repeat *rep;
  unsigned int h; /* index in the hash table */

  /*
   * Increment count of occurrences for the digram or trigram
   * hashed as h.
   */
  h = trigtohash(trig);
  if (tab[h] == NULL) {
    tab[h] = emalloc(sizeof(*tab[h]));
    *tab[h] = (struct Trigram){
        .next = *last,
        .trig[0] = trig[0],
        .trig[1] = trig[1],
        .trig[2] = trig[2],
        .rep = NULL,
    };
    *last = tab[h];
  } else {
    n -= tab[h]->lastpos;
    rep = emalloc(sizeof(*rep));
    *rep = (struct Repeat){
        .next = tab[h]->rep,
        .pos = n,
    };
    tab[h]->rep = rep;
    ++(*nreps);
  }
  tab[h]->lastpos = n;
}

static void factorise(struct Factor *factors[], struct Factor **head,
                      size_t val) {
  size_t i;

  if (val == 0 || val == 1)
    return;
  for (i = 0; val > 1; i++) {
    /* Look for the smallest factor. */
    do {
      if (val % (long)primes[i] == 0)
        break;
    } while (++i < NPRIMES);

    /* Watch for primes larger than the table. */
    if (i >= NPRIMES)
      err("damn, we got a big number!");

    if (factors[i] == NULL) {
      factors[i] = emalloc(sizeof(*factors[i]));
      *factors[i] = (struct Factor){
          .next = *head,
          .val = primes[i],
          .nrep = 0,
          .nall = 0,
      };
      *head = factors[i];
    }
    ++factors[i]->nrep;

    /* Divide factor out until none are left. */
    do {
      ++factors[i]->nall;
      val /= (long)primes[i];
    } while ((val % (long)primes[i]) == 0);
  }
}

static double sqrsum(double *freq) {
  double sqrsum = 0;
  int i;

  for (i = 0; i < ALPHASIZE; i++)
    sqrsum += freq[i] * freq[i];
  return sqrsum;
}

static void freq(char *msg, size_t msgbeg, size_t msglen, double freq[],
                 size_t sum) {
  size_t occ[ALPHASIZE] = {0};
  size_t len = 0;
  size_t i;

  for (i = msgbeg; i < msglen; i += sum) {
    occ[CTOI(msg[i])]++;
    len++;
  }
  for (i = 0; i < ALPHASIZE; i++) {
    freq[i] = occ[i] / (double)len;
  }
}

static struct Trigram *calctrigs(char *msg, size_t *nreps) {
  struct Trigram *tab[HASHSIZE] = {
      NULL}; /* hash table of trigrams to access in constant time */
  struct Trigram *last = NULL; /* list of trigrams to traverse in linear time */
  size_t i, j;
  char trig[TRIGRAPHSIZE];

  /*
   * I hope you have lots of memory, because our hash table is BIG!
   * (for me to not have to handle colisions :)
   */
  *nreps = 0;
  for (i = 0; msg[i] != '\0'; i++) {
    for (j = 0; msg[i + j] != '\0' && j < TRIGRAPHSIZE; j++)
      trig[j] = msg[i + j];
    if (j == TRIGRAPHSIZE)
      inccount(tab, &last, nreps, trig, i);
  }
  return last;
}

static struct Factor *calcfactors(struct Trigram *trigs) {
  struct Trigram *trigp, *tmptrigp;
  struct Repeat *rep, *tmprep;
  struct Factor *factors[NPRIMES] = {NULL};
  struct Factor *head = NULL;

  tmptrigp = trigp = trigs;
  while (trigp != NULL) {
    trigp = trigp->next;
    tmprep = rep = tmptrigp->rep;
    while (rep != NULL) {
      factorise(factors, &head, rep->pos);
      rep = rep->next;
      free(tmprep);
      tmprep = rep;
    }
    free(tmptrigp);
    tmptrigp = trigp;
  }
  return head;
}

static size_t calckeylen(struct Factor *head, size_t nreps) {
  struct Factor *f;
  uint32_t keylen = 1;
  size_t k;

  keylen = 1;
  f = head;
  while (head != NULL) {
    head = head->next;
    if (f->nrep / (double)nreps > THRESHOLD) {
      k = f->nall / f->nrep;
      while (k-- > 0) {
        keylen *= f->val;
      }
    }
    free(f);
    f = head;
  }
  return keylen;
}

static void guesskey(char *msg, size_t keylen, double langfreq[]) {
  double cipherfreq[ALPHASIZE];
  double msgsum;  /* sum of squares of letter frequency in messages */
  double shifsum; /* sum of product of frequencies for given shift */
  double diff;    /* difference of msgsum and shifsum for current shift */
  double mindiff; /* minor difference for all shifts */
  size_t i;
  size_t msglen;
  int j, k, shift;

  msgsum = sqrsum(langfreq);
  msglen = strlen(msg);
  for (i = 0; i < keylen; i++) {
    freq(msg, i, msglen, cipherfreq, keylen);
    shift = 0;
    mindiff = 1.0;
    for (j = 0; j < ALPHASIZE; j++) {
      shifsum = 0;
      for (k = 0; k < ALPHASIZE; k++)
        shifsum += langfreq[k] * cipherfreq[(k + j) % ALPHASIZE];
      diff = fabs(msgsum - shifsum);
      if (diff < mindiff) {
        mindiff = diff;
        shift = j;
      }
    }
    printf("%c", shift + 'a');
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  struct Trigram *trigs;  /* list of trigraphs and their repetitions */
  struct Factor *factors; /* list of all found prime factors */
  double *lfreq;          /* array of letter frequency in given language */
  size_t keylen;          /* length of the secret key */
  size_t nreps;           /* number of trigraph repetitions */
  char *text;             /* ciphertext */

  lfreq = parseargs(argc, argv);   /* read threshold and letter frequency */
  text = readstdin();              /* read message from standard input */
  trigs = calctrigs(text, &nreps); /* compute trigrams and their repetitions  */
  factors =
      calcfactors(trigs); /* compute prime factors and their occurrences */
  keylen = calckeylen(factors, nreps); /* guess length of key */
  guesskey(text, keylen, lfreq);       /* guess key */
  free(text);
  return EXIT_SUCCESS;
}
