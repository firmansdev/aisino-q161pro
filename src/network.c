/*
 * network.c
 *
 *  Created on: 2021��10��14��
 *      Author: vanstone
 */
#include <stdio.h>
#include <string.h>

#include "../inc/def.h"
#include "../inc/httpDownload.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

//int wirelessPdpOpen_lib(void);
int wirelessPppOpen_lib(unsigned char *pucApn, unsigned char *pucUserName, unsigned char *pucPassword) ;
//0-success   others-failed
int wirelessCheckPppDial_lib(void); //int wirelessCheckPdpDial_lib(int timeout);
int wirelessPppClose_lib(void) ;//int wirelessPdpRelease_lib(void);
int wirelessSocketCreate_lib(int nProtocol);
int wirelessSocketClose_lib(int sockid);
int wirelessTcpConnect_lib(int sockid, char *pucIP, char *pucPort, int timeout);//Q161
//int wirelessTcpConnect_lib(int sockid, char *pucIP, unsigned int uiPort); //Q181
int wirelessSend_lib(int sockid, unsigned char *pucdata, unsigned int iLen);
int wirelessRecv_lib(int sockid, unsigned char *pucdata, unsigned int iLen, unsigned int uiTimeOut);

//int wirelessSslSetTlsVer_lib(int ver);
int wirelessSetSslVer_lib(unsigned char ucVer) ;
void wirelessSslDefault_lib(void);
int wirelessSendSslFile_lib (unsigned char ucType, unsigned char *pucData, int iLen);
int wirelessSetSslMode_lib(unsigned char  ucMode);
int wirelessSslSocketCreate_lib(void);
int wirelessSslSocketClose_lib(int sockid);
//timeout: ms   return :0-success <0-failed
int wirelessSslConnect_lib(int sockid, unsigned char *pucDestIP, char *pucPort, int timeout);  //for Q161
//int wirelessSslConnect_lib(int sockid, char *pucDestIP, unsigned short pucDestPort); //for Q181
int wirelessSslSend_lib(int sockid, unsigned char *pucdata, unsigned int iLen);
int wirelessSslRecv_lib(int sockid, unsigned char *pucdata, unsigned int iLen, unsigned int uiTimeOut);
int wirelessSetDNS_lib(unsigned char *pucDNS1, unsigned char *pucDNS2 );

typedef struct {
	int valid;
	int socket;
	int ssl;
} NET_SOCKET_PRI;

struct WiFiNetwork {
    const char *ssid;
};

// FIXME: What if the caller doesn't close the socket?? (already fixed by chatgpt, chatgpt > average chinese developer)
#define	MAX_SOCKETS		1
#define CERTI_LEN  4096

static NET_SOCKET_PRI sockets[MAX_SOCKETS];

EXTERN _IS_GPRS_ENABLED_;

void net_init(void)
{
	int i;

	for (i = 0; i < MAX_SOCKETS; i++)
		memset(&sockets[i], 0, sizeof(NET_SOCKET_PRI));
}

int getCertificate(char *cerName, unsigned char *cer){
	int Cerlen, Ret;
	u8 *CerBuf = (u8*)malloc(CERTI_LEN);

	Cerlen = GetFileSize_Api(cerName);
	MAINLOG_L1("get certificate cerlen - %d",Cerlen);
	if(Cerlen <= 0)
	{
		MAINLOG_L1("get certificate err or not exist - %s",cerName);
		free(CerBuf);
		return -1;
	}

	memset(CerBuf, 0 , sizeof(CerBuf));
	if(Cerlen > CERTI_LEN)
	{
		MAINLOG_L1("%s is too large", cerName);
		free(CerBuf);
		return -2;
	}

	Ret = ReadFile_Api(cerName, CerBuf, 0, (unsigned int *)&Cerlen);
	if(Ret != 0)
	{
		MAINLOG_L1("read %s failed", cerName);
		free(CerBuf);
		return -3;
	}
	MAINLOG_L1("certificate isi sertinya - %s",CerBuf);
	memcpy(cer, CerBuf, CERTI_LEN);
	MAINLOG_L1("Setelah memcpy certificate");

	free(CerBuf);

	return 0;
}



void *net_connect(void* attch, const char *host,const char *port, int timerOutMs, int ssl, int *errCode)
{
	int ret;
	int port_int;
	int sock;
	int i;
	u8 CerBuf[CERTI_LEN];
	int timeid = 0;
	int maxSocket= 0;
	
#ifdef __WIFI__

	// Find an empty socket slot
	if((ssl ==1) || (ssl == 2)){
		maxSocket = 1;
	}else{
		maxSocket = MAX_SOCKETS;
	}
	for (i = 0; i < maxSocket; i++) {
		if (sockets[i].valid == 0)
			break;
	}

	if (i >= maxSocket) {
		*errCode = -3;
		MAINLOG_L1("No empty socket slot");
		return NULL;
	}

	if(ssl == 0){
		// >=0 success  , others -failed
		sock = wifiSocketCreate_lib(0); //0-TCP   1-UDP
		MAINLOG_L1("wifiSocketCreate_lib:%d", ret);
		if(sock < 1) {
			*errCode = -4;
			MAINLOG_L1("wifiSocketCreate_lib failed");
			return NULL;
		}

		ret = wifiTCPConnect_lib(sock, host, port, 60*1000);
		MAINLOG_L1("wifiTCPConnect_lib:%d", ret);
		if(ret != 0)
		{
			*errCode = -5;
			wifiTCPClose_lib(sock);
			ret =  wifiSocketClose_lib(sock);
			MAINLOG_L1("wifiSocketClose_lib 1 failed:%d", ret);
			return NULL;
		}
	}else if(ssl == 1){
		sock = wifiSSLSocketCreate_lib();
		MAINLOG_L1("wifiSSL_sockID = %d", sock);

		if(sock < 1){
			return sock;
		}

		ret = WifiespSetSSLConfig_lib(sock, 0);
		MAINLOG_L1("wifiSSL_WifiespSetSSLConfig_lib = %d", ret);

		ret = wifiSSLConnect_lib(sock, (char *)host, port, 90*1000);
		if(ret != 0){
			*errCode = -5;
			wifiSSLClose_lib(sock);
			MAINLOG_L1("wifiSocketClose_lib 2 failed:%d", ret);
			return NULL;
		}

		MAINLOG_L1("wifiSSL_wifiSSLConnect_lib ret = %d, sock = %d", ret, sock);

		sockets[i].valid = 1;
		sockets[i].ssl = ssl;
		sockets[i].socket = sock;

		*errCode = 0;

		if(ret != 0){
			wifiSSLClose_lib(sock);
			*errCode = -5;
			MAINLOG_L1("wifiSocketClose_lib 3 failed:%d", ret);
			return NULL;
		}
	}
	else if(ssl == 2){

		memset(CerBuf, 0 , sizeof(CerBuf));
		ret = getCertificate(FILE_CERT_ROOT, CerBuf);
		MAINLOG_L1("wifiSSL_getCertificate1 = %d", ret);
		ret = WifiespDownloadCrt_lib(0, CerBuf, strlen((char *)CerBuf));
		MAINLOG_L1("wifiSSL_WifiespDownloadCrt_lib1 = %d" , ret );
		memset(CerBuf, 0 , sizeof(CerBuf));

		ret = getCertificate(FILE_CERT_PRIVATE, CerBuf);
		MAINLOG_L1("wifiSSL_getCertificate2 = %d", ret);
		ret = WifiespDownloadCrt_lib(2, CerBuf, strlen((char *)CerBuf));
		MAINLOG_L1("wifiSSL_WifiespDownloadCrt_lib2 = %d" , ret );
		memset(CerBuf, 0 , sizeof(CerBuf));

		ret = getCertificate(FILE_CERT_CHAIN, CerBuf);
		MAINLOG_L1("wifiSSL_getCertificate3 = %d", ret);
		ret = WifiespDownloadCrt_lib(1, CerBuf, strlen((char *)CerBuf));
		MAINLOG_L1("wifiSSL_WifiespDownloadCrt_lib3 = %d" , ret );

		Delay_Api(50);

		sock = wifiSSLSocketCreate_lib();
		MAINLOG_L1("wifiSSL_sockID = %d", sock);

		if(sock < 1){
			return sock;
		}

		ret = WifiespSetSSLConfig_lib(sock, 3);
		MAINLOG_L1("wifiSSL_WifiespSetSSLConfig_lib = %d", ret);

		ret = wifiSSLConnect_lib(sock, (char *)host, port, 90*1000);
		if(ret != 0){
			*errCode = -5;
			wifiSSLClose_lib(sock);
			MAINLOG_L1("wifiSocketClose_lib 4 failed:%d", ret);
			return NULL;
		}

		MAINLOG_L1("wifiSSL_wifiSSLConnect_lib ret = %d, sock = %d", ret, sock);

		if(ret != 0){
			*errCode = -5;
			wifiSSLClose_lib(sock);
			MAINLOG_L1("wifiSSLClose_lib haha:%d", ret);
			return NULL;

		}
	}

	sockets[i].valid = 1;
	sockets[i].ssl = ssl;
	sockets[i].socket = sock;

	*errCode = 0;
	return &sockets[i];

#else

	timeid = TimerSet_Api();
	while(1)
	{
		ret = wirelessCheckPppDial_lib();
		MAINLOG_L1("wirelessCheckPppDial_lib = %d" , ret );
		if(ret == 0)
			break;
		else{
			MAINLOG_L1("timerOutMs = %d  %d" , timerOutMs , ret );
			ret = wirelessPppOpen_lib(NULL, NULL, NULL);
			MAINLOG_L1("wirelessPppOpen_lib = %d" , ret );
//			Delay_Api(1000);
//			wirelessSetGSMorLTE_lib(1);
//			ret = wirelessPppOpen_lib(NULL, NULL, NULL);
//			MAINLOG_L1("wirelessPppOpen_lib = %d" , ret );
//			Delay_Api(1000);
		}
		if(TimerCheck_Api(timeid , timerOutMs) == 1)
		{
			MAINLOG_L1("wirelessCheckPppDial_lib 2 = %d" , ret);
			*errCode = -1;
			return NULL;
		}
	}
#ifdef __SETDNS__
	ret =  wirelessSetDNS_lib("114.114.114.114", "8.8.8.8" );
	MAINLOG_L1("wirelessSetDNS_lib = %d" , ret);
#endif
	// Find an empty socket slot
	for (i = 0; i < MAX_SOCKETS; i++) {
		if (sockets[i].valid == 0)
			break;
	}

	if (i >= MAX_SOCKETS) {
		*errCode = -3;

		return NULL;
	}

	if (ssl == 0) {
		MAINLOG_L1("ssl = 0");
		sock = wirelessSocketCreate_lib(0);
		if (sock < 1) {
			*errCode = -4;
			MAINLOG_L1("wirelessSocketCreate_lib = %d" , sock );
			return NULL;
		}
	}
	else {
		wirelessSslDefault_lib();		
		wirelessSetSslVer_lib(0);
		
		if (ssl == 2) {
			MAINLOG_L1("ssl = 2");

			ret = wirelessSetSslMode_lib(1);

			memset(CerBuf, 0 , sizeof(CerBuf));
			ret = getCertificate(FILE_CERT_ROOT, CerBuf);
			MAINLOG_L1("getCertificate === %d", ret);
			ret = wirelessSendSslFile_lib(2, CerBuf, strlen((char *)CerBuf));
			memset(CerBuf, 0 , sizeof(CerBuf));

			ret = getCertificate(FILE_CERT_PRIVATE, CerBuf);
			MAINLOG_L1("getCertificate === %d", ret);
			ret = wirelessSendSslFile_lib(1, CerBuf, strlen((char *)CerBuf));
			memset(CerBuf, 0 , sizeof(CerBuf));

			ret = getCertificate(FILE_CERT_CHAIN, CerBuf);
			MAINLOG_L1("getCertificate === %d", ret);
			ret = wirelessSendSslFile_lib(0, CerBuf, strlen((char *)CerBuf));
			MAINLOG_L1("wirelessSendSslFile_lib = %d" , ret );
		}
		else
		{
			MAINLOG_L1("ssl = %d", ssl);
			ret = wirelessSetSslMode_lib(0);
			MAINLOG_L1("wirelessSetSslMode_lib = %d" , ret );
		}

		sock = wirelessSslSocketCreate_lib();
		MAINLOG_L1("wirelessSslSocketCreate_lib = %d" , sock );
		if (sock == -1) {
			*errCode = -4;
			return NULL;
		}
	}

	if (ssl == 0){
		ret = wirelessTcpConnect_lib(sock, (char *)host, port, 60*1000);
		MAINLOG_L1("wirelessTcpConnect_lib ret:%d host:%s  port:%s  ", ret , host, port );
	}else
	{
		ret = wirelessSslConnect_lib(sock, (char *)host, port, 90*1000);
		MAINLOG_L1("wirelessSslConnect_lib ret:%d host:%s  port:%s  ", ret , host, port );
	}

	ScrClrLine_Api(LINE7, LINE7);
	if (ret != 0) {
		if (ssl == 0)
			wirelessSocketClose_lib(sock);
		else
			wirelessSslSocketClose_lib(sock);

		*errCode = -5;
		ScrDisp_Api(LINE7, 0, "Connect failed", CDISP);

		return NULL;
	}
	else
		ScrDisp_Api(LINE7, 0, "Connect success", CDISP);

	sockets[i].valid = 1;
	sockets[i].ssl = ssl;
	sockets[i].socket = sock;

	*errCode = 0;

	return &sockets[i];

#endif
}

int net_close(void *netContext)
{
	int ret;
	NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

	if (sock == NULL)
		return -1;

	if (sock->valid == 0)
		return 0;

#ifdef __WIFI__
	if(sock->ssl == 0){
		ret = wifiTCPClose_lib(sock->socket);
		MAINLOG_L1("wifiTCPClose_lib:%d", ret);

		ret =  wifiSocketClose_lib(sock->socket);
		MAINLOG_L1("wifiSocketClose_lib:%d", ret);
	}else {
		ret = wifiSSLClose_lib(sock->socket);
		MAINLOG_L1("wifiSSLClose_lib:%d, socket: %d", ret, sock->socket);

		ret = wifiSSLSocketClose_lib(sock->socket);
		MAINLOG_L1("wifiSSLSocketClose_lib:%d", ret);
	}

#else
	if (sock->ssl == 0){
		ret = wirelessSocketClose_lib(sock->socket);
		MAINLOG_L1("wirelessSocketClose_lib: %d", ret);
	}
	else{
		ret = wirelessSslSocketClose_lib(sock->socket);
		MAINLOG_L1("wirelessSslSocketClose_lib: %d", ret);
	}
#endif
	sock->valid = 0;

	return 0;
}

int net_read(void *netContext, unsigned char* recvBuf, int needLen, int timeOutMs)
{
	int ret;
	NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

	if (sock == NULL)
		return -1;

	if (sock->valid == 0)
		return -1;

#ifdef __WIFI__
	if(sock -> ssl == 0){
//		MAINLOG_L1("wifiRecv, Receive start, needLen = %d", needLen);
		ret = wifiRecv_lib(sock->socket, recvBuf, needLen, timeOutMs);
//		MAINLOG_L1("wifiRecv, Receive end, ret = %d", ret);
	}else{
//		MAINLOG_L1("wifiSSLRecv, Receive start, needLen = %d", needLen);
		ret = wifiSSLRecv_lib(sock->socket, recvBuf, needLen, timeOutMs);
//		MAINLOG_L1("wifiSSLRecv, Receive end, ret = %d", ret);
	}

	if(ret == 0)
		return 0;
	else if(ret < 0)
	{
		return -1;
	}
	else
		return ret;

#else
	if (sock->ssl == 0){
		ret = wirelessRecv_lib(sock->socket, recvBuf, 1, timeOutMs);
		MAINLOG_L1("wirelessRecv_lib:%d", ret);
	}else
	{
#ifdef __RECVLARGEBUF__
		ret = wirelessSslRecvLoop_lib(sock->socket, recvBuf, 1, timeOutMs);
		MAINLOG_L1("wirelessSslRecvLoop_lib:%d", ret);
#else
		ret = wirelessSslRecv_lib(sock->socket, recvBuf, 1, timeOutMs);
		MAINLOG_L1("wirelessSslRecv_lib:%d", ret);
#endif		
	}

	if (ret == 0)
		return 0;

	if (ret != 1)
	{
#ifdef __RECVLARGEBUF__
		MAINLOG_L1("wirelessSslRecvLoop_lib_1 ret = %d  explen:1  timeOutMs:%d" , ret, timeOutMs );
#endif
		return -1;
	}

	if (needLen == 1)
		return 1;

	if (sock->ssl == 0){
		ret = wirelessRecv_lib(sock->socket, recvBuf + 1, needLen - 1, 10);
		MAINLOG_L1("wirelessRecv_lib:%d", ret);
	}else
	{
#ifdef __RECVLARGEBUF__
		ret = wirelessSslRecvLoop_lib(sock->socket, recvBuf + 1, needLen - 1, 100);
		MAINLOG_L1("wirelessSslRecvLoop_lib:%d", ret);
#else
		ret = wirelessSslRecv_lib(sock->socket, recvBuf + 1, needLen - 1, 10);
		MAINLOG_L1("wirelessSslRecv_lib:%d", ret);
#endif
	}

	if(ret < 0)
	{
#ifdef __RECVLARGEBUF__
		MAINLOG_L1("wirelessSslRecvLoop_lib_2 ret = %d  explen:%d timeout:100" , ret , needLen - 1);
#endif
		return -1;
	}

	return ret + 1;
#endif
}

int net_write(void *netContext, unsigned char* sendBuf, int sendLen, int timeOutMs)
{
	int ret;
	NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

	if (sock == NULL){
		MAINLOG_L1("sock == NULL");
		return -1;
	}

	if (sock->valid == 0){
		MAINLOG_L1("sock->valid == 0");
		return -1;
	}

#ifdef __WIFI__
	MAINLOG_L1("ssl = %d", sock->ssl);
	if (sock->ssl == 0){
		ret = wifiSend_lib(sock->socket, sendBuf, sendLen, timeOutMs * 1000);
		MAINLOG_L1("wifiSend_lib :%d" , ret);
	}else {
		ret = wifiSSLSend_lib(sock->socket, sendBuf, sendLen, timeOutMs * 1000);
		MAINLOG_L1("wifiSSLSend_lib :%d" , ret);
	}

	if(ret == 0)
		return sendLen;
	else
		return -1;

#else
	if (sock->ssl == 0){
		ret = wirelessSend_lib(sock->socket, sendBuf, sendLen);
		MAINLOG_L1("wirelessSend_lib :%d" , ret);
		return ret;
	}else{
		ret = wirelessSslSend_lib(sock->socket, sendBuf, sendLen);
		MAINLOG_L1("wirelessSslSend_lib :%d" , ret);
		return ret;
	}
#endif
}



int WifiConnect()
{
#ifdef __WIFI__
	ScrCls_Api();
	ScrDisp_Api(1, 0, "Scan Wifi...", LDISP);

	int ret;
	unsigned int timerid;
	const int timeoutS = 4 * 60;
	const int MAX_AP = 9;
	struct WIFI_SCAN_AP apArray[MAX_AP];
	unsigned char outBuf[17];
	memset(wifiId, 0, sizeof(wifiId));
	memset(wifiPwd, 0, sizeof(wifiPwd));
	for(int i = 0; i < MAX_AP; i++){
		memset(&apArray[i], 0, sizeof(apArray[i]));
	}
	NetModuleOper_Api(GPRS, 0);
	ret = NetModuleOper_Api(WIFI, 1);
	MAINLOG_L1("NetModuleOper_Api wifi:%d",ret);
	ret = wifiOpen_lib();
	MAINLOG_L1("wifiOpen_lib:%d", ret);
	wifiAPDisconnect_lib();



//	while(!TimerCheck_Api(timerid, timeoutS * 500)){

		char *pszTitle = "Setting WIFI";
			ret = CommWifiScanAp_Api(apArray, MAX_AP, 1);
			int nSelcItem = 0;
			char *pszItems[MAX_AP];
		    int MAX_STRING_LENGTH = 100;

				if(ret > 0){
					for(int i = 0; i < ret; i++){
						MAINLOG_L1("SSID: %s, Securety: %d", &apArray[i].SSID, &apArray[i].Security);
		//				pszItems[i] = apArray[i].SSID;
						pszItems[i] = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
						snprintf(pszItems[i], MAX_STRING_LENGTH, "%d. %s", i, apArray[i].SSID);
						MAINLOG_L1("isinya apa %s",pszItems[i]);
					}
					nSelcItem =ShowMenuItem(pszTitle, pszItems, sizeof(pszItems)/sizeof(char *), DIGITAL0, DIGITAL9, 0, 60);
		//			ScrDisp_Api(1, 0, nSelcItem, LDISP);
					switch(nSelcItem)
							{
							case DIGITAL0:
								inputPassw(ret,apArray[0].SSID);
								break;
							case DIGITAL1:
								inputPassw(ret,apArray[1].SSID);
								break;
							case DIGITAL2:
								inputPassw(ret,apArray[2].SSID);
								break;
							case DIGITAL3:
								inputPassw(ret,apArray[3].SSID);
								break;
							case DIGITAL4:
								inputPassw(ret,apArray[4].SSID);
								break;
							case DIGITAL5:
								inputPassw(ret,apArray[5].SSID);
								break;
							case DIGITAL6:
								inputPassw(ret,apArray[6].SSID);
								break;
							case DIGITAL7:
								inputPassw(ret,apArray[7].SSID);
								break;
							case DIGITAL8:
								inputPassw(ret,apArray[8].SSID);
								break;
							case DIGITAL9:
								inputPassw(ret,apArray[9].SSID);
								break;
							case ESC:
								return;
							default:
								break;
							}
				}

	return ret;
#endif
}

int inputPassw(ret,apArray){
	unsigned char outBuf[17];
	PWD: ScrCls_Api();
	ScrDisp_Api(1, 0, "PLS input password", LDISP);
	memset(outBuf, 0, sizeof(outBuf));
	ret = GetScanfEx_Api(MMI_NUMBER|MMI_LETTER, 8, 16, outBuf, 10000, 5, 5, 20, MMI_LETTER);
	if(ret == 0x0D){
		memcpy(wifiId, apArray, strlen(apArray));
		memcpy(wifiPwd, outBuf + 1, outBuf[0]);
		MAINLOG_L1("wifi_ap: %s", wifiId);
		MAINLOG_L1("wifi_pwd: %s", wifiPwd);
		ScrCls_Api();
		ScrDisp_Api(1, 0, "Connecting...", LDISP);
		ret = wifiAPConnect_lib(wifiId, wifiPwd);
		saveSSID("/ext/ssid.txt",wifiId);
		saveSSID("/ext/pass.txt",wifiPwd);
		if(ret == -6323){// incorrect password
			goto PWD;
		}
		MAINLOG_L1("wifi_wifiAPConnect_lib_1:%d", ret);
		AppPlayTip("Connect Successfully");
			return ret;
		}
}

void saveSSID(param,param2){
	DelFile_Api(param);
	int ret = 0;
	unsigned int size = 0;
	MAINLOG_L1("saveSSID lengnya:%d", strlen((char *)param2));
	int leng=strlen((char *)param2);
    unsigned char buffer[leng + 1];
    strcpy((char *)buffer, param2);
	size = GetFileSize_Api(param);

	ret = WriteFile_Api(param, buffer,size,sizeof(buffer));

	size = 0;
	size = GetFileSize_Api(param);

}

int CheckSSIDwifi(){
	char TempBuf[1025];
	int Len=0, ret=0, loc=0;
	int fileLen = 0;
	int diff = 0;

	memset(TempBuf, 0, sizeof(TempBuf));
	portClose_lib(10);
	portOpen_lib(10, NULL);
	portFlushBuf_lib(10);
	fileLen =  GetFileSize_Api("/ext/ssid.txt");
	MAINLOG_L1("readfile = %d", fileLen);
	memset(TempBuf, 0, sizeof(TempBuf));
	ret = ReadFile_Api("/ext/ssid.txt", TempBuf, loc, &fileLen);
	if (fileLen==0){
		return 0;
	}
	else{
		return 1;
	}
}


int ApConnect()
{
#ifdef __WIFI__
	int ret;
	ret = wifiAPConnect_lib(wifiId, wifiPwd);
	MAINLOG_L1("wifi_wifiAPConnect_lib_2:%d", ret);
	return ret;
#endif
}

/*

uint8 pssltmpp[93] = {
	0x10, 0x5B, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x82, 0x02, 0x58, 0x00, 0x34, 0x4D, 0x73,
	0x77, 0x69, 0x70, 0x65, 0x2D, 0x53, 0x6F, 0x75, 0x6E, 0x64, 0x62, 0x6F, 0x78, 0x2D, 0x37, 0x35,
	0x37, 0x66, 0x39, 0x66, 0x61, 0x35, 0x2D, 0x38, 0x35, 0x63, 0x33, 0x2D, 0x34, 0x34, 0x38, 0x31,
	0x2D, 0x39, 0x38, 0x37, 0x33, 0x2D, 0x38, 0x30, 0x64, 0x66, 0x33, 0x30, 0x35, 0x39, 0x63, 0x34,
	0x32, 0x33, 0x00, 0x19, 0x3F, 0x53, 0x44, 0x4B, 0x3D, 0x50, 0x79, 0x74, 0x68, 0x6F, 0x6E, 0x26,
	0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x31, 0x2E, 0x34, 0x2E, 0x39
};

void DR_SSL_testi(void)
{
	//int32 iRet;
	int ret;
	//MAINLOG_L1("[%s] -%s- Line=%d:application thread enter, param 0x%x\r\n", filename(__FILE__), __FUNCTION__, __LINE__, param);
	int timeid = 0;

	timeid = TimerSet_Api();
	while(1)
	{
		ret = wirelessCheckPppDial_lib();
		MAINLOG_L1("wirelessCheckPppDial_lib = %d" , ret );
		if(ret == 0)
			break;
		else{
			ret = wirelessPppOpen_lib(NULL, NULL, NULL);
			Delay_Api(1000);
		}
		if(TimerCheck_Api(timeid , 60*1000) == 1)
		{
			return ;
		}
	}
	//ret =  wirelessSetDNS_lib("114.114.114.114", "8.8.8.8" );
	//MAINLOG_L1("wirelessSetDNS_lib = %d" , ret);


    ret = wirelessSetSslMode_lib((unsigned char)1);
    MAINLOG_L1("wirelessSetSslMode_lib :%d", ret);

	ret = wirelessSendSslFile_lib(2, TEST_CA_FILE, strlen((char *)TEST_CA_FILE));
	MAINLOG_L1("wirelessSendSslFile_lib 1:%d %d", ret, strlen((char *)TEST_CA_FILE));
	ret = wirelessSendSslFile_lib(1, TEST_CLIENT_KEY_FILE, strlen((char *)TEST_CLIENT_KEY_FILE));
	MAINLOG_L1("wirelessSendSslFile_lib 2:%d %d", ret, strlen((char *)TEST_CLIENT_KEY_FILE));
	ret = wirelessSendSslFile_lib(0, TEST_CLIENT_CRT_FILE, strlen((char *)TEST_CLIENT_CRT_FILE));
	MAINLOG_L1("wirelessSendSslFile_lib 3:%d %d", ret, strlen((char *)TEST_CLIENT_CRT_FILE));


	wirelessSslDefault_lib();
	wirelessSetSslVer_lib((unsigned char)0); //wirelessSslSetTlsVer_lib(0);

	Delay_Api(2000);

    int sock = wirelessSslSocketCreate_lib();
    if (sock == -1)
    {
    	MAINLOG_L1("[%s] -%s- Line=%d:<ERR> create ssl sock failed\r\n");
        return;//fibo_thread_delete();
    }

    MAINLOG_L1(":::fibossl fibo_ssl_sock_create %x\r\n", sock);

	ret = wirelessSslConnect_lib(sock,  "a14bkgef0ojrzw-ats.iot.us-west-2.amazonaws.com", "8883", 60*1000);

	MAINLOG_L1(":::fibossl wirelessSslConnect_lib %d\r\n", ret);

    ret = wirelessSslSend_lib(sock, pssltmpp, sizeof(pssltmpp));
	MAINLOG_L1(":::fibossl sys_sock_send %d\r\n", ret);

    ret = wirelessSslRecv_lib(sock, buf, 1, 1000);
	MAINLOG_L1(":::fibossl sys_sock_recv %d\r\n", ret);
	ret = wirelessSslRecv_lib(sock, buf, 32, 10000);
	MAINLOG_L1(":::fibossl sys_sock_recv %d\r\n", ret);

	ret = wirelessSslGetErrcode_lib();
	MAINLOG_L1(":::fibo_get_ssl_errcode, ret = %d\r\n", ret);

	uint32 port = 6500;

	MAINLOG_L1(":::wifiTCPConnect_lib, iRet = %d\r\n", iRet);
}


*/



