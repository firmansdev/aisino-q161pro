#include <string.h>
#include "../inc/def.h"
#include "../inc/EmvCommon.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#define		MAX_LCDWIDTH				21

struct _CtrlParam gCtrlParam;

int GetScanf(u32 mode, int Min, int Max, char *outBuf, u32 TimeOut, u8 StartRow, u8 EndRow, int howstr)
{
	return GetScanfEx_Api(mode, Min, Max, outBuf, TimeOut, StartRow, EndRow, howstr, MMI_NUMBER);
}

int GetAmount(u8 *pAmt)
{
	int ret;
	char buf[32], temp[32];

	memset( buf, 0, sizeof( buf));
	memset( temp, 0, sizeof( temp));

	while(1)
	{
		memset(buf, 0, sizeof(buf));
		ret = GetScanf(MMI_POINT, 1, 12, buf, 60, LINE4, LINE4, MAX_LCDWIDTH);
		if(ret == ENTER)
		{
			memset(temp, 0x30, 12-buf[0]);
			strcpy(&temp[12-buf[0]], &buf[1]);
			AscToBcd_Api(pAmt, temp, 12);
			return 0;
		}
		else
		{
			return -1;
		}
	}
}


void GetPanNumber()
{
	int tlvLen = 0, i,ret;
	u8 buf[64];
	u8 TEM[64];

	memset(buf, 0, sizeof(buf));
	memset(TEM, 0, sizeof(TEM));
	ret = Common_GetTLV_Api(0x5A, buf, &tlvLen);
	if(ret == 0) //
	{
		BcdToAsc_Api(PosCom.stTrans.MainAcc, buf, tlvLen * 2);
		for(i = tlvLen * 2 - 1; i >= 0; --i)
		{
			if(PosCom.stTrans.MainAcc[i] == 'F' || PosCom.stTrans.MainAcc[i] == 'f')
				PosCom.stTrans.MainAcc[i] = 0;
			else
				break;
		}
	}
	else
	{
		ret = Common_GetTLV_Api(0x57, buf, &tlvLen);
		if (ret == 0)
		{

			BcdToAsc_Api(TEM, buf, tlvLen * 2);
			for(i = tlvLen * 2 - 1; i >= 0; --i)
			{
				if(TEM[i] == 'D'){
					TEM[i] = 0;
					memcpy(PosCom.stTrans.MainAcc,TEM, i);
					break;
				}
			}
		}
	}

}

int EnterPIN(u8 flag)
{
	int ret;
	u8 DesFlag;
	u8 temp[128];
	char dispbuf[32];

	memset(dispbuf,0,sizeof(dispbuf));
	ScrClrLine_Api(LINE2, LINE5);
	if(flag != 0)
		ScrDisp_Api(LINE2, 0, "Password Is Wrong, Please Input Again:", LDISP);
	else
		ScrDisp_Api(LINE2, 0, "Please Input Password:", LDISP);

	if(gCtrlParam.DesType == 1) //Des
		DesFlag = 0x01;
	else
		DesFlag = 0x03;


	ret = PEDGetPwd_Api(gCtrlParam.PinKeyIndes, 4, 8, PosCom.stTrans.MainAcc, PosCom.sPIN, DesFlag);

	if(ret != 0)
		return -1;

	if(memcmp(PosCom.sPIN, "\0\0\0\0\0\0\0\0", 8) == 0)
		PosCom.stTrans.EntryMode[1] = PIN_NOT_INPUT;
	else
		PosCom.stTrans.EntryMode[1] = PIN_HAVE_INPUT;

	return 0;
}

int GetCardNoFromTrack2Data(char *cardNo, u8 *track2Data)
{
	char tmp[MCARDNO_MAX_LEN + 2], *p = NULL ;
	u32 len;

	memset(tmp, 0, sizeof(tmp));

	len = MCARDNO_MAX_LEN+1 < track2Data[0]*2 ? MCARDNO_MAX_LEN+1 : track2Data[0]*2;

	FormBcdToAsc( tmp, track2Data+1, len);
	tmp[len] = '\0';
	p = strchr(tmp, '=');//
	if(p != NULL)
	{
		*p = 0;
		strcpy(cardNo, tmp);
		return TRUE;
	}

	return FALSE;
}

int DispCardNo(void)
{
	int iRet;
	u8 pTrackBuf[256];

	memset (pTrackBuf, 0, sizeof(pTrackBuf));
	iRet = GetEmvTrackData(pTrackBuf);
	if(iRet == 0)
	{
		GetCardNoFromTrack2Data(PosCom.stTrans.MainAcc, pTrackBuf);
		//ScrClrLineRam_Api(LINE2, LINE5);
		//ScrCls_Api(line);
		ScrClrLine_Api(LINE2, LINE10);
		ScrDispRam_Api(LINE3, 0,"PLS confirm:", LDISP);
		ScrDispRam_Api(LINE4, 0,PosCom.stTrans.MainAcc, RDISP);
		ScrDispRam_Api(LINE6, 0,"ENTER to continue", RDISP);
		ScrBrush_Api();
	}
	KBFlush_Api ();
	iRet = WaitEnterAndEscKey_Api(30);
	if(iRet != ENTER)
		return ERR_NOTACCEPT;
	return 0;
}

void RemoveTailChars(char* pString, char cRemove)
{
	int nLen = 0;

	nLen = strlen(pString);
	while(nLen)
	{
		nLen--;
		if(pString[nLen] == cRemove)
			pString[nLen] = 0;
		else
			break;
	};
}
int MatchTrack2AndPan(u8 *pTrack2, u8 *pPan)
{
	int  i = 0;
	char szTemp[19+1], sTrack[256], sPan[256];

	memset(szTemp, 0, sizeof(szTemp));
	memset(sTrack, 0, sizeof(sTrack));
	memset(sPan, 0, sizeof(sPan));

	//track2
	BcdToAsc_Api(sTrack, &pTrack2[1], (u16)(pTrack2[0]*2));
	RemoveTailChars(sTrack, 'F');		// erase padded 'F' chars
	for(i=0; sTrack[i]!='\0'; i++)		// convert 'D' to '='
	{
		if(sTrack[i]=='D' )
		{
			sTrack[i] = '=';
			break;
		}
	}
	for(i=0; i<19 && sTrack[i]!='\0'; i++)
	{
		if(sTrack[i] == '=') break;
		szTemp[i] = sTrack[i];
	}
	szTemp[i] = 0;
	//pan
	BcdToAsc_Api(sPan, &pPan[1], (int)(pPan[0]*2));
	RemoveTailChars(sPan, 'F');         // erase padded 'F' chars

	if(strcmp(szTemp, sPan)==0)
		return 0;
	else
		return 1;
}
