#include "../inc/def.h"
#include <coredef.h>
#include <string.h>
#include <struct.h>
#include <poslib.h>
#include "../inc/nobu_nfc.h"

// --------------------------------------------------------------------
// Helper function to send an APDU command using PiccIsoCommand_Api.
// It fills in the APDU_RESP structure and returns a 16-bit response code (SWA:SWB).
static uint16_t send_apdu_command(APDU_SEND *cmd, APDU_RESP *resp) {
    // Log the APDU command details
    MAINLOG_L1("Header: %02X %02X %02X %02X", cmd->Command[0], cmd->Command[1], cmd->Command[2], cmd->Command[3]);
    MAINLOG_L1("Lc: %d", cmd->Lc);

    // Log DataIn in a single MAINLOG_L1 call
    if (cmd->Lc > 0) {
        char dataInStr[cmd->Lc * 3 + 1];  // Each byte needs 3 chars: "XX " + null terminator
        char *ptr = dataInStr;
        for (int i = 0; i < cmd->Lc; i++) {
            ptr += sprintf(ptr, "%02X ", cmd->DataIn[i]);
        }
        MAINLOG_L1("DataIn: %s", dataInStr);
    }

    MAINLOG_L1("Le: %d", cmd->Le);
    MAINLOG_L1("EnableCancel: %d", cmd->EnableCancel);

    // Clear the response struct
    memset(resp, 0, sizeof(APDU_RESP));

    // Send APDU command
    PiccIsoCommand_Api(cmd, resp);

    // Log the response
    MAINLOG_L1("ReadCardDataOk = %d", resp->ReadCardDataOk);
    MAINLOG_L1("SWA: %02X SWB: %02X", resp->SWA, resp->SWB);

    return ((uint16_t)resp->SWA << 8) | resp->SWB;
}


// --------------------------------------------------------------------
// Function: read_qris_cpm_data_using_picc
// Performs the multi-step APDU sequence to retrieve QRIS CPM data.
// 1st Command: SELECT AID (00A4040007A0000006022020)
// 2nd Command: READ RECORD (Part 1) (00B2010007A0000006022020)
// 3rd Command: READ RECORD (Part 2) (00B2020007A0000006022020) if needed.
int read_qris_cpm_data_using_picc(char *data, int *data_len) {
    APDU_SEND apdu;
    APDU_RESP resp;
    uint16_t respCode;
    unsigned char combinedData[512];
    int combinedLength = 0;
    // AID for QRIS CPM: A0000006022020 (7 bytes)
    unsigned char aid_data[7] = { 0xA0, 0x00, 0x00, 0x06, 0x02, 0x20, 0x20 };

    // ----------------------------------------------------------------
    // 1st Command: SELECT AID
    // Header: 00 A4 04 00, Lc: 07, DataIn: aid_data, Le: 0 (no expected data)
    // ----------------------------------------------------------------
    memset(&apdu, 0, sizeof(APDU_SEND));
    apdu.Command[0] = 0x00;  // CLA
    apdu.Command[1] = 0xA4;  // INS: SELECT
    apdu.Command[2] = 0x04;  // P1: Select by name
    apdu.Command[3] = 0x00;  // P2: First occurrence
    apdu.Lc = 7;
    memcpy(apdu.DataIn, aid_data, 7);
    apdu.Le = 0;           // Only need status
    apdu.EnableCancel = 1; // Cancel not allowed

    MAINLOG_L1("Sending SELECT AID command...\n");
    respCode = send_apdu_command(&apdu, &resp);
    if (respCode != SUCCESS_RESPONSE_CODE) {
        MAINLOG_L1("Error: SELECT AID failed, response code: %04X\n", respCode);
        return -1;
    }
    MAINLOG_L1("SELECT AID succeeded, response code: %04X\n", respCode);

    // ----------------------------------------------------------------
    // 2nd Command: READ RECORD (Part 1)
    // Header: 00 B2 01 00, Lc: 07, DataIn: aid_data, Le: 256 (expect data)
    // ----------------------------------------------------------------
    memset(&apdu, 0, sizeof(APDU_SEND));
    apdu.Command[0] = 0x00;  // CLA
    apdu.Command[1] = 0xB2;  // INS: READ RECORD
    apdu.Command[2] = 0x01;  // P1: Record number 1
    apdu.Command[3] = 0x00;  // P2
    apdu.Lc = 7;
    memcpy(apdu.DataIn, aid_data, 7);
    apdu.Le = 256;         // Request data (length unknown)
    apdu.EnableCancel = 0;

    MAINLOG_L1("Sending READ RECORD command (part 1)...\n");
    respCode = send_apdu_command(&apdu, &resp);
    if (respCode == SUCCESS_RESPONSE_CODE) {
        // Full data received in part 1.
        combinedLength = resp.LenOut;
        memcpy(combinedData, resp.DataOut, combinedLength);
        MAINLOG_L1("READ RECORD part 1 returned full data, response: %04X\n", respCode);
    } else if (respCode == CONTINUE_RESPONSE_CODE) {
        // Partial data received; more data available.
        combinedLength = resp.LenOut;
        memcpy(combinedData, resp.DataOut, combinedLength);
        MAINLOG_L1("READ RECORD part 1 returned partial data, response: %04X\n", respCode);

        // ----------------------------------------------------------------
        // 3rd Command: READ RECORD (Part 2)
        // Header: 00 B2 02 00, Lc: 07, DataIn: aid_data, Le: 256
        // ----------------------------------------------------------------
        memset(&apdu, 0, sizeof(APDU_SEND));
        apdu.Command[0] = 0x00;  // CLA
        apdu.Command[1] = 0xB2;  // INS: READ RECORD
        apdu.Command[2] = 0x02;  // P1: Record number 2
        apdu.Command[3] = 0x00;  // P2
        apdu.Lc = 7;
        memcpy(apdu.DataIn, aid_data, 7);
        apdu.Le = 256;
        apdu.EnableCancel = 0;

        MAINLOG_L1("Sending READ RECORD command (part 2)...\n");
        respCode = send_apdu_command(&apdu, &resp);
        if (respCode != SUCCESS_RESPONSE_CODE) {
            MAINLOG_L1("Error: READ RECORD part 2 failed, response code: %04X\n", respCode);
            return -1;
        }
        MAINLOG_L1("READ RECORD part 2 succeeded, response: %04X\n", respCode);
        memcpy(combinedData + combinedLength, resp.DataOut, resp.LenOut);
        combinedLength += resp.LenOut;
    } else {
        MAINLOG_L1("Error: READ RECORD part 1 failed, response code: %04X\n", respCode);
        return -1;
    }

    char cardString[513];  // 512 bytes for data + 1 byte for '\0'
    if (combinedLength < sizeof(cardString)) {
        memcpy(cardString, combinedData, combinedLength);
        cardString[combinedLength] = '\0';  // Null-terminate the string
        // MAINLOG_L1("Card data as string: %s\n", cardString);
    } else {
        MAINLOG_L1("Error: Card data length is too large to be represented as a string.\n");
        return -1;
    }

    MAINLOG_L1("Before mempcy 1");
    MAINLOG_L1("QR length = %d", combinedLength);
    mempcpy(data, cardString, combinedLength);
    MAINLOG_L1("After mempcy 2");

    *data_len = combinedLength;
    MAINLOG_L1("After assigning to data_len");

    return 0;
}

#ifdef UNIT_TEST
// If building for unit test, include a main() function.
int main(void) {
    int ret = read_qris_cpm_data_using_picc();
    if(ret == 0) {
        MAINLOG_L1("QRIS CPM data read successfully using PiccIsoCommand_Api.\n");
    } else {
        MAINLOG_L1("Failed to read QRIS CPM data using PiccIsoCommand_Api.\n");
    }
    return 0;
}
#endif
