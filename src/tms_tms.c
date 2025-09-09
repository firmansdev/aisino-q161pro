#include <coredef.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define EXTERN extern

#include <struct.h>
#include <poslib.h>

#include "../inc/def.h"
#include "../inc/tms_md5.h"
#include "../inc/tms_tms.h"

#define ERR_NO_CONTENTLEN       -1
#define ERR_CONLEN_FORMAT_WRONG -2

unsigned char TMS_UPDATE_TYPE[3];

static void *s;
static unsigned char *t_sBuf=NULL;
static unsigned char *t_rBuf=NULL;

int tms_tms_flag = -1;
int NO_UPDATE = 0;

URL_ENTRY tmsEntry;
// ======================================== TMS PART ========================================
int TmsUpdate_Api(u8 *appCurrVer)
{
	MAINLOG_L1("TMS: TmsUpdate_Api()");

	int i, len, ret;
	char tmp[64], buf[64], *p = NULL;

	struct __TmsTrade__ GFfile;

	len = sizeof(struct __TmsTrade__);

	ret = ReadFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, (unsigned int*)&len);
	MAINLOG_L1("TMS: ReadFile_Api() = %d", ret);

	if (ret != 0) { return -1; }

	for (i = 0; i < GFfile.fnum; i++) {
		if (GFfile.file[i].status == STATUS_DLUNCOMPLETE) { return -2; }
	}

	// ========== Update Font & Parameter Files
	for (i = 0; i < GFfile.fnum; i++) {

		if (GFfile.file[i].status == STATUS_DLCOMPLETE) { // downloading finished but not replaced

			if (strcmp(GFfile.file[i].type, TYPE_PARAM) == 0) {
				memset(tmp, 0, sizeof(tmp)); // .bak name
				memset(buf, 0, sizeof(buf)); // old name

				sprintf(buf, "%s%s", TMS_FILE_DIR, GFfile.file[i].name);
				sprintf(tmp, "%s.bak", buf);

				ret = DelFile_Api(buf);
				MAINLOG_L1("TMS: DelFile_Api() = %d", ret);

				ret = ReNameFile_Api(tmp, buf);
				MAINLOG_L1("TMS: ReNameFile_Api() = %d", ret);

				if (ret != 0) {
					ret = ReNameFile_Api(tmp, buf);
					MAINLOG_L1("TMS: ReNameFile_Api() = %d", ret);
				}

				// -------------------- Update MP3
				if (strcmp(TMS_UPDATE_TYPE, TMS_CLIENT_UPDATE_TYPE_MP3) == 0) {
					if (ret == 0) {
						char info[72] = "";
						sprintf(info, "FILE_NAME[%d] = %s", i, GFfile.file[i].name);
						MAINLOG_L1(info);

						char *p = strstr(GFfile.file[i].name, ".zip");

						if (p != NULL) {
							char *result    = strtok(GFfile.file[i].name, ".");
							char *file_name = NULL;

							while (result != NULL) {
								file_name = result;
								break;
							}

							char info[72] = "";
							sprintf(info, "file_name = %s", file_name);
							MAINLOG_L1(info);

							file_name = strupr(file_name);

							// Double Check
							if (strcmp(file_name, TMS_UPDATE_TYPE) == 0) {
								SetTmsUpdateProgress(LINE7, "Find New Version");

								char tmp[48] = "";
								sprintf(tmp, "DOWNLOAD FILE PATH = %s", buf);
								MAINLOG_L1(tmp);

								Delay_Api(1000);

								ScrClrLine_Api(LINE7, LINE7);

								char *pos = NULL;
								pos = strstr(buf, "english");

								// English
								if (pos != NULL)
								{
									SetTmsUpdateProgress(LINE6, "Updating English MP3");

									ret = fileunZip_lib((unsigned char*)buf, ENGLISH_AUDIOS_PATH);
									if (ret == 0) {
										SetTmsUpdateProgress(LINE6, "Update Successful");
									} else {
										char info[24] = "";
										sprintf(info, "Update Failed(%d)", ret);

										SetTmsUpdateProgress(LINE6, info);
									}
								}
								else
								{
									// Hindi
									pos = strstr(buf, "hindi");

									if (pos != NULL) {
										SetTmsUpdateProgress(LINE6, "Updating Hindi MP3");

										ret = fileunZip_lib((unsigned char*)buf, HINDI_AUDIOS_PATH);
										if (ret == 0) {
											SetTmsUpdateProgress(LINE6, "Update Successful");
										} else {
											char info[24] = "";
											sprintf(info, "Update Failed(%d)", ret);

											SetTmsUpdateProgress(LINE6, info);
										}
									}
									else
									{
										// Tamil
										pos = strstr(buf, "tamil");

										if (pos != NULL) {
											SetTmsUpdateProgress(LINE6, "Updating Tamil MP3");

											ret = fileunZip_lib((unsigned char*)buf, TAMIL_AUDIOS_PATH);
											if (ret == 0) {
												SetTmsUpdateProgress(LINE6, "Update Successful");
											} else {
												char info[24] = "";
												sprintf(info, "Update Failed(%d)", ret);

												SetTmsUpdateProgress(LINE6, info);
											}
										}
										else
										{
											// Kannada
											pos = strstr(buf, "kannada");

											if (pos != NULL) {
												SetTmsUpdateProgress(LINE6, "Updating Kannada MP3");

												ret = fileunZip_lib((unsigned char*)buf, KANNADA_AUDIOS_PATH);
												if (ret == 0) {
													SetTmsUpdateProgress(LINE6, "Update Successful");
												} else {
													char info[24] = "";
													sprintf(info, "Update Failed(%d)", ret);

													SetTmsUpdateProgress(LINE6, info);
												}
											}
										}
									}
								}

								ret = DelFile_Api(buf);
								MAINLOG_L1("TMS: DelFile_Api() = %d", ret);

								#ifdef __GPAY_API__
									if (ret == 0) {
										memset(G_sys_param.mp3NameVer, 0, sizeof(G_sys_param.mp3NameVer));
										strcpy(G_sys_param.mp3NameVer, GFfile.file[i].version);

										saveParam();
									}
								#endif
							}
						}
					}

					if (ret == 0) {
						GFfile.file[i].status = STATUS_UPDATE;

						ret = WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, sizeof(GFfile));
						MAINLOG_L1("TMS: WriteFile_Api() = %d", ret);
					}

					if (ret != 0) {
						Tms_DelAllDownloadInfo(&GFfile);
						return -3;
					}
				}
			}
		}
	}
	// ========== Update Font & Parameter Files

	// ========== Update Firmware
	for (i = 0; i < GFfile.fnum; i++) {

		if (GFfile.file[i].status == STATUS_DLCOMPLETE) { // downloading finished but not replaced

			if (strcmp(GFfile.file[i].type, TYPE_FIRMWARE) == 0) {

				if (strcmp(TMS_UPDATE_TYPE, TMS_CLIENT_UPDATE_TYPE_FW) == 0) {
					SetTmsUpdateProgress(LINE7, "Find New Version");

					AppPlayTip("Downloading");

					ret = 0;

					memset(tmp, 0, sizeof(tmp));
					sprintf(tmp, "%s%s", TMS_FILE_DIR, GFfile.file[i].name);

					char print_data[48] = "";
					sprintf(print_data, "DOWNLOAD FILE PATH = %s", tmp);
					MAINLOG_L1(print_data);

					Delay_Api(1000);

					ScrClrLine_Api(LINE7, LINE7);
					SetTmsUpdateProgress(LINE6, "Updating");

					p = strstr(GFfile.file[i].name, ".zip");

					if (p != NULL) {
						ret = fileunZip_lib((unsigned char*)tmp, TMS_FILE_DIR);
						MAINLOG_L1("TMS: fileunZip_lib() = %d", ret);

						ret = DelFile_Api(tmp);
						MAINLOG_L1("TMS: DelFile_Api() = %d", ret);

						if (ret == 0) {
							memcpy(p, ".bin", 4);

							ret = WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, sizeof(GFfile));
							MAINLOG_L1("TMS: WriteFile_Api() = %d", ret);
						}
					}

					if (ret == 0) {
						ret = tmsUpdateFile_lib(TMS_FLAG_DIFFOS, tmp, NULL);
						MAINLOG_L1("TMS: tmsUpdateFile_lib() = %d", ret);

						if (ret == 0) {
							SetTmsUpdateProgress(LINE6, "Update Successful");
							AppPlayTip("Firmware Updated, Device Will Reboot Soon");
						} else {
							char buf[24] = "";
							sprintf(buf, "Update Failed(%d)", ret);

							SetTmsUpdateProgress(LINE7, buf);
						}
					}

					if (ret != 0) {
						Tms_DelAllDownloadInfo(&GFfile);
						return -4;
					}
				}
			}
		}
	}
	// ========== Update Firmware

	// ========== Update APP(PS: firmware must be updating before app because new lib may not work with old firmware)
	for (i = 0; i < GFfile.fnum; i++) {

		if (GFfile.file[i].status == STATUS_DLCOMPLETE) { // downloading finished but not replaced

			if (strcmp(GFfile.file[i].type, TYPE_APP) == 0) {

				if (strcmp(TMS_UPDATE_TYPE, TYPE_APP) == 0) {
					SetTmsUpdateProgress(LINE7, "Find New Version");

					AppPlayTip("Downloading");

					ret = 0;

					memset(tmp, 0, sizeof(tmp));
					sprintf(tmp, "%s%s",  TMS_FILE_DIR, GFfile.file[i].name);

					char print_data[48] = "";
					sprintf(print_data, "DOWNLOAD FILE PATH = %s", tmp);
					MAINLOG_L1(print_data);

					Delay_Api(1000);

					ScrClrLine_Api(LINE7, LINE7);
					SetTmsUpdateProgress(LINE6, "Updating");

					p = strstr(GFfile.file[i].name, ".zip");

					if (p != NULL) {
						ret = fileunZip_lib((unsigned char*)tmp, TMS_FILE_DIR);
						MAINLOG_L1("TMS: fileunZip_lib() = %d", ret);

						ret = DelFile_Api(tmp);
						MAINLOG_L1("TMS: DelFile_Api() = %d", ret);

						if (ret == 0) {
							memcpy(p, ".img", 4);

							ret = WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, sizeof(GFfile));
							MAINLOG_L1("TMS: WriteFile_Api() = %d", ret);
						}
					}

					if (ret == 0) {
						ret = tmsUpdateFile_lib(TMS_FLAG_APP, tmp, NULL);
						MAINLOG_L1("TMS: tmsUpdateFile_lib() = %d", ret);

						if (ret == 0) {
							SetTmsUpdateProgress(LINE6, "Update Successful");
							AppPlayTip("Application Updated, Device Will Reboot Soon");
						} else {
							char buf[24] = "";
							sprintf(buf, "Update Failed(%d)", ret);

							SetTmsUpdateProgress(LINE7, buf);
						}

					} else {
						Tms_DelAllDownloadInfo(&GFfile);
						return -5;
					}
				}
			}
		}
	}
	// ========== Update APP

	// ========== Update Dynamic Library
	for (i = 0; i < GFfile.fnum; i++) {

		if (GFfile.file[i].status == STATUS_DLCOMPLETE) { // downloading finished but not replaced

			if (strcmp(GFfile.file[i].type, TYPE_LIB) == 0) {

				if (strcmp(TMS_UPDATE_TYPE, TMS_CLIENT_UPDATE_TYPE_DL) == 0) {
					SetTmsUpdateProgress(LINE7, "Find New Version");

					AppPlayTip("Downloading");

					ret = 0;

					memset(tmp, 0, sizeof(tmp));
					sprintf(tmp, "%s%s", TMS_FILE_DIR, GFfile.file[i].name);

					char print_data[48] = "";
					sprintf(print_data, "DOWNLOAD FILE PATH = %s", tmp);
					MAINLOG_L1(print_data);

					Delay_Api(1000);

					ScrClrLine_Api(LINE7, LINE7);
					SetTmsUpdateProgress(LINE6, "Updating");

					p = strstr(GFfile.file[i].name, ".zip");

					if (p != NULL) {
						ret = fileunZip_lib((unsigned char*)tmp, TMS_FILE_DIR);
						MAINLOG_L1("TMS: fileunZip_lib() = %d", ret);

						ret = DelFile_Api(tmp);
						MAINLOG_L1("TMS: DelFile_Api() = %d", ret);

						if (ret == 0) {
							memcpy(p, ".bin", 4);

							ret = WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, sizeof(GFfile));
							MAINLOG_L1("TMS: WriteFile_Api() = %d", ret);
						}
					}

					if (ret == 0) {
						ret = tmsUpdateFile_lib(TMS_FLAG_VOS, tmp, NULL);
						MAINLOG_L1("TMS: tmsUpdateFile_lib() = %d", ret);

						if (ret == 0) {
							SetTmsUpdateProgress(LINE6, "Update Successful");
							AppPlayTip("Dynamic Library Updated, Device Will Reboot Soon");
						} else {
							char buf[24] = "";
							sprintf(buf, "Update Failed = %d", ret);

							SetTmsUpdateProgress(LINE7, buf);
						}
					}

					if (ret != 0) {
						Tms_DelAllDownloadInfo(&GFfile);
						return -4;
					}
				}
			}
		}
	}
	// ========== Update Firmware

	ret = DelFile_Api(_DOWN_STATUS_);
	MAINLOG_L1("TMS: DelFile_Api() = %d", ret);

	return 0;
}

int Tms_Notify(void)
{
	MAINLOG_L1("TMS: Tms_Notify()");

	int PackLen = 0;
	int ret = 0, i, flag = 0;

	for (i = 0; i < TmsTrade.fnum; i++) {
		if (TmsTrade.file[i].status == STATUS_DLUNCOMPLETE) {
			flag = 1;
			break;
		}
	}

	if (flag == 1) { return _TMS_E_DOWNUNCOMPLETE; }

	memset(TmsTrade.respCode, 0, sizeof(TmsTrade.respCode));
	memset(TmsTrade.respMsg,  0, sizeof(TmsTrade.respMsg));

	TmsTrade.trade_type = TYPE_NOTIFY;

	memset(t_sBuf, 0, SENDPACKLEN);

	ret = Tms_CreatePacket(t_sBuf, &PackLen);
	MAINLOG_L1("TMS: Tms_CreatePacket() = %d", ret);

	if (ret != 0) { return ret; }

	memset(t_rBuf, 0, RECVPACKLEN);

	ret = Tms_SendRecvPacket(t_sBuf, PackLen, t_rBuf, &PackLen);
	MAINLOG_L1("TMS: Tms_SendRecvPacket() = %d", ret);

	if (ret != 0) { return ret; }

	ret = Tms_ResolveHTTPPacket(t_rBuf);
	MAINLOG_L1("TMS: Tms_ResolveHTTPPacket() = %d", ret);

	return ret;
}

int Tms_NeedReConnect(u8 *packdata)
{
	//MAINLOG_L1("TMS: Tms_NeedReConnect()");

	u8 *p = NULL,  *q = NULL, *s = NULL, *e = NULL;
	u8 tmp[16], buf[16];

	p = packdata;

	if ((q = (u8 *)strstr((char *)packdata, "\r\n\r\n")) == NULL) { return -1; }
	if ((s = (u8 *)strstr((char *)p, "Connection:")) == NULL) { return -1; }

	s += strlen("Connection:");

	if ((e = (u8 *)strstr((char *)s, "\r\n")) == NULL) { return -1; }

	if (e > q) { return -1; }

	memset(buf, 0, sizeof(buf));
	memset(tmp, 0, sizeof(tmp));

	memcpy(tmp,	s, e - s);

	if (Tms_Trim(tmp, buf) != 0) { return -1; }

	if (strcmp((char *)buf, "close") == 0) { return 0; }

	return -1;
}

int Tms_DownloadUrlFilesOneByOne()
{
	MAINLOG_L1("TMS: Tms_DownloadUrlFilesOneByOne()");

	int i, Ret, curfidx, clen, stlen = 0, thisLen, tlen = 0, len = 0, mlen = 0, sidx = 0, PackLen = 0;
	char tmp[32], dname[64];
	char *p = NULL;

	while ((TmsTrade.file[TmsTrade.curfindex].status == STATUS_DLUNCOMPLETE) && (TmsTrade.curfindex < TmsTrade.fnum)) {
		curfidx = TmsTrade.curfindex;

		if (TmsTrade.file[TmsTrade.curfindex].startPosi > TmsTrade.file[TmsTrade.curfindex].fsize) {
			Ret = _TMS_E_FILESIZE;
			break;
		}
		if (TmsTrade.file[TmsTrade.curfindex].startPosi == 0) {
			memset(&TmsTrade.context, 0, sizeof(TmsTrade.context));
			_tms_MD5Init (&TmsTrade.context);
		}

		memset(dname, 0, sizeof(dname));
		Tms_getDomainName(TmsTrade.file[TmsTrade.curfindex].filePath, dname);

		for (i = TmsTrade.file[TmsTrade.curfindex].startPosi; i < TmsTrade.file[TmsTrade.curfindex].fsize; i += EXFCONTENT_LEN) {
			memset(t_sBuf, 0, SENDPACKLEN);

			thisLen = (TmsTrade.file[TmsTrade.curfindex].fsize - TmsTrade.file[TmsTrade.curfindex].startPosi) >
					  EXFCONTENT_LEN ? EXFCONTENT_LEN : (TmsTrade.file[TmsTrade.curfindex].fsize - TmsTrade.file[TmsTrade.curfindex].startPosi);

			//LogPrintWithRet(0, "thisLen = ", thisLen);

			sprintf((char *)t_sBuf, "GET %s HTTP/1.1\r\n", TmsTrade.file[TmsTrade.curfindex].filePath);

			strcat((char *)t_sBuf, "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n");

			strcat((char *)t_sBuf, "User-Agent: Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)\r\n");

			sprintf((char *)t_sBuf + strlen((char *)t_sBuf), "Range: bytes=%d-%d\r\n", TmsTrade.file[TmsTrade.curfindex].startPosi, TmsTrade.file[TmsTrade.curfindex].startPosi+thisLen - 1);

			sprintf((char *)t_sBuf + strlen((char *)t_sBuf), "Host: %s\r\n\r\n", dname);

			PackLen = strlen((char *)t_sBuf);

			Ret = dev_send(t_sBuf, PackLen);
			//MAINLOG_L1("TMS: dev_send(): ", Ret);

			if (Ret != 0) {
				if (Ret != 0xff) {
					Ret = _TMS_E_SEND_PACKET;
					break;
				}

				Ret = Tms_ReSend(t_sBuf,  PackLen);
				MAINLOG_L1("TMS: Tms_ReSend() = %d", Ret);

				if (Ret != 0) {
					Ret = _TMS_E_SEND_PACKET;
					break;
				}
			}

			tlen = 0;
			memset(t_rBuf, 0, RECVPACKLEN);

			while (1) {
				Tms_StartJumpSec();

				Ret = dev_recv(t_rBuf + tlen, RECEIVE_BUF_SIZE - 1 - tlen, RECEIVE_TIMEOUT);
				//MAINLOG_L1("TMS: dev_recv(): ", Ret);

				Tms_StopJumpSec();

				if (Ret <= 0) {
					Ret = _TMS_E_RECV_PACKET;
					break;
				}

				len = Ret;
				Ret = 0;

				tlen += len;

				if (Tms_getContentLen(t_rBuf, &clen, &stlen) != 0) { continue; }

				if ((p = strstr((char *)t_rBuf, "\n")) == NULL) { return _TMS_E_RESOLVE_PACKET; }

				memset(tmp, 0, sizeof(tmp));
				memcpy(tmp, t_rBuf, ((unsigned char*)p) - t_rBuf);

				if ((strstr(tmp, "200") == NULL) && (strstr(tmp, "206") == NULL)) {
					memset(TmsTrade.respMsg, 0, sizeof(TmsTrade.respMsg));

					if (strlen(tmp) > (sizeof(TmsTrade.respMsg)-1)) {
						memcpy(TmsTrade.respMsg, tmp, sizeof(TmsTrade.respMsg) - 1);
					} else {
						memcpy(TmsTrade.respMsg, tmp, strlen(tmp));
					}

					return _TMS_E_TRANS_FAIL;
				}

				p = strstr((char *)t_rBuf, "\r\n\r\n");

				if (tlen > stlen) {

					Ret = _TMS_E_RECV_PACKET;
					break;

				} else if (tlen == stlen) {
					if ((tlen - (p + 4 - (char *)t_rBuf)) > 0) {
						#ifdef DISP_STH
							ScrClrLine_Api(LINE3, LINE4);

							memset(tmp, 0, sizeof(tmp));
							sprintf(tmp, "%d%% %d/%d", (TmsTrade.file[TmsTrade.curfindex].startPosi*100)/TmsTrade.file[TmsTrade.curfindex].fsize, TmsTrade.file[TmsTrade.curfindex].startPosi, TmsTrade.file[TmsTrade.curfindex].fsize);
							ScrDisp_Api(LINE3, 0, TmsTrade.file[TmsTrade.curfindex].name, FDISP|CDISP);
							ScrDisp_Api(LINE4, 0, tmp,FDISP|CDISP);
						#endif

						Ret = Tms_WriteFile(&TmsTrade.file[TmsTrade.curfindex], (unsigned char*)(p + 4), thisLen);
						//MAINLOG_L1("TMS: Tms_WriteFile(): ", Ret);

						if (Ret != 0) {
							Ret = _TMS_E_FILE_WRITE;
							break;
						}
					}

					if (Tms_NeedReConnect(t_rBuf) == 0) {
						dev_disconnect();

						Ret = TmsConnect_Api();
						//MAINLOG_L1("TMS: TmsConnect_Api(): ", Ret);

						if (Ret != 0) { Ret = _TMS_E_ERR_CONNECT; }
					}

					break;
				}
			}

			if (Ret != 0) { break; }

			if (TmsTrade.file[curfidx].startPosi > TmsTrade.file[curfidx].fsize) {
				Ret = _TMS_E_FILESIZE;
				break;
			}

			if (TmsTrade.file[curfidx].status == STATUS_DLCOMPLETE) {
				#ifdef DISP_STH
					ScrClrLine_Api(LINE3, LINE4);

					memset(tmp, 0, sizeof(tmp));

					sprintf(tmp, "100%% %d/%d", TmsTrade.file[curfidx].startPosi, TmsTrade.file[curfidx].fsize);
					ScrDisp_Api(LINE3, 0, TmsTrade.file[curfidx].name, FDISP|CDISP);
					ScrDisp_Api(LINE4, 0, tmp,FDISP|CDISP);
				#endif

				break;
			}
		}

		if (Ret != 0) { break; }
	}

	if (Ret != 0) { dev_disconnect(); }

	return Ret;
}

void Tms_DelAllDownloadInfo(struct __TmsTrade__ *GFfile)
{
	MAINLOG_L1("TMS: Tms_DelAllDownloadInfo()");

	int ret, i;
	char tmp[64], buf[64];

	for (i = 0; i <= GFfile->curfindex; i++) {
		if ((strcmp(GFfile->file[i].type, TYPE_FIRMWARE) == 0) || (strcmp(GFfile->file[i].type, TYPE_APP) == 0)) {
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "%s%s", TMS_FILE_DIR, GFfile->file[i].name);

			ret = DelFile_Api(tmp);
			MAINLOG_L1("TMS: DelFile_Api() = %d", ret);

		} else if (strcmp(GFfile->file[i].type, TYPE_PARAM) == 0) {
			memset(tmp, 0, sizeof(tmp));
			memset(buf, 0, sizeof(buf));

			sprintf(tmp, "%s%s", TMS_FILE_DIR, GFfile->file[i].name);
			strcat(buf, ".bak");

			ret = DelFile_Api(tmp);
			MAINLOG_L1("TMS: DelFile_Api() = %d", ret);

			ret = DelFile_Api(buf);
			MAINLOG_L1("TMS: DelFile_Api() = %d", ret);
		}
	}

	ret = DelFile_Api(_DOWN_STATUS_);
	MAINLOG_L1("TMS: DelFile_Api() = %d", ret);
}

int Tms_CompareVersions(struct __TmsTrade__ *Curr)
{
	MAINLOG_L1("TMS: Tms_CompareVersions()");

	int i, flag = 0, ret, len;
	struct __TmsTrade__ GFfile;

	len = sizeof(struct __TmsTrade__);

	ret = ReadFile_Api(_DOWN_STATUS_, (unsigned char*)&GFfile, 0, (unsigned int *)&len);
	MAINLOG_L1("TMS: ReadFile_Api() = %d", ret);

	if (ret != 0) { goto _TMS_DEL_ALL; }

	if ((Curr->fnum != GFfile.fnum)) { // file exist, need to compare all files and versions
		goto _TMS_DEL_ALL;
	}

	for (i = 0; i <= GFfile.fnum; i++) {
		if ((strcmp(GFfile.file[i].name, Curr->file[i].name) == 0) &&
		    (strcmp(GFfile.file[i].type, Curr->file[i].type) == 0) &&
		    (strcmp(GFfile.file[i].version, Curr->file[i].version) == 0) && (GFfile.file[i].fsize == Curr->file[i].fsize))
		{
			continue;

		} else {
			flag = 1;
			break;
		}
	}

	if (flag == 0) { // all same, use information in the files
		memcpy(Curr, &GFfile, sizeof(GFfile));
		return 0;
	}

	_TMS_DEL_ALL:
		Tms_DelAllDownloadInfo(&GFfile);
		return 1;
}

int Tms_UnPackPacket(char *data, int reslt)
{
	MAINLOG_L1("TMS: Tms_UnPackPacket()");

	cJSON * pItem = NULL, * pitem = NULL;
	int len;

	if (pRoot != NULL) {
		cJSON_Delete(pRoot);
		pRoot = NULL;
	}

	pRoot = cJSON_Parse(data);
	if (pRoot == NULL) { return _TMS_E_RESOLVE_PACKET; }

	pItem = cJSON_GetObjectItem(pRoot, "version");
	if ((pItem != NULL) && (pItem->type != cJSON_NULL)) {
		if (strcmp(pItem->valuestring, TmsStruct.version)) { return _TMS_E_PACKAGE_WRONG; }
	}

	pItem = cJSON_GetObjectItem(pRoot, "sn");
	if ((pItem != NULL) && (pItem->type != cJSON_NULL)) {
		if (strcmp(pItem->valuestring, TmsStruct.sn) != 0) { return _TMS_E_PACKAGE_WRONG; }
	}

	pItem = cJSON_GetObjectItem(pRoot, "manufacturer");
	if ((pItem != NULL) && (pItem->type != cJSON_NULL)) {
		if (strcmp(pItem->valuestring , TmsStruct.manufacturer) != 0) { return _TMS_E_PACKAGE_WRONG; }
	}

	pItem = cJSON_GetObjectItem(pRoot, "deviceType");
	if ((pItem != NULL) && (pItem->type != cJSON_NULL)) {
		if (strcmp(pItem->valuestring , TmsStruct.deviceType) != 0) { return _TMS_E_PACKAGE_WRONG; }
	}

	pItem = cJSON_GetObjectItem(pRoot, "respCode");
	if ((pItem != NULL) && (pItem->type != cJSON_NULL)) {
		memcpy(TmsTrade.respCode, pItem->valuestring, strlen(pItem->valuestring));
	}

	pItem = cJSON_GetObjectItem(pRoot, "respMsg");
	if ((pItem != NULL) && (pItem->type != cJSON_NULL)) {
		memcpy(TmsTrade.respMsg, pItem->valuestring, strlen(pItem->valuestring));
	}

	char *result = cJSON_PrintUnformatted(pRoot);

	char result_data[1024] = "";
	sprintf(result_data, "\n%s\n", result);
	MAINLOG_L1(result_data);

	if ((strcmp(TmsTrade.respCode, "00") != 0) || (reslt != 0)) { return _TMS_E_TRANS_FAIL; }

	switch (TmsTrade.trade_type) {

		case TYPE_CHECKVERSION:
			pItem = cJSON_GetObjectItem(pRoot, "fileList");
			if ((pItem == NULL) && (pItem->type != cJSON_Array)) { break; }

			pItem = pItem->child;
			if (pItem == NULL) {
				SetTmsUpdateProgress(LINE7, "No Update");
				NO_UPDATE = 1;

				return reslt;
			}

			while (pItem != NULL) {
				pitem = cJSON_GetObjectItem(pItem, "fileName");

				if ((pitem != NULL) && (pitem->type != cJSON_NULL)) {
					len = sizeof(TmsTrade.file[TmsTrade.fnum].name) - 1;
					memcpy(TmsTrade.file[TmsTrade.fnum].name, pitem->valuestring, strlen(pitem->valuestring) > len ? len:strlen(pitem->valuestring));
				}

				pitem = cJSON_GetObjectItem(pItem, "fileType");
				if ((pitem != NULL) && (pitem->type != cJSON_NULL)) {
					len = sizeof(TmsTrade.file[TmsTrade.fnum].type) - 1;
					memcpy(TmsTrade.file[TmsTrade.fnum].type, pitem->valuestring, strlen(pitem->valuestring) > len ? len:strlen(pitem->valuestring));
				}

				pitem = cJSON_GetObjectItem(pItem, "fileVersion");
				if ((pitem != NULL) && (pitem->type != cJSON_NULL)) {
					len = sizeof(TmsTrade.file[TmsTrade.fnum].version) - 1;
					memcpy(TmsTrade.file[TmsTrade.fnum].version, pitem->valuestring, strlen(pitem->valuestring) > len ? len:strlen(pitem->valuestring));
				}

				pitem = cJSON_GetObjectItem(pItem, "fileSize");
				if ((pitem != NULL) && (pitem->type != cJSON_NULL)) {
					TmsTrade.file[TmsTrade.fnum].fsize = (int)AscToLong_Api((unsigned char *)(pitem->valuestring), strlen(pitem->valuestring));
				}

				pitem = cJSON_GetObjectItem(pItem, "filePath");
				if ((pitem != NULL) && (pitem->type != cJSON_NULL)) {
					len = sizeof(TmsTrade.file[TmsTrade.fnum].filePath) - 1;
					memcpy(TmsTrade.file[TmsTrade.fnum].filePath, pitem->valuestring, strlen(pitem->valuestring) > len ? len:strlen(pitem->valuestring));
				}

				pitem = cJSON_GetObjectItem(pItem, "md5");
				if ((pitem != NULL) && (pitem->type != cJSON_NULL)) {
					AscToBcd_Api(TmsTrade.file[TmsTrade.fnum].md5, pitem->valuestring, 32);
				}

				if (Tms_CheckNeedDownFile(&TmsTrade.file[TmsTrade.fnum]) == 0) {
					TmsTrade.fnum++;
				} else {
					memset(&TmsTrade.file[TmsTrade.fnum], 0, sizeof(TmsTrade.file[TmsTrade.fnum]));
				}

				pItem = pItem->next;
				if (TmsTrade.fnum > _TMS_MAX_APPLIB) { return _TMS_E_TOO_MANY_FILES; }
			}
			break;

		case TYPE_NOTIFY: break;

		default: return 1;
	}

	return 0;
}

u8 Tms_HttpGetTranResult(u8 * data)
{
	MAINLOG_L1("TMS: Tms_HttpGetTranResult()");

	u8 *p = NULL, tmp[64] = {0};
	if ((p = (u8 *)strstr((char *)data, "\n")) == NULL) {
		return _TMS_E_RESOLVE_PACKET;
	}

	memcpy(tmp, data, p - data);
	MAINLOG_L1("TMS: Tms_HttpGetTranResult() if data = %s", data);

	if ((p = (u8 *)strstr((char *)tmp, "200")) == NULL) {
		memset(TmsTrade.respMsg, 0, sizeof(TmsTrade.respMsg));
		MAINLOG_L1("TMS: Tms_HttpGetTranResult() if 1 = %s", TmsTrade);
		if (strlen((char *)tmp) > (sizeof(TmsTrade.respMsg) - 1)) {
			memcpy(TmsTrade.respMsg, tmp, sizeof(TmsTrade.respMsg) - 1);
			MAINLOG_L1("TMS: Tms_HttpGetTranResult() if 2 = %s", TmsTrade);
		} else {
			memcpy(TmsTrade.respMsg, tmp, strlen((char *)tmp));
			MAINLOG_L1("TMS: Tms_HttpGetTranResult() else = %s", TmsTrade);
		}
		MAINLOG_L1("TMS: Tms_HttpGetTranResult() TmsTrade = %s", TmsTrade);
		return _TMS_E_TRANS_FAIL;
	}

	return 0;
}

int Tms_ResolveHTTPPacket(u8 *recvPack)
{
	MAINLOG_L1("TMS: Tms_ResolveHTTPPacket()");
	MAINLOG_L1("TMS: Tms_ResolveHTTPPacket() recvPack : %s", recvPack);

	char *ptr = NULL;

	int ret = Tms_HttpGetTranResult(recvPack);
	MAINLOG_L1("TMS: Tms_HttpGetTranResult() ret : %d", ret);
	MAINLOG_L1("TMS: Tms_HttpGetTranResult() recvPack : %s", recvPack);

	if (ret != 0) { return ret; }

	if ((ptr = strstr((char *)recvPack, "{")) == NULL) {
		if (ret == 0) {
			MAINLOG_L1("TMS: Tms_UnPackPacket(): %s", _TMS_E_RESOLVE_PACKET);
			return _TMS_E_RESOLVE_PACKET;
		}
		else {
			MAINLOG_L1("TMS: Tms_UnPackPacket(): %d", ret);
			return ret;
		}
	}

	ret = Tms_UnPackPacket(ptr, ret);
	MAINLOG_L1("TMS: Tms_UnPackPacket(): %d", ret);
	MAINLOG_L1("TMS: Tms_UnPackPacket(): %s", pRoot);

	if (pRoot != NULL) {
		cJSON_Delete(pRoot);
		pRoot = NULL;
	}

	return ret;
}

/// ========== Send Data
int Tms_RecvPacket(u8 *Packet, int *PacketLen, int WaitTime)
{
	MAINLOG_L1("TMS: Tms_RecvPacket()");

	int Ret, clen, stlen;
	unsigned short tlen = 0;

	while (1) {
		Tms_StartJumpSec();

		Ret = dev_recv(Packet + tlen, RECEIVE_BUF_SIZE - 1 - tlen, RECEIVE_TIMEOUT);
		MAINLOG_L1("TMS: dev_recv(): ", Ret);

		Tms_StopJumpSec();

		if (Ret <= 0) {
			Ret = RECEIVE_ERROR;
			break;
		}

		tlen += Ret;

		clen  = 0;
		stlen = 0;
		Ret   = 0;

		if (Tms_getContentLen(Packet, &clen, &stlen) == 0) {
			if (tlen > stlen) {

				Ret = _TMS_E_RECV_PACKET;
				break;

			} else if (tlen == stlen) {
				*PacketLen = tlen;
				Ret = 0;
				break;
			}
		}
	}

	if (Ret != 0) { dev_disconnect(); }

	return Ret;
}

int Tms_UrlRecvPacket(u8 *Packet, int *PacketLen, int WaitTime)
{
	MAINLOG_L1("TMS: Tms_UrlRecvPacket()");

	unsigned short len;
	int Ret;

	u8 *p = NULL;
	int tlen = 0, curfidx;
	int clen = 0, stlen = 0, hflag = 0, floc = 0;

    curfidx = TmsTrade.curfindex;

	floc = Tms_GetFileSize(&(TmsTrade.file[TmsTrade.curfindex]));
	MAINLOG_L1("TMS: Tms_GetFileSize() = %d", floc);

	while (1) {
		Tms_StartJumpSec();

		if (hflag == 0) {

			Ret = dev_recv(Packet+tlen, RECEIVE_BUF_SIZE - 1 - tlen, RECEIVE_TIMEOUT);
			MAINLOG_L1("TMS: dev_recv() = %d", Ret);

		} else {
			Ret = dev_recv(Packet, RECEIVE_BUF_SIZE - 1 - tlen, RECEIVE_TIMEOUT);
			MAINLOG_L1("TMS: dev_recv() = %d", Ret);
		}
		Tms_StopJumpSec();

		if (Ret <= 0) {
			Ret = RECEIVE_ERROR;
			break;
		}

		#ifdef DISP_STH
			memset(tmp, 0, sizeof(tmp));

			ScrClrLine_Api(LINE3, LINE4);

			sprintf(tmp, "%d%% %d/%d", (TmsTrade.file[TmsTrade.curfindex].startPosi*100)/TmsTrade.file[TmsTrade.curfindex].fsize, TmsTrade.file[TmsTrade.curfindex].startPosi, TmsTrade.file[TmsTrade.curfindex].fsize);
			ScrDisp_Api(LINE3, 0, TmsTrade.file[TmsTrade.curfindex].name, FDISP|CDISP);
			ScrDisp_Api(LINE4, 0, tmp,FDISP|CDISP);
		#endif

		len = Ret;
		Ret = 0;

		tlen += len;

		if (hflag == 0) {
			if (Tms_getContentLen(Packet, &clen, &stlen) != 0) { continue; }

			if (floc + clen != TmsTrade.file[TmsTrade.curfindex].fsize) {
				Ret = _TMS_E_RECV_PACKET;
				break;
			}

			p = (u8 *)strstr((char *)Packet, "\r\n\r\n");

			if (tlen - (p + 4 - Packet) > 0) {
				Ret = Tms_WriteFile(&TmsTrade.file[TmsTrade.curfindex], p + 4, tlen - (p + 4 - Packet));
				MAINLOG_L1("TMS: Tms_WriteFile() = %d", Ret);

				if (Ret != 0) {
					Ret = _TMS_E_FILE_WRITE;
					break;
				}
				floc += (tlen - (p + 4 - Packet));
			}

			hflag = 1;

		} else {
			Ret = Tms_WriteFile(&TmsTrade.file[TmsTrade.curfindex], Packet, len);
			MAINLOG_L1("TMS: Tms_WriteFile() = %d", Ret);

			if (Ret != 0) {
				Ret = _TMS_E_FILE_WRITE;
				break;
			}
			floc += len;
		}

		if (tlen > stlen) {
			Ret = _TMS_E_RECV_PACKET;
			break;

		} else if(tlen == stlen) {
			Ret = 0;
			*PacketLen = tlen;
			break;
		}
	}

	if (Ret != 0) { dev_disconnect(); }

	return Ret;
}

int TmsConnect_Api()
{
	MAINLOG_L1("TMS: TmsConnect_Api()");

	int ret = dev_connect(&tmsEntry, CONNECT_TIMEOUT);

	if (ret != 0) { return ret; }

	return 0;
}

int Tms_ReConnect()
{
	MAINLOG_L1("TMS: Tms_ReConnect()");

	int i, ret;

	for (i = 0; i < 2; i++) {
		dev_disconnect();

		ret = TmsConnect_Api();
		MAINLOG_L1("TMS: TmsConnect_Api() = %d", ret);

		if (ret == 0) { break; }
	}

	return ret;
}

int Tms_ReSend(u8 *packData, int PackLen)
{
	MAINLOG_L1("TMS: Tms_ReSend()");

	int ret;

	#ifdef DISP_STH
		ScrDisp_Api(LINE2, 0, "ReConnecting..", FDISP|CDISP);
	#endif

	ret = Tms_ReConnect();
	MAINLOG_L1("TMS: Tms_ReConnect() = %d", ret);

	#ifdef DISP_STH
		ScrClrLine_Api(LINE2, LINE2);
	#endif

	if (ret != 0) { return _TMS_E_SEND_PACKET; }

	ret = dev_send((unsigned char *)packData,  PackLen);
	MAINLOG_L1("TMS: dev_send() = %d", ret);

	if (ret != 0) { return _TMS_E_SEND_PACKET; }

	return ret;
}

int Tms_SendRecvData(unsigned char *SendBuf, int Senlen, unsigned char *RecvBuf, int *RecvLen,int psWaitTime)
{
	MAINLOG_L1("TMS: Tms_SendRecvData()");

	int ret = 0;

	ret = dev_send(SendBuf, Senlen);
	MAINLOG_L1("TMS: dev_send() = %d", ret);

	if (ret != 0) {
		if (ret != 0xff) { return _TMS_E_SEND_PACKET; }

		ret = Tms_ReSend(SendBuf,  Senlen);
		MAINLOG_L1("TMS: Tms_ReSend() = %d", ret);

		if (ret != 0) { return _TMS_E_SEND_PACKET; }
	}

	*RecvLen = 0;
	if (TmsTrade.trade_type == TYPE_URLGETFILE) {

		ret = Tms_UrlRecvPacket(RecvBuf, RecvLen, psWaitTime);
		MAINLOG_L1("TMS: Tms_UrlRecvPacket() = %d", ret);

	} else {

		ret = Tms_RecvPacket(RecvBuf, RecvLen, psWaitTime);
		MAINLOG_L1("TMS: Tms_RecvPacket() = %d", ret);
	}

	return ret;
}

int Tms_SendRecvPacket(u8 *SendBuf, int Senlen, u8 *RecvBuf, int *pRecvLen)
{
	MAINLOG_L1("TMS: Tms_SendRecvPacket()");

	int ret = Tms_SendRecvData(SendBuf, Senlen, RecvBuf, pRecvLen,60);
	MAINLOG_L1("TMS: Tms_SendRecvData() = %d", ret);

	return ret;
}
/// ========== Send Data

int Tms_CreatePacket(u8 * packData, int * packLen)
{
	MAINLOG_L1("TMS: Tms_CreatePacket()");

	cJSON *root = NULL;
	char *out   = NULL;

	int dataLen, i;
	u8 buf[64], tmp[64], timestamp[16], urlPath[128], md5src[1024];

	memset(urlPath, 0, sizeof(urlPath));
	root = cJSON_CreateObject();

	cJSON_AddStringToObject((cJSON *)root, "version",      TmsStruct.version); // version is un-useful
	cJSON_AddStringToObject((cJSON *)root, "sn",           TmsStruct.sn);
	cJSON_AddStringToObject((cJSON *)root, "manufacturer", TmsStruct.manufacturer);
	cJSON_AddStringToObject((cJSON *)root, "deviceType",   TmsStruct.deviceType);

	switch(TmsTrade.trade_type) {
		case TYPE_CHECKVERSION:
			sprintf((char *)urlPath, "POST /tms/checkVersion");
			break;

		case TYPE_NOTIFY:
			sprintf((char *)urlPath, "POST /tms/resultNotify");

			if (Tms_CheckDownloadOver() == 0) {
				cJSON_AddStringToObject(root, "upgradeResult", "00");
			} else {
				cJSON_AddStringToObject(root, "upgradeResult", "99");
			}
			break;

		default: break;
	}

	out     = cJSON_PrintUnformatted(root);
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
	memcpy(buf, timestamp,    strlen((char *)timestamp));

	for (i = 0; i < 50; i++) { tmp[i] ^= buf[i]; }

	memset(buf, 0x20, sizeof(buf));
	memcpy(buf, "998876511QQQWWeerASDHGJKL", strlen("998876511QQQWWeerASDHGJKL"));

	for (i = 0; i < 50; i++) { tmp[i] ^= buf[i]; }

	tmp[50] = 0;
	memset(buf, 0, sizeof(buf));
	memset(md5src, 0, sizeof(md5src));

	memcpy(md5src, out, dataLen);
	BcdToAsc_Api((char *)(md5src + dataLen), (unsigned char *)tmp, 100);

	// MD5
	_tms_MDString((char *)md5src,(unsigned char *)buf);

	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api((char *)tmp, (unsigned char *)buf, 32);

	sprintf((char *)packData + strlen((char *)packData), "sign: %s\r\n", 	  tmp);
	sprintf((char *)packData + strlen((char *)packData), "timestamp: %s\r\n", timestamp);
	/// ========== TimeStamp & Sign

	sprintf((char *)packData + strlen((char *)packData), "Content-Length: %d\r\n", strlen((char *)out));

	strcat((char *)packData, "Content-Type: application/json;charset=UTF-8\r\n");

	sprintf((char *)packData + strlen((char *)packData), "Host: %s\r\n", TmsStruct.hostDomainName);

	strcat((char *)packData, "Connection: Keep-Alive\r\n");

	strcat((char *)packData, "User-Agent: Apache-HttpClient/4.3.5 (java 1.5)\r\n");

	strcat((char *)packData, "Accept-Encoding: gzip,deflate\r\n\r\n");

	memcpy(packData+strlen((char *)packData), out, dataLen);

	*packLen = strlen((char *)packData);

	free(out);
	cJSON_Delete(root);

	return 0;
}

int Tms_Request(void)
{
	MAINLOG_L1("TMS: Tms_Request()");

	int PackLen = 0;
	int ret = 0, flen;

	memset(TmsTrade.respCode, 0, sizeof(TmsTrade.respCode));
	memset(TmsTrade.respMsg,  0, sizeof(TmsTrade.respMsg));

	TmsTrade.trade_type = TYPE_CHECKVERSION;

	memset(t_sBuf, 0, SENDPACKLEN);

	ret = Tms_CreatePacket(t_sBuf, &PackLen);
	MAINLOG_L1("TMS: Tms_CreatePacket() = %d", ret);

	if (ret != 0) { return ret; }

	memset(t_rBuf, 0, RECVPACKLEN);

	ret = Tms_SendRecvPacket(t_sBuf, PackLen, t_rBuf, &PackLen);
	MAINLOG_L1("TMS: Tms_SendRecvPacket() = %d", ret);

	if (ret != 0) { return ret; }

	ret = Tms_ResolveHTTPPacket(t_rBuf);
	MAINLOG_L1("TMS: Tms_ResolveHTTPPacket() = %d", ret);

	if (ret != 0) { return ret; }

	if (TmsTrade.fnum <= 0) {
		memset(TmsTrade.respCode, 0, sizeof(TmsTrade.respCode));
		memset(TmsTrade.respMsg,  0, sizeof(TmsTrade.respMsg));

		return _TMS_E_NOFILES_D;
	}

	flen = GetFileSize_Api(_DOWN_STATUS_);
	MAINLOG_L1("TMS: GetFileSize_Api() = %d", flen);

	if (flen > 0) {
		Tms_CompareVersions(&TmsTrade);
	}

	return ret;
}

int Tms_CommProcess()
{
	MAINLOG_L1("TMS: Tms_CommProcess()");

	int ret = 0;

	ret = Tms_Request();
	MAINLOG_L1("TMS: Tms_Request() = %d", ret);

	if (ret != 0) { return ret; }

	if (TmsTrade.trade_type == TYPE_UPDATE) { return 0; }
	if (TmsTrade.trade_type == TYPE_NOTIFY) { goto _TMS_NOTIFY_; }

	ret = Tms_DownloadUrlFilesOneByOne();
	MAINLOG_L1("TMS: Tms_DownloadUrlFilesOneByOne() = %d", ret);

	if (ret != 0) { return ret; }

	_TMS_NOTIFY_:
		ret = Tms_Notify();
		MAINLOG_L1("TMS: Tms_Notify() = %d", ret);

		if (ret == 0) {
			TmsTrade.trade_type = TYPE_UPDATE;

			ret = WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&TmsTrade, 0, sizeof(TmsTrade));
			MAINLOG_L1("TMS: WriteFile_Api() = %d", ret);
		}

		return ret;
}

void Tms_SetTermParam(u8 *oldAppVer)
{
	MAINLOG_L1("TMS: Tms_SetTermParam()");

	char tmp[64], ret;

	pRoot = NULL;

	strcpy(TmsStruct.manufacturer, "Vanstone"); 	// Manufacture
	strcpy(TmsStruct.version, 	   "1.0");        	// Version

	TmsStruct.tradeTimeoutValue = 60;               // Timeout
	strcpy(TmsStruct.oldAppVer, (char *)oldAppVer); // App_Msg.Version

	if (strlen(DEVICE_TYPE) != 0) {
		strcpy(TmsStruct.deviceType, DEVICE_TYPE);
	} else {
		MAINLOG_L1("!!! DEVICE_TYPE == NULL !!!");
		return;
	}
	strcpy(TmsTrade.deviceType, TmsStruct.deviceType); // Device Type

	sprintf(TmsStruct.sn, "%s", G_sys_param.sn); 	   // SN
}

int TmsDownload_Api(u8 *appCurrVer)
{
	MAINLOG_L1("TMS: TmsDownload_Api()");
	MAINLOG_L1(appCurrVer);

	int ret;

	t_sBuf = malloc(SENDPACKLEN);
	if (t_sBuf == NULL) {
		MAINLOG_L1("!!! t_sBuf == NULL, MALLOC FAILED !!!");
		return _TMS_E_MALLOC_NOTENOUGH;
	}

	t_rBuf = malloc(RECVPACKLEN);
	if (t_rBuf == NULL) {
		MAINLOG_L1("!!! t_rBuf == NULL, MALLOC FAILED !!!");
		free(t_sBuf);

		return _TMS_E_MALLOC_NOTENOUGH;
	}

	Tms_SetTermParam(appCurrVer); // set terminal parameters according to device

	ret = Tms_CommProcess();
	MAINLOG_L1("TMS: Tms_CommProcess() = %d", ret);

	dev_disconnect();

	free(t_sBuf);
	free(t_rBuf);

	return ret;
}

int TmsParamSet_Api(char *hostIp, char *hostPort, char *hostDomainName)
{
	MAINLOG_L1("TMS: TmsParamSet_Api()");

	memset(&TmsTrade,  0, sizeof(struct __TmsTrade__));
	memset(&TmsStruct, 0, sizeof(struct __TmsStruct__));

	if ((hostDomainName == NULL) || (strlen(hostDomainName) == 0)) { return -1; }

	strcpy(TmsStruct.hostDomainName, hostDomainName);

	if ((hostPort != NULL) && (strlen(hostPort) != 0)) {
		strcpy(TmsStruct.hostPort, hostPort);
	}
	if ((hostIp != NULL) && (strlen(hostIp) != 0)) {
		strcpy(TmsStruct.hostIP, hostIp);
	}

	return 0;
}

int TmsUpdate()
{
	MAINLOG_L1("TMS: TmsUpdate()");

	int ret;
	char tmp[32] = "";

	#ifdef TMSFILE_EXTPATH
		sprintf(tmp, "%s%s", TMS_FILE_DIR, "1e");
		MAINLOG_L1(tmp);

		ret = GetFileSize_Api(tmp);
		MAINLOG_L1("TMS: GetFileSize_Api() = %d", ret);

		if (ret <= 0) {
			ret = WriteFile_Api(tmp, (unsigned char*)"exist", 0, 5);
			MAINLOG_L1("TMS: WriteFile_Api() = %d", ret);

			if (ret != 0) { return ret; }

			ret = GetFileSize_Api(tmp);
			MAINLOG_L1("TMS: GetFileSize_Api() = %d", ret);

			if (ret <= 0) {
				PlayMP3("Creating_folder_failed", "Creating Folder Failed");
				return ret;
			}
		}
	#else
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, "%s%s", TMS_FILE_DIR, "1e");

		ret = GetFileSize_Api(tmp);
		MAINLOG_L1("TMS: GetFileSize_Api() = %d", ret);

		if (ret <= 0) {
			ret = fileMkdir_lib(TMS_FILE_DIR);
			MAINLOG_L1("TMS: fileMkdir_lib() = %d", ret);

			ret = dev_savefile(tmp, (unsigned char*)"exist", 0, 5);
			MAINLOG_L1("TMS: dev_savefile() = %d", ret);

			ret = GetFileSize_Api(tmp);
			MAINLOG_L1("TMS: GetFileSize_Api() = %d", ret);

			if (ret <= 0) {
				AppPlayTip("Creating Folder Failed");
				return ret;
			}
		}
	#endif

	memset(&tmsEntry, 0, sizeof(tmsEntry));
	tmsEntry.protocol = PROTOCOL_HTTP;

	strcpy(tmsEntry.domain, _TMS_HOST_DOMAIN_);
	strcpy(tmsEntry.port,   _TMS_HOST_PORT_);

	ret = TmsParamSet_Api(_TMS_HOST_IP_, _TMS_HOST_PORT_, _TMS_HOST_DOMAIN_);
	MAINLOG_L1("TMS: TmsParamSet_Api() = %d", ret);

	if (ret != 0) {
		AppPlayTip("Set Parameters Failed");
		return ret;
	}

	AppPlayTip("Connecting to TMS Server");

	ret = dev_connect(&tmsEntry, CONNECT_TIMEOUT);
	MAINLOG_L1("TMS: dev_connect() = %d", ret);

	if (ret != 0){
		AppPlayTip("Connect to TMS Server Failed");
		dev_disconnect();

		return ret;
	}

	ret = TmsDownload_Api((u8 *)App_Msg.Version);
	MAINLOG_L1("TMS: TmsDownload_Api() = %d", ret);

	if (ret == _TMS_E_NOFILES_D) {
		AppPlayTip("Already been the Latest Version");
		return ret;

	} else if(ret != 0) {
		AppPlayTip("Download Failed");
		return ret;
	}
	
	AppPlayTip("Updating");

	ret = TmsUpdate_Api((u8 *)App_Msg.Version);
	MAINLOG_L1("TMS: TmsUpdate_Api() = %d", ret);

	if (ret != 0) {
		AppPlayTip("Update Failed");
	}

	return ret;
}

void TmsStartUpdate(int tms_flag, char *update_type)
{
	MAINLOG_L1("TMS: TmsStartUpdate()");

	tms_tms_flag = tms_flag;

	memset(TMS_UPDATE_TYPE, 0, 			 sizeof(TMS_UPDATE_TYPE));
	memcpy(TMS_UPDATE_TYPE, update_type, strlen(update_type));

	char tmp[24] = "";
	sprintf(tmp, "TMS_UPDATE_TYPE = %s", TMS_UPDATE_TYPE);
	MAINLOG_L1(tmp);
}

// TMS Thread
void tms_TMSThread(void)
{
	MAINLOG_L1("TMS: tms_TMSThread()");

	int ret;

	MAINLOG_L1("tms_tms_flag = %d", tms_tms_flag);

	while (1)
	{
//		while (tms_tms_flag == -1) { Delay_Api(1000); }

		ret = TmsUpdate();
		MAINLOG_L1("TMS: TmsUpdate() = %d", ret);

		if (ret != 0) {
			if (!NO_UPDATE) {
				SetTmsUpdateProgress(LINE6, "Download Failed,");
				SetTmsUpdateProgress(LINE7, "Please Try Again");
			}
		}

		tms_tms_flag = -1;
	}
}
// ======================================== TMS PART ========================================

// ======================================== NETWORK PART ========================================
int dev_connect(URL_ENTRY *entry, int timeout)
{
	MAINLOG_L1("TMS: dev_connect()");
	MAINLOG_L1("TMS: dev_connect() domain akhir= %s",  entry->domain);
	MAINLOG_L1("TMS: dev_connect() port akhir= %s", entry->port);
	MAINLOG_L1("TMS: dev_connect() protocol akhir= %d", entry->protocol);

	int err = 0;

	s = net_connect(NULL, entry->domain, entry->port, timeout * 1000, entry->protocol, &err);

	if (s == NULL) { return CONNECT_ERROR; }

	return 0;
}

int dev_disconnect(void)
{
	MAINLOG_L1("TMS: dev_disconnect()");
	return net_close(s);
}

int dev_send(unsigned char *buf, int length)
{
	MAINLOG_L1("TMS: nembak api dev_send()");

	int ret = net_write(s, buf, length, 60);
	MAINLOG_L1("TMS: nembak api dev send() => ret: %d", ret);
	MAINLOG_L1("TMS: nembak api dev send() => s: %d", s);
	MAINLOG_L1("TMS: nembak api dev send() => buf: %s", buf);
	MAINLOG_L1("TMS: nembak api dev send() => length: %d", length);

	if (ret < 0) { return SEND_ERROR; }

	return 0;
}

int dev_recv(unsigned char *buf, int max, int timeout)
{
	MAINLOG_L1("TMS: nembak api dev_recv()");
	MAINLOG_L1("TMS - nembak api net_read() test buf= %s", buf);
	MAINLOG_L1("TMS - nembak api net_read() test max= %d", max);
	MAINLOG_L1("TMS - nembak api net_read() test timeout= %d", timeout);

	int ret = net_read(s, buf, max, timeout * 1000);
	MAINLOG_L1("TMS - nembak api net_read() test ret= %d", ret);
	MAINLOG_L1("TMS - nembak api net_read() test buf after= %s", buf);

	if (ret < 0) {
		return RECEIVE_ERROR;
	}

	return ret;
}
// ======================================== NETWORK PART ========================================

// ======================================== UTILS PART ========================================
int Tms_GetFileSize(struct __FileStruct__ *file)
{
	MAINLOG_L1("TMS: Tms_GetFileSize()");

	char tmp[64] = {0};
	sprintf(tmp, "%s%s", TMS_FILE_DIR, file->name);

	if ((memcmp(file->type, TYPE_APP, 3) == 0) || (memcmp(file->type, TYPE_FIRMWARE, 2) == 0)) {

		return GetFileSize_Api(tmp);

	} else if(memcmp(file->type, TYPE_PARAM, 2) == 0) {

		strcat(tmp, ".bak");
		return GetFileSize_Api(tmp);
	}

	return 0;
}

int Tms_CheckDownloadOver()
{
	MAINLOG_L1("TMS: Tms_CheckDownloadOver()");

	for (int i = 0; i < TmsTrade.fnum; i++) {
		if (TmsTrade.file[i].status != STATUS_DLCOMPLETE) { return -1; }
	}

	return 0;
}

int Tms_StartJumpSec(void)
{
	//MAINLOG_L1("TMS: Tms_StartJumpSec()");

	#ifdef JUMPER_EXIST
		_tms_g_MyJumpSec.Timer = CommStartJumpSec_Api(Tms_DispJumpSec);
	#endif

	return 0;
}

void Tms_StopJumpSec(void)
{
	//MAINLOG_L1("TMS: Tms_StopJumpSec()");

	#ifdef JUMPER_EXIST
		_tms_g_MyJumpSec.Sec = 0;
		CommStopJumpSec_Api(_tms_g_MyJumpSec.Timer);
	#endif
}

int Tms_getContentLen(u8 *packdata, int *Clen, int *Tlen)
{
	//MAINLOG_L1("TMS: Tms_getContentLen()");

	u8 *p = NULL;
	char *q = NULL,  *s = NULL, *e = NULL;

	u8 tmp[16], buf[16];

	p = packdata;

	if ((q = strstr((char *)packdata, (char *)"\r\n\r\n")) == NULL) { return ERR_NO_CONTENTLEN; }
	if ((s = strstr((char *)p, (char *)"Content-Length:")) == NULL) { return ERR_NO_CONTENTLEN; }

	s += strlen("Content-Length:");

	if ((e = strstr(s, "\r\n")) == NULL) { return ERR_CONLEN_FORMAT_WRONG; }

	if (e > q) { return ERR_CONLEN_FORMAT_WRONG; }

	memset(buf, 0, sizeof(buf));
	memset(tmp, 0, sizeof(tmp));

	memcpy(tmp,	s, e - s);

	if (Tms_Trim(tmp, buf) != 0) {
		*Clen = 0;
	} else {
		*Clen = (int)atoi((char *)buf);
	}

	*Tlen = (q - (char *)packdata + 4 + (*Clen));

	return 0;
}

int Tms_Trim(u8 *str, u8 *out)
{
	//MAINLOG_L1("TMS: Tms_Trim()");

	u8 *p = NULL;
	u8 *q = NULL;

	if ((str == NULL) || (strlen((char *)str) <= 0)) { return -1; }

	p = str;
	q = str + strlen((char *)str) - 1;

	while (p <= q) {
		if (*p != ' ') { break; }
		p++;
	}
	while (q >= p) {
		if (*q != ' ') { break; }
		q--;
	}

	memcpy(out, p, q - p + 1);

	return 0;
}

int Tms_WriteFile(struct __FileStruct__ *file, unsigned char *Buf,unsigned int Length)
{
	//MAINLOG_L1("TMS: Tms_WriteFile()");

	int ret = -1, flen = 0;
	char tmp[128] = {0};

	sprintf(tmp, "%s%s", TMS_FILE_DIR, file->name);

	if (strcmp(TYPE_PARAM, file->type) == 0) {
		strcat(tmp, ".bak");
	}

	flen = GetFileSize_Api(tmp);
	ret  = WriteFile_Api(tmp, Buf, file->startPosi, Length);

	if (ret != 0) { return _TMS_E_FILE_WRITE; }

	file->startPosi += Length;

	_tms_MD5Update(&TmsTrade.context, (unsigned char*)Buf, Length);

	if (file->startPosi == file->fsize) {
		memset(tmp, 0, sizeof(tmp));

		_tms_MD5Final((unsigned char*)tmp, &TmsTrade.context);

		if (memcmp(file->md5, tmp, 16) != 0) {
			file->startPosi = 0;
			Tms_DelFile(file);

			ret = WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&TmsTrade, 0, sizeof(TmsTrade));
			//MAINLOG_L1("TMS: WriteFile_Api() = %d", ret);

			return _TMS_E_MD5_ERR;

		} else {
			file->status = STATUS_DLCOMPLETE;
			TmsTrade.curfindex++;
		}
	} else if(file->startPosi > file->fsize) {
		file->startPosi = 0;
		Tms_DelFile(file);

		ret = WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&TmsTrade, 0, sizeof(TmsTrade));
		MAINLOG_L1("TMS: WriteFile_Api() = %d", ret);

		return _TMS_E_FILESIZE;
	}

	ret = WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&TmsTrade, 0, sizeof(TmsTrade));
	//MAINLOG_L1("TMS: WriteFile_Api() = %d", ret);

	if (ret != 0) { return _TMS_E_FILE_WRITE; }

	return 0;
}

void Tms_DelFile(struct __FileStruct__ *file)
{
	MAINLOG_L1("TMS: Tms_DelFile()");

	char tmp[64] = {0};
	sprintf(tmp, "%s%s", TMS_FILE_DIR, file->name);

	if (strcmp(TYPE_PARAM, file->type) == 0) { strcat(tmp, ".bak"); }

	int ret = DelFile_Api(tmp);
	MAINLOG_L1("TMS: DelFile_Api() = %d", ret);
}

int Tms_getDomainName(char *url, char *DomainName)
{
	MAINLOG_L1("TMS: Tms_getDomainName()");

	char *p = NULL, *q = NULL;

	if ((p = strstr(url, "//")) == NULL) { return -1; }
	if ((q = strstr(p+2, "/")) == NULL) { return -1; }

	memcpy(DomainName, p + 2, (q - p - 2));

	return (q - p - 2);
}

int Tms_CheckNeedDownFile(struct __FileStruct__ *file)
{
	MAINLOG_L1("TMS: Tms_CheckNeedDownFile()");

	char ver[64] = {0};

	if (strcmp(file->type, TYPE_APP) == 0) {
		if (strcmp(file->version, TmsStruct.oldAppVer)) { return 0; }
		else { return 1; }

	} else if (strcmp(file->type, TYPE_PARAM) == 0) {
		return 0;

	} else if (strcmp(file->type, TYPE_FIRMWARE) == 0) {
		if (Tms_GetFirmwareVerSion(ver) == 0) { // version of lib with same name in terminal is found

			if (strcmp(file->version, ver) > 0) { return 0; }
			else { return 1; }

		} else { return 0; }
	}

	return 1;
}

int Tms_GetFirmwareVerSion(char *VerSion)
{
	MAINLOG_L1("TMS: Tms_GetFirmwareVerSion()");

	unsigned char ver[64] = {0};

	int ret = sysReadBPVersion_lib(ver);
	MAINLOG_L1("TMS: sysReadBPVersion_lib() = %d", ret);

	if (ret < 0) { return -1; }

	memcpy(VerSion, ver, strlen(ver));

	return 0;
}
// ======================================== UTILS PART ========================================

// ======================================== OTHERS PART ========================================
int dev_savefile(char *filename, unsigned char *buf, int start, int len)
{
	MAINLOG_L1("TMS: dev_savefile()");

	int ret = WriteFile_Api(filename, buf, start, len);
	MAINLOG_L1("TMS: WriteFile_Api() = %d", ret);

	return 0;
}
// ======================================== OTHERS PART ========================================

/// ??? NO USE ???
int TmsStatusCheck_Api(u8 *appCurrVer)
{
	MAINLOG_L1("TMS: TmsStatusCheck_Api()");

	int i, len, ret;
	unsigned char tmp[64];

	struct __TmsTrade__ GFfile;

	len = sizeof(struct __TmsTrade__);

	ret = ReadFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, (unsigned int*)&len);
	MAINLOG_L1("TMS: ReadFile_Api() = %d", ret);

	if (ret != 0) { return -1; }

	for (i = 0; i < GFfile.fnum; i++) {
		if (GFfile.file[i].status == STATUS_DLUNCOMPLETE) { return -2; }
	}

	for (i = 0; i < GFfile.fnum; i++) {

		if (GFfile.file[i].status == STATUS_DLCOMPLETE) {

			if (strcmp(GFfile.file[i].type, TYPE_PARAM) == 0) {
				return -3;

			} else if (strcmp(GFfile.file[i].type, TYPE_FIRMWARE) == 0) {
				memset(tmp, 0, sizeof(tmp));

				if (Tms_GetFirmwareVerSion((char *)tmp) == 0) {

					if(strcmp(GFfile.file[i].version, (char *)tmp) != 0) { //lib not latest
						return -5;
					}
				}
			}
		}
	}

	for (i = 0; i < GFfile.fnum; i++) {

		if (GFfile.file[i].status == STATUS_DLCOMPLETE) {

			if (strcmp(GFfile.file[i].type, TYPE_APP) == 0) {

				if (strcmp(GFfile.file[i].version, (char *)appCurrVer) != 0) { //APP not latest
					return -4;
				}
			}
		}
	}

	ret = DelFile_Api(_DOWN_STATUS_);
	MAINLOG_L1("TMS: DelFile_Api() = %d", ret);

	return 0;
}

void CheckTmsStatus()
{
	MAINLOG_L1("TMS: CheckTmsStatus()");

	int ret = TmsStatusCheck_Api((u8 *)App_Msg.Version);
	MAINLOG_L1("TMS: TmsStatusCheck_Api() = %d", ret);

	if (ret == 0 || ret == -1) {
		return;
	} else if(ret == -2) {
		PlayMP3("DownIncompleteAgain", "Downloading Incomplete, Please Download Again");
	} else if(ret == -3 || ret == -4 || ret == -5) {
		PlayMP3("Update_locally_now", "Update Locally Now");
		TmsUpdate_Api((u8 *)App_Msg.Version);
	}
}

#ifdef JUMPER_EXIST
	void Tms_DispJumpSec(void) {
		#ifdef DISP_STH
			char DispBuf[20] = {0};

			_tms_g_MyJumpSec.Sec++;
			sprintf(DispBuf, "%d", _tms_g_MyJumpSec.Sec);
			ScrDisp_Api(LINE5, 0, DispBuf, CDISP);
		#endif
	}
#endif
