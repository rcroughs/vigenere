/*******************************************************************************
 * Project: Vigenere Cipher Encoder
 * Version: 1.0
 *
 * Author: Romain Croughs
 * Email: romain.croughs@ulb.be
 * Created: February 13, 2025
 *
 * Description:
 * This program encodes a message using the Vigenere cipher. The user provides
 * the message and the key as input, and the program encodes the message using
 * the key. The program supports two modes of operation:
 *    1. Command line arguments
 *    2. Interactive prompts
 *
 * Usage:
 * 1. Command line arguments:
 *    ./encode "message" "key"
 *    Example: ./encode "Hello, world!" "key"
 *    Output: "Rijvs, yvznh!"
 *
 *    Note: The message and key must be enclosed in double quotes.
 *    Note: The message and key must be less than 1000 characters.
 *    Note: The key must contain only alphabetic characters.
 *    Note: The program preserves case (upper/lowercase).
 *    Note: The program maintains spacing and punctuation.
 *    Note: The program handles command line arguments and stdin.
 *    Note: The program outputs the encoded message to stdout.
 *
 *
 * Build:
 *     gcc -Wall -Wextra -o encode encode.c
 *
 * Dependencies:
 *     - Standard C libraries: stdio.h, stdlib.h, string.h, ctype.h
 *
 *
 * License:
 *     This software is released under the MIT License.
 *     Copyright (c) 2025 Romain Croughs
 ******************************************************************************/

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 1000

typedef struct {
  char text[MAX_LENGTH];
  char key[MAX_LENGTH];
} DecoderConfig;

void encodeVigenere(char *text, char *key) {
  int textLength = strlen(text);
  int keyLength = strlen(key);
  int i = 0;
  int j = 0;

  for (i = 0; i < keyLength; i++) {
    if (!isalpha(key[i])) {
      fprintf(stderr, "Error: Key must contain only alphabetic characters.\n");
      exit(EXIT_FAILURE);
    }
    key[i] = toupper(key[i]);
  }

  for (i = 0; i < textLength; i++) {
    if (!isalpha(text[i])) {
      continue;
    }
    char base = isupper(text[i]) ? 'A' : 'a';
    int shift = key[j % keyLength] - 'A';
    text[i] = (text[i] - base + shift) % 26 + base;
    j++;
  }
}

void removeNewline(char *text) {
  size_t len = strlen(text);
  if (len > 0 && text[len - 1] == '\n') {
    text[len - 1] = '\0';
  }
}

DecoderConfig parseArguments(int argc, char *argv[]) {
  DecoderConfig config = {};

  switch (argc) {
  case 1:
    printf("Enter the text to decode: ");
    fgets(config.text, MAX_LENGTH, stdin);
  case 2:
    printf("Enter the key: ");
    fgets(config.key, MAX_LENGTH, stdin);
    if (argc == 2)
      strncpy(config.text, argv[1], MAX_LENGTH - 1);
    break;
  default:
    strncpy(config.text, argv[1], MAX_LENGTH - 1);
    strncpy(config.key, argv[2], MAX_LENGTH - 1);
  }

  removeNewline(config.text);
  removeNewline(config.key);
  return config;
}

int main(int argc, char *argv[]) {
  DecoderConfig config = parseArguments(argc, argv);

  encodeVigenere(config.text, config.key);
  printf("%s\n", config.text);

  return 0;
}
