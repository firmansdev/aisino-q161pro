/*
 * param.c
 *
 *  Created on: 2021��10��14��
 *      Author: vanstone
 */
#include <string.h>

#include "../inc/def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

SYS_PARAM G_sys_param;
int Device_Type;

#ifdef __WIFI__
unsigned char wifiId[32];
unsigned char wifiPwd[32];
#endif

#define MQTT_SERVER "108.137.21.51"
#define MQTT_PORT "1883"

static void readSN(char *sn)
{
	int ret;
	unsigned char buf[40];

	ret = PEDReadPinPadSn_Api(buf);
	if(ret != 0)
	{
		// Serial number lost, warning and poweroff.
		AppPlayTip("Serial number lost, power off now");

		WaitAnyKey_Api(10);
		SysPowerOff_Api();

		while (1);
	}

	ret = AscToLong_Api(buf, 2);
	buf[ret + 2] = 0;

	strcpy(sn, (char *)(buf + 2));
}

void initParam(void){
	int ret;
	unsigned int len;

	/*ret = GetFileSize_Api(SYS_PARAM_NAME);
	if (ret == sizeof(SYS_PARAM)) {
		len = ret;
		ret = ReadFile_Api(SYS_PARAM_NAME, (unsigned char *)&G_sys_param, 0, &len);
		if ((ret == 0) && (len == sizeof(SYS_PARAM))) {
			readSN(G_sys_param.sn);

			return;
		}
	}*/

	// Create default param
	memset(&G_sys_param, 0, sizeof(SYS_PARAM));

	readSN(G_sys_param.sn);

//	strcpy(G_sys_param.mqtt_server, "115.159.28.147");
//	strcpy(G_sys_param.mqtt_port, "1883");
//	G_sys_param.mqtt_ssl = 2;
	G_sys_param.mqtt_qos = 0;
//	sprintf(G_sys_param.mqtt_topic, "topic_%s", G_sys_param.sn);
//	sprintf(G_sys_param.mqtt_client_id, "clientId-%s", G_sys_param.sn); //"clientId-u43acUJtL7"
//
	G_sys_param.mqtt_keepalive = 60;
	G_sys_param.sound_level = 1;
//
//	MAINLOG_L1("G_sys_param.mqtt_keepalive init = %d", G_sys_param.mqtt_keepalive);
//	MAINLOG_L1("mqtt_client_id = %s", G_sys_param.mqtt_client_id);
//	MAINLOG_L1("mqtt_topic = %s", G_sys_param.mqtt_topic);
//	MAINLOG_L1("terminal SN = %s", G_sys_param.sn);


	strcpy(G_sys_param.mqtt_server, MQTT_SERVER);
	strcpy(G_sys_param.mqtt_port, MQTT_PORT);
	G_sys_param.mqtt_ssl = 2;

//	strcpy(G_sys_param.mqtt_topic, "home/firman/firman");
	snprintf(G_sys_param.mqtt_client_id, sizeof(G_sys_param.mqtt_client_id), "aisino-%s", G_sys_param.sn); // unique client id per device
//	strcpy(G_sys_param.publish_topic, "topic_1");

//	sprintf(G_sys_param.mqtt_topic, "topic/soundbox-%s", G_sys_param.sn) ;
//	sprintf(G_sys_param.mqtt_client_id, "Mswipe-Soundbox-%s", G_sys_param.sn);
//	G_sys_param.mqtt_qos = 0;
//	G_sys_param.mqtt_keepalive = 60;
//	G_sys_param.sound_level= 1;

	saveParam();
}

void saveParam(void)
{
//	SaveWholeFile_Api(SYS_PARAM_NAME, (unsigned char *)&G_sys_param, sizeof(SYS_PARAM));
}

void initDeviceType(){
	char machineType[26];
	int Result = 0,ret;

	memset(machineType,0,sizeof(machineType));
	Result = sysGetTermType_lib(&machineType);
	if (Result>=0)
	{
		MAINLOG_L1("device type : %s", machineType);

		ret = memcmp(machineType,"Q180D",5);
		if (ret==0)
		{
			MAINLOG_L1("define Q180D");
			Device_Type = 2;
		}

		ret = memcmp(machineType,"Q190",4);
		if (ret==0)
		{
			MAINLOG_L1("define Q190");
			Device_Type = 3;
			QRCodeDisp();
		}
	}
}

int readWifiParam()
{
#ifdef __WIFI__
	int ret, len = 64;
	char buf[65];
	unsigned char *ptr = NULL;

	memset(buf, 0, sizeof(buf));
//	ret = ReadFile_Api("/ext/wifiparam.dat", buf, 0, &len);
//	if(ret != 0)
//	{
//		MAINLOG_L1("ReadFile_Api wifiparam:%d", ret);
//		return ret;
//	}
//
//	ptr = strstr(buf, ",");
//	memcpy(wifiPwd, ptr+1, strlen(ptr+1));
//	memcpy(wifiId, buf, strlen(buf)-strlen(ptr));
	memcpy(wifiId, "JW-B317", 7);
	memcpy(wifiPwd, "jiewen@8804", 11);
	MAINLOG_L1("wifiId:%s:  wifiPwd:%s:", wifiId, wifiPwd);
	return 0;
#endif
}

