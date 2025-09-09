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
#include <coredef.h>
#include <struct.h>
#include <poslib.h>
#include <cJSON.h>
#include "../inc/httpDownload.h"

#define QRBMP "QRBMP.bmp"



void DispMainFaceNobu(void)
{
	int nSelcItem = 1, ret;
//	ScrClsRam_Api();
//	ScrDispRam_Api(LINE2, 0, "URUZ SOLUSI TEKNOLOGI", CDISP);
//	ScrDispRam_Api(LINE3, 0, "QR Nobu", CDISP);
//	ScrBrush_Api();
	char *pszTitle = "QR Nobu";
	const char *pszItems[] = {
		"1.Input Nominal" ,
	};
    char postData[200];
	while(1)
	{
		nSelcItem = ShowMenuItem(pszTitle, pszItems, sizeof(pszItems)/sizeof(char *), DIGITAL1,DIGITAL1, 0, 60);
		MAINLOG_L1("ShowMenuItem = %d %d %d %d %d %d",nSelcItem, DIGITAL1);
		switch(nSelcItem)
		{
		case DIGITAL1:
//			sprintf(postData, "{\"referenceNo\":\"%s\",\"amount\":%d,\"validTime\":%d,\"source\":\"%s\",\"branch_id\":\"%s\"}", "cooluruz", 5000, 900, "aisino", "PUSAT");
//			ret = httpDownload(postData, "https://cool.uruz.id/api/trans-nobu/qrAisino", METHOD_POST, TMS_DOWN_FILE);
//
//			char ret_str[100]; // Make sure the buffer is large enough to hold the string representation
//			sprintf(ret_str, "%d", postData);
//
//			TipAndWaitEx_Api(ret_str);
			break;
		case ESC:
			return ;
		default:
			break;
		}
	}
}

void GetScanfTest2(){

	u8 buf[10];
	memset(buf, 0, sizeof(buf));

	GetScanfEx_Api(MMI_NUMBER|MMI_LETTER, 1, 10, buf, 60, LINE3, LINE3, 10, MMI_LETTER|MMI_NOSCRSCAN);
}

int WaitEvent2(void)
{
	u8 Key;
	int TimerId;

	TimerId = TimerSet_Api();
	while(1)
	{
		if(TimerCheck_Api(TimerId , 30*1000) == 1)
		{
			SysPowerStand_Api();
			return 0xfe;
		}
		Key = GetKey_Api();
		if(Key != 0)
			return Key;
	}

	return 0;
}

void QRDispTest2()
{
	int ret;

	unsigned char *qrStr = "00020101021226670016COM.NOBUBANK.WWW01189360050300000886180214120500006898500303UMI51440014ID.CO.QRIS.WWW0215ID20222327903540303UMI520454995303360540115802ID5922URUZ MANAGEMENT SYSTEM6013Jakarta Barat61051171062620114050306158826620625613bc5b0-08fd-11ef-b5b7-00703A010804POSP630461C3";

	ret = QREncodeString(qrStr, 20, 5, QRBMP, 4);
	ScrCls_Api();
	ScrDispImage_Api(QRBMP, 80, 80);

	TipAndWaitEx_Api("Press to show next QR");

//	ret = QREncodeString(qrStr, 15, 3, QRBMP, 2);
//	ScrCls_Api();
//	ScrDispImage_Api(QRBMP, 10, 10);
//
//	TipAndWaitEx_Api("Press to show logo");

//	ScrDispImage_Api(AISINO_LOGO, 86, 94);
}
