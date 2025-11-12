#ifndef NUMBER_HELPER_H

void numberToWords(int num, char *result);

// Convert integer number to Bahasa Indonesia words (e.g., 12500 -> "dua belas ribu lima ratus")
void numberToWordsID(long long num, char *out);

// Parse amount string (may contain separators/decimals), convert to Indonesian words (integer part only)
// Example: "123456.00" -> "seratus dua puluh tiga ribu empat ratus lima puluh enam"
void amountToWordsID(const char *amountStr, char *out);

#endif // NUMBER_HELPER_H