#ifndef NOBU_NFC_H
#define NOBU_NFC_H

#include <stdint.h>

// Response codes for APDU commands.
#define SUCCESS_RESPONSE_CODE   0x9000
#define CONTINUE_RESPONSE_CODE  0x61FF
#define FAILURE_RESPONSE_CODE   0x0000

// --------------------------------------------------------------------
// Function prototype for reading QRIS CPM data using the multi-step APDU process.
int read_qris_cpm_data_using_picc(char *data, int *data_len);

#endif // NOBU_NFC_H
