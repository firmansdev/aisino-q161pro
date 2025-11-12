#include <string.h>

#include "../inc/def.h"
#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#define ZIPPATH "/ext/"


u8* filename(u8* file)
{
	return file;
}

void filegetlistcbtesting(const char *pchDirName, uint32 size, uint8 filetype, void *arg)
{
	char *pChar;
	int ret,num;

//	MAINLOG_L1("[%s] -%s- Line=%d: size = %d,filetype=%d,pchDirName:%s, arg=%s\r\n", filename(__FILE__), __FUNCTION__, __LINE__,size, filetype, pchDirName, arg);

	//delete all mp3 files
	if (fileFilter==1) {
		pChar = strstr(pchDirName,".mp3");
		if (pChar!=NULL) {
			DelFile_Api(pchDirName);
		}
	}

	//delete all .img files
	if (fileFilter==2) {
		pChar = strstr(pchDirName,".img");
		if (pChar!=NULL) {
			DelFile_Api(pchDirName);
		}
	}

	//search app to update
	if (fileFilter==3) {
		pChar = strstr(pchDirName,".img");
		if (pChar!=NULL) {
			num = 9;
			memset(updateAppName,0,sizeof(updateAppName));
			memcpy(updateAppName,pchDirName,strlen(pchDirName));
			MAINLOG_L1("pchDirName = %s",pchDirName);
			needUpdate = 1;
		}
	}

}
void folderFileDisplay(unsigned char *filePath)
{
	int iRet = -1;

	uint8 *rP = NULL;

	iRet = fileGetFileListCB_lib(filePath, filegetlistcbtesting, rP);

}

int fileGetFileListCB_lib(
    char *pchDirName,
    void (*cb)(const char *pchDirName, uint32 size, uint8 filetype, void *arg),
    void *args
);

void fileGetListCbTest(const char *pchDirName, uint32 size, uint8 filetype, void *arg)
{
	MAINLOG_L1(0, "size = %d,filetype=%d,pchDirName:%s, arg=%s\r\n", size, filetype, pchDirName, arg);
}

void removeFileCb(const char *pchDirName, uint32 size, uint8 filetype, void *arg)
{
    if (filetype == 0) {
        
        int ret = DelFile_Api(pchDirName);
        MAINLOG_L1(0, "Delete file %s, ret=%d\r\n", pchDirName, ret);
    } else if (filetype == 1) {
      
        fileGetFileListCB_lib((char *)pchDirName, removeFileCb, NULL);
        int ret = fileRmdir_lib(pchDirName);
        MAINLOG_L1(0, "Remove folder %s, ret=%d\r\n", pchDirName, ret);
    }
}
int removeDirWithContent(const char *path)
{

    int ret = fileGetFileListCB_lib((char *)path, removeFileCb, NULL);
    MAINLOG_L1(0, "removeDirWithContent path=%s, ret=%d\r\n", path, ret);

    ret = fileRmdir_lib(path);
    MAINLOG_L1(0, "Final remove folder %s, ret=%d\r\n", path, ret);

	return ret;
}

void logFileCb(const char *pchDirName, uint32 size, uint8 filetype, void *arg)
{
    if (filetype == 0) {
        MAINLOG_L1(0, "[LOG] File: %s (size=%u)\r\n", pchDirName, size);
    } else if (filetype == 1) {
        MAINLOG_L1(0, "[LOG] Folder: %s\r\n", pchDirName);
        fileGetFileListCB_lib((char *)pchDirName, logFileCb, NULL);
    }
}



int unzipDownFile(unsigned char *fileName){
    int ret;

    MAINLOG_L1("[unzipDownFile] Start unzip a process for %s", fileName);

    MAINLOG_L1("[unzipDownFile] Cleaning directory /ext/q161pro ...");
	MAINLOG_L1("[unzipDownFile] ZIPPATH = %s", ZIPPATH);

	// ret = removeDirWithContent("/ext/q161pro");
	// MAINLOG_L1("[unzipDownFile] removeDirWithContent ret=%d", ret);

    ScrCls_Api();
    ScrDisp_Api(LINE4, 0, "Extracting...", CDISP);

    AppPlayTip("Extracting");

	MAINLOG_L1("[unzipDownFile] fileName = %s", fileName);
	MAINLOG_L1("[unzipDownFile] ZIPPATH = %s", ZIPPATH);
    ret = fileunZip_lib(fileName, ZIPPATH);
    MAINLOG_L1("[unzipDownFile] fileunZip_lib ret=%d", ret);

    if(ret != 0){
        MAINLOG_L1("[unzipDownFile] Unzip FAILED for %s", fileName);
        AppPlayTip("Extract file fail");
        return -1;
    }

    MAINLOG_L1("[unzipDownFile] Unzip SUCCESS for %s", fileName);
    AppPlayTip("Extract file success");

    MAINLOG_L1("[unzipDownFile] Deleting zip file %s ...", fileName);
    DelFile_Api(fileName);
    MAINLOG_L1("[unzipDownFile] Finished unzip process");
    return 0;
}

void CheckAppFile(){
	int ret;
	unsigned char buf[32];

	ret = GetFileSize_Api(SAVE_UPDATE_FILE_NAME);
	MAINLOG_L1("GetFileSize_Api = %d", ret);
	if (ret>0) {
		memset(buf,0,sizeof(buf));
		ReadFile_Api(SAVE_UPDATE_FILE_NAME, buf, 0, (unsigned int *)&ret);
		MAINLOG_L1("buf = %s", buf);
		DelFile_Api(buf);
		DelFile_Api(SAVE_UPDATE_FILE_NAME);
	}
}




