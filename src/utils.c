#include "../inc/utils.h"
#include <string.h>

#define MAX_TOKENS 100
#define MAX_TOKEN_LEN 1024

static char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
static int token_count = 0;

int split(const char* str, const char* delim) {
    token_count = 0;
    const char *start = str;
    const char *pos;

    size_t delim_len = strlen(delim);
    if (delim_len == 0) return 0;  // avoid infinite loop

    while ((pos = strstr(start, delim)) != NULL) {
        size_t len = pos - start;
        if (len >= MAX_TOKEN_LEN) len = MAX_TOKEN_LEN - 1;  // truncate
        if (token_count >= MAX_TOKENS) break;

        // Copy token
        strncpy(tokens[token_count], start, len);
        tokens[token_count][len] = '\0';

        token_count++;
        start = pos + delim_len;
    }

    // Last token
    if (token_count < MAX_TOKENS) {
        strncpy(tokens[token_count], start, MAX_TOKEN_LEN - 1);
        tokens[token_count][MAX_TOKEN_LEN - 1] = '\0';
        token_count++;
    }

    return token_count;
}

const char* get_token(int index) {
    if (index < 0 || index >= token_count) return NULL;
    return tokens[index];
}