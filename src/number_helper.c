#include <stdio.h>
#include <string.h>
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
