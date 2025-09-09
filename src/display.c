/*
 * display.c
 *
 *  Created on: Oct 25, 2021
 *      Author: Administrator
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/def.h"
#include "../inc/tms_md5.h"
#include "../inc/tms_tms.h"
#include "../inc/display.h"
#include "../inc/number_helper.h"
#include "../inc/uthash/utarray.h"
#include "../inc/mqtt.h"
#include "../inc/nobu_nfc.h"
#include "../inc/Card.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>
#include <cJSON.h>

static unsigned char *t_sBuf = NULL;
static unsigned char *t_rBuf = NULL;
static unsigned char *t_rBufQR = NULL;
static unsigned char *t_sBuf2 = NULL;
static unsigned char *t_rBuf2 = NULL;
static unsigned char *t_rBufQR2 = NULL;
static unsigned char *t_sBufQR2 = NULL;

int payment_amount = 0;
typedef void (*func_ptr)();

void DispMainNobu();
void QRpos();
int WifiConnect();
void DispMainNobuSetting();
void SyncData();
void listTransaction();
void downloadUpdate();

typedef struct RTC_TIME
{
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint16_t year;
	uint8_t wDay;
} RTC_time;

URL_ENTRY tmsEntry;

void SetTmsUpdateProgress(int row, const char *data);
char *preprocessJsonString(char *input);
int httpSyncV2JSON(int *packLen, char *extracted, const char *url, const char* method);
char g_qr_static[512] = {0};
char g_merchant_id_nobu[128] = {0};
char g_external_store_id[128] = {0};
char g_aisino_title[128] = {0};

const char *pszTitle = "Menu";
// daftar text menu function
const char *pszItems[] = {
	"Dynamic QR",
	"Serial Number",
	"List Transaction",
	"Setting Wifi",
	"Sync Data",
	"Update App",
};
const int pszItemsCount = sizeof(pszItems) / sizeof(pszItems[0]);

// daftar semua menu function
const func_ptr menu_functions_list[] = {
	DispMainNobu,
	DispMainNobuSetting,
	listTransaction,
	WifiConnect,
	SyncData,
	downloadUpdate,
};

void CardNFC(const char *partnerReferenceNo, const int amount) 
{
	char  *response = (char *)calloc(RECVPACKLEN, sizeof(char));
	MAINLOG_L1("saya disini cardnfc ");
	// get serial number
	int fileLength = 0;
	char *serialNumber = (char*)calloc(2048, sizeof(char));
	fileLength = GetFileSize_Api("/ext/serial.txt");
	int readFileReturn = ReadFile_Api("/ext/serial.txt", serialNumber, 0, &fileLength);

	MAINLOG_L1("Serial numbernya = %s", serialNumber);
	
	if (readFileReturn == 3) {
		free(serialNumber);
		free(response);
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Tidak Ada Serial Number Harap ", CDISP);
		ScrDisp_Api(LINE2, 0, "Sync Terlebih Dahulu", CDISP);
		WaitEnterAndEscKey_Api(12);
		return;
	}

	// int amount = 1000;

	char tempAmount[20] = {0};
	sprintf(tempAmount, "%d", amount);

	char* data = (char *)calloc(513, sizeof(char));
	int data_len = 0;
	int key = 0;

	ScrCls_Api();
	ScrDisp_Api(LINE4, 0, "Tap Your Card Here", CDISP);
	ScrDisp_Api(LINE6, 0, "Amount:", CDISP);
	ScrDisp_Api(LINE7, 0, tempAmount, CDISP);
	KBFlush_Api();

	while (1) {
		key = GetKey_Api();
		// MAINLOG_L1("Pressed key = %d", key);
		if (key == ESC || key == ENTER) break;

		int check_card = PiccCheck();
		if (check_card != 0x00) continue;

		int ret = read_qris_cpm_data_using_picc(data, &data_len);
		MAINLOG_L1("Data QR = %s", data);
		MAINLOG_L1("Card NFC status: %d", ret);

		// int ret = 0;
		// strcpy(data, "hQVDUFYwMWFfTwegAAAGAiAgUAdxcmlzY3BtWgqTYARBMBIwhRAPXyAURUFTVE1BTiBOVVJVTCBISUtNQUhfLQRpZGVuX1AQdGVsOjA4NTc3MTg3ODAwMZ8lAlEBYwmfdAYxMjM0NTY=");

		if (ret == 0) {
			MAINLOG_L1("Card NFC data: %s", data);
			MAINLOG_L1("Card NFC Length : %d", data_len);

			// Tembak api
			int   response_length = 0;
			const char *url       = "/transactions-services/nobu-cpm/create-payment";

			MAINLOG_L1("Checkpoint 1");

			cJSON *body = cJSON_CreateObject();
			cJSON_AddStringToObject(body, "qrContent", data);
			cJSON_AddStringToObject(body, "serialNumber", serialNumber);
			cJSON_AddStringToObject(body, "partnerReferenceNo", partnerReferenceNo);
			cJSON_AddNumberToObject(body, "amount", amount);

			char *payloadStr = cJSON_PrintUnformatted(body);
			if (payloadStr != NULL) {
				MAINLOG_L1("Payload JSON = %s", payloadStr);
				free(payloadStr);
			}

			MAINLOG_L1("Checkpoint 2"); 

			memset(response, 0, sizeof(char) * RECVPACKLEN);

			MAINLOG_L1("Checkpoint 3");

			int httpReturn = httpRequestJSON(&response_length, response, url, "POST", body);

			MAINLOG_L1("Checkpoint 4");

			cJSON_Delete(body);

			MAINLOG_L1("Checkpoint 5");

			if (httpReturn != 0) {
				MAINLOG_L1("Gagal create payment");
				continue;
			}

			MAINLOG_L1("JSON to parse = %s", response);
			cJSON *responseJSON = cJSON_Parse(response);

			MAINLOG_L1("Checkpoint 6");

			if (responseJSON == NULL) {
				MAINLOG_L1("Gagal parse response");
				continue;
			}

			MAINLOG_L1("Checkpoint 7");

			cJSON *status = cJSON_GetObjectItem(responseJSON, "status");
			cJSON *message = cJSON_GetObjectItem(responseJSON, "message");

			MAINLOG_L1("Checkpoint 8");

			if (status == NULL) {
				MAINLOG_L1("status null");
			}

			MAINLOG_L1("status = %s", status->valuestring);

			if (status != NULL && strcmp(status->valuestring, "OK") == 0) {
				ScrCls_Api();
				ScrDisp_Api(LINE6, 0, "Payment Received!", CDISP);
				// WaitEnterAndEscKey_Api(12);
				break;
			}

			if (message != NULL && strcmp(message->valuestring, "TRY AGAIN") == 0) {
				ScrCls_Api();
				ScrDisp_Api(LINE5, 0, "ERROR", CDISP);
				ScrDisp_Api(LINE7, 0, "Please Restart NFC", CDISP);
				ScrDisp_Api(LINE8, 0, "System!", CDISP);
				break;
			}

			MAINLOG_L1("Checkpoint 9");
		} else {
			MAINLOG_L1("Card NFC error: %d", ret);
		}
	}
	
	free(response);
	free(data);
	free(serialNumber);
}

void QRCodeDisp()
{
	u8 OutBuf[1024];
	int ret;
	struct _LCDINFO lcdInfo;

	memset(&lcdInfo, 0, sizeof(lcdInfo));
	ScrGetInfo_Api(&lcdInfo);

	memset(OutBuf, 0, sizeof(OutBuf));
	strcpy(OutBuf, "https://www.vanstone.com.cn/en");
	ret = QREncodeString(OutBuf, 3, 3, QRBMP, 2.5);
	ScrCls_Api();
	scrSetBackLightMode_lib(2, 1000);
	ScrDispImage_Api(QRBMP, 0, 10);

}


int hitCallbackApi(const char *partnerReferenceNo, const char *callback, const char *serialNumber, char *responseOut)
{
    int packLen = 0;
	cJSON *root = NULL;
    int ret = 0;
    unsigned char packData[RECEIVE_BUF_SIZE] = {0};

    MAINLOG_L1("hitCallbackApi START partnerReferenceNo=%s, callback=%s, serialNumber=%s", partnerReferenceNo, callback, serialNumber);
	MAINLOG_L1("before create cJSON");

    root = cJSON_CreateObject();
    if (!root) { MAINLOG_L1("ERROR: gagal create cJSON object"); return -1; }
	MAINLOG_L1("Created cJSON object");
    cJSON_AddStringToObject(root, "partnerReferenceNo", partnerReferenceNo);
	MAINLOG_L1("Added partnerReferenceNo to cJSON = %s", partnerReferenceNo);
    cJSON_AddStringToObject(root, "callback", callback);
	MAINLOG_L1("Added callback to cJSON = %s", callback);
    cJSON_AddStringToObject(root, "serialNumber", serialNumber);
	MAINLOG_L1("Added serialNumber to cJSON = %s", serialNumber);

    char *bodyStr = cJSON_PrintUnformatted(root);
    if (!bodyStr) {
        MAINLOG_L1("ERROR: gagal create bodyStr dari cJSON");
        cJSON_Delete(root);
        return -1;
    }
    int dataLen = strlen(bodyStr);
    MAINLOG_L1("Body JSON = %s", bodyStr);

    int headerLen = snprintf((char *)packData,
        sizeof(packData),
        "POST /transactions-service/nobu-mpm/query-qris HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: Apache-HttpClient/4.3.5 (java 1.5)\r\n"
        "Connection: Keep-Alive\r\n"
        "Accept-Encoding: gzip,deflate\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "\r\n",
        DOMAIN,
        dataLen);

    if (headerLen < 0 || headerLen + dataLen >= sizeof(packData)) {
        MAINLOG_L1("ERROR: headerLen invalid, headerLen=%d, dataLen=%d", headerLen, dataLen);
        free(bodyStr);
        cJSON_Delete(root);
        return -1;
    }

    memcpy(packData + headerLen, bodyStr, dataLen);
    packData[headerLen + dataLen] = '\0';
    MAINLOG_L1("Full request = %s", packData);

    free(bodyStr);
    cJSON_Delete(root);

    packLen = headerLen + dataLen;
    MAINLOG_L1("Total packLen = %d", packLen);

    ret = dev_send(packData, packLen);
    MAINLOG_L1("dev_send ret = %d", ret);
    if (ret != 0) return ret;
    
    int recvLen = dev_recv(packData, RECEIVE_BUF_SIZE, 10);
    MAINLOG_L1("dev_recv recvLen = %d", recvLen);
    if (recvLen <= 0) {
        MAINLOG_L1("ERROR: tidak ada response dari server");
        return -1;
    }
    packData[recvLen] = '\0';
    MAINLOG_L1("Raw response = %s", packData);

    char *json_start = strchr((char *)packData, '{');
    if (!json_start) {
        MAINLOG_L1("ERROR: response tidak mengandung JSON");
        return -1;
    }

    strcpy(responseOut, json_start);
    MAINLOG_L1("Final JSON response = %s", responseOut);

    MAINLOG_L1("hitCallbackApi END sukses");
    return 0;
}


void DispMainFace(void)
{
    int fileLen = GetFileSize_Api("/ext/qr_static.txt");
    if (fileLen > 0) {
        int ret = ReadFile_Api("/ext/qr_static.txt", g_qr_static, 0, &fileLen);
        MAINLOG_L1("ReadFile_Api qr_static.txt ret = %d, len = %d, isi = %s", ret, fileLen, g_qr_static);
        g_qr_static[fileLen] = '\0';
    } else {
        g_qr_static[0] = '\0';
    }

    int fileLen2 = GetFileSize_Api("/ext/aisino_title.txt");
    if (fileLen2 > 0) {
        int ret = ReadFile_Api("/ext/aisino_title.txt", g_aisino_title, 0, &fileLen2);
        MAINLOG_L1("ReadFile_Api aisino_title.txt ret = %d, len = %d, isi = %s", ret, fileLen2, g_aisino_title);
        g_aisino_title[fileLen2] = '\0';
    } else {
        g_aisino_title[0] = '\0';
    }

    unsigned char *qrStr;
    const char *title;

    if (g_qr_static[0] == '\0' && g_aisino_title[0] == '\0') {
        qrStr = "www.uruz.id";
        title = "URUZ TECH";
        int ret = QREncodeString(qrStr, 3, 3, QRBMP, 2.9);
        ScrClsRam_Api();
        ScrDispRam_Api(LINE4, 0, title, CDISP);
        ScrDispImage_Api(QRBMP, 92, 110);
        ScrDispRam_Api(LINE9, 0, App_Msg.Version, CDISP);
        ScrBrush_Api();
    } else {
        qrStr = g_qr_static[0] ? g_qr_static : "www.uruz.id";
        title = g_aisino_title[0] ? g_aisino_title : "URUZ TECH";
        int ret = QREncodeString(qrStr, 3, 3, QRBMP, 2.9);
        ScrClsRam_Api();
        ScrDispRam_Api(LINE2, 0, title, CDISP);
        ScrDispImage_Api(QRBMP, 60, 60);
        ScrDispRam_Api(LINE9, 0, App_Msg.Version, CDISP);
// 		ScrDisp_Api(10, 0, "[1] = Proses", CDISP);
        ScrBrush_Api();

// 		while (1) {
// 			int key = GetKey_Api();

// 			if (key == '1') {
// 				char serialNumber[128] = {0};
// 				int fileLenSerial = GetFileSize_Api("/ext/serial.txt");
// 				ReadFile_Api("/ext/serial.txt", serialNumber, 0, &fileLenSerial);
// 				MAINLOG_L1("Serial numbernya = %s", serialNumber);

// 				const char *partnerReferenceNo = "SII-xxxx-openAPIV3"; 
// 				const char *callback = "1"; 

// 				char responseOut[RECVPACKLEN] = {0};
// 				int cbRet = hitCallbackApi(partnerReferenceNo, callback, serialNumber, responseOut);

// 				ScrDisp_Api(0, 0, cbRet == 0 ? "Callback sukses" : "Callback gagal", CDISP);
// 				ScrDisp_Api(1, 0, responseOut, CDISP);

// 				int timer = 30;
// 				while (timer > 0) {
// 					char timerStr[32];
// 					sprintf(timerStr, "Tunggu %d detik...", timer);
// 					ScrDisp_Api(2, 0, timerStr, CDISP);
// 					ScrBrush_Api();
// 					Delay_Api(1000);
// 					timer--;
// 				}
// 				ScrClrLine_Api(0, 0);
// 				ScrClrLine_Api(1, 1);
// 				ScrClrLine_Api(2, 2);
// 				ScrBrush_Api();
// 				break;
// }
// 			if (key == ESC || key == ENTER) {
// 				break;
// 			}

// 			Delay_Api(100); 
// 		}

// 		return 0;
	}
}

int WaitEvent(void)
{
	u8 Key;
	int TimerId;

	TimerId = TimerSet_Api();
	while (1)
	{
		if (TimerCheck_Api(TimerId, 30 * 1000) == 1)
		{
			SysPowerStand_Api();
			return 0xfe;
		}
		Key = GetKey_Api();
		if (Key != 0)
			return Key;
	}

	return 0;
}
#define TIMEOUT -2
int ShowMenuItem(char *Title, const char *menu[], u8 ucLines, u8 ucStartKey, u8 ucEndKey, int IsShowX, u8 ucTimeOut)
{
	MAINLOG_L1("ucStartKey = %d", ucStartKey);
	MAINLOG_L1("ucEndKey = %d", ucEndKey);

	u8 IsShowTitle, cur_screen, OneScreenLines, Cur_Line, i, t;
	int nkey;
	char dispbuf[50];

	memset(dispbuf, 0, sizeof(dispbuf));

	if (Title != NULL)
	{
		IsShowTitle = 1;
		OneScreenLines = 12;
	}
	else
	{
		IsShowTitle = 0;
		OneScreenLines = 13;
	}
	IsShowX -= 1;
	cur_screen = 0;

	while (1)
	{
		ScrClsRam_Api();
		if (IsShowTitle)
			ScrDisp_Api(LINE1, 0, Title, CDISP);
		Cur_Line = LINE1 + IsShowTitle;

		for (i = 0; i < OneScreenLines; i++)
		{
			t = i + cur_screen * OneScreenLines;
			if (t >= ucLines || menu[t] == NULL) //
			{
				break;
			}
			memset(dispbuf, 0, sizeof(dispbuf));
			strcpy(dispbuf, menu[t]);
			ScrDispRam_Api(Cur_Line++, 0, dispbuf, FDISP);
		}

		ScrBrush_Api();
		MAINLOG_L1("after ScrBrush_Api");
		nkey = WaitAnyKey_Api(ucTimeOut);
		MAINLOG_L1("WaitAnyKey_Api aa:%d", nkey);
		switch (nkey)
		{
		case ESC:
		case TIMEOUT:
			return nkey;
		default:
			if ((nkey >= ucStartKey) && (nkey <= ucEndKey))
				return nkey;
			break;
		}
	};
}

void MenuThread()
{
	int Result = 0;

	while (1)
	{
		DispMainFace();
		mqttMainThreadV2();
		SelectMainMenu();
	}
}

// Function to check if a value exists in the truth array
int exists_in_truth_values(const char *value, const char *truth_values[], size_t truth_size)
{
	for (size_t i = 0; i < truth_size; i++)
	{
		if (strcmp(truth_values[i], value) == 0)
		{
			return 1; // Found
		}
	}
	return 0; // Not found
}

// Function to find the index of a cJSON item in an array
int find_index_in_cJSON_array(cJSON *array, cJSON *item)
{
	int index = 0;
	cJSON *element;
	cJSON_ArrayForEach(element, array)
	{
		if (element == item)
		{
			return index;
		}
		index++;
	}
	return -1; // Not found (should not happen in our case)
}

void remove_invalid_items_from_json(cJSON *json_array, const char *truth_values[], size_t truth_size)
{
	int index = 0;
	cJSON *item = NULL;

	// Iterate from last to first to avoid shifting issues
	for (int i = cJSON_GetArraySize(json_array) - 1; i >= 0; i--)
	{
		item = cJSON_GetArrayItem(json_array, i);
		if (!exists_in_truth_values(item->valuestring, truth_values, truth_size))
		{
			cJSON_DeleteItemFromArray(json_array, i);
		}
	}
}

void SelectMainMenu(void)
{

	ScrBrush_Api();
	char resultJSON;
	int ret;
	unsigned char recv[512];
	RTC_time rtcTime;
	int nSelcItem = 1;
	int PackLen = 0;

	unsigned char outData[64], outKSN[32], data[25000 + 1];
	memset(outData, 0, sizeof(outData));
	memset(outKSN, 0, sizeof(outKSN));

	while (1)
	{
		// struktur json enabled_menu
		// {
		// 	"enabled_menu": ["POS QR", "Dynamic QR", "Sync Data", "Setting Wifi", "List Transaction", "Serial Number", "Update App", "Static QR"]
		// }

		cJSON *enabled_menu_array_orig = cJSON_GetObjectItem(enabled_menu, "enabled_menu");
		cJSON *enabled_menu_array = cJSON_Duplicate(enabled_menu_array_orig, 1);

		remove_invalid_items_from_json(enabled_menu_array, pszItems, pszItemsCount);

		// print enabled_menu_array json
		char *json_str = cJSON_Print(enabled_menu_array);
		MAINLOG_L1("enabled_menu json_str = %s", json_str);
		free(json_str);

		int enabled_menu_size = cJSON_GetArraySize(enabled_menu_array);
		MAINLOG_L1("enabled_menu_size = %d", enabled_menu_size);

		// char menus[TOTAL_MENU][50]; // menampung string menu yang ditampilkan
		char *menus[TOTAL_MENU];
		char *excluded_menus[TOTAL_MENU];
		char *included_menus[TOTAL_MENU];

		for (int i = 0; i < TOTAL_MENU; ++i) {
			menus[i] = (char *)malloc(sizeof(char) * 50);
			excluded_menus[i] = (char *)malloc(sizeof(char) * 50);
			included_menus[i] = (char *)malloc(sizeof(char) * 50);
		}

		UT_array *menu_functions_used;
		utarray_new(menu_functions_used, &ut_ptr_icd);

		// masukkin ke excluded menus
		int excluded_menu_size = 0;
		for (int i = 0; i<TOTAL_MENU; ++i) {
			if (strcmp(pszItems[i], "Setting Wifi") == 0 ||
				strcmp(pszItems[i], "Sync Data") == 0 ||
				strcmp(pszItems[i], "Update App") == 0 ||
				strcmp(pszItems[i], "Card") == 0) {
				strcpy(excluded_menus[excluded_menu_size], pszItems[i]);

				++excluded_menu_size;
			}
		}

		// masukking ke included menus
		int included_menu_size = 0;
		for (int i = 0; i<enabled_menu_size; ++i) {
			cJSON *item = cJSON_GetArrayItem(enabled_menu_array, i);

			if (strcmp(item->valuestring, "Setting Wifi") == 0 ||
				strcmp(item->valuestring, "Sync Data") == 0 ||
				strcmp(item->valuestring, "Update App") == 0 ||
				strcmp(item->valuestring, "Card") == 0 ||
				strcmp(item->valuestring, "POS QR") == 0) {
				continue;
			}
				strcpy(included_menus[included_menu_size], item->valuestring);
				++included_menu_size;
		}

		// filter included menu dengan menu dari enabled_menu
		int inserted_menu_size = 0;
		for (int i = 0; i < included_menu_size; ++i) {
			char *item = included_menus[i];
	
			for (int j = 0; j < TOTAL_MENU; ++j) {
				if (strcmp(pszItems[j], item) == 0) {
					sprintf(menus[i], "%d. %s", i + 1, pszItems[j]);
					MAINLOG_L1("Masukkin Menu %d = %s", i, menus[i]);

					utarray_push_back(menu_functions_used, &menu_functions_list[j]);
					MAINLOG_L1("Masukkin Function %d = %s", i, pszItems[j]);

					++inserted_menu_size;
					break;
				}
			}
		}

		MAINLOG_L1("inserted_menu_size = %d", inserted_menu_size);

		// masukkin semua menu dari excluded menus
		for (int i = 0; i < excluded_menu_size; ++i) {
			char *item = excluded_menus[i];
	
			for (int j = 0; j < TOTAL_MENU; ++j) {
				if (strcmp(pszItems[j], item) == 0) {
					sprintf(menus[i + inserted_menu_size], "%d. %s", i + inserted_menu_size + 1, pszItems[j]);
					MAINLOG_L1("Masukkin Menu %d = %s", i + inserted_menu_size, menus[i + inserted_menu_size]);

					utarray_push_back(menu_functions_used, &menu_functions_list[j]);
					MAINLOG_L1("Masukkin Function %d = %s", i + inserted_menu_size, pszItems[j]);
					break;
				}
			}
		}

		nSelcItem = ShowMenuItem(pszTitle, menus, inserted_menu_size + excluded_menu_size, DIGITAL1, DIGITAL1 + inserted_menu_size + excluded_menu_size - 1, 0, 60);
		MAINLOG_L1("Selected menu number = %d", nSelcItem);

		//		char sound_string[1000];
		//		numberToWords(1234567, sound_string);
		//		strcat(sound_string, " rupiah");
		//
		//		MAINLOG_L1("SOUND PLAYED: %s", sound_string);
		//		AppPlayTip(sound_string);

		switch (nSelcItem)
		{
		case DIGITAL1:
			( *((func_ptr*) utarray_eltptr(menu_functions_used, 0)) )();
			break;
		case DIGITAL2:
			( *((func_ptr*) utarray_eltptr(menu_functions_used, 1)) )();
			break;
		case DIGITAL3:
			( *((func_ptr*) utarray_eltptr(menu_functions_used, 2)) )();
			break;
		case DIGITAL4:
			( *((func_ptr*) utarray_eltptr(menu_functions_used, 3)) )();
			break;
		case DIGITAL5:
			( *((func_ptr*) utarray_eltptr(menu_functions_used, 4)) )();
			break;
		case DIGITAL6:
			( *((func_ptr*) utarray_eltptr(menu_functions_used, 5)) )();
			break;
		case DIGITAL7:
			( *((func_ptr*) utarray_eltptr(menu_functions_used, 6)) )();
			break;
		case DIGITAL8:
			( *((func_ptr*) utarray_eltptr(menu_functions_used, 7)) )();
			break;
		case DIGITAL9:
			( *((func_ptr*) utarray_eltptr(menu_functions_used, 8)) )();
			break;
		case ESC:
			return;
		default:
			break;
		}

		
		cJSON_Delete(enabled_menu_array);
		utarray_free(menu_functions_used);
		for (int i = 0; i < TOTAL_MENU; ++i) {
			free(menus[i]);
			free(excluded_menus[i]);
			free(included_menus[i]);
		}
	}

}

void downloadUpdate()
{
	ScrCls_Api();
	MAINLOG_L1("ABC, 1023423");
	set_tms_download_flag(1);
}

void listTransaction()
{
	ScrClsRam_Api();
	ScrDisp_Api(LINE1, 0, "List Transaction", CDISP);
	char *input_json = "| | | {\"data\":[{\"id\":\"111\",\"tanggal\":\"12/06/24\",\"status\":\"0\",\"jam\":\"15:20\"},{\"id\":\"112\",\"tanggal\":\"12/06/24\",\"status\":\"0\",\"jam\":\"15:20\"}]} + + +";

	char *json_string = preprocessJsonString(input_json);
	MAINLOG_L1("parsing JSON:%s", json_string);
	cJSON *json = cJSON_Parse(json_string);

	if (json == NULL)
	{
		MAINLOG_L1("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
		free(json_string); // Free dynamically allocated memory
		return;
	}
	MAINLOG_L1("Parsed JSON:\n%s\n", cJSON_Print(json));
	cJSON_Delete(json);
	free(json_string);
	return;
}

char *preprocessJsonString(char *input)
{
	char *processed = strdup(input); // Duplicate the input string
	char *ptr = processed;

	// Remove "|" characters
	char *pipe;
	while ((pipe = strchr(ptr, '|')) != NULL)
	{
		memmove(pipe, pipe + 1, strlen(pipe));
	}

	// Remove "+" characters
	char *plus;
	while ((plus = strchr(ptr, '+')) != NULL)
	{
		memmove(plus, plus + 1, strlen(plus));
	}

	return processed;
}

void StaticQR()
{
	ScrClsRam_Api();
	ScrDisp_Api(LINE1, 0, "Static QR Nobu", CDISP);
	memset(&tmsEntry, 0, sizeof(tmsEntry));
	int ret = 0;
	unsigned char recv[512];
	RTC_time rtcTime;
	int nSelcItem = 1;
	int PackLen = 0;
	char numStr[5];
	dev_disconnect();
	memset(&tmsEntry, 0, sizeof(tmsEntry));
	tmsEntry.protocol = PROTOCOL;
	strcpy(tmsEntry.domain, DOMAIN);
	strcpy(tmsEntry.port, PORT);
	MAINLOG_L1("TMS: nembak api dev_connect() hasil connect= %d", ret);
	if (ret < 0)
	{
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Gagal konek Pastikan", CDISP);
		ScrDisp_Api(LINE2, 0, "Device Terhubung", CDISP);
		ScrDisp_Api(LINE3, 0, "Dengan Internet", CDISP);
		WaitEnterAndEscKey_Api(5);
		return;
	}

	t_sBuf = malloc(SENDPACKLEN);
	if (t_sBuf == NULL)
	{
		MAINLOG_L1("!!! t_sBuf == NULL, MALLOC FAILED !!!");
		return _TMS_E_MALLOC_NOTENOUGH;
	}

	t_rBuf = malloc(RECVPACKLEN);
	if (t_rBuf == NULL)
	{
		MAINLOG_L1("!!! t_rBuf == NULL, MALLOC FAILED !!!");
		free(t_sBuf);

		return _TMS_E_MALLOC_NOTENOUGH;
	}
	t_rBufQR = malloc(RECVPACKLEN);
	if (t_rBufQR == NULL)
	{
		MAINLOG_L1("!!! t_rBuf == NULL, MALLOC FAILED !!!");
		free(t_rBufQR);

		return _TMS_E_MALLOC_NOTENOUGH;
	}
	memset(t_sBuf, 0, SENDPACKLEN);
	memset(t_rBufQR, 0, SENDPACKLEN);
	ScrCls_Api();
	ScrDisp_Api(1, 0, "Create QR...", LDISP);
	ret = httpPost(t_sBuf, &PackLen, t_rBufQR, "0");
	MAINLOG_L1("TMS: nembak api httpPost() hasil ret= %d", ret);
	if (ret == 0)
	{
		dev_disconnect();
		QRDispTestNobu(t_rBufQR, numStr, "static");
	}
	else
	{
		dev_disconnect();
		ScrDisp_Api(1, 0, "Gagal Membuat QR", CDISP);
		WaitEnterAndEscKey_Api(5);
	}
	dev_disconnect();
	free(t_rBufQR);
	free(t_sBuf);
	free(t_rBufQR2);
	free(t_sBuf2);
}
void testMqtt()
{
	MAINLOG_L1("testMqtt");
	ScrClsRam_Api();
	ScrDisp_Api(LINE1, 0, "Mqtt", CDISP);
	mQTTMainThread("payment");
	ScrBrush_Api();
}

void QRpos()
{
	MAINLOG_L1("QRpos");
	char *param = "POS";

	ScrClsRam_Api();
	ScrDisp_Api(LINE1, 0, "POS QR", CDISP);
	// ScrDisp_Api(LINE1, 0, "Waiting QR ...", CDISP);
	if (strcmp((char *)param, "POS") == 0)
	{
		dev_disconnect();
		mQTTMainThread("payment-pos");
	}

	ScrBrush_Api();
}

void SyncData(void)
{
	dev_disconnect();

	char resultJSON;
	int ret = 0;
	unsigned char recv[512];
	RTC_time rtcTime;
	int nSelcItem = 1;

	memset(&tmsEntry, 0, sizeof(tmsEntry));
	tmsEntry.protocol = PROTOCOL;
	strcpy(tmsEntry.domain, DOMAIN);
	strcpy(tmsEntry.port, PORT);

	MAINLOG_L1("Protocol = %d", tmsEntry.protocol);
	MAINLOG_L1("Domain = %s", tmsEntry.domain);
	MAINLOG_L1("PORT = %s", tmsEntry.port);

	ScrCls_Api();
	ScrDisp_Api(1, 0, "Sync...", LDISP);

	ret = dev_connect(&tmsEntry, 5);
	MAINLOG_L1("TMS: nembak api dev_connect() hasil connect= %d", ret);
	if (ret < 0)
	{
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Gagal konek Pastikan", CDISP);
		ScrDisp_Api(LINE2, 0, "Device Terhubung", CDISP);
		ScrDisp_Api(LINE3, 0, "Dengan Internet", CDISP);
		WaitEnterAndEscKey_Api(5);
		return;
	}

	// make request
	int   response_length       = 0;
	const char *url             = "/settings-services/settings/syncDataAisino";
	char  response[RECVPACKLEN] = {0};
	
	ret = httpSyncV2JSON(&response_length, response, url, "POST");
	dev_disconnect();
	MAINLOG_L1("TMS: nembak api httpSyncV2JSON hasil ret = %d", ret);
	if (ret != 0)
	{
		ScrCls_Api();
		// ScrDisp_Api(LINE1, 0, "Sync Gagal Pastikan", CDISP);
		// ScrDisp_Api(LINE2, 0, "Serial Number Benar", CDISP);
		ScrDisp_Api(LINE1, 0, "Sync Gagal", CDISP);
		ScrDisp_Api(LINE2, 0, "Mohon Coba Lagi!", CDISP);
		WaitEnterAndEscKey_Api(12);
		return;
	}
	MAINLOG_L1("response sync data = %s", response);

	// Struktur response
	// {
  //   "status": "OK",
  //   "sync_data": {
  //       "id": 0,
  //       "merchant_name": "Uruzdemofnb",
  //       "merchant_status": "0"
  //   },
  //   "sync_menu": {
  //       "enabled_menu": [
  //           "Dynamic QR",
  //           "POS QR",
  //           "Setting Wifi",
  //           "Serial Number",
  //           "Sync Data",
  //           "List Transaction",
  //           "Update App",
  //           "Display QR"
  //       ]
  //   }
	// }

	cJSON *responseJSON = cJSON_Parse(response);
	if (responseJSON == NULL) {
		MAINLOG_L1("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
		return;
	}

	MAINLOG_L1("Checkpoint 1");

	cJSON *sync_data = cJSON_GetObjectItem(responseJSON, "sync_data");
	cJSON *sync_menu = cJSON_GetObjectItem(responseJSON, "sync_menu");

	saveSSID("/ext/qr_static.txt", "");
	g_qr_static[0] = '\0';
	saveSSID("/ext/aisino_title.txt", "");
	g_aisino_title[0] = '\0';

	saveSSID("/ext/merchant_id_nobu.txt", "12345");
	g_merchant_id_nobu[0] = '\0';
	saveSSID("/ext/external_store_id.txt", "54321");
	g_external_store_id[0] = '\0';	

	// sync_data
	if(sync_data) {
		cJSON *merchant_id_nobu = cJSON_GetObjectItem(sync_data, "merchant_id_nobu");
		cJSON *external_store_id = cJSON_GetObjectItem(sync_data, "external_store_id");
		if(merchant_id_nobu && merchant_id_nobu->valuestring) {
			strncpy(g_merchant_id_nobu, merchant_id_nobu->valuestring, sizeof(g_merchant_id_nobu)-1);
			g_merchant_id_nobu[sizeof(g_merchant_id_nobu)-1] = '\0';
			int saveMerchantId = saveSSID("/ext/merchant_id_nobu.txt", g_merchant_id_nobu);
			MAINLOG_L1("saveMerchant = %d", saveMerchantId);
		}

		if(external_store_id && external_store_id->valuestring) {
			strncpy(g_external_store_id, external_store_id->valuestring, sizeof(g_external_store_id)-1);
			g_external_store_id[sizeof(g_external_store_id)-1] = '\0';
			int saveStoreId = saveSSID("/ext/external_store_id.txt", g_external_store_id);
			MAINLOG_L1("saveStore = %d", saveStoreId);
		}
	}
	
	// sync settings
	cJSON *sync_settings = cJSON_GetObjectItem(responseJSON, "sync_settings");
	if (sync_settings) {
		cJSON *qr_static = cJSON_GetObjectItem(sync_settings, "qr_static");
		cJSON *aisino_title = cJSON_GetObjectItem(sync_settings, "aisino_title");
		if (qr_static && qr_static->valuestring) {
			strncpy(g_qr_static, qr_static->valuestring, sizeof(g_qr_static)-1);
			g_qr_static[sizeof(g_qr_static)-1] = '\0';
			int saveStatic = saveSSID("/ext/qr_static.txt", g_qr_static);
			MAINLOG_L1("saveStatic = %d", saveStatic);
			
		}
		if (aisino_title && aisino_title->valuestring) {
			strncpy(g_aisino_title, aisino_title->valuestring, sizeof(g_aisino_title)-1);
			g_aisino_title[sizeof(g_aisino_title)-1] = '\0';
			int saveTitle = saveSSID("/ext/aisino_title.txt", g_aisino_title);
			MAINLOG_L1("saveTitle = %d", saveTitle);
		}
	}

	MAINLOG_L1("Checkpoint 2");

	// sync data
	cJSON *alias = cJSON_GetObjectItem(sync_data, "id");
	cJSON *merchant_name = cJSON_GetObjectItem(sync_data, "merchant_name");
	cJSON *merchant_status = cJSON_GetObjectItem(sync_data, "merchant_status");

	MAINLOG_L1("Checkpoint 3");

	MAINLOG_L1("alias = %s", alias->valuestring);
	MAINLOG_L1("merchant_name = %s", merchant_name->valuestring);
	MAINLOG_L1("merchant_status = %s", merchant_status->valuestring);
	saveSSID("/ext/alias.txt", alias->valuestring);
	saveSSID("/ext/merchant.txt", merchant_name->valuestring);
	saveSSID("/ext/status.txt", merchant_status->valuestring);

	MAINLOG_L1("Checkpoint 4");

	// sync menu
	cJSON_Delete(enabled_menu);
	enabled_menu = cJSON_Duplicate(sync_menu, 1);

	if (enabled_menu) {
    char *menu_str_dbg = cJSON_Print(enabled_menu);
    MAINLOG_L1("After SyncData, enabled_menu = %s", menu_str_dbg);
    free(menu_str_dbg);
}

	MAINLOG_L1("Checkpoint 5");

	char *sync_menu_str = cJSON_Print(enabled_menu);
	saveSSID("/ext/sync_menu.txt", sync_menu_str);
	MAINLOG_L1("sync_menu_str = %s", sync_menu_str);

	MAINLOG_L1("Checkpoint 6");

	cJSON_Delete(responseJSON);
	free(sync_menu_str);

	MAINLOG_L1("Checkpoint 7");

	ScrCls_Api();
	ScrDisp_Api(1, 0, "Sync Berhasil", CDISP);
	WaitEnterAndEscKey_Api(10);
	ScrBrush_Api();
}



// accept JSON as response
int httpSyncV2JSON(int *packLen, char *extracted, const char *url, const char *method)
{
	cJSON *root = cJSON_CreateObject();
	char *out = NULL;
	
	int      dataLen                    = 0;
	int      i                          = 0;
	unsigned char packData[SENDPACKLEN] = {0};
	u8       buf[64]                    = {0};
	u8       tmp[64]                    = {0};
	u8       timestamp[16]              = {0};
	u8       md5src[1024]               = {0};

	unsigned char *serialNumber[2];
	unsigned char result;
	result = PEDReadPinPadSn_Api(serialNumber);
	cJSON_AddStringToObject(root, "id", serialNumber);
	MAINLOG_L1("serial = %s", serialNumber);

	out = cJSON_PrintUnformatted(root);
	dataLen = strlen(out);

	strcat((char *)packData, method);
	strcat((char *)packData, " ");
	strcat((char *)packData, url);
	strcat((char *)packData, " HTTP/1.1\r\n");

	/// ========== TimeStamp & Sign
	memset(timestamp, 0, sizeof(timestamp));
	GetSysTime_Api(tmp);
	BcdToAsc_Api((char *)timestamp, (unsigned char *)tmp, 14);

	memset(tmp, 0x20, sizeof(tmp));
	memset(buf, 0x20, sizeof(buf));

	memcpy(tmp, TmsStruct.sn, strlen(TmsStruct.sn));
	memcpy(buf, timestamp, strlen((char *)timestamp));

	for (i = 0; i < 50; i++)
	{
		tmp[i] ^= buf[i];
	}

	memset(buf, 0x20, sizeof(buf));
	memcpy(buf, "998876511QQQWWeerASDHGJKL", strlen("998876511QQQWWeerASDHGJKL"));

	for (i = 0; i < 50; i++)
	{
		tmp[i] ^= buf[i];
	}
	tmp[50] = 0;

	memcpy(md5src, out, dataLen);
	BcdToAsc_Api((char *)(md5src + dataLen), (unsigned char *)tmp, 100);

	// MD5
	_tms_MDString((char *)md5src, (unsigned char *)buf);

	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api((char *)tmp, (unsigned char *)buf, 32);

	sprintf((char *)packData + strlen((char *)packData), "Content-Length: %d\r\n", strlen((char *)out));

	strcat((char *)packData, "Content-Type: application/json\r\n");

	sprintf((char *)packData + strlen((char *)packData), "Host: %s\r\n", DOMAIN);

	strcat((char *)packData, "Connection: Keep-Alive\r\n");

	strcat((char *)packData, "User-Agent: Apache-HttpClient/4.3.5 (java 1.5)\r\n");

	strcat((char *)packData, "Accept-Encoding: gzip,deflate\r\n\r\n");

	memcpy(packData + strlen((char *)packData), out, dataLen);

	*packLen = strlen((char *)packData);
	free(out);
	cJSON_Delete(root);

	int ret = 0;
	MAINLOG_L1("payload = %s", packData);
	MAINLOG_L1("payload length %d = ", *packLen);
	ret = dev_send(packData, *packLen);
	int ret2 = 0;
	MAINLOG_L1("return code dev_send = %d", ret);
	ret2 = dev_recv(packData, RECEIVE_BUF_SIZE, 10);
	MAINLOG_L1("return code dev_recv = %d", ret2);
	MAINLOG_L1("response = %s", packData);

	if (ret != 0) {
		MAINLOG_L1("dev_send gagal");
		return ret;
	}

	// Find the start of the JSON (first occurrence of '{')
	const char *json_start = strchr(packData, '{');

	if (json_start == NULL) {
		MAINLOG_L1("Error: No JSON found in the response!\n");
		return -1;
	}

	strcpy(extracted, json_start);
	return 0;
}

int httpRequestJSON(int *packLen,
					char *extracted,
					const char *url,
					const char *method,
					cJSON *body)
{
	dev_disconnect();

	memset(&tmsEntry, 0, sizeof(tmsEntry));
	tmsEntry.protocol = PROTOCOL;
	strcpy(tmsEntry.domain, DOMAIN);
	strcpy(tmsEntry.port, PORT);

	MAINLOG_L1("Protocol = %d", tmsEntry.protocol);
	MAINLOG_L1("Domain = %s", tmsEntry.domain);
	MAINLOG_L1("PORT = %s", tmsEntry.port);

	int devConnectRet = dev_connect(&tmsEntry, 5);
	MAINLOG_L1("TMS: nembak api dev_connect() hasil connect= %d", devConnectRet);
	MAINLOG_L1("devConnectRet = %d", devConnectRet);
	if (devConnectRet < 0)
	{
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Gagal konek Pastikan", CDISP);
		ScrDisp_Api(LINE2, 0, "Device Terhubung", CDISP);
		ScrDisp_Api(LINE3, 0, "Dengan Internet", CDISP);
		WaitEnterAndEscKey_Api(5);
		return -1;
	}

	// print body content
	char *printed = cJSON_Print(body);
	if (printed) {
		MAINLOG_L1("JSON body = %s", printed);
		free(printed);
	}

	// 1) Serialize the JSON body once
	char *out = cJSON_PrintUnformatted(body);
	if (!out)
	{
		MAINLOG_L1("Error: failed to serialize JSON body");
		return -1;
	}
	int dataLen = strlen(out);

	// 2) Prepare signature (unchanged from yours, but be SURE none of those routines overflow)
	//    ... your timestamp, XOR, MD5 code here ...
	//    Assume the result string (hex MD5) ends up in `char signature[33]` (32 hex + NUL).

	// 3) Build the HTTP header + blank line in one snprintf()
	//    Leave enough room for headers + body; SENDPACKLEN must be big enough!
	unsigned char packData[RECEIVE_BUF_SIZE] = {0};
	int headerLen = snprintf((char *)packData,
							 sizeof(packData),
							 "%s %s HTTP/1.1\r\n"
							 "Host: %s\r\n"
							 "User-Agent: Apache-HttpClient/4.3.5 (java 1.5)\r\n"
							 "Connection: Keep-Alive\r\n"
							 "Accept-Encoding: gzip,deflate\r\n"
							 "Content-Type: application/json\r\n"
							 "Content-Length: %d\r\n"
							 "\r\n",
							 method,
							 url,
							 DOMAIN,
							 dataLen);

	if (headerLen < 0 || headerLen + dataLen >= sizeof(packData))
	{
		MAINLOG_L1("Error: headers too large (%d + %d >= %zu)",
				   headerLen, dataLen, sizeof(packData));
		free(out);
		return -1;
	}

	// 4) Append the body
	memcpy(packData + headerLen, out, dataLen);
	packData[headerLen + dataLen] = '\0'; // not strictly needed for send(), but useful for debug

	free(out);

	*packLen = headerLen + dataLen;
	MAINLOG_L1("Sending request:\n%s", packData);

	// 5) Send & receive
	int ret = dev_send(packData, *packLen);
	if (ret != 0)
	{
		MAINLOG_L1("dev_send failed: %d", ret);
		return ret;
	}

	// memset(packData, 0, sizeof(char) * RECEIVE_BUF_SIZE);
	int recvLen = dev_recv(packData, RECEIVE_BUF_SIZE, 10);
	MAINLOG_L1("recvLen = %d", recvLen);
	if (recvLen <= 0)
	{
		MAINLOG_L1("dev_recv failed: %d", recvLen);
		return recvLen;
	}
	packData[recvLen] = '\0';
	MAINLOG_L1("Response: %s", packData);

	// 6) Extract JSON payload
	char *json_start = strchr((char *)packData, '{'); 
	if (!json_start)
	{
		MAINLOG_L1("Error: no JSON found in response");
		return -1;
	}
	strcpy(extracted, json_start);
	return 0;
}

void formatRupiah(const char *number, char *output)
{
	int len = strlen(number);
	int outputIndex = 0;
	int commaCount = (len - 1) / 3;

	// Iterate through the input number string
	for (int i = 0; i < len; ++i)
	{
		// Copy the digit
		output[outputIndex++] = number[i];

		// Insert a period if needed
		if ((len - i - 1) % 3 == 0 && i != len - 1)
		{
			output[outputIndex++] = '.';
		}
	}

	// Null-terminate the output string
	output[outputIndex] = '\0';
}

void DispMainNobu(void)
{
	char TempBuf2[1025];
	int Len2 = 0, ret4 = 0, loc2 = 0;
	int fileLen2 = 0;
	int diff2 = 0;

	memset(TempBuf2, 0, sizeof(TempBuf2));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen2 = GetFileSize_Api("/ext/status.txt");
	memset(TempBuf2, 0, sizeof(TempBuf2));
	ret4 = ReadFile_Api("/ext/status.txt", TempBuf2, loc2, &fileLen2);
	MAINLOG_L1("readfile = %s", TempBuf2);
	int number = atoi(TempBuf2);
	if (ret4 == 3)
	{
		return;
	}
	if (number == 1)
	{
		MAINLOG_L1("readfile masuk if = %s", TempBuf2);
		ScrClsRam_Api();
		ScrDisp_Api(LINE1, 0, "QR Nobu", CDISP);
		GetScanfTest();
	}
}

void DispMainNobuSetting(void)
{
	char TempBuf2[1025];
	int Len2 = 0, ret2 = 0, loc2 = 0;
	int fileLen2 = 0;
	int diff2 = 0;

	memset(TempBuf2, 0, sizeof(TempBuf2));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen2 = GetFileSize_Api("/ext/serial.txt");
	MAINLOG_L1("readfile = %d", fileLen2);
	memset(TempBuf2, 0, sizeof(TempBuf2));
	ret2 = ReadFile_Api("/ext/serial.txt", TempBuf2, loc2, &fileLen2);
	MAINLOG_L1("readfile = %d", ret2);
	MAINLOG_L1("readfile = %s", TempBuf2);
PWD:
	ScrCls_Api();
	if (ret2 == 0)
	{
		char combinedText[50]; // Make sure the size is sufficient to hold the combined string
		ScrClsRam_Api();
		sprintf(combinedText, "Serial Number : %s", TempBuf2);
		ScrDisp_Api(LINE1, 0, "Serial Number", CDISP);
		ScrDisp_Api(LINE2, 0, TempBuf2, CDISP);
		ScrBrush_Api();
		WaitEnterAndEscKey_Api(120);
	}
}

void readFilewifi()
{
	char TempBuf[1025];
	int Len = 0, ret = 0, loc = 0;
	int fileLen = 0;
	int diff = 0;

	memset(TempBuf, 0, sizeof(TempBuf));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen = GetFileSize_Api("/ext/myfile.txt");
	MAINLOG_L1("readfile = %d", fileLen);
	memset(TempBuf, 0, sizeof(TempBuf));
	ret = ReadFile_Api("/ext/myfile.txt", TempBuf, loc, &fileLen);
	MAINLOG_L1("readfile = %d", ret);
	MAINLOG_L1("readfile = %s", TempBuf);
}

void SelectSettingsMenu(void)
{
	int nSelcItem = 1, ret;

	char *pszTitle = "Menu";
	const char *pszItems[] = {
			"1.Volum up",
			"2.Volum down",
	};
	while (1)
	{
		nSelcItem = ShowMenuItem(pszTitle, pszItems, pszItemsCount, DIGITAL1, DIGITAL2, 0, 60);
		MAINLOG_L1("ShowMenuItem = %d  %d  %d", nSelcItem, DIGITAL1, DIGITAL2);
		switch (nSelcItem)
		{
		case DIGITAL1:
			if (G_sys_param.sound_level >= 5)
				AppPlayTip("This is the maximum volume");
			else
			{
				G_sys_param.sound_level++;
				AppPlayTip("Volume up");
				saveParam();
			}
			break;
		case DIGITAL2:
			if (G_sys_param.sound_level <= 1)
				AppPlayTip("This is the minimum volume");
			else
			{
				G_sys_param.sound_level--;
				AppPlayTip("Volume down");
				saveParam();
			}
			break;
		case ESC:
			return;
		default:
			break;
		}
	}
}

void OutputAPDU(char *FileName)
{
	char TempBuf[1025];
	int Len = 0, ret = 0, loc = 0;
	int fileLen = 0;
	int diff = 0;

	memset(TempBuf, 0, sizeof(TempBuf));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen = GetFileSize_Api(FileName);

	while (loc < fileLen)
	{
		diff = fileLen - loc;
		if (diff > 1024)
		{
			Len = 1024;
		}
		else if (diff <= 1024)
		{
			Len = diff;
		}

		memset(TempBuf, 0, sizeof(TempBuf));
		ret = ReadFile_Api(FileName, TempBuf, loc, &Len);

		if (ret == 0)
		{
			portSends_lib(10, TempBuf, Len);
			Delay_Api(20);
			loc += Len;
		}
		else
		{
			break;
		}
	}

	TipAndWaitEx_Api("finish");
	portClose_lib(10);
	DelFile_Api(FileName);
}

// ***********************************************************   Test Functions **************************************************************

void segmentScreed()
{
	secscrOpen_lib();
	secscrSetBackLightMode_lib(2);
	secscrSetAttrib_lib(4, 1);
	secscrCls_lib();
	secscrPrint_lib(0, 0, 0, "Q161 TMS");
}

void delFolder()
{
	int ret = -1;

	ret = DelFile_Api("/ext/hindi/aaa.mp3");
	TipAndWaitEx_Api("ret = %d", ret);
}

void testKeyValue()
{
	int ret = -1;
	while (1)
	{
		ret = GetKey_Api();
		MAINLOG_L1("GetKey = %d", ret);
	}
}

void AESEncryption()
{
	// The length of data to be encrypted and decrypted should be times of 16

	int ret = -1;
	unsigned char key[32];
	unsigned char data[16 * 3];
	unsigned char encryptedData[64];
	unsigned char decryptedData[16 * 3];
	unsigned char tmp[16 * 3];

	memset(key, 0, sizeof(key));
	memset(data, 0, sizeof(data));
	AscToBcd_Api(key, "11223344556677889900AABBCCDDEEFF00112233445566778899AABBCCDDFFEE", 64);
	AscToBcd_Api(data, "ABCDEF1234567890ABCDEFBCDEAF982FABCDEF1234567890ABCDEFBCDEAF982F7890ABCDEFBCDEAF982FABCDEF123456", 96);

	ret = AES_Api(data, 48, encryptedData, key, 256, 1);
	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api(tmp, encryptedData, 2 * sizeof(encryptedData));
	MAINLOG_L1("AES Encrypt = %d, data = %s", ret, tmp);

	ret = AES_Api(encryptedData, 48, decryptedData, key, 256, 0);
	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api(tmp, decryptedData, 96);
	MAINLOG_L1("AES Decrypt = %d, data = %s", ret, tmp);
}

void QRDispTest()
{
	int ret;

	unsigned char *qrStr = "upi://pay?pa=staticbp.BJP0000000000113@axisbank&pn=TESTONBOARD&am=1.00&mc=5099&cu=INR&tr=GTZDQR010682024320134924";

	ret = QREncodeString(qrStr, 15, 3, QRBMP, 1.5);
	ScrCls_Api();
	ScrDispImage_Api(QRBMP, 10, 10);

	TipAndWaitEx_Api("Press to show next QR");

	ret = QREncodeString(qrStr, 15, 3, QRBMP, 2);
	ScrCls_Api();
	ScrDispImage_Api(QRBMP, 10, 10);

	TipAndWaitEx_Api("Press to show logo");

	ScrDispImage_Api(AISINO_LOGO, 86, 94);
}

void GetScanfTest()
{
	u8 buf[10];
	memset(buf, 0, sizeof(buf));

	int ret;
	ScrDisp_Api(LINE2, 0, "Input Amount", CDISP);
	//	ScrDisp_Api(LINE3, 0, "And Refrence", CDISP);
	ScrDisp_Api(LINE4, 0, "PLS input Amount", LDISP);
	ret = inputNumber(buf, 4, 1, 3, 0, RDISP);
	MAINLOG_L1("GetScanfTest = %s", buf);
	MAINLOG_L1("GetScanfTest = %d", ret);
	if (ret == 0)
	{
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Pastikan Jumlah Lebih", CDISP);
		ScrDisp_Api(LINE2, 0, "Dari 1 Rupiah", CDISP);
		WaitEnterAndEscKey_Api(120);
		return;
	}
	else if (ret > 0)
	{
		unsigned char outBuf[17];
		unsigned char reference[17];
		char tampil[20];
		memset(outBuf, 0, sizeof(outBuf));
		snprintf(tampil, sizeof(tampil), "Rp. %s", buf);
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "QR Nobu", CDISP);
		ScrDisp_Api(LINE2, 0, "Input Amount", CDISP);
		ScrDisp_Api(LINE4, 0, tampil, RDISP);
		ScrDisp_Api(LINE5, 0, "Ref (Optional)", RDISP);
		ret = GetScanfEx_Api(MMI_NUMBER | MMI_LETTER, 0, 16, outBuf, 10000, 5, 5, 20, MMI_LETTER);
		MAINLOG_L1("GetScanfTest = %d", ret);
		memcpy(reference, outBuf + 1, outBuf[0]);
		if (ret == 0x0D)
		{
			prosesQr(buf, reference);
		}
	}
}

/**
 * buffer: buffer to save the input value
 * fontSize:
 *		ScrFontSet_Api(0), ScrFontSet_Api(2): max length = 42
 *		ScrFontSet_Api(1): max length = 32
 * 		ScrFontSet_Api(3), ScrFontSet_Api(4), ScrFontSet_Api(5);: max length = 10
 * dispOnMainScreen:
 * 		0 - don't display on main screen
 * 		1 - display on both of main screen and second screen
 * mainScreenRow: the row to display on main screen
 * mainScreenColum: the start column to display on main screen
 * atr: Align, LDISP, CDISP, RDISP
 */
int inputNumber(unsigned char *buffer, int fontSize, int dispOnMainScreen, int mainScreenRow, int mainScreenColum, unsigned char atr)
{
	int finish = 0;
	int loc = 0;
	int ret = 0;
	int maxLen = 0;
	memset(buffer, '\0', sizeof(buffer));
	char formattedNumber[20];
	char originalFormattedNumber[20];

	secscrOpen_lib();
	secscrSetBackLightMode_lib(2);
	secscrSetAttrib_lib(4, 1);
	secscrCls_lib();

	if (fontSize == 0 || fontSize == 2)
	{
		maxLen = 42;
	}
	else if (fontSize == 1)
	{
		maxLen = 32;
	}
	else if (fontSize == 3 || fontSize == 4 || fontSize == 5)
	{
		maxLen = 10;
	}

	while (finish == 0)
	{
		ret = 0;
		ret = GetKey_Api();

		if ((ret != 0) && (ret != 8) && (ret != 13) && (ret != 27) && (loc >= maxLen))
		{
			Beep_Api(1);
			continue;
		}

		switch (ret)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			buffer[loc] = ret;
			loc++;
			formatRupiah(buffer, formattedNumber);
			secscrPrint_lib(0, 0, 0, formattedNumber);
			if (dispOnMainScreen == 1)
			{
				snprintf(originalFormattedNumber, sizeof(originalFormattedNumber), "Rp. %s", formattedNumber);
				ScrDisp_Api(mainScreenRow, mainScreenColum, originalFormattedNumber, atr);
			}
			break;

		case 8: // Delete
			if (loc > 0)
			{
				loc--;
				buffer[loc] = '\0';
				secscrCls_lib();
				ScrClrLine_Api(mainScreenRow, mainScreenRow);

				if (strlen(buffer) > 0)
				{
					formatRupiah(buffer, formattedNumber);
					secscrPrint_lib(0, 0, 0, formattedNumber);
					if (dispOnMainScreen == 1)
					{
						snprintf(originalFormattedNumber, sizeof(originalFormattedNumber), "Rp. %s", formattedNumber);
						ScrDisp_Api(mainScreenRow, mainScreenColum, originalFormattedNumber, atr);
					}
				}
			}
			else
			{
				Beep_Api(1);
			}
			break;
		case 13: // confirm
			finish = 1;
			secscrCls_lib();
			if (dispOnMainScreen == 1)
			{
				ScrClrLine_Api(mainScreenRow, mainScreenRow);
			}
			return loc;
		case 27: // Cancel
			memset(buffer, '\0', sizeof(buffer));
			loc = 0;
			finish = 1;
			secscrCls_lib();
			if (dispOnMainScreen == 1)
			{
				ScrClrLine_Api(mainScreenRow, mainScreenRow);
			}
			return -1;
		default:
			break;
		}
	}

	return 0;
}

int prosesQr(char *buf, unsigned char *reference)
{
	char resultJSON;
	int ret = 0;
	unsigned char recv[512];
	RTC_time rtcTime;
	int nSelcItem = 1;
	int PackLen = 0;
	ScrCls_Api();
	ScrDisp_Api(1, 0, "Create QR...", LDISP);
	int condition = 1;

	MAINLOG_L1("GetScanfTest kondisi %d", condition);
	MAINLOG_L1("GetScanfTest berapa lama");
	if (condition == 1)
	{
		dev_disconnect();
		memset(&tmsEntry, 0, sizeof(tmsEntry));
		tmsEntry.protocol = PROTOCOL;
		strcpy(tmsEntry.domain, DOMAIN);
		strcpy(tmsEntry.port, PORT);
		ret = dev_connect(&tmsEntry, 5);
		MAINLOG_L1("TMS: nembak api dev_connect() hasil connect= %d", ret);
		if (ret < 0)
		{
			dev_disconnect();
			ScrCls_Api();
			ScrDisp_Api(LINE1, 0, "Gagal konek Pastikan", CDISP);
			ScrDisp_Api(LINE2, 0, "Device Terhubung", CDISP);
			ScrDisp_Api(LINE3, 0, "Dengan Internet", CDISP);
			WaitEnterAndEscKey_Api(5);
			return;
		}
		MAINLOG_L1("GetScanfTest berapa lama");
		t_sBuf = malloc(SENDPACKLEN);
		if (t_sBuf == NULL)
		{
			MAINLOG_L1("!!! t_sBuf == NULL, MALLOC FAILED !!!");
			return _TMS_E_MALLOC_NOTENOUGH;
		}

		t_rBuf = malloc(RECVPACKLEN);
		if (t_rBuf == NULL)
		{
			MAINLOG_L1("!!! t_rBuf == NULL, MALLOC FAILED !!!");
			free(t_sBuf);

			return _TMS_E_MALLOC_NOTENOUGH;
		}
		t_rBufQR = malloc(RECVPACKLEN);
		if (t_rBufQR == NULL)
		{
			MAINLOG_L1("!!! t_rBuf == NULL, MALLOC FAILED !!!");
			free(t_rBufQR);

			return _TMS_E_MALLOC_NOTENOUGH;
		}
		memset(t_sBuf, 0, SENDPACKLEN);
		memset(t_rBufQR, 0, SENDPACKLEN);
		ret = httpPost(t_sBuf, &PackLen, t_rBufQR, buf);
		MAINLOG_L1("TMS: nembak api httpPost() hasil ret= %d", ret);
		if (ret == 0)
		{
			dev_disconnect();
			QRDispTestNobu(t_rBufQR, buf, "payment-pos");
		}
		else
		{
			dev_disconnect();
			ScrDisp_Api(1, 0, "Gagal Membuat QR", CDISP);
			WaitEnterAndEscKey_Api(5);
		}
		dev_disconnect();
		free(t_rBufQR);
		free(t_sBuf);
		free(t_rBufQR2);
		free(t_sBuf2);
	}
	if (condition == 0)
	{
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Pastikan Jumlah Lebih", CDISP);
		ScrDisp_Api(LINE2, 0, "Dari 1 Rupiah", CDISP);
		WaitEnterAndEscKey_Api(120);
		GetScanfTest();
	}
}

void RSATest()
{
	int ret, pubkidx, prikidx, DataOutLen, encryptedDataLen;
	unsigned char pucModulus[1024 + 1];
	unsigned short usExpLen = 0;
	unsigned int puiExp = 0;
	unsigned char pucKeyInfoOut[512];
	unsigned char pucDataOut[512];
	unsigned char pucDataIn[30];
	unsigned char tmp[1024];
	unsigned char encryptedData[512];

	pubkidx = 1;
	prikidx = 2;
	memset(pucModulus, 0, sizeof(pucModulus));
	ret = RsaGenStoreKey_lib(pubkidx, prikidx, 1024, pucModulus, &usExpLen, &puiExp);
	Delay_Api(5000);
	MAINLOG_L1("RSA - RsaGenStoreKey_lib = %d", ret);

	memset(pucDataIn, 0, sizeof(pucDataIn));
	memset(pucDataIn, 0x4B, 16);
	memset(pucDataOut, 0, sizeof(pucDataOut));
	ret = RsaPublicEncrypt_lib(pubkidx, pucDataOut, &DataOutLen, pucKeyInfoOut, pucDataIn, 16, 1);
	MAINLOG_L1("RSA - RsaPublicEncrypt_lib = %d ,DataOutLen:%d", ret, DataOutLen);
	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api(tmp, pucDataOut, 2 * DataOutLen);
	MAINLOG_L1("RSA - pucDataOut = %s", tmp);
	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api(tmp, pucKeyInfoOut, 512);
	MAINLOG_L1("RSA - pucKeyInfoOut = %s", tmp);

	memset(encryptedData, 0, sizeof(encryptedData));
	memcpy(encryptedData, pucDataOut, DataOutLen);
	memset(pucDataOut, 0, sizeof(pucDataOut));
	memset(pucKeyInfoOut, 0, sizeof(pucKeyInfoOut));
	ret = RsaPrivDecrypt_lib(prikidx, pucDataOut, &DataOutLen, pucKeyInfoOut, encryptedData, DataOutLen, 1);
	MAINLOG_L1("RSA - RsaPrivDecrypt_lib = %d, DataOutLen:%d", ret, DataOutLen);
	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api(tmp, pucDataOut, 2 * DataOutLen);
	MAINLOG_L1("RSA - pucDataOut = %s", tmp);
	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api(tmp, pucKeyInfoOut, 512);
	MAINLOG_L1("RSA - pucKeyInfoOut = %s", tmp);

	memset(pucDataIn, 0, sizeof(pucDataIn));
	memset(pucDataIn, 0x31, 16);
	memset(pucDataOut, 0, sizeof(pucDataOut));
	ret = RsaPrivEncrypt_lib(prikidx, pucDataOut, &DataOutLen, pucKeyInfoOut, pucDataIn, 16, 0);
	MAINLOG_L1("RSA - RsaPrivEncrypt_lib = %d ,DataOutLen:%d", ret, DataOutLen);
	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api(tmp, pucDataOut, 2 * DataOutLen);
	MAINLOG_L1("RSA - pucDataOut = %s", tmp);
	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api(tmp, pucKeyInfoOut, 512);
	MAINLOG_L1("RSA - pucKeyInfoOut = %s", tmp);
}

void CBCEn()
{

	//	TMK    under Master TMK    --> F7F7AC0DA56696E793C0BC5804CD0DE3903921B4AFCEADE3 DD73F6
	//	Clear TMK                --> 5D16BF67F7AD04677F4A10B915A1929EFDE32FAD26F44CDF DD73F6
	//
	//
	//
	//	Pin Session Under TMK    -->    DDF8EE4CA6E4E559A3E107F7BE5E6F67101F1720D00E5552 E6A469
	//	Clear Pin session        --> 464FFDE9D37C250DAE8FC8B5AD6D2591C2FE68618C9402C2 E6A469
	//
	//
	//
	//	Data Session Under TMK    --> 7CF6D607C5D977E96571674E56B6217691487F2522A97806 201F33
	//	Clear Data Session        --> 0B37E9DC3B672FDF025837542FE93179A8F470EC6713D361 201F33
	//
	//	Before Ecnrypt,Part 1, Len 16
	//	303030354B616C616930303030303030
	//
	//	After Ecnrypt,Part 1, Len 16
	//	82788AD2B22D201402DBA99E604D1443

	unsigned char tmp[16];
	unsigned char tmp1[16];
	unsigned char iv[8];
	unsigned char outKCV[17], outKCVLen[3];

	int ret = -1;
	unsigned char mKeyIdx = 0;
	unsigned char wKeyIdx = 1;

	memset(tmp, 0, sizeof(tmp));
	memset(tmp1, 0, sizeof(tmp1));
	memset(outKCVLen, 0, sizeof(outKCVLen));
	memset(outKCV, 0, sizeof(outKCV));
	memset(iv, 0, sizeof(iv));

	ret = PedWriteMkWithKCV(mKeyIdx, 0x03, 1, "\x46\x4F\xFD\xE9\xD3\x7C\x25\x0D\xAE\x8F\xC8\xB5\xAD\x6D\x25\x91\xC2\xFE\x68\x61\x8C\x94\x02\xC2", 0x06, 0x03, "\xE6\xA4\x69", 3, outKCV, outKCVLen, NULL);
	MAINLOG_L1("PedWriteMkWithKCV-MK = %d", ret);
	ret = PedWriteMkWithKCV(mKeyIdx, wKeyIdx, 2, "\xF7\xF7\xAC\x0D\xA5\x66\x96\xE7\x93\xC0\xBC\x58\x04\xCD\x0D\xE3\x90\x39\x21\xB4\xAF\xCE\xAD\xE3", 0x86, 0x03, "\xFE\xFC\x4B", 3, outKCV, outKCVLen, NULL);
	MAINLOG_L1("PedWriteMkWithKCV-WK = %d", ret);

	AscToBcd_Api(tmp, "303030354B616C616930303030303030", 32);
	AscToBcd_Api(iv, "0000000000000000", 16);
	ret = PEDDesCBC_Api(wKeyIdx, 0x03, 0x02, tmp, 16, iv, tmp1);
	MAINLOG_L1("PEDDesCBC_Api = %d", ret);
	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api(tmp, tmp1, 32);
	MAINLOG_L1("ENC DATA new = %s", tmp);

	/*unsigned char plaintextMasterKey[16];
	unsigned char ciphertextWorkKey[16];
	int masterKeyIndex = 0;
	int workKeyIndex = 1;

	memset(plaintextMasterKey, 0, sizeof(plaintextMasterKey));
	memset(ciphertextWorkKey, 0, sizeof(ciphertextWorkKey));
	AscToBcd_Api(plaintextMasterKey, "ABABABABABABABABABABABABABABABAB", 32);
	AscToBcd_Api(ciphertextWorkKey, "BCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBC", 32);
	PEDWriteMKey_Api(masterKeyIndex, 0x03, plaintextMasterKey);
	PEDWriteWKey_Api(masterKeyIndex, workKeyIndex, 0x83, ciphertextWorkKey);*/
}

void writeFile()
{
	DelFile_Api("/ext/myfile.txt");
	int ret = 0;
	unsigned int size = 0;
	unsigned char buffer[13] = "Hello World!";
	size = GetFileSize_Api("/ext/myfile.txt");
	TipAndWaitEx_Api("size=%d", size);

	ret = WriteFile_Api("/ext/myfile.txt", buffer, size, sizeof(buffer));
	TipAndWaitEx_Api("ret=%d", ret);

	size = 0;
	size = GetFileSize_Api("/ext/myfile.txt");
	TipAndWaitEx_Api("size=%d", size);
}

double convertStringToDouble1(const char *str)
{
	double result;
	sscanf(str, "%lf", &result);
	return result;
}

double convertStringToDouble2(const char *str)
{
	double sign = 1.0;
	double result = 0.0;
	int decimalCount = 0;
	int i = 0;

	if (str[0] == '-')
	{
		sign = -1.0;
		i = 1;
	}

	for (; str[i] != '\0'; i++)
	{
		if (str[i] == '.')
		{
			decimalCount = 1;
			continue;
		}

		int digit = str[i] - '0';
		result = result * 10 + digit;
		if (decimalCount > 0)
			decimalCount *= 10;
	}

	result /= decimalCount;
	result *= sign;

	return result;
}

void json()
{
	// 1697735560000
	cJSON *setting = NULL;
	cJSON *settingVer = NULL;
	double f = 0.0;
	char *p = NULL;
	char *msg = "{\"success\":true,\"success\":true,\"status\":\"PENDING\",\"states\":[\"PENDING\"],\"setting\":{\"mqttEnabled\":true,\"settingVer\":\"1697735560000.3322\"}}";

	cJSON *settingJson = cJSON_Parse(msg);
	MAINLOG_L1("settingJson = %s", cJSON_Print(settingJson));
	setting = cJSON_GetObjectItem(settingJson, "setting");
	settingVer = cJSON_GetObjectItem(setting, "settingVer");
	p = settingVer->valuestring;
	MAINLOG_L1("settingJson-value:%s", p);
	f = convertStringToDouble1(p);
	MAINLOG_L1("settingJson-value:%.4lf", f);
	f = convertStringToDouble2(p);
	MAINLOG_L1("settingJson-value:%.4lf", f);
	free(settingJson);
	settingJson = NULL;
}

void dukptTDES()
{
	int ret = -1;
	int i = 0;
	u8 IPEK[17];
	u8 KSN[11];
	u8 KSNOut[11];
	u8 EcryptedData[529];
	u8 originalData[529];
	char ksnAsc[21];
	char pinBlockAsc[17];

	memset(IPEK, 0, sizeof(IPEK));
	memset(KSN, 0, sizeof(KSN));
	memset(KSNOut, 0, sizeof(KSNOut));
	memset(EcryptedData, 0, sizeof(EcryptedData));
	memset(ksnAsc, 0, sizeof(ksnAsc));
	memset(pinBlockAsc, 0, sizeof(pinBlockAsc));

	AscToBcd_Api(IPEK, "59CE902D70AD9E8215A522EC0C8ADC30", 32);
	AscToBcd_Api(KSN, "FFFF0060000227000000", 20);
	AscToBcd_Api(originalData, "708201019F3303E0F8C88701019F0607A00000052410104F07A000000524101057136522361000423154D27032260660000000000F9F0E0510100000009F0F05A468FC98009F0D05A468FC98005F24032703315F25032203019F2608F83F9D5F1F4925B39F2701805F280203569F3602026A9F3704C6EE23549F3901009F40057000B0A001950500800088009B02E8008407A00000052410105F2A0203565F3401019F0702A9009F080200029F090200649F1A0203569F1E0836303030303233349F3501229F4104049629425008444F4D45535449439F34031F03029A032401119C01009F02060000000022559F03060000000000009F1208444F4D45535449438A025A31000000", 528);

	ret = PedDukptWriteTIK_Api(1, 0, 16, IPEK, KSN, 0);
	MAINLOG_L1("DUKPT- write key = %d", ret);

	while (i < 20)
	{
		ret = -1;
		memset(originalData, 0, sizeof(originalData));
		AscToBcd_Api(originalData, "708201019F3303E0F8C88701019F0607A00000052410104F07A000000524101057136522361000423154D27032260660000000000F9F0E0510100000009F0F05A468FC98009F0D05A468FC98005F24032703315F25032203019F2608F83F9D5F1F4925B39F2701805F280203569F3602026A9F3704C6EE23549F3901009F40057000B0A001950500800088009B02E8008407A00000052410105F2A0203565F3401019F0702A9009F080200029F090200649F1A0203569F1E0836303030303233349F3501229F4104049629425008444F4D45535449439F34031F03029A032401119C01009F02060000000022559F03060000000000009F1208444F4D45535449438A025A31000000", 528);

		memset(EcryptedData, 0, sizeof(EcryptedData));
		memset(KSNOut, 0, sizeof(KSNOut));
		ret = PedDukptTdes_Api(1, 2, 0, "\x00\x00\x00\x00\x00\x00\x00\x00", originalData, 528 / 2, 3, EcryptedData, KSNOut);
		memset(originalData, 0, sizeof(originalData));
		memset(ksnAsc, 0, sizeof(ksnAsc));
		BcdToAsc_Api(originalData, EcryptedData, 528);
		BcdToAsc_Api(ksnAsc, KSNOut, 20);
		MAINLOG_L1("DUKPT-EN  data = %s", originalData);
		MAINLOG_L1("DUKPT-EN  ret = %d, ksn = %s", ret, ksnAsc);

		memset(originalData, 0, sizeof(originalData));
		ret = PedDukptTdes_Api(1, 2, 0, "\x00\x00\x00\x00\x00\x00\x00\x00", EcryptedData, 528 / 2, 2, originalData, KSNOut);
		memset(EcryptedData, 0, sizeof(EcryptedData));
		BcdToAsc_Api(EcryptedData, originalData, 528);
		MAINLOG_L1("DUKPT-DE  data = %s", EcryptedData);

		i++;
	}
}

void dukpt_16()
{
	int ret = -1;
	u8 IPEK[17];
	u8 KSN[11];
	u8 KSNOut[11];
	u8 PINBlockOut[9];
	char ksnAsc[21];
	char pinBlockAsc[17];

	memset(IPEK, 0, sizeof(IPEK));
	memset(KSN, 0, sizeof(KSN));
	memset(KSNOut, 0, sizeof(KSNOut));
	memset(PINBlockOut, 0, sizeof(PINBlockOut));
	memset(ksnAsc, 0, sizeof(ksnAsc));
	memset(pinBlockAsc, 0, sizeof(pinBlockAsc));

	AscToBcd_Api(IPEK, "59CE902D70AD9E8215A522EC0C8ADC30", 32);
	AscToBcd_Api(KSN, "FFFF0060000227000000", 20);

	ret = PedDukptWriteTIK_Api(1, 0, 16, IPEK, KSN, 0);
	MAINLOG_L1("DUKPT- write key = %d", ret);

	ret = PedGetPinDukpt_Api(1, 1, 4, 6, "6799900000070017", KSNOut, PINBlockOut);
	BcdToAsc_Api(ksnAsc, KSNOut, 20);
	BcdToAsc_Api(pinBlockAsc, PINBlockOut, 16);
	MAINLOG_L1("DUKPT- PedGetPinDukpt = %d, KSNOut = %s, PINBlock = %s", ret, ksnAsc, pinBlockAsc);
}

void dukpt_19()
{
	int ret = -1;
	u8 IPEK[17];
	u8 KSN[11];
	u8 KSNOut[11];
	u8 PINBlockOut[9];
	char ksnAsc[21];
	char pinBlockAsc[17];

	memset(IPEK, 0, sizeof(IPEK));
	memset(KSN, 0, sizeof(KSN));
	memset(KSNOut, 0, sizeof(KSNOut));
	memset(PINBlockOut, 0, sizeof(PINBlockOut));
	memset(ksnAsc, 0, sizeof(ksnAsc));
	memset(pinBlockAsc, 0, sizeof(pinBlockAsc));

	AscToBcd_Api(IPEK, "59CE902D70AD9E8215A522EC0C8ADC30", 32);
	AscToBcd_Api(KSN, "FFFF0060000227000000", 20);

	ret = PedDukptWriteTIK_Api(1, 0, 16, IPEK, KSN, 0);
	MAINLOG_L1("DUKPT- write key = %d", ret);

	ret = PedGetPinDukpt_Api(1, 1, 4, 6, "6799998900000070017", KSNOut, PINBlockOut);
	BcdToAsc_Api(ksnAsc, KSNOut, 20);
	BcdToAsc_Api(pinBlockAsc, PINBlockOut, 16);
	MAINLOG_L1("DUKPT- PedGetPinDukpt = %d, KSNOut = %s, PINBlock = %s", ret, ksnAsc, pinBlockAsc);
}

int httpPost(u8 *packData, int *packLen, char *extracted, char *amount)
{
	MAINLOG_L1("TMS: Tms_CreatePacket()");

	cJSON *root = NULL;
	char *out = NULL;

	int dataLen, i;
	u8 buf[64], tmp[64], timestamp[16], urlPath[128], md5src[1024];

	memset(urlPath, 0, sizeof(urlPath));
	root = cJSON_CreateObject();

	char TempBuf2[1025];
	int Len2 = 0, ret4 = 0, loc2 = 0;
	int fileLen3 = 0;
	int diff2 = 0;

	memset(TempBuf2, 0, sizeof(TempBuf2));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen3 = GetFileSize_Api("/ext/alias.txt");
	MAINLOG_L1("readfile = %d", fileLen3);
	memset(TempBuf2, 0, sizeof(TempBuf2));
	ret4 = ReadFile_Api("/ext/alias.txt", TempBuf2, loc2, &fileLen3);
	MAINLOG_L1("readfile = %d", ret4);
	MAINLOG_L1("readfile = %s", TempBuf2);
	if (ret4 == 3)
	{
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Tidak Ada Serial Number Harap ", CDISP);
		ScrDisp_Api(LINE2, 0, "Sync Terlebih Dahulu", CDISP);
		WaitEnterAndEscKey_Api(12);
		return;
	}
	int number = atoi(TempBuf2);
	int fileLength = 0;
	char *serialNumber = (char*)calloc(2048, sizeof(char));
	fileLength = GetFileSize_Api("/ext/serial.txt");
	int readFileReturn = ReadFile_Api("/ext/serial.txt", serialNumber, 0, &fileLength);
	MAINLOG_L1("serial number = %s", serialNumber);
	MAINLOG_L1("read serial number = %d", readFileReturn);
	cJSON_AddStringToObject(root, "serialNumber", serialNumber);
	cJSON_AddNumberToObject(root, "id", number);
	cJSON_AddStringToObject(root, "payment_type", "qris_dynamic");
	if (strcmp((char *)amount, "0") != 0)
	{
		cJSON_AddNumberToObject(root, "amount", atoi(amount));
	}

	if (strcmp((char *)amount, "0") != 0)
	{
		sprintf((char *)urlPath, "POST /transactions-services/transactions/aisinoQRV2");
	}
	else
	{
		sprintf((char *)urlPath, "POST /transactions-services/transactions/aisinoQRV2");
	}

	out = cJSON_PrintUnformatted(root);
	dataLen = strlen(out);

	memcpy((char *)packData + strlen((char *)packData), urlPath, strlen((char *)urlPath));
	strcat((char *)packData, " HTTP/1.1\r\n");

	/// ========== TimeStamp & Sign
	memset(timestamp, 0, sizeof(timestamp));
	GetSysTime_Api(tmp);
	BcdToAsc_Api((char *)timestamp, (unsigned char *)tmp, 14);

	memset(tmp, 0x20, sizeof(tmp));
	memset(buf, 0x20, sizeof(buf));

	memcpy(tmp, TmsStruct.sn, strlen(TmsStruct.sn));
	memcpy(buf, timestamp, strlen((char *)timestamp));

	for (i = 0; i < 50; i++)
	{
		tmp[i] ^= buf[i];
	}

	memset(buf, 0x20, sizeof(buf));
	memcpy(buf, "998876511QQQWWeerASDHGJKL", strlen("998876511QQQWWeerASDHGJKL"));

	for (i = 0; i < 50; i++)
	{
		tmp[i] ^= buf[i];
	}

	tmp[50] = 0;
	memset(buf, 0, sizeof(buf));
	memset(md5src, 0, sizeof(md5src));

	memcpy(md5src, out, dataLen);
	BcdToAsc_Api((char *)(md5src + dataLen), (unsigned char *)tmp, 100);

	// MD5
	_tms_MDString((char *)md5src, (unsigned char *)buf);

	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api((char *)tmp, (unsigned char *)buf, 32);

	sprintf((char *)packData + strlen((char *)packData), "Content-Length: %d\r\n", strlen((char *)out));

	strcat((char *)packData, "Content-Type: application/json\r\n");

	sprintf((char *)packData + strlen((char *)packData), "Host: %s\r\n", DOMAIN);

	strcat((char *)packData, "Connection: Keep-Alive\r\n");

	strcat((char *)packData, "User-Agent: Apache-HttpClient/4.3.5 (java 1.5)\r\n");

	strcat((char *)packData, "Accept-Encoding: gzip,deflate\r\n\r\n");

	memcpy(packData + strlen((char *)packData), out, dataLen);

	*packLen = strlen((char *)packData);
	free(out);
	cJSON_Delete(root);

	int ret = 0;
	ret = dev_send(packData, *packLen);
	int ret2 = 0;
	MAINLOG_L1("nembak api httpPost()- ret1 cuy= %d", ret);
	ret2 = dev_recv(packData, RECEIVE_BUF_SIZE, 10);
	MAINLOG_L1("nembak api httpPost()- ret2 after send = %d", ret2);
	if (strstr(packData, "| | |") != NULL && strstr(packData, " + + +") != NULL)
	{
		char *start = strstr(packData, "| | | ");
		start += strlen("| | | ");
		char *end = strstr(start, " + + +");
		if (end == NULL)
		{
			dev_disconnect();
			return -1;
		}

		int length = end - start;
		*extracted = (char *)malloc(length);
		if (extracted == NULL)
		{
			dev_disconnect();
			return -1;
		}
		strncpy(extracted, start, length);
		extracted[length] = '\0';
		MAINLOG_L1("nembak api httpPost()- extracted= %s", extracted);
		if (strcmp((char *)extracted, "ERROR") == 0)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

void SetTmsUpdateProgress(int row, const char *data)
{
	MAINLOG_L1("TMS: SetTmsUpdateProgress(row = %d)", row);

	if (row == -1)
	{
		return;
	}

	ScrClrLine_Api(row, row);
	ScrDispRam_Api(row, 0, data, CDISP);
	ScrBrush_Api();
}

int QRInvoice(char *final_url)
{
	int ret;
	ret = QREncodeString(final_url, 3, 3, QRBMP, 2.9);
	MAINLOG_L1("qr terakhir = %s", final_url);
	char TempBuf2[1025];
	int Len2 = 0, ret4 = 0, loc2 = 0;
	int fileLen2 = 0;
	int diff2 = 0;

	memset(TempBuf2, 0, sizeof(TempBuf2));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen2 = GetFileSize_Api("/ext/merchant.txt");
	memset(TempBuf2, 0, sizeof(TempBuf2));
	ScrBrush_Api();
	ret4 = ReadFile_Api("/ext/merchant.txt", TempBuf2, loc2, &fileLen2);
	MAINLOG_L1("readfile = %s", TempBuf2);
	ScrCls_Api();
	ScrDispImage_Api(QRBMP, 60, 65);
	ScrDisp_Api(LINE2, 0, "Scan Invoice", CDISP);
	int leng = 60;
	char str[256];
	while (1)
	{
		sprintf(str, "%d", leng);
		sprintf(str + strlen(str), "s");
		ScrDisp_Api(LINE1, 0, str, CDISP);
		leng--;
		if (leng == 0)
		{
			ScrBrush_Api();
			break;
		}
		int Result = 0;
		Result = GetKey_Api();
		if (Result == 0x1B)
		{
			ScrBrush_Api();
			break;
		}
		Delay_Api(1000);
	}
	return 0;
}

int QRDispTestNobu(unsigned char *qrStr, char *amount, char *param)
{
	int ret;
	char amountnya[200] = "";
	char partnerRefBuff[256] = {0};
	char formattedNumber[20];
	formatRupiah(amount, formattedNumber);
	ret = QREncodeString(qrStr, 3, 3, QRBMP, 2.9);
	MAINLOG_L1("qr terakhir = %s", qrStr);
	char TempBuf2[1025];
	int Len2 = 0, ret4 = 0, loc2 = 0;
	int fileLen2 = 0;
	int diff2 = 0;

	int fileLenPartnerRef = GetFileSize_Api("/ext/partner_ref_pos.txt");
	if (fileLenPartnerRef > 0 && fileLenPartnerRef < sizeof(partnerRefBuff)) {
		ReadFile_Api("/ext/partner_ref_pos.txt", partnerRefBuff, 0, &fileLenPartnerRef);
		MAINLOG_L1("PartnerRef dari file = %s", partnerRefBuff);
	} else {
		MAINLOG_L1("File partner_ref_pos.txt kosong atau terlalu besar");
	}

	memset(TempBuf2, 0, sizeof(TempBuf2));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen2 = GetFileSize_Api("/ext/merchant.txt");
	memset(TempBuf2, 0, sizeof(TempBuf2));
	ret4 = ReadFile_Api("/ext/merchant.txt", TempBuf2, loc2, &fileLen2);
	MAINLOG_L1("readfile = %s", TempBuf2);
	ScrCls_Api();
	ScrDisp_Api(LINE2, 0, TempBuf2, CDISP);
	sprintf(amountnya, "Rp. %s", formattedNumber);
	ScrDispImage_Api(QRBMP, 45, 65);
	ScrDisp_Api(LINE10, 0, amountnya, CDISP);
	ScrDisp_Api(11, 0, "[1] = Cek Pembayaran", CDISP);

	// while (1) {
	// int key = GetKey_Api();
	// if (key == '1') {
	// 	char serialNumber[128] = {0};
	// 	int fileLenSerial = GetFileSize_Api("/ext/serial.txt");
	// 	ReadFile_Api("/ext/serial.txt", serialNumber, 0, &fileLenSerial);
	// 	MAINLOG_L1("Serial numbernya = %s", serialNumber);

	// 	const char *partnerReferenceNo = "SII-xxxx-openAPIV3"; 
	// 	const char *callback = "1"; 

	// 	char responseOut[RECVPACKLEN] = {0};
	// 	int cbRet = hitCallbackApi(partnerReferenceNo, callback, serialNumber, responseOut);

	// 	ScrDisp_Api(0, 0, cbRet == 0 ? "Callback sukses" : "Callback gagal", CDISP);
	// 	ScrDisp_Api(1, 0, responseOut, CDISP);

	// 	int timer = 30;
	// 	while (timer > 0) {
	// 		char timerStr[32];
	// 		sprintf(timerStr, "Tunggu %d detik...", timer);
	// 		ScrDisp_Api(2, 0, timerStr, CDISP);
	// 		ScrBrush_Api();
	// 		Delay_Api(1000);
	// 		timer--;
	// 	}
	// 	ScrClrLine_Api(0, 0);
	// 	ScrClrLine_Api(1, 1);
	// 	ScrClrLine_Api(2, 2);
	// 	ScrBrush_Api();
	// 	break;
	// }
	// if (key == ESC || key == ENTER) {
	// 	break;
	// }

	// 	Delay_Api(100); 
	// }

	// return 0;

	
	
	MAINLOG_L1("sebelum if payment");
	MAINLOG_L1("param value='%s'", param);
	if (strcmp((char *)param, "payment-pos") == 0)
	{
		MAINLOG_L1("masuk if payment");
		mQTTMainThread(param);
		MAINLOG_L1("selesai if payment");
	}
	else if (strcmp((char *)param, "static") == 0)
	{
		MAINLOG_L1("masuk if static");
		mQTTMainThread(param);
		MAINLOG_L1("selesai if static");
	}
	MAINLOG_L1("selesai QRDispTestNobu");

	// WaitKey_Api();
	ScrBrush_Api();
	return 0;
}

void translateThreeDigits(int num, char *result)
{
	const char *digits[] = {"", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
	const char *tens[] = {"", "ten", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
	const char *teens[] = {"ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};

	int hundreds = num / 100;
	int tensUnits = num % 100;

	strcpy(result, "");

	if (hundreds > 0)
	{
		strcat(result, digits[hundreds]);
		strcat(result, " hundred ");
	}

	if (tensUnits >= 20)
	{
		strcat(result, tens[tensUnits / 10]);
		strcat(result, " ");
		strcat(result, digits[tensUnits % 10]);
	}
	else if (tensUnits >= 10)
	{
		strcat(result, teens[tensUnits % 10]);
	}
	else if (tensUnits > 0)
	{
		strcat(result, digits[tensUnits]);
	}
}

// Function to translate number to Indonesian words
void translateNumberToWords(int num, char *result)
{
	if (num == 0)
	{
		strcpy(result, "zero");
		return;
	}

	const char *units[] = {"", "thousand", "million", "billion", "trillion"}; // Extend as needed

	int group = 0;
	while (num > 0)
	{
		int threeDigits = num % 1000;
		char groupWords[50] = "";
		translateThreeDigits(threeDigits, groupWords);

		if (groupWords[0] != '\0')
		{
			strcat(groupWords, " ");
			strcat(groupWords, units[group]);
			strcat(groupWords, " ");
		}

		strcat(groupWords, result);
		strcpy(result, groupWords);

		num /= 1000;
		group++;
	}
}

int cekPembayaran(u8 *packData, int *packLen, char *qris, char *extracted)
{
	MAINLOG_L1("TMS: Tms_CreatePacket()");

	cJSON *root = NULL;
	char *out = NULL;

	int dataLen, i;
	u8 bufs[64], tmp[64], timestamp[16], urlPathNobu[128], md5src[1024];

	memset(urlPathNobu, 0, sizeof(urlPathNobu));
	root = cJSON_CreateObject();

	char TempBuf2[1025];
	int Len2 = 0, ret4 = 0, loc2 = 0;
	int fileLen2 = 0;
	int diff2 = 0;

	memset(TempBuf2, 0, sizeof(TempBuf2));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen2 = GetFileSize_Api("/ext/alias.txt");
	MAINLOG_L1("readfile = %d", fileLen2);
	memset(TempBuf2, 0, sizeof(TempBuf2));
	ret4 = ReadFile_Api("/ext/alias.txt", TempBuf2, loc2, &fileLen2);
	MAINLOG_L1("readfile = %d", ret4);
	MAINLOG_L1("readfile = %s", TempBuf2);
	if (ret4 == 3)
	{
		return;
	}
	cJSON_AddStringToObject(root, "qrisData", qris);

	sprintf((char *)urlPathNobu, "POST /transactions-services/transactions/checkTransactionStatus");

	out = cJSON_PrintUnformatted(root);
	dataLen = strlen(out);

	memcpy((char *)packData + strlen((char *)packData), urlPathNobu, strlen((char *)urlPathNobu));
	strcat((char *)packData, " HTTP/1.1\r\n");

	/// ========== TimeStamp & Sign
	memset(timestamp, 0, sizeof(timestamp));
	GetSysTime_Api(tmp);
	BcdToAsc_Api((char *)timestamp, (unsigned char *)tmp, 14);

	memset(tmp, 0x20, sizeof(tmp));
	memset(bufs, 0x20, sizeof(bufs));

	memcpy(tmp, TmsStruct.sn, strlen(TmsStruct.sn));
	memcpy(bufs, timestamp, strlen((char *)timestamp));

	for (i = 0; i < 50; i++)
	{
		tmp[i] ^= bufs[i];
	}

	memset(bufs, 0x20, sizeof(bufs));
	memcpy(bufs, "998876511QQQWWeerASDHGJKL", strlen("998876511QQQWWeerASDHGJKL"));

	for (i = 0; i < 50; i++)
	{
		tmp[i] ^= bufs[i];
	}

	tmp[50] = 0;
	memset(bufs, 0, sizeof(bufs));
	memset(md5src, 0, sizeof(md5src));

	memcpy(md5src, out, dataLen);
	BcdToAsc_Api((char *)(md5src + dataLen), (unsigned char *)tmp, 100);

	// MD5
	_tms_MDString((char *)md5src, (unsigned char *)bufs);

	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api((char *)tmp, (unsigned char *)bufs, 32);
	/// ========== TimeStamp & Sign
	sprintf((char *)packData + strlen((char *)packData), "Content-Length: %d\r\n", strlen((char *)out));

	strcat((char *)packData, "Content-Type: application/json;charset=UTF-8\r\n");

	sprintf((char *)packData + strlen((char *)packData), "Host: %s\r\n", DOMAIN);

	strcat((char *)packData, "Connection: Keep-Alive\r\n");

	strcat((char *)packData, "User-Agent: Apache-HttpClient/4.3.5 (java 1.5)\r\n");

	strcat((char *)packData, "Accept-Encoding: gzip,deflate\r\n\r\n");

	memcpy(packData + strlen((char *)packData), out, dataLen);
	*packLen = strlen((char *)packData);
	free(out);
	cJSON_Delete(root);
	MAINLOG_L1("nembak api cekPembayaran()- packData after send = %s", packData);
	int ret = 0;
	ret = dev_send(packData, *packLen);
	MAINLOG_L1("nembak api cekPembayaran() - ret1 cuy= %d", ret);
	int ret2 = 0;
	ret2 = dev_recv(packData, RECEIVE_BUF_SIZE, 10);
	MAINLOG_L1("nembak api cekPembayaran() - ret2 after send = %d", ret2);
	MAINLOG_L1("nembak api cekPembayaran() - ret2 after send = %s", packData);
	if (strstr(packData, "| | |") != NULL && strstr(packData, " + + +") != NULL)
	{
		char *start = strstr(packData, "| | | ");
		start += strlen("| | | ");
		char *end = strstr(start, " + + +");
		if (end == NULL)
		{
			dev_disconnect();
			return;
		}

		int length = end - start;
		*extracted = (char *)malloc(length);
		if (extracted == NULL)
		{
			dev_disconnect();
			return;
		}
		strncpy(extracted, start, length);
		extracted[length] = '\0';
		return 0;
	}
	else
	{
		return -1;
	}
}