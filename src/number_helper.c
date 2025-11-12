#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../inc/number_helper.h"

const char *units[] = {"", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
const char *teens[] = {"ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
const char *tens[] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
const char *thousands[] = {"", "thousand", "million", "billion"}; // Extend as needed

void convertChunk(int n, char *out)
{
  if (n >= 100)
  {
    strcat(out, units[n / 100]);
    strcat(out, " hundred ");
    n %= 100;
  }
  if (n >= 20)
  {
    strcat(out, tens[n / 10]);
    strcat(out, " ");
    n %= 10;
  }
  else if (n >= 10)
  {
    strcat(out, teens[n - 10]);
    strcat(out, " ");
    n = 0; // Already handled
  }
  if (n > 0)
  {
    strcat(out, units[n]);
    strcat(out, " ");
  }
}

void numberToWords(int num, char *result)
{
  if (num == 0)
  {
    strcpy(result, "zero");
    return;
  }
  if (num < 0)
  {
    strcat(result, "minus ");
    num = -num; // Handle negative numbers
  }

  int chunk[4]; // To handle chunks of thousands, millions, etc.
  int chunkIndex = 0;

  while (num > 0)
  {
    chunk[chunkIndex++] = num % 1000;
    num /= 1000;
  }

  for (int i = chunkIndex - 1; i >= 0; i--)
  {
    if (chunk[i] > 0)
    {
      convertChunk(chunk[i], result);
      if (i > 0)
      { // Add thousands, millions, etc. (if applicable)
        strcat(result, thousands[i]);
        strcat(result, " ");
      }
    }
  }

  // Remove trailing space (if any)
  int len = strlen(result);
  if (len > 0 && result[len - 1] == ' ')
  {
    result[len - 1] = '\0';
  }
}

// ========== Indonesian number to words ==========
static void trim_space(char *s) {
  int len = (int)strlen(s);
  while (len > 0 && s[len - 1] == ' ') { s[--len] = '\0'; }
}

static void id_append(char *out, const char *word) {
  if (word == NULL || *word == '\0') return;
  if (*out) strcat(out, " ");
  strcat(out, word);
}

static void threeDigitsToIDWords(int n, char *out) {
  const char *satuan[] = {"", "satu", "dua", "tiga", "empat", "lima", "enam", "tujuh", "delapan", "sembilan"};

  if (n == 0) return;

  int ratus = n / 100;
  int sisa = n % 100;

  if (ratus > 0) {
    if (ratus == 1) id_append(out, "seratus");
    else { id_append(out, satuan[ratus]); id_append(out, "ratus"); }
  }

  if (sisa >= 20) {
    int puluh = sisa / 10;
    int satu = sisa % 10;
    id_append(out, satuan[puluh]);
    id_append(out, "puluh");
    if (satu > 0) id_append(out, satuan[satu]);
  } else if (sisa >= 12) {

    int satu = sisa % 10;
    id_append(out, satuan[satu]);
    id_append(out, "belas");
  } else if (sisa == 11) {
    id_append(out, "sebelas");
  } else if (sisa == 10) {
    id_append(out, "sepuluh");
  } else if (sisa > 0) {
    // 1..9
    id_append(out, satuan[sisa]);
  }
}

void numberToWordsID(long long num, char *out) {
  out[0] = '\0';
  if (num == 0) { strcpy(out, "nol"); return; }
  if (num < 0) { id_append(out, "minus"); num = -num; }

  const char *units[] = {"", "ribu", "juta", "miliar", "triliun"};
  long long part[5] = {0};
  int idx = 0;
  while (num > 0 && idx < 5) {
    part[idx++] = num % 1000;
    num /= 1000;
  }

  for (int i = idx - 1; i >= 0; --i) {
    if (part[i] == 0) continue;
    char chunk[128] = {0};
    threeDigitsToIDWords((int)part[i], chunk);

    if (i == 1 && part[i] == 1) {
      id_append(out, "seribu");
    } else {
      if (chunk[0]) id_append(out, chunk);
      if (units[i][0]) id_append(out, units[i]);
    }
  }
}

void amountToWordsID(const char *amountStr, char *out) {
  char digits[32] = {0};
  int j = 0;
  for (int i = 0; amountStr && amountStr[i] != '\0' && j < (int)sizeof(digits) - 1; ++i) {
    if (isdigit((unsigned char)amountStr[i])) digits[j++] = amountStr[i];
    else if (amountStr[i] == '.' || amountStr[i] == ',') break;
  }
  if (j == 0) { strcpy(out, ""); return; }

  long long val = 0;
  for (int i = 0; i < j; ++i) { val = val * 10 + (digits[i] - '0'); }
  numberToWordsID(val, out);
  trim_space(out);
}
