#include <coredef.h>
/*#include <struct.h>
#include <poslib.h>*/

#include "../inc/def.h"
#include "../inc/EmvCommon.h"
#include "../inc/Card.h"

int g_ucKerType = 0;
int DispQRAmtflag; //0-display  1-paid

int PiccInit(void)
{
	if( PiccOpen_Api() != 0x00 )
		return 1;
	
	return 0;
}
int PiccStop(void)
{
	if( PiccClose_Api() != 0x00 )
		return 1;
	
	return 0;
}

int PiccCheck()
{
	u8 CardType[8], SerialNo[32];
	u8 ret;
	
	memset(CardType, 0, sizeof(CardType));
	memset(SerialNo, 0, sizeof(SerialNo));

	ret = PiccCheck_Api(0, CardType, SerialNo);
	if(ret != 0x00)
		return -1;
	else
		return 0;
}

int DetectCardEvent(u8 *CardData, u8 timeoutS)
{
	u8 key;
	unsigned int timerid;
		 
	timerid = TimerSet_Api();
	while(!TimerCheck_Api(timerid, timeoutS * 1000))
	{
		key = GetKey_Api();
		if ( key == ESC )
		{
			return ESC;
		}
 
		if(PiccCheck() == 0x00)
		{
			return 0;
		}
		else
			continue;
	}
    return TIMEOUT;
}

int GetCard()
{
	u8 CardData[256];
	int ret, event;

	MAINLOG_L1("GetCard start");

	memset( CardData, 0, sizeof( CardData));
	if(PiccInit() != 0)
		return -1;

	initPayPassWaveConfig(0x00);
	CTLPreProcess();

	while(1)
	{
		ScrClrLine_Api(LINE2, LINE10);
		ScrDisp_Api(LINE3, 0, "Tap card", CDISP);
		event = DetectCardEvent(CardData, 60);
		MAINLOG_L1("DetectCardEvent == %d", event);

		switch(event)
		{
		case 0:
			Common_SetIcCardType_Api(PEDPICCCARD, 0);
			g_ucKerType = App_CommonSelKernel();
			MAINLOG_L1("App_CommonSelKernel == %d", g_ucKerType);
			if(g_ucKerType==TYPE_KER_PAYWAVE)
				ret = App_PaywaveTrans();				
			else if(g_ucKerType==TYPE_KER_PAYPASS)
				ret = App_PaypassTrans();
			else
			{
				// AppPlayTip("no app match");
				MAINLOG_L1("no app match");
				ret = -1;
			}
			return ret;
		case ESC:
		case TIMEOUT:
		default:
			ret = -1;
			return ret;
		}
	};
	return ret;
}

// test

/*
int GetTrackData(char *Inbuf )
{
	int iPos, track1Len, track2Len, track3Len;
	char track1[256],track2[256],track3[256];
	
	track1Len = track2Len =track3Len = 0;
	
	memset(track1, 0, sizeof(track1));
	memset(track2, 0, sizeof(track2));
	memset(track3, 0, sizeof(track3));

	iPos = 0;
	if(Inbuf[iPos] != 0)					 
	{ 
		memcpy(track2,&Inbuf[iPos+1], Inbuf[iPos]);
		track2Len = Inbuf[iPos];
		iPos += Inbuf[iPos];
	}
	else
	{
		return 1;
	}
	
	iPos += 1 ;							 
	                                     
	if(Inbuf[iPos] != 0)				 
	{
		memcpy(track3, &Inbuf[iPos+1], Inbuf[iPos]);
		track3Len = Inbuf[iPos];
		iPos += Inbuf[iPos];
	}
	                                         
	iPos += 1 ;								 
	                                         
	if(Inbuf[iPos] != 0)					 
	{                                        
		memcpy(track1, &Inbuf[iPos+1], Inbuf[iPos]);
		track1Len = Inbuf[iPos];
		iPos += Inbuf[iPos];
	}
	   
	if( track1Len != 0 )
	{
		memset(PosCom.stTrans.HoldCardName, 0, sizeof(PosCom.stTrans.HoldCardName));
		GetNameFromTrack1(track1, PosCom.stTrans.HoldCardName);
		memcpy(PosCom.Track1, track1,  track1Len);
		PosCom.Track1Len = track1Len;
	}
	
	if(track2Len!= 0)
	{
		FormBcdToAsc( (char *)PosCom.Track2, (u8*)track2, track2Len * 2);
		PosCom.Track2Len = track2Len;
		
		if(PosCom.Track2[strlen((char *)PosCom.Track2) -1] == 0x3f)
		{
			PosCom.Track2[strlen((char *)PosCom.Track2) -1] = 0;
			PosCom.Track2Len--;
		}
	}
	
	if(track3Len != 0)
	{
		BcdToAsc_Api((char *)PosCom.Track3, (u8*)track3, track3Len*2);
		PosCom.Track3Len = track3Len;
		if(PosCom.Track3[strlen((char *)PosCom.Track3) -1] == 0x3f)
		{
			PosCom.Track3[strlen((char *)PosCom.Track3) -1] = 0;
			PosCom.Track3Len--;
		}
	}
	
	return 0;
}
 
int GetCardFromTrack(u8 *CardNo,u8 *track2,u8 *track3)
{
	int i;
	
	track2[37] = 0;
	track3[104] = 0;
	
	if(strlen((char *)track2) != 0)			 
	{
		i = 0;
		while (track2[i] != '=')
		{
			if(i > 19)
			{
				return E_ERR_SWIPE;
			}
			i++;
		}
		if( i < 13 || i > 19)
		{
			return E_ERR_SWIPE;
		}
		memcpy(CardNo, track2, i);		
		CardNo[i] = 0;
	}
	else if(strlen((char *)track3 )!= 0) 
	{
		i = 0;
		while(track3[i] != '=') {
			if(i > 21)
			{
				return E_ERR_SWIPE;
			}
			i++;
		}			    
		if( i < 15 || i > 21)
		{
			return E_ERR_SWIPE;	
		}
		memcpy(CardNo,track3+2,i-2);		
		CardNo[i-2]=0;
	}
	
	return 0;
}
 
int DispCardNumber(char *CardNo,int len)
{
	ScrDisp_Api(LINE2, 0, "Confirm The Number:", LDISP);
	ScrDisp_Api(LINE3, 0, CardNo, RDISP);
	ScrDisp_Api(LINE4, 0, "According Continue", RDISP);
	if(WaitEnterAndEscKey_Api(30) != ENTER)
	{
		return(E_TRANS_CANCEL);
	}
	
	return 0;
}
 */

void TransTapCard()
{
	int ret = 0, amt;
	unsigned char tmp[32], buf[12];

	memset(&PosCom, 0, sizeof(PosCom));

	ScrCls_Api();
	ScrDisp_Api(LINE1, 0, "        Sale               ", CDISP);
	ScrDisp_Api(LINE3, 0, "Please input amount:", LDISP);
	if(GetAmount(PosCom.stTrans.TradeAmount) != 0)
		return ;

	ret = GetCard();
	MAINLOG_L1("GetCard: %d", ret);
	if((ret != 0) || (DispCardNo() != 0))
	{
		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "        Sale               ", CDISP);
		ScrDisp_Api(LINE3, 0, "Transaction failed.", CDISP);
		AppPlayTip("Transaction failed.");
		return ;
	}

	ScrCls_Api();
	ScrDisp_Api(LINE1, 0, "        Sale               ", CDISP);
	ScrDisp_Api(LINE3, 0, "Connecting...", CDISP);
	Delay_Api(2000);
	ScrDisp_Api(LINE3, 0, "Sending MSG...", CDISP);
	Delay_Api(2000);
	ScrDisp_Api(LINE3, 0, "Receiving MSG...", CDISP);
	Delay_Api(2000);

	//send data
	//receive data
	//if(PosCom.stTrans.EntryMode[0] == PAN_PAYWAVE)
	 //	ret = PaywaveTransComplete();

	if(ret == 0)
	{
		//ScrDisp_Api(LINE1, 0, "        Sale               ", CDISP);
		//ScrDisp_Api(LINE3, 0, "RespCode: 00", LDISP);
		//ScrDisp_Api(LINE4, 0, "Trade: Success", LDISP);

		amt = BcdToLong_Api(PosCom.stTrans.TradeAmount, 6);
		memset(buf, 0, sizeof(buf));
		memset(tmp, 0, sizeof(tmp));
		sprintf(buf, "%d.%02d", amt/100, amt%100);
		sprintf(tmp, "Paid:%s", buf);

		ScrCls_Api();
		ScrDisp_Api(LINE1, 0, "        Sale               ", CDISP);
		ScrDisp_Api(LINE5, 0, tmp, CDISP);
		MAINLOG_L1( "PlayEnArabicAmt hah ");
		PlayEnArabicAmt(buf);
		WaitAnyKey_Api(5);
	}

	secscrCls_lib();
	secscrSetBackLightValue_lib(0);  //secscrSetBackLightValue_lib is necessary before secscrSetBackLightMode_lib
	secscrSetBackLightMode_lib(0, 300);

	PiccStop();
}

#define QR_BMP "QR.bmp"
void QRDisp(unsigned char *str)
{
	unsigned char OutBuf[1024];
 	unsigned char tmp[128], buf[24];
	int ret;
	ret = QREncodeString(str, 3, 3, QR_BMP, 5);
//	"upi://pay?ver=01&mode=22&tr=27019964b16b46d5b13795aaecff18ff&pa=LC1111111111.lyra@rbl&pn=Testmerchant&mc=0000&am=100.00&cu=INR&qrMedium=02"
//	ret = QREncodeString("upi://pay?ver=01&mode=22&tr=27019964b16b46d5b13795aaecff18ff&pa=LC1111111111.lyra@rbl&pn=Testmerchant&mc=0000&am=100.00&cu=INR&qrMedium=02", 3, 3, QR_BMP, 3);
	ScrCls_Api();
	ScrDisp_Api(LINE1, 0,"tete", CDISP);
//	ScrDisp_Api(LINE1, 0,"isinya", CDISP);
//	ScrDispImage_Api(QR_BMP, 23, 10);

//	ScrDispImage_Api(AISINO_LOGO, 86, 94);
}

void TransGenQRcode()
{
 	int ret = 0;
 	long amt;
 	unsigned char key;
 	unsigned char tmp[128], buf[24];

 	memset(&PosCom, 0, sizeof(PosCom));

 	ScrCls_Api();
 	ScrDisp_Api(LINE1, 0, "        Sale               ", CDISP);
 	ScrDisp_Api(LINE3, 0, "Please input amount:", LDISP);
 	if(GetAmount(PosCom.stTrans.TradeAmount) != 0)
 		return ;

 	amt = BcdToLong_Api(PosCom.stTrans.TradeAmount, 6);
 	memset(tmp, 0, sizeof(tmp));
 	sprintf(buf, "%d.%02d", amt/100, amt%100);

	memset(tmp, 0, sizeof(tmp));
	//"http://115.159.28.147:54321/default.htm?id=topic_2&msg=123.45"
//	sprintf(tmp, "http://115.159.28.147:54321/default.htm?id=%s&msg=%s", G_sys_param.mqtt_topic, buf);

	MAINLOG_L1("amount:%s", tmp);
	DispQRAmtflag = 0;
	QRDisp(tmp);
//	testApi();

//	memset(tmp, 0, sizeof(tmp));
//	sprintf(tmp, "Amount:%s", buf);
//	ScrDisp_Api(10, 0, tmp, CDISP);

	while(1)
	{
		key = GetKey_Api();
		if ( key == ESC )
			break;

		if(DispQRAmtflag == 0)
			Delay_Api(1000);
		else
		{
			Delay_Api(3000);
			break;
		}
	}
}

void UnzipMp3()
{
	int ret;

	AppPlayTip("Processing");
	ret = fileunZip_lib("/ext/mp3.zip", "/ext/");
	MAINLOG_L1( "fileunZip_lib: %d", ret);
	AppPlayTip("Done");
}

//int testApi() {
//    char url[] = "https://cool.uruz.id/api/trans-nobu/qrAisino";
//    char postData[] =
//        "{"
//            "\"referenceNo\": \"uruz_firman\","
//            "\"amount\": 5000,"
//            "\"validTime\": 900,"
//            "\"source\": \"aisino\","
//            "\"branch_id\": \"PUSAT\""
//        "}";
//
//    int result = httpPost(url, postData);
//
//    if (result == 0) {
//        printf("HTTP POST request successful.\n");
//    } else {
//        printf("HTTP POST request failed with error code: %d\n", result);
//    }
//
//}
