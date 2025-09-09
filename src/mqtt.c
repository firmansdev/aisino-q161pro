/*
 * mqtt.c
 *
 *  Created on: 2021��10��14��
 *      Author: vanstone
 */

#include <string.h>

#include <MQTTFreeRTOS.h>

#include "../inc/def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>
#include "../inc/number_helper.h"
#include <MQTTClient.h>
#include <MQTTFreeRTOS.h>
#include <cJSON.h>
#include "../inc/mqtt.h"
#include "../inc/display.h"
#include "../inc/utils.h"

#define QRBMP "QRBMP.bmp"
int mqttTopicRouter(MessageData *md);
char qr_title[1024] = {0};
Network network;
volatile int mqtt_connection_active = 0;

void safe_mqtt_cleanup(MQTTClient *client, Network *net) {
    if (client) MQTTDisconnect(client);
    if (net && net->netContext) {
        net->mqttclose(net->netContext);
        net->netContext = NULL;
    }
    dev_disconnect();
    mqtt_connection_active = 0;
}


void initMqttOs(void)
{
	MQTTOS os;

	memset(&os, 0, sizeof(os));

	os.timerSet = (unsigned long (*)())TimerSet_Api;
	MQTTClientOSInit(os);
}

static void onDefaultMessageArrived(MessageData *md)
{
	MAINLOG_L1("MQTT defaultMessageArrived");
}

/*
 * Message comes in the following JSON format:
 * 1. transaction broadcasting		{"type":"TB", "msg":"Hello"}
 * 2. update notification			{"type":"UN"}
 * 3. synchronize time				{"type":"ST", "time":"20211016213200"}
 * 4. play MP3 file					{"type":"MP", "file":"2.mp3+1.mp3+3.mp3+2.mp3"}
 * 5. play amount					{"type":"TS", "amount":"12.33"}
 * Invalid messages are just ignored.
 *
 * Message should be less than 1024 bytes
 */
static void onTopicMessageArrived(MessageData *md)
{
	MAINLOG_L1("function onTopicMessageArrived");
	unsigned char buf[1024];
	unsigned char bcdtime[64], asc[12] = {0};
	char *pChar, *pDot = NULL;
	int countMP3 = 0, loc = 0, num = 0, ret, amount;
	unsigned char mp3File[10][10];

	MQTTMessage *m = md->message;

	MAINLOG_L1("MQTT message arrived");

	memcpy(buf, md->topicName->lenstring.data, md->topicName->lenstring.len);
	buf[md->topicName->lenstring.len] = 0;
	MAINLOG_L1("MQTT recv QR topic:%s", buf);

	memcpy(buf, m->payload, m->payloadlen);
	buf[m->payloadlen] = 0;
	MAINLOG_L1("MQTT recv QR message:%s", buf);

	splitStringOK(buf, 0);
}

void splitStringOK(unsigned char *extracted, int *condition)
{
    MAINLOG_L1("SplitStringOK");
    MAINLOG_L1("Isinya apa %s", extracted);
    
    char *token;
    char status[1024];
    char id[256];
    char amount[256];
    char final_url[1024];

    status[0] = '\0';
    id[0] = '\0';
    amount[0] = '\0';
    final_url[0] = '\0';

    int index = 0;
    token = strtok(extracted, ";");

    while (token != NULL) {
        if (index == 0) {
            strcpy(status, token);
        } else if (index == 1) {
            strcpy(id, token);
        } else if (index == 2) {
            strcpy(amount, token);
        } else if (index == 3) {
            strcpy(final_url, token); 
        }
        index++;
        token = strtok(NULL, ";");
    }

		if (index < 4) {
				MAINLOG_L1("Data tidak lengkap, hanya %d bagian", index);
				ScrCls_Api();
				ScrDisp_Api(LINE5, 0, "Data tidak lengkap", CDISP);
				WaitEnterAndEscKey_Api(5);
				return;
		}


    MAINLOG_L1("Status: %s, ID: %s, Amount: %s, URL: %s", status, id, amount, final_url);

    if (strcmp(status, "OK") == 0) {
        AppPlayTip("Payment Success");

        ScrCls_Api();
        ScrBrush_Api();
        ScrDisp_Api(LINE2, 0, "Scan QR", CDISP);

        int ret = QREncodeString(final_url, 3, 3, QRBMP, 2.9);

        if (ret == 0) {
            WaitEnterAndEscKey_Api(30);
        } else {
            ScrDisp_Api(LINE4, 0, "Gagal generate QR", CDISP);
            WaitEnterAndEscKey_Api(3);
        }

        if (condition == 0) {
            int err;
            Network n;
            MQTTClient c;
            MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

            MAINLOG_L1("MQTT Connecting to server");
            memset(&c, 0, sizeof(MQTTClient));
            c.defaultMessageHandler = onDefaultMessageArrived;
            n.mqttconnect = net_connect;
            n.mqttclose = net_close;
            n.mqttread = net_read;
            n.mqttwrite = net_write;
            n.netContext = n.mqttconnect(NULL, G_sys_param.mqtt_server, G_sys_param.mqtt_port, 90000, 0, &err);

            MQTTDisconnect(&c);
            n.mqttclose(n.netContext);
            G_sys_param.con = 1;
            ScrBrush_Api();
            return;
        } else {
            QRpos("POS2");
        }
    } else {
        ScrBrush_Api();
        ScrCls_Api();
        ScrDisp_Api(LINE5, 0, "Pembayaran Gagal", CDISP);
        WaitEnterAndEscKey_Api(5);
        ScrBrush_Api();
    }
}


// void splitStringOK(unsigned char *extracted, int *condition)
// {
// 	MAINLOG_L1("SplitStringOK");
// 	MAINLOG_L1("Isinya apa %s", extracted);
// 	if (strcmp((char *)extracted, "OK") == 0)
// 	{
// 		int ret, err;
// 		Network n;
// 		MQTTClient c;
// 		MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
// 		MAINLOG_L1("MQTT Connecting to server");
// 		memset(&c, 0, sizeof(MQTTClient));
// 		c.defaultMessageHandler = onDefaultMessageArrived;
// 		n.mqttconnect = net_connect;
// 		n.mqttclose = net_close;
// 		n.mqttread = net_read;
// 		n.mqttwrite = net_write;
// 		n.netContext = n.mqttconnect(NULL, G_sys_param.mqtt_server, G_sys_param.mqtt_port, 90000, 0, &err);
// 		AppPlayTip("Payment Success");
// 		MQTTDisconnect(&c);
// 		n.mqttclose(n.netContext);
// 		G_sys_param.con = 1;
// 		ScrBrush_Api();
// 		ScrCls_Api();
// 		ScrDisp_Api(LINE5, 0, "Pembayaran Berhasil", CDISP);
// 		WaitEnterAndEscKey_Api(5);
// 		ScrBrush_Api();
// 		return;
// 	}
// 	MAINLOG_L1("split masuk sini");
// 	char *token;
// 	char status[1024]; // Allocate memory for qr
// 	char id[256];			 // Allocate memory for amount
// 	status[0] = '\0';
// 	id[0] = '\0';
// 	token = strtok(extracted, ";");
// 	if (token != NULL)
// 	{
// 		//		int number = atoi(token);  // Convert the token to an integer
// 		MAINLOG_L1("split 1 %s", token);
// 		strcpy(status, token);
// 		if (strcmp((char *)token, "OK") == 0)
// 		{
// 			AppPlayTip("Payment Success");
// 		}
// 	}

// 	// Get the second token (string part)
// 	token = strtok(NULL, ";");
// 	if (token != NULL)
// 	{
// 		MAINLOG_L1("split 2 %s", token);
// 		strcpy(id, token);
// 	}

// 	// get the third part (amount part)
// 	// token = strtok(NULL, ";");
// 	// if (token != NULL)
// 	// {
// 	// 	MAINLOG_L1("split 3 %s", token);
// 	// 	int amount = atoi(token);
// 	// 	char payment_alert[250] = {0};
// 	// 	numberToWords(amount, payment_alert);

// 	// 	sprintf(payment_alert, "Your payment of %s rupiah has been received", payment_alert);

// 	// 	AppPlayTip(payment_alert);
// 	// }
// 	char link[] = "https://hot.uruz.id/transactions-services/transactions/downloadInvoice";
// 	char amount[256]; // Example initialization, replace with actual logic
// 	amount[0] = '\0';
// 	strcpy(amount, "100");
// 	char merchant[256]; // Example initialization, replace with actual logic
// 	merchant[0] = '\0';
// 	int fileLen2, ret4, loc2 = 0;
// 	char TempBuf2[1025];
// 	fileLen2 = GetFileSize_Api("/ext/merchant.txt");
// 	memset(TempBuf2, 0, sizeof(TempBuf2));
// 	ret4 = ReadFile_Api("/ext/merchant.txt", TempBuf2, loc2, &fileLen2);
// 	MAINLOG_L1("readfile = %s", TempBuf2);
// 	strcpy(merchant, TempBuf2);
// 	int len_a = strlen(link);
// 	int len_b = strlen(id);
// 	int len_c = strlen(merchant);
// 	int final_len = len_a + 1 + len_b + 1 + len_c + 1 + 7;
// 	char final_url[final_len];
// 	int ret2;
// 	sprintf(final_url, "%s?id=%s&merchant_name=%s", link, id, merchant);
// 	MAINLOG_L1("MQTT message arrived POS %s", final_url);
// 	ret2 = QRInvoice(final_url);

// 	if (ret2 == 0)
// 	{
// 		if (condition == 0)
// 		{
// 			int ret, err;
// 			Network n;
// 			MQTTClient c;
// 			MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
// 			MAINLOG_L1("MQTT Connecting to server");
// 			memset(&c, 0, sizeof(MQTTClient));
// 			c.defaultMessageHandler = onDefaultMessageArrived;
// 			n.mqttconnect = net_connect;
// 			n.mqttclose = net_close;
// 			n.mqttread = net_read;
// 			n.mqttwrite = net_write;
// 			n.netContext = n.mqttconnect(NULL, G_sys_param.mqtt_server, G_sys_param.mqtt_port, 90000, 0, &err);
// 			MQTTDisconnect(&c);
// 			n.mqttclose(n.netContext);
// 			G_sys_param.con = 1;
// 			ScrBrush_Api();
// 			return;
// 		}
// 		else
// 		{
// 			QRpos("POS2");
// 		}
// 		//		return;
// 	}
// }
static void onTopicMessageArrivedPOS(MessageData *md)
{
	// network.mqttclose(network.netContext);

	unsigned char buf[1024];
	unsigned char bcdtime[64], asc[12] = {0};
	char *pChar, *pDot = NULL;
	int countMP3 = 0, loc = 0, num = 0, ret, amount;
	unsigned char mp3File[10][10];

	MQTTMessage *m = md->message;

	MAINLOG_L1("MQTT message arrived POS");

	memcpy(buf, md->topicName->lenstring.data, md->topicName->lenstring.len);
	buf[md->topicName->lenstring.len] = 0;
	MAINLOG_L1("MQTT recv topic:%s", buf);

	memcpy(buf, m->payload, m->payloadlen);
	buf[m->payloadlen] = 0;
	MAINLOG_L1("MQTT recv payload:%s", buf);

	if (strcmp((char *)buf, "OK") == 0)
	{
		ScrBrush_Api();
		AppPlayTip("Payment Success");
		ScrCls_Api();
		ScrDisp_Api(LINE5, 0, "Pembayaran Berhasil", CDISP);
		WaitEnterAndEscKey_Api(5);
		ScrBrush_Api();
		QRpos("POS2");
	}
	else
	{
		MAINLOG_L1("Before calling splitStringPOS");
		splitStringPOS(buf);
		MAINLOG_L1("After calling splitStringPOS");
	}
}

static void onTopicMessageArrivedStatic(MessageData *md)
{
	unsigned char buf[1024];
	unsigned char bcdtime[64], asc[12] = {0};
	char *pChar, *pDot = NULL;
	int countMP3 = 0, loc = 0, num = 0, ret, amount;
	unsigned char mp3File[10][10];

	MQTTMessage *m = md->message;

	MAINLOG_L1("MQTT message arrived POS");

	memcpy(buf, md->topicName->lenstring.data, md->topicName->lenstring.len);
	buf[md->topicName->lenstring.len] = 0;
	MAINLOG_L1("MQTT recv topic:%s", buf);

	memcpy(buf, m->payload, m->payloadlen);
	buf[m->payloadlen] = 0;
	MAINLOG_L1("MQTT recv message:%s", buf);

	if (strcmp((char *)buf, "OK") == 0)
	{
		AppPlayTip("Payment Success");
	}
}

void splitStringPOS(unsigned char *extracted)
{

	MAINLOG_L1("split masuk sini POS");
	MAINLOG_L1("extracted: %s", extracted);
	char *urlInvoice = get_token(3);
	char buf[3];  // enough for "10", "9", ..., "1" and '\0'
	int result = split(extracted, ";");
	if (result > 0)
	{
		if (strcmp((char *)get_token(0), "OK") == 0)
		{
				//index 0 = ok
	  		//index 1 = UUID
			  //index 2 = amount
				//index 3 = url invoice
			MAINLOG_L1("Result is not null");
			MAINLOG_L1("Status: %s", get_token(0));
			MAINLOG_L1("UUID: %s", get_token(1));
			MAINLOG_L1("Amount: %s", get_token(2));
			MAINLOG_L1("URLINVOICE: %s", get_token(3));

			MAINLOG_L1("DIA OK DONG");
			ScrCls_Api();
			ScrBrush_Api();
		if (urlInvoice != NULL && strcmp(urlInvoice, "null") != 0 && strlen(urlInvoice) > 0) {
				  AppPlayTip("Payment Success");
			    int ret = QREncodeString(urlInvoice, 3, 3, QRBMP, 3);
					if (ret == 0) {
						ScrCls_Api();  // clear entire screen         // show QR
						ScrDisp_Api(LINE2, 0, "E-Receipt", CDISP);        // show title
						ScrDispImage_Api(QRBMP, 30, 65);         
						for (int i = 30; i > 0; i--) {
								char buf[4];  // to hold "30" + '\0'
								if (i >= 100) {
										// We never hit this, but it's safe
										buf[0] = '9';
										buf[1] = '9';
										buf[2] = '\0';
								} else if (i >= 10) {
										buf[0] = '0' + (i / 10);
										buf[1] = '0' + (i % 10);
										buf[2] = '\0';
								} else {
										buf[0] = '0' + i;
										buf[1] = '\0';
								}
								ScrDisp_Api(LINE1, 0, "          ", CDISP);        // clear old number
								ScrDisp_Api(LINE1, 0, buf, CDISP);                // show countdown
								Delay_Api(1000);                                  // wait 1 second
						}
						DispMainFace();
						
					} 
					else {
						MAINLOG_L1("RET BUKAN 0 FIX SOMETHING BAD HAPPENED");
						ScrDisp_Api(LINE4, 0, "Gagal generate QR", CDISP);
					}
					// QRpos("POS2");
			} else {
				AppPlayTip("Payment Success");
				ScrCls_Api();
				ScrDisp_Api(LINE2, 0, "Payment Success", CDISP);
			}
		}
		else
		{
				//index 0 = Content
				//index 1 = Amount
				//index 2 = UUID
				
			MAINLOG_L1("Result is not null");
			MAINLOG_L1("QRContent: %s", get_token(0));
			MAINLOG_L1("Amount: %s", get_token(1));
			MAINLOG_L1("UUID: %s", get_token(2));
			QRDispTestNobu(get_token(0), get_token(1), "qr");
		}
	} else {
		MAINLOG_L1("Result is null");
	}
}
static void onTopicMessageArrivedQR(MessageData *md)
{
	unsigned char buf[1024];

	MQTTMessage *m = md->message;

	MAINLOG_L1("MQTT message arrived isomedik registrasi/discharge");

	memcpy(buf, md->topicName->lenstring.data, md->topicName->lenstring.len);
	buf[md->topicName->lenstring.len] = 0;
	MAINLOG_L1("MQTT recv topic:%s", buf);

	memcpy(buf, m->payload, m->payloadlen);
	buf[m->payloadlen] = 0;
	MAINLOG_L1("MQTT recv message:%s", buf);

	MAINLOG_L1("qr_title = %s", qr_title);

	struct _LCDINFO lcdInfo;

	memset(&lcdInfo, 0, sizeof(lcdInfo));
	ScrGetInfo_Api(&lcdInfo);

	QREncodeString(buf, 4, 3, QRBMP, 5);
	ScrCls_Api();
	ScrDisp_Api(LINE1, 0, qr_title, CDISP);
	scrSetBackLightMode_lib(2, 1000);
	ScrDispImage_Api(QRBMP, 40, 90);
}

// kalo semuanya udh update ke versi V.0.0.16, onTopicMessageArrivedQR yang lama boleh dihapus
static void onTopicMessageArrivedQRV2(MessageData *md)
{
	unsigned char buf[1024];

	MQTTMessage *m = md->message;

	MAINLOG_L1("MQTT message arrived display QR");

	memcpy(buf, md->topicName->lenstring.data, md->topicName->lenstring.len);
	buf[md->topicName->lenstring.len] = 0;
	MAINLOG_L1("MQTT recv topic:%s", buf);

	memcpy(buf, m->payload, m->payloadlen);
	buf[m->payloadlen] = 0;
	MAINLOG_L1("MQTT recv message:%s", buf);

	struct _LCDINFO lcdInfo;

	memset(&lcdInfo, 0, sizeof(lcdInfo));
	ScrGetInfo_Api(&lcdInfo);

	// tampilin QR
	// struktur JSON
	// {
  //   "title": "ini title", // maksimal 19 karakter, default "QR Code"
  //   "qr": "Ini dari set display QR" // maksimal 84 karakter
	// }
	cJSON *json = cJSON_Parse(buf);

	if (json == NULL) {
		MAINLOG_L1("Failed to parse JSON");
		return 1;
	}

	cJSON *title = cJSON_GetObjectItem(json, "title");
	cJSON *qr = cJSON_GetObjectItem(json, "qr");

	QREncodeString(qr->valuestring, 15, 3, QRBMP, 2);
	ScrCls_Api();
	ScrDisp_Api(LINE2, 0, title->valuestring, CDISP);
	scrSetBackLightMode_lib(2, 1000);
	ScrDispImage_Api(QRBMP, 40, 90);
}


static unsigned char pSendBuf[1024];
static unsigned char pReadBuf[1024];
int mQTTMainThread(char *param)
{
 	u8 Key;
	int ret, err;
	Network n;
	MQTTClient c;
	char string[200] = "";

	
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	char TempBuf2[1025];
	int Len2 = 0, ret4 = 0, loc2 = 0;
	int fileLen2 = 0;
	int diff2 = 0;

	// if (mqtt_connection_active) {
	// 		MAINLOG_L1("MQTT connection already active, mQTTMainThread exited early");
	// 		return -1;
	// }
	// mqtt_connection_active = 1;

	memset(TempBuf2, 0, sizeof(TempBuf2));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen2 = GetFileSize_Api("/ext/serial.txt");
	MAINLOG_L1("readfile = %d", fileLen2);
	memset(TempBuf2, 0, sizeof(TempBuf2));
	ret4 = ReadFile_Api("/ext/serial.txt", TempBuf2, loc2, &fileLen2);
	MAINLOG_L1("readfile function mQTTMainThread  = %d", ret4);
	MAINLOG_L1("readfile function mQTTMainThread = %s", TempBuf2);
	MAINLOG_L1("MQTT Param function mQTTMainThread = %s", param);
	if (ret4 == 3)
	{
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Tidak Ada Serial Number Harap ", CDISP);
		ScrDisp_Api(LINE2, 0, "Sync Terlebih Dahulu", CDISP);
		WaitEnterAndEscKey_Api(12);
		return;
	}
	if (strcmp((char *)param, "payment-pos") == 0)
	{
		MAINLOG_L1("masuk if");
		strcpy(G_sys_param.mqtt_topic, "aisino/dynamicQR/payment-pos/");
	}
	else if (strcmp((char *)param, "payment") == 0)
	{
		MAINLOG_L1("masuk else if");
		strcpy(G_sys_param.mqtt_topic, "aisino/dynamicQR/payment/");
	}
	else if (strcmp((char *)param, "static") == 0)
	{
		MAINLOG_L1("masuk else if");
		strcpy(G_sys_param.mqtt_topic, "aisino/staticQR/payment/");
	}
	else if (strcmp((char *)param, "isomedik-registrasi") == 0)
	{
		MAINLOG_L1("masuk else if");
		strcpy(G_sys_param.mqtt_topic, "aisino/dynamicQR/isomedik-registrasi/");
		strcpy(qr_title, "QR Registrasi");
	}
	else if (strcmp((char *)param, "isomedik-discharge") == 0)
	{
		MAINLOG_L1("masuk else if");
		strcpy(G_sys_param.mqtt_topic, "aisino/dynamicQR/isomedik-discharge/");
		strcpy(qr_title, "QR Discharge");
	}
	else
	{
		MAINLOG_L1("masuk else");
		strcpy(G_sys_param.mqtt_topic, "aisino/dynamicQR/payment/");
	}
	strcat(G_sys_param.mqtt_topic, TempBuf2);
	G_sys_param.con = 0;
	MAINLOG_L1("MQTT topic = %s", G_sys_param.mqtt_topic);

	MAINLOG_L1("MQTT Connecting to server");

	memset(&c, 0, sizeof(MQTTClient));
	MQTTClientInit(&c, &n, 20000, pSendBuf, 1024, pReadBuf, 1024);

	c.defaultMessageHandler = onDefaultMessageArrived;

	n.mqttconnect = net_connect;
	n.mqttclose = net_close;
	n.mqttread = net_read;
	n.mqttwrite = net_write;

	n.netContext = n.mqttconnect(NULL, G_sys_param.mqtt_server, G_sys_param.mqtt_port, 5000, 0, &err);
	if (n.netContext == NULL)
	{
		MAINLOG_L1("MQTT n.netContext == NUL, err = %d", err);
		MQTTDisconnect(&c);
		n.mqttclose(n.netContext);
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Tidak Ada Koneksi", CDISP);
		ScrDisp_Api(LINE2, 0, "Internet", CDISP);
		WaitEnterAndEscKey_Api(5);
		//		ScrCls_Api();
		//		ScrDisp_Api(LINE1, 0, "Connecting ...", CDISP);
		//		connectWifi();
		//		WaitEnterAndEscKey_Api(10);
		return;
	}

	data.willFlag = 0;
	data.MQTTVersion = 4; // 3.1.1
	data.clientID.cstring = G_sys_param.mqtt_client_id;
	data.username.cstring = "aisino";
	data.password.cstring = "uruzaisino@123";
	data.keepAliveInterval = G_sys_param.mqtt_keepalive;
	data.cleansession = 1;

	MAINLOG_L1("MQTT cleansession : %d", data.cleansession);
	ret = MQTTConnect(&c, &data);
	MAINLOG_L1("MQTTConnect : %d", ret);
	MAINLOG_L1("Ini Paramsnya : %d", (char *)param);
	if (ret != 0)
	{
		MQTTDisconnect(&c);
		n.mqttclose(n.netContext);
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Gagal Konek Ke", CDISP);
		ScrDisp_Api(LINE2, 0, "Server", CDISP);
		WaitEnterAndEscKey_Api(12);
		return;
	}
	if (strcmp((char *)param, "payment-pos") == 0)
	{
		MAINLOG_L1("if payment-pos mqttMainThread");
		ret = MQTTSubscribe(&c, G_sys_param.mqtt_topic, G_sys_param.mqtt_qos, onTopicMessageArrivedPOS);
	}
	else if (strcmp((char *)param, "payment") == 0)
	{
		MAINLOG_L1("MQTT subscribe : %s", G_sys_param.mqtt_topic);
		ret = MQTTSubscribe(&c, G_sys_param.mqtt_topic, G_sys_param.mqtt_qos, onTopicMessageArrived);
	}
	else if (strcmp((char *)param, "static") == 0)
	{
		ret = MQTTSubscribe(&c, G_sys_param.mqtt_topic, G_sys_param.mqtt_qos, onTopicMessageArrivedStatic);
	}
	else if (strcmp((char *)param, "isomedik-registrasi") == 0)
	{
		ret = MQTTSubscribe(&c, G_sys_param.mqtt_topic, G_sys_param.mqtt_qos, onTopicMessageArrivedQR);
	}
	else if (strcmp((char *)param, "isomedik-discharge") == 0)
	{
		ret = MQTTSubscribe(&c, G_sys_param.mqtt_topic, G_sys_param.mqtt_qos, onTopicMessageArrivedQR);
	}
	else
	{
		ret = MQTTSubscribe(&c, G_sys_param.mqtt_topic, G_sys_param.mqtt_qos, onTopicMessageArrivedStatic);
	}
	MAINLOG_L1("MQTTSubscribe : %d", ret);
	if (ret != 0)
	{
		MQTTDisconnect(&c);
		n.mqttclose(n.netContext);
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Gagal Konek Ke", CDISP);
		ScrDisp_Api(LINE2, 0, "Server", CDISP);
		WaitEnterAndEscKey_Api(12);
		return;
	}

	MAINLOG_L1("MQTT Connected");
	int limit = 90;
	char str[256];
	while (1)
	{
		//		ScrBrush_Api();
		//		ScrCls_Api();
		if (strcmp((char *)param, "payment") == 0)
		{
			MAINLOG_L1("Checkpoint 1");
			sprintf(str, "%d", limit);
			sprintf(str + strlen(str), "s");
			ScrDisp_Api(LINE1, 0, str, CDISP);
			limit--;
			if (limit == 0)
			{
				MAINLOG_L1("MQTTYield: %d", ret);
				dev_disconnect();
				ScrCls_Api();
				ScrDisp_Api(LINE1, 0, "Timeout", CDISP);
				WaitEnterAndEscKey_Api(5);
				return;
				break;
			}
		}

		if (G_sys_param.con == 0)
		{
			int Result = 0;
			Result = GetKey_Api();
			if (Result == 0x1B)
			{
				MAINLOG_L1("Checkpoint 2");
				ScrCls_Api();
				MQTTDisconnect(&c);
				n.mqttclose(n.netContext);
				// safe_mqtt_cleanup(&c, &n);
				return;
				break;
			}
			ret = MQTTYield(&c, 1000); // set heartbeat time
			if (ret < 0)
			{
				MAINLOG_L1("Checkpoint 3");
				MAINLOG_L1("MQTTYield: %d", ret);
				dev_disconnect();
				ScrCls_Api();
				ScrDisp_Api(LINE1, 0, "Koneksi Terputus", CDISP);
				WaitEnterAndEscKey_Api(12);
				return;
				break;
			}
			// TODO: Check SMS message
			Delay_Api(10);
		}
		else if (G_sys_param.con == 1)
		{
			ScrCls_Api();
			MQTTDisconnect(&c);
			n.mqttclose(n.netContext);
			return;
			break;
		}
	}

	MQTTDisconnect(&c);
	n.mqttclose(n.netContext);
	return 0;
	
}

char receivedTopic[256];
char specificTopic[256] = {0};	// if specific contains non null string, only listen to this topic
// ini buat mqtt yang ada di sebelum menu utama
int mqttMainThreadV2()
{
	u8 Key;
	int ret, err;
	MQTTClient client;
	char string[200] = "";

	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	char TempBuf2[1025];
	int Len2 = 0, ret4 = 0, loc2 = 0;
	int fileLen2 = 0;
	int diff2 = 0;

	memset(TempBuf2, 0, sizeof(TempBuf2));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen2 = GetFileSize_Api("/ext/serial.txt");
	MAINLOG_L1("readfile = %d", fileLen2);
	memset(TempBuf2, 0, sizeof(TempBuf2));
	ret4 = ReadFile_Api("/ext/serial.txt", TempBuf2, loc2, &fileLen2);
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

	strcpy(G_sys_param.mqtt_topic, "aisino/dynamicQR/+/");
	strcat(G_sys_param.mqtt_topic, TempBuf2);
	G_sys_param.con = 0;
	MAINLOG_L1("MQTT topic = %s", G_sys_param.mqtt_topic);
	MAINLOG_L1("MQTT Connecting to server");

	memset(&client, 0, sizeof(MQTTClient));
	MQTTClientInit(&client, &network, 20000, pSendBuf, 1024, pReadBuf, 1024);

	client.defaultMessageHandler = onDefaultMessageArrived;

	network.mqttconnect = net_connect;
	network.mqttclose   = net_close;
	network.mqttread    = net_read;
	network.mqttwrite   = net_write;

	network.netContext = network.mqttconnect(NULL, G_sys_param.mqtt_server, G_sys_param.mqtt_port, 5000, 0, &err);
	if (network.netContext == NULL)
	{
		MAINLOG_L1("MQTT network.netContext == NUL, err = %d", err);
		MQTTDisconnect(&client);
		network.mqttclose(network.netContext);
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Tidak Ada Koneksi", CDISP);
		ScrDisp_Api(LINE2, 0, "Internet", CDISP);
		WaitEnterAndEscKey_Api(5);
		return;
	}

	data.willFlag = 0;
	data.MQTTVersion = 4; // 3.1.1
	data.clientID.cstring = G_sys_param.mqtt_client_id;
	data.username.cstring = "aisino";
	data.password.cstring = "uruzaisino@123";
	data.keepAliveInterval = G_sys_param.mqtt_keepalive;
	data.cleansession = 1;

	MAINLOG_L1("MQTT cleansession : %d", data.cleansession);
	ret = MQTTConnect(&client, &data);
	MAINLOG_L1("MQTTConnect : %d", ret);
	if (ret != 0)
	{
		MQTTDisconnect(&client);
		network.mqttclose(network.netContext);
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Gagal Konek Ke", CDISP);
		ScrDisp_Api(LINE2, 0, "Server", CDISP);
		WaitEnterAndEscKey_Api(12);
		return;
	}

	ret = MQTTSubscribe(&client, G_sys_param.mqtt_topic, G_sys_param.mqtt_qos, mqttTopicRouter);
	MAINLOG_L1("mqtt topic return");

	MAINLOG_L1("MQTTSubscribe : %d", ret);
	if (ret != 0)
	{
		MQTTDisconnect(&client);
		network.mqttclose(network.netContext);
		dev_disconnect();
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "Gagal Konek Ke", CDISP);
		ScrDisp_Api(LINE2, 0, "Server", CDISP);
		WaitEnterAndEscKey_Api(12);
		return;
	}

	MAINLOG_L1("MQTT Connected");

	
	int limit = 90;
	char str[256];
	while (1)
	{
		if (strcmp((char *)receivedTopic, "payment") == 0)
		{
			MAINLOG_L1("Checkpoint 1");
			sprintf(str, "%d", limit);
			sprintf(str + strlen(str), "s");
			ScrDisp_Api(LINE1, 0, str, CDISP);
			limit--;
			if (limit == 0)
			{
				MAINLOG_L1("MQTTYield : %d", ret);
				ScrCls_Api();
				ScrDisp_Api(LINE1, 0, "Timeout", CDISP);
				WaitEnterAndEscKey_Api(5);
				break;
			}
		}

		if (G_sys_param.con == 0)
		{
			int Result = 0;
			Result = GetKey_Api();
			if (Result == ESC || Result == ENTER)
			{
				MAINLOG_L1("Checkpoint 2");
				
				ScrCls_Api();
				MAINLOG_L1("Call mqttclose");
				break;
			}
			ret = MQTTYield(&client, 1000); // set heartbeat time
			if (ret < 0)
			{
				MAINLOG_L1("MQTTYield : Koneksi Terputus");
				// ScrCls_Api();
				// ScrDisp_Api(LINE1, 0, "Koneksi Terputus", CDISP);
				// WaitEnterAndEscKey_Api(12);
				break;
			}
			Delay_Api(10);
		}
		else if (G_sys_param.con == 1)
		{
			MAINLOG_L1("Checkpoint 3");
			ScrCls_Api();
			break;
		}
	}
	
	dev_disconnect();
	MQTTDisconnect(&client);
	network.mqttclose(network.netContext);
	MAINLOG_L1("Call mqttclose");
	return 0;
}

// extract third segment of given topic
// aisino/dynamicQR/payment/<serial_number>
// aisino/dynamicQR/pos/<serial_number>
// aisino/dynamicQR/registration/<serial_number>
// aisino/dynamicQR/nobu_nfc/<serial_number>
int extract_third_segment(const char *topic, char *third_segment) {
    if (!topic || !third_segment) {
        MAINLOG_L1("[TOK3] NULL input pointer");
        return 1;
    }

    char copy[512];
    strncpy(copy, topic, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    MAINLOG_L1("[TOK3] Original Topic: [%s]", copy);

    // Optional: Print hex of topic string to debug hidden characters
    for (int i = 0; copy[i] != '\0'; i++) {
        MAINLOG_L1("[TOK3] char[%d] = 0x%02X (%c)", i, (unsigned char)copy[i], copy[i]);
    }

    char *token = strtok(copy, "/");
    int count = 0;

    while (token != NULL) {
        count++;
        MAINLOG_L1("[TOK3] Token %d: %s", count, token);

        if (count == 3) {
            strncpy(third_segment, token, 99);
            third_segment[99] = '\0';
            MAINLOG_L1("[TOK3] ✅ Third segment extracted: %s", third_segment);
            return 0;
        }

        token = strtok(NULL, "/");
    }

    MAINLOG_L1("[TOK3] ❌ Not enough segments in topic");
    return 1;
}



void onTopicMessageArrivedNFC(MessageData *md) {
	network.mqttclose(network.netContext);

	MAINLOG_L1("MQTT NFC Arrived");
	unsigned char buf[512];
	MQTTMessage *m = md->message;

	memcpy(buf, m->payload, m->payloadlen);
	buf[m->payloadlen] = 0;

	cJSON *mqttBody = cJSON_Parse(buf);
	if (json == NULL) {
		MAINLOG_L1("MQTT parse json failed");
		return;
	}

	cJSON *amount = cJSON_GetObjectItem(mqttBody, "amount");
	if (cJSON_IsNumber(amount) || (amount->valueint64 == NULL)) {	
		MAINLOG_L1("MQTT parse amount failed");
		return;
	}
	MAINLOG_L1("amount = %d", amount->valueint64);

	cJSON *partnerReferenceNo = cJSON_GetObjectItem(mqttBody, "partnerReferenceNo");
	if ((partnerReferenceNo->valuestring == NULL)) {
		MAINLOG_L1("MQTT parse partnerReferenceNo failed");
		return;
	}
	MAINLOG_L1("partnerReferenceNo = %s", partnerReferenceNo->valuestring);

	CardNFC(partnerReferenceNo->valuestring, amount->valueint64);
}

// int mqttTopicRouter(MessageData *md) {
// 	MAINLOG_L1("mqtt Topic Router Run");

// 	unsigned char buf[512];
// 	MQTTMessage *m = md->message;
// 	memcpy(buf, md->topicName->lenstring.data, md->topicName->lenstring.len);
// 	buf[md->topicName->lenstring.len] = 0;
// 	MAINLOG_L1("MQTT recv topic:%s", buf);

// 	memcpy(buf, m->payload, m->payloadlen);
// 	buf[m->payloadlen] = 0;
// 	MAINLOG_L1("MQTT recv message:%s", buf);

// 	char third_segment[256];
// 	MAINLOG_L1("AKU DISINIII >>>>>>>");
// 	int ret = extract_third_segment(md->topicName->lenstring.data, third_segment);
// 	MAINLOG_L1("ret: %d", ret);
// 	if (ret != 0) {
// 		return -1;
// 	}
// 	MAINLOG_L1("third_segment23: %s", third_segment);

// 	strcpy(receivedTopic, third_segment);

// 	if (strcmp(third_segment, "payment-pos") == 0) {
// 		MAINLOG_L1("if payment-pos mqttTopicRouter");
// 		onTopicMessageArrivedPOS(md);
// 	} else if (strcmp(third_segment, "static") == 0) {
// 		onTopicMessageArrivedStatic(md);
// 	} else if (strcmp(third_segment, "display-qr") == 0) {
// 		onTopicMessageArrivedQRV2(md);
// 	} else if (strcmp(third_segment, "nobu_nfc") == 0) {
// 		onTopicMessageArrivedNFC(md);
// 	} 

// 	return 0;
// }

int mqttTopicRouter(MessageData *md) {
	MAINLOG_L1("mqtt Topic Router Run");

	unsigned char buf[512];
	MQTTMessage *m = md->message;

	memcpy(buf, md->topicName->lenstring.data, md->topicName->lenstring.len);
	buf[md->topicName->lenstring.len] = 0;
	MAINLOG_L1("MQTT recv topic:%s", buf);

	memcpy(buf, m->payload, m->payloadlen);
	buf[m->payloadlen] = 0;
	MAINLOG_L1("MQTT recv message:%s", buf);

	char third_segment[256];
	MAINLOG_L1("AKU DISINIII >>>>>>>");
	int ret = extract_third_segment(md->topicName->lenstring.data, third_segment);
	MAINLOG_L1("ret: %d", ret);
	if (ret != 0) {
		return -1;
	}
	MAINLOG_L1("third_segment23: %s", third_segment);

	strcpy(receivedTopic, third_segment);

	if (strcmp(third_segment, "payment-pos") == 0) {
		int found = 0;

		if (enabled_menu != NULL) {
			char *menu_str = cJSON_Print(enabled_menu);
			MAINLOG_L1("enabled_menu content = %s", menu_str);
			free(menu_str); 
			cJSON *arr = cJSON_GetObjectItem(enabled_menu, "enabled_menu");
			if (arr && cJSON_IsArray(arr)) {
				int count = cJSON_GetArraySize(arr);
				for (int i = 0; i < count; i++) {
					cJSON *item = cJSON_GetArrayItem(arr, i);
					if (item && strcmp(item->valuestring, "POS QR") == 0) {
						found = 1;
						break;
					}
				}
			}
		}

		if (found) {
			MAINLOG_L1("POS QR ditemukan di enabled_menu");
			onTopicMessageArrivedPOS(md);
		} else {
			MAINLOG_L1("POS QR tidak ditemukan di enabled_menu. Abaikan.");
		}
	}
	else if (strcmp(third_segment, "static") == 0) {
		onTopicMessageArrivedStatic(md);
	}
	else if (strcmp(third_segment, "display-qr") == 0) {
		onTopicMessageArrivedQRV2(md);
	}
	else if (strcmp(third_segment, "nobu_nfc") == 0) {
		onTopicMessageArrivedNFC(md);
	}

	return 0;
}


