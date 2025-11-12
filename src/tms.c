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
	folderFileDisplay("/ext/q161pro/appfile");

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

void safeRemoveDir(const char *path) {
    int ret;

    ret = fileGetFileListCB_lib((char *)path, NULL, NULL);

    if (ret < 0) {
        MAINLOG_L1("[safeRemoveDir] Skip, directory not exist or empty: %s", path);
        return;
    }

    MAINLOG_L1("[safeRemoveDir] Cleaning directory: %s", path);
    removeDirWithContent(path);
}


void TMSThread(void)
{
    int ret;
    long long tick1, tick2;

    while (1) {
        while (tms_download_flag == 0) {
            MAINLOG_L1("[TMSThread] No TMS task yet...");
            Delay_Api(1000);
        }
        MAINLOG_L1("[TMSThread] Start TMS download task...");

        DelFile_Api(TMS_DOWN_FILE);
        MAINLOG_L1("[TMSThread] DelFile_Api(%s) ret=%d", TMS_DOWN_FILE, ret);

        MAINLOG_L1("[TMSThread] Cleaning old directories...");
        // removeDirWithContent("/ext/images");
        // removeDirWithContent("/ext/hindi");
        // removeDirWithContent("/ext/q161pro");

        // Mulai download file baru
        tick1 = sysGetTicks_lib();
        MAINLOG_L1("[TMSThread] Downloading new package...");

        ret = httpDownload("http://cool.uruz.id/q161pro.zip", METHOD_GET, TMS_DOWN_FILE);

        tick2 = sysGetTicks_lib();
        MAINLOG_L1("[TMSThread] httpDownload ret=%d, file=%s, size=%d, elapsed=%lld ms",
                   ret, TMS_DOWN_FILE, GetFileSize_Api(TMS_DOWN_FILE), tick2 - tick1);
			
        if (ret > 0) {
            MAINLOG_L1("[TMSThread] Download success, start unzip...");

            // Extract file
            ret = unzipDownFile(TMS_DOWN_FILE);
            MAINLOG_L1("[TMSThread] unzipDownFile ret=%d", ret);

            // Check update app
            ret = checkAppUpdate();
            MAINLOG_L1("[TMSThread] checkAppUpdate ret=%d", ret);

            if (ret == 2) {
                MAINLOG_L1("[TMSThread] No new app to update");
            }
        } else {
            MAINLOG_L1("[TMSThread] Download failed!");
            AppPlayTip("Download fail");
            DelFile_Api(TMS_DOWN_FILE);
        }

        MAINLOG_L1("[TMSThread] TMS task finished");
        tms_download_flag = 0;
    }
}
