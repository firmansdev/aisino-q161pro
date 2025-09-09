#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/def.h"
#include "../inc/tms_tms.h"
#include "../inc/tms_md5.h"
#include "../inc/display.h"
#include "../inc/mqtt.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#define WIFI_ALERT 1

const APP_MSG App_Msg = {
		"Demo",
		"Demo-App",
		"V.2.0.0",
		"VANSTONE",
		__DATE__ " " __TIME__,
		"",
		0,
		0,
		0,
		"00001001140616"};
#define ERR_NO_CONTENTLEN -1
#define ERR_CONLEN_FORMAT_WRONG -2
int _IS_GPRS_ENABLED_ = 0;

unsigned char TMS_UPDATE_TYPE[3];

void alert_wifi();

URL_ENTRY tmsEntry;

int PMflag = 1; // 1-English    2-Arabic
void MenuThread();
cJSON *enabled_menu = NULL;

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

void InitSys(void)
{

	// DelFile_Api("/ext/myfile/sslfile/ca_uruz.crt");
	// DelFile_Api("/ext/myfile/sslfile/privkey_uruz.pem");
	// DelFile_Api("/ext/myfile/sslfile/cli_uruz.crt");

	int ret;
	unsigned char bp[32];
	char disBuff[5];
	unsigned char recv[512];
	RTC_time rtcTime;
	int sock = -1;

	// Turn on debug output to serial COM
	SetApiCoreLogLevel(1); // SetApiCoreLogLevel(5);

	// Load parameters
	initParam();

	// initializes device type
	initDeviceType();

	piccLight_lib(0x01, 1);
	Delay_Api(1000);
	piccLight_lib(0x01, 0);
	Delay_Api(1000);

	piccLight_lib(0x02, 1);
	Delay_Api(1000);
	piccLight_lib(0x02, 0);
	Delay_Api(1000);

	piccLight_lib(0x03, 1);
	Delay_Api(1000);
	piccLight_lib(0x03, 0);
	Delay_Api(1000);

	piccLight_lib(0x04, 1);
	Delay_Api(1000);
	piccLight_lib(0x04, 0);
	Delay_Api(1000);

#ifdef __WIFI__
	connectWifi();
#else

	// Turn off WIFI and turn on 4G network
	NetModuleOper_Api(WIFI, 0);
	NetModuleOper_Api(GPRS, 1);

#endif

	// Network initialization
	net_init();

	// MQTT client initialization
	initMqttOs();

	ret = fibo_thread_create(MenuThread, "mainMenu", 14 * 1024, NULL, 24);
	MAINLOG_L1("fibo_thread_create: %d", ret);

#ifdef __TMSTHREAD__
	ret = fibo_thread_create(TMSThread, "TMSThread", 14 * 1024, NULL, 24);
	MAINLOG_L1("fibo_thread_create: %d", ret);
#endif
	// check if any app to update
	// ret = checkAppUpdate();
	// if(ret<0)
	// 	set_tms_download_flag(1);

	// secscrOpen_lib();

	// //CTLS card part initialization
	// Common_Init_Api();

	// DelFile_Api("ComLog.dat");
#ifdef APDU_DEBUG
	Common_DbgEN_Api(1);
#else
	Common_DbgEN_Api(0);
#endif

	PayPass_Init_Api();
	PayWave_Init_Api();
	AddCapkExample();
	PayPassAddAppExp(0);
	PayWaveAddAppExp();

	// Common_DbgEN_Api(1);

	memset(bp, 0, sizeof(bp));
	ret = sysReadBPVersion_lib(bp);
	MAINLOG_L1("Firmware Version == %d, version: %s", ret, bp);

	memset(bp, 0, sizeof(bp));
	ret = sysReadVerInfo_lib(5, bp);
	//	MAINLOG_L1("lib Version == %d, version: %s, APP Version: %s", ret, bp, App_Msg.Version);
	MAINLOG_L1("Version: %s", bp);

	apply_sync_menu_from_file();
	PiccOpen_Api();

	// buat thread yang bilang "Please connect to wifi" setiap 30 detik kalo gak terhubung
#ifdef WIFI_ALERT
	ret = fibo_thread_create(alert_wifi, "alert_wifi", 14 * 1024, NULL, 24);
	MAINLOG_L1("fibo_thread_create alert wifi: %d", ret);
#endif
}

void alert_wifi()
{
	while (1)
	{
		Delay_Api(30 * 1000);

		if (wifiGetLinkStatus_lib() != 2)
		{
			AppPlayTip("Please connect this device to wifi");
		}
	}
}

void init_menus()
{
	MAINLOG_L1("Masuk init_menus()");

	enabled_menu = cJSON_CreateObject();
	cJSON *menu_array = cJSON_CreateArray();

	for (int i = 0; i < pszItemsCount; i++)
	{
		MAINLOG_L1("Inserting menu item: %s", pszItems[i]);
		cJSON_AddItemToArray(menu_array, cJSON_CreateString(pszItems[i]));
	}

	MAINLOG_L1("Inserting menu item into enabled_menu");
	cJSON_AddItemToObject(enabled_menu, "enabled_menu", menu_array);

	MAINLOG_L1("enabled_menu location from init_menus() = %p", enabled_menu);

	char *json_str = cJSON_Print(enabled_menu);
	MAINLOG_L1("json_str = %s", json_str);
	free(json_str);
}

void apply_sync_menu_from_file()
{
	char str_buffer[1025];
	memset(str_buffer, 0, sizeof(str_buffer));

	int ret = 0;
	int fileLen = 0;
	int loc = 0;

	fileLen = GetFileSize_Api("/ext/sync_menu.txt");
	ret = ReadFile_Api("/ext/sync_menu.txt", str_buffer, loc, &fileLen);
	// init_menus();

	MAINLOG_L1("sync_menu ret = %d", ret);
	MAINLOG_L1("sync_menu str_buffer = %s", str_buffer);

	if (ret != 0)
	{
		init_menus();
		return;
	}

	cJSON *menuJson = cJSON_Parse(str_buffer);

	cJSON *enabled_menu_array = cJSON_GetObjectItem(menuJson, "enabled_menu");

	if (menuJson != NULL && cJSON_IsArray(enabled_menu_array))
	{
		MAINLOG_L1("Assign dari sync_menu.txt");
		enabled_menu = menuJson;
	}
	else
	{
		init_menus();
		cJSON_Delete(menuJson);
	}
}

void connectWifi()
{
	int ret;
	unsigned char bp[32];
	char disBuff[5];
	unsigned char recv[512];
	RTC_time rtcTime;
	int sock = -1;
	int ssidwifi = CheckSSIDwifi();
	if (ssidwifi == 1)
	{
		char TempBuf[1025];
		int Len = 0, ret = 0, loc = 0;
		int fileLen = 0;
		int diff = 0;
		memset(TempBuf, 0, sizeof(TempBuf));
		portClose_lib(10);
		portOpen_lib(10, NULL);
		portFlushBuf_lib(10);
		fileLen = GetFileSize_Api("/ext/ssid.txt");
		MAINLOG_L1("readfile = %d", fileLen);
		memset(TempBuf, 0, sizeof(TempBuf));
		ret = ReadFile_Api("/ext/ssid.txt", TempBuf, loc, &fileLen);
		MAINLOG_L1("readfile = %d", ret);
		MAINLOG_L1("readfile = %s", TempBuf);
		char TempBuf2[1025];
		int Len2 = 0, ret2 = 0, loc2 = 0;
		int fileLen2 = 0;
		int diff2 = 0;

		memset(TempBuf2, 0, sizeof(TempBuf2));
		portClose_lib(10);
		portOpen_lib(10, NULL);
		portFlushBuf_lib(10);
		fileLen2 = GetFileSize_Api("/ext/pass.txt");
		MAINLOG_L1("readfile = %d", fileLen2);
		memset(TempBuf2, 0, sizeof(TempBuf2));
		ret2 = ReadFile_Api("/ext/pass.txt", TempBuf2, loc2, &fileLen2);
		MAINLOG_L1("readfile = %d", ret2);
		MAINLOG_L1("readfile = %s", TempBuf2);
		int ret3 = wifiAPConnect_lib(TempBuf, TempBuf2);
		if (ret3 == -6323)
		{ // incorrect password
			//				goto PWD;
		}
		MAINLOG_L1("wifi_wifiAPConnect_lib_1:%d", ret3);
		if (ret3 == 0)
		{
			// get network time
			WifiespSetTimezone_lib(32);
			ret = wifiCtrl_lib(0, "edu.ntp.org.cn", 13, recv, 512);
			ret = wifiCtrl_lib(1, "edu.ntp.org.cn", 13, recv, 512);
			ret = sysGetRTC_lib(&rtcTime); // get system time
		}
	}
	else
	{
		ret = WifiConnect();
		if (ret == 0)
		{
			// get network time
			WifiespSetTimezone_lib(32);
			ret = wifiCtrl_lib(0, "edu.ntp.org.cn", 13, recv, 512);
			ret = wifiCtrl_lib(1, "edu.ntp.org.cn", 13, recv, 512);
			ret = sysGetRTC_lib(&rtcTime); // get system time
		}
	}
}

void GetIccID()
{
	int ret;

	unsigned char ICCID[13];

	memset(ICCID, 0, sizeof(ICCID));
	ret = wirelessGetccID_lib(ICCID);
	MAINLOG_L1("ICCID === %d:%s", ret, ICCID);
}

int AppMain(int argc, char **argv)
{
	int ret;
	int signal_lost_count;
	int mobile_network_registered;

	SystemInit_Api(argc, argv);

	mainThreadID = fibo_thread_id();

	InitSys();

	unsigned char *serialNumber[2];
	unsigned char result;
	result = PEDReadPinPadSn_Api(serialNumber);
	MAINLOG_L1("serial = %s", serialNumber);
	saveSSID("/ext/serial.txt", serialNumber);

	signal_lost_count = 0;
	mobile_network_registered = 0;

	while (1)
	{
#ifdef __WIFI__

#else

#endif
	}

	return 0;
}
