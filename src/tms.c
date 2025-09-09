/*
 * tms.c
 *
 *      Author: vanstone
 */
#include "stdio.h"
#include "../inc/def.h"
#include <coredef.h>
#include <struct.h>
#include <poslib.h>
#include "../inc/httpDownload.h"

int tms_download_flag = 0;
static int tmsPlayTimer = -1;
static int tmsTotleTime = TMSMAXTIME;

int fileFilter = -1;
int needUpdate = -1;
unsigned char updateAppName[32];

int tmsRestTimer()
{
	tmsPlayTimer = TimerSet_Api();
}
int tmsCheckOnceTimer()
{
	if(tmsPlayTimer == -1)
	{
		tmsRestTimer();
		return 0;
	}

	return TimerCheck_Api(tmsPlayTimer, TMSPLAYONCETIME*1000);
}
int tmsCheckUpdate()
{
	char tips[128];

	if(tmsCheckOnceTimer())
	{
		if(tmsTotleTime == TMSMAXTIME)
		{
			tmsTotleTime -= TMSPLAYONCETIME;
			sprintf(tips, "New app found, update in %d seconds", tmsTotleTime);
			AppPlayTip(tips);
		}
		else
		{
			tmsTotleTime -= TMSPLAYONCETIME;
			if(tmsTotleTime <= 0){
				return 0; // time is up, start updating
			}
			if(tmsTotleTime%10 == 0)
			{
				sprintf(tips, "%d Seconds to update", tmsTotleTime);
				AppPlayTip(tips);
			}
		}
		tmsRestTimer();
	}
	return 2;
}

int checkAppUpdate(){
	int ret = -1;

	CheckAppFile();

	fileFilter = 3;
	folderFileDisplay("/ext/myfile/appfile");

	MAINLOG_L1("needUpdate == %d",needUpdate);
	if(needUpdate==1){
		while(1){
			ret = tmsCheckUpdate();
			if(ret==0)
				break;
		}

		ret = WriteFile_Api(SAVE_UPDATE_FILE_NAME, updateAppName, 0, strlen(updateAppName));
		AppPlayTip("start updating");
		ret = tmsUpdateFile_lib(TMS_FLAG_APP, updateAppName, NULL);
		MAINLOG_L1("tmsUpdateFile_lib = %d, file name = %s", ret, updateAppName);
		MAINLOG_L1("tmsUpdateFile_lib = %d, file name = %s", TMS_FLAG_APP, updateAppName);

		if(ret<0)
			DelFile_Api(SAVE_UPDATE_FILE_NAME);

	}else {
		return 2; //without app to update
	}
	return ret;
}

// type is to be defined for different download tasks.
// here we just use non-zero value to indicate a download action.
void set_tms_download_flag(int type)
{
	tms_download_flag = type;
}

int get_tms_download_flag(){
	return tms_download_flag;
}

int fileGetFileListCB_lib(char *pchDirName, void (*cb)(const char *pchDirName, uint32 size, uint8 filetype, void *arg), void *args);

void fileGetListCbTest(const char *pchDirName, uint32 size, uint8 filetype, void *arg)
{
	sysLOG(0, "size = %d,filetype=%d,pchDirName:%s, arg=%s\r\n", size, filetype, pchDirName, arg);
}


void TMSThread(void)
{
	int ret;
	long long tick1, tick2;

	while (1) {
		while (tms_download_flag == 0) {
			MAINLOG_L1("no tms task yet");
			Delay_Api(1000);
		}

		DelFile_Api(TMS_DOWN_FILE);
		// fileGetFileListCB_lib("/ext/images");
		// fileGetFileListCB_lib("/ext/hindi");
		// fileGetFileListCB_lib("/ext/myfile");

		tick1 = sysGetTicks_lib(); //ms
//		ret = httpDownload("https://support-f.vanstone.com.cn/resource/r_f318b0f7833e440cb81342a66a0cca32/English.zip", METHOD_GET, TMS_DOWN_FILE);
//		ret = httpDownload("https://demo1.ezetap.com/portal/device/firmware/download/ff3a2d4e-f2f0-4af2-83ac-6f396df90e0d", METHOD_GET, TMS_DOWN_FILE);
		// ret = httpDownload("http://cool.uruz.id/myfile.zip", METHOD_GET, TMS_DOWN_FILE);
		ret = httpDownload("http://cool.uruz.id/Q161Pro.zip", METHOD_GET, TMS_DOWN_FILE);


		MAINLOG_L1("Q161Demo: httpDownload ret:%d  downFile.zip size = %d  tick time:%lld", ret, GetFileSize_Api(TMS_DOWN_FILE), sysGetTicks_lib()-tick1);

		if (ret > 0) {
			// Prepare for update after reboot, may need to prompt user
			ret = unzipDownFile(TMS_DOWN_FILE);

			//or check update app when terminal power on
			ret = checkAppUpdate();
			if (ret==2) {
				MAINLOG_L1("without new app to update");
			}
		}else{
			AppPlayTip("Download fail");
			DelFile_Api(TMS_DOWN_FILE);
		}

		tms_download_flag = 0;
	}
}
