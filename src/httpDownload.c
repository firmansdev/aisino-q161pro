#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#include "../inc/httpDownload.h"

#define	PROTOCOL_HTTP	0
#define	PROTOCOL_HTTPS	1

typedef struct {
	int protocol;
	char domain[MAX_DOMAIN_LENGTH];
	char path[MAX_PATH_LENGTH];
	char port[8];
} URL_ENTRY;

static int parseURL(char *url, URL_ENTRY *entry)
{
	char *p, *p2;

	memset(entry, 0, sizeof(URL_ENTRY));

	// Protocol analysis
	p = strstr(url, "://");
	if (p == NULL) {
		MAINLOG_L1("Q161Demo: HTTP, p==NULL");
		entry->protocol = PROTOCOL_HTTP;
		p = url;
	}
	else {
		if (((p - url) == 5) && (memcmp(url, "https", 5) == 0)){
			MAINLOG_L1("Q161Demo: HTTPS");
			entry->protocol = PROTOCOL_HTTPS;
		}else if (((p - url) == 4) && (memcmp(url, "http", 4) == 0)){
			MAINLOG_L1("Q161Demo: HTTP");
			entry->protocol = PROTOCOL_HTTP;
		}else{
			MAINLOG_L1("Q161Demo: Unsupported URL");
			return UNSUPPORTED_URL;
		}
		p += 3;
	}

	// Domain analysis (now p points to the beginning of domain name)
	p2 = strchr(p, '/');
	if (p2 == NULL) {
		// Format as http://domain
		if (strlen(p) >= MAX_DOMAIN_LENGTH){
			MAINLOG_L1("Q161Demo: Fail to get domain");
			return URL_DOMAIN_TOO_LONG;
		}

		strcpy(entry->domain, p);
		MAINLOG_L1("Q161Demo: domain=%s", entry->domain);
	}
	else {
		if ((p2 - p) >= MAX_DOMAIN_LENGTH){
			MAINLOG_L1("Q161Demo: Fail to get domain");
			return URL_DOMAIN_TOO_LONG;
		}

		memcpy(entry->domain, p, p2 - p);
		entry->domain[p2 - p] = 0;
		MAINLOG_L1("Q161Demo: domain=%s", entry->domain);
	}

	// Path analysis (now p2 is either NULL or pointing to the begining of path)
	if (p2 == NULL){
		strcpy(entry->path, "/");
		MAINLOG_L1("Q161Demo: path=%s", entry->path);
	}else {
		if (strlen(p2) >= MAX_PATH_LENGTH){
			MAINLOG_L1("Q161Demo: Fail to get path, length=%d, p2=%s", strlen(p2), p2);
			return URL_PATH_TOO_LONG;
		}

		strcpy(entry->path, p2);
		MAINLOG_L1("Q161Demo: path=%s", entry->path);
	}

	// Port analysis (may be part of domain)
	p2 = strchr(entry->domain, ':');
	if (p2 == NULL) {
		if (entry->protocol == PROTOCOL_HTTP){
			strcpy(entry->port, "80");
		}else if (entry->protocol == PROTOCOL_HTTPS){
			strcpy(entry->port, "443");
		}
	}
	else {
		p2[0] = 0;	// cut the ":port" from domain

		strcpy(entry->port, p2 + 1);
	}

	return 0;
}

static void *s;
static int dev_connect(URL_ENTRY *entry, int timeout)
{
	int err;

	MAINLOG_L1("Q161Demo: dev_connect_domain = %s, port = %s", entry->domain, entry->port);

	if (entry->protocol == PROTOCOL_HTTP)
		s = net_connect(NULL, entry->domain, entry->port, timeout * 1000, 0, &err);
	else
		s = net_connect(NULL, entry->domain, entry->port, timeout * 1000, 1, &err);

	if (s == NULL)
		return CONNECT_ERROR;

	return 0;
}

static int dev_send(unsigned char *buf, int length)
{
	int ret;

	ret = net_write(s, buf, length, 0);
	if (ret < 0)
		return SEND_ERROR;

	return 0;
}

// Return bytes of received, or error
static int dev_recv(unsigned char *buf, int max, int timeout)
{
	int ret;

	ret = net_read(s, buf, max, timeout * 1000);
	if (ret < 0)
		return RECEIVE_ERROR;

	return ret;
}

static int dev_disconnect(void)
{
	return net_close(s);
}

// For appending, offset is at the end of current file size.
// Make sure this works for your device
static int dev_savefile(char *filename, unsigned char *buf, int start, int len)
{
	int ret = 0;
	ret = WriteFile_Api(filename, buf, start, len);
	// MAINLOG_L1("Q161Demo: WriteFile_Api ret = %d", ret);
	// return 0;

	if (ret < 0) {
		MAINLOG_L1("Q161Pro:WriteFile_Api error %d", ret);
		return ret;
	}
}

// Remove the space in the begining and end of the str
static char *trim(char *str)
{
	int length, start, end;

	length = strlen(str);

	for (start = 0; start < length; start++) {
		if (str[start] != ' ')
			break;
	}

	for (end = length - 1; end > start; end--) {
		if (str[end] != ' ')
			break;
	}

	str[end + 1] = 0;

	return str + start;
}

#ifdef __RECVLARGEBUF__
char *GlBuf = 0;
#endif
// Partial download
// Return content length downloaded or error
static int __httpDownload(URL_ENTRY *entry, char *url, int method, char *filename, int start, int end)
{
	int ret;
	int length, total, offset;
	int chunked_total = 0;
	int payload_flag;
#ifdef __RECVLARGEBUF__
	char *buf = (char *)GlBuf;
#else
	unsigned char buf[RECEIVE_BUF_SIZE];
#endif
	char *p = NULL, *p1 = NULL, *p2 = NULL;
	int reblen = RECEIVE_BUF_SIZE;

relocate_301_302:
	memset(buf, 0, reblen);

	// Prepare the HTTP request
	if (method == METHOD_GET)
		strcpy((char *)buf, "GET ");
	else
		strcpy((char *)buf, "POST ");

	MAINLOG_L1("Q161Demo: start-end = %d-%d", start, end);

	if (end < 0){
		sprintf((char *)buf, "%s %s HTTP/1.1\r\nHost:%s\r\nRange:bytes=%d-\r\n\r\n",
				(method == METHOD_GET) ? "GET" : "POST",
						entry->path,
						entry->domain,
						start);
	}else{
		sprintf((char *)buf, "%s %s HTTP/1.1\r\nHost:%s\r\nRange:bytes=%d-%d\r\n\r\n",
				(method == METHOD_GET) ? "GET" : "POST",
						entry->path,
						entry->domain,
						start, end);
	}

	ret = dev_send(buf, strlen((char *)buf));
	if (ret != 0) {
		dev_disconnect();

		return ret;
	}

	memset(buf, 0, reblen);
	// Check HTTP status ("HTTP/1.1 200 OK\r\n")
	length = 0;
	while (1) {
		ret = dev_recv(buf + length, RECEIVE_BUF_SIZE - 1 - length, RECEIVE_TIMEOUT);
		if (ret <= 0) {
			dev_disconnect();

			return ret;
		}

		length += ret;
		buf[length] = 0;
#ifdef DEBUG_MSG
MAINLOG_L1("===============================\n");
MAINLOG_L1("%s\n", buf);
MAINLOG_L1("===============================\n");
#endif

		// Check "\r\n" after HTTP status line
		p = strstr((char *)buf, "\r\n");
		if (p == NULL)
			// Not received yet, continue receiving
			continue;

		//p is end of status line(before \r\n)
		p[0] = 0;
		p += 2;
		length -= (p - (char *)buf);
		// Now p points to the start of header, length is the length
		// of header already received.

		// An ' ' should exist before response code "200"
		p1 = strchr((char *)buf, ' ');
		if (p1 == NULL) {
			dev_disconnect();

			return RECEIVE_ERROR;
		}

		p1[4] = 0;
		ret = atoi(p1 + 1);

		if ((ret != 200) && (ret != 301) && (ret != 302) && (ret != 206)) {
			dev_disconnect();

			return -ret;
		}

		break;
	}

	// Check HTTP headers
	// Find headers "Content-Length: xxx" or "Transfer-Encoding: chunked" for 200
	// payload_flag:
	// 	1	- total size known, receive all along
	// 	2	- payload is chunked, receive each chunk one by one
	// 	0	- no payload length info, error
	// Find header "Location: xxx" for 301 and 302
	payload_flag = 0; ////p is end of status line(after \r\n)
	while (1) {
		p1 = strstr(p, "\r\n");  //p: after HTTP status line(after \r\n)
		if (p1 == NULL) {
			// Header not complete, move to the
			// begining of buf, continue receiving.
			int i;

			for (i = 0; i < length; i++)
				buf[i] = p[i];

			p = (char *)buf;

			ret = dev_recv(buf + length, RECEIVE_BUF_SIZE - 1 - length, RECEIVE_TIMEOUT);
			if (ret <= 0) {
				dev_disconnect();

				return RECEIVE_ERROR;
			}

			length += ret;
			buf[length] = 0;

#ifdef DEBUG_MSG
MAINLOG_L1("===============================\n");
MAINLOG_L1("%s\n", buf);
MAINLOG_L1("===============================\n");
#endif
			continue;
		}

		if (p1 == p) {
			// No more header, just break;
			p += 2;
			length -= 2;

			break;
		}

		p1[0] = 0;

		p2 = p;		// p2 points to current header  //p: after HTTP status line(after \r\n) //p1: next \r\n
		p = p1 + 2;	// p points to the next header
		length -= (p - p2);

		p1 = strchr(p2, ':');
		if (p1 == NULL) {
			dev_disconnect();

			return RECEIVE_ERROR;
		}

		p1[0] = 0;
		p2 = trim(p2);
		if (strcasecmp(p2, "Location") == 0) {

//			MAINLOG_L1("Q161Demo: Location = %s", trim(p1 + 1));

			dev_disconnect();
			ret = parseURL(trim(p1 + 1), entry);
			ret = dev_connect(entry, CONNECT_TIMEOUT);

//			MAINLOG_L1("Q161Demo: dev_connect_Location = %d", ret);

			if (ret != 0){
				return ret;
			}
			goto relocate_301_302;
		}
		else if (strcasecmp(p2, "Content-Length") == 0) {
			payload_flag = 1;
			total = atoi(trim(p1 + 1));
		}
		else if (strcasecmp(p2, "Transfer-Encoding") == 0) {
			if (strcmp(trim(p1 + 1), "chunked") == 0)
				payload_flag = 2;
		}
	}

	//p is after hearder now
	if (payload_flag == 1) {
		// Save the already-received payload first
		if (length > 0)
			dev_savefile(filename, (unsigned char *)p, start, length);

		offset = length;

		while (offset < total) {
			length = total - offset;

			if (length > RECEIVE_BUF_SIZE)
				length = RECEIVE_BUF_SIZE;

			length = dev_recv(buf, length, RECEIVE_TIMEOUT);
			if (length <= 0) {
				dev_disconnect();

				return RECEIVE_ERROR;
			}

#ifdef DEBUG_MSG
buf[length] = 0;
MAINLOG_L1("===============================\n");
MAINLOG_L1("%s\n", buf);
MAINLOG_L1("===============================\n");
#endif
			dev_savefile(filename, buf, start + offset, length);

			offset += length;
		}
	}
	else if (payload_flag == 2) {	// Chunked
		offset = 0;
		chunked_total = 0;
		while (1) {
			p1 = strstr(p, "\r\n");
			if (p1 == NULL) {
				// chunk size not complete, move to the
				// begining of the buffer, and continue
				// receiving.
				int i;

				for (i = 0; i < length; i++)
					buf[i] = p[i];

				p = (char *)buf;

				ret = dev_recv(buf + length, RECEIVE_BUF_SIZE - 1 - length, RECEIVE_TIMEOUT);
				if (ret <= 0) {
					dev_disconnect();

					return RECEIVE_ERROR;
				}

				length += ret;
				buf[length] = 0;

#ifdef DEBUG_MSG
MAINLOG_L1("===============================\n");
MAINLOG_L1("%s\n", buf);
MAINLOG_L1("===============================\n");
#endif
				continue;
			}

			p1[0] = 0;
			p1 += 2;
			length -= (p1 - p);

			sscanf(p, "%x", &total);
			if (total == 0)
				break;

			chunked_total += total;

			while (length < total) {
				dev_savefile(filename, (unsigned char *)p1, start + offset, length);

				offset += length;
				total -= length;

				length = total;
				if (length > RECEIVE_BUF_SIZE - 1)
					length = RECEIVE_BUF_SIZE - 1;

				length = dev_recv(buf, length, RECEIVE_TIMEOUT);
				if (length <= 0) {
					dev_disconnect();

					return RECEIVE_ERROR;
				}

				buf[length] = 0;
				p1 = (char *)buf;
#ifdef DEBUG_MSG
MAINLOG_L1("===============================\n");
MAINLOG_L1("%s\n", buf);
MAINLOG_L1("===============================\n");
#endif
			}

			dev_savefile(filename, (unsigned char *)p1, start + offset, total);

			offset += total;
			length -= total;
			p = p1 + total;

			// After chunk messaged, there should be "\r\n"
			if (length < 2) {
				// \r\n not complete, move to the beginning
				// of buffer, continue receiving.
				int i;

				for (i = 0; i < length; i++)
					buf[i] = p[i];

				p = (char *)buf;

				ret = dev_recv(buf + length, RECEIVE_BUF_SIZE - 1 - length, RECEIVE_TIMEOUT);
				if (ret <= 0) {
					dev_disconnect();

					return RECEIVE_ERROR;
				}

				length += ret;
				buf[length] = 0;
#ifdef DEBUG_MSG
MAINLOG_L1("===============================\n");
MAINLOG_L1("%s\n", buf);
MAINLOG_L1("===============================\n");
#endif
			}

			if ((p[0] != '\r') || (p[1] != '\n')) {
				dev_disconnect();

				return RECEIVE_ERROR;
			}

			p += 2;
			length -= 2;
		}
	}
	else {
		dev_disconnect();

		return RECEIVE_ERROR;
	}

	if (payload_flag == 1)
		return total;
	else
		return chunked_total;

	return 0;
}

int httpDownload(char *url, int method, char *filename)
{
	int ret;
	int total;
	URL_ENTRY entry;

	ret = parseURL(url, &entry);

	if (ret != 0)
		return ret;

	ScrCls_Api();
	ScrDisp_Api(LINE4, 0, "Connecting...", CDISP);
	AppPlayTip("Connecting");

	ret = dev_connect(&entry, CONNECT_TIMEOUT);
	MAINLOG_L1("Q161Pro: dev_connect = %d", ret);
	if (ret != 0){
		AppPlayTip("Fail to connect to server");
		ScrCls_Api();
		return ret;
	}

#if 0
	// Receive all payload within one request, small files can be used in this way
	return __httpDownload(&entry, url, method, filename, 0, -1);
#else
#ifdef __RECVLARGEBUF__
	GlBuf = malloc(RECEIVE_BUF_SIZE);
	if(GlBuf == 0)
	{
		MAINLOG_L1("malloc failed!");
		return 0;
	}
	else
		MAINLOG_L1("malloc ok!");
#endif
	// Receive payload by range, will ensure large file download stability
	total = 0;

	ScrCls_Api();
	ScrDisp_Api(LINE4, 0, "Downloading...", CDISP);
	AppPlayTip("Downloading");
	while (1) {
		ret = __httpDownload(&entry, url, method, filename, total, total + RECEIVE_RANGE - 1);
		MAINLOG_L1("ret = %d, rangenya = %d", ret,RECEIVE_RANGE);
		if (ret == -416)
			// Requested range beyond file size, just finish
			break;

		if (ret < 0)
		{
#ifdef __RECVLARGEBUF__
			free(GlBuf);
#endif
			return ret;
		}

		total += ret;
		 MAINLOG_L1("[httpDownload] Received %d bytes, total downloaded: %d", ret, total);
		// If received payload is less than requested Range, this is the last range.
		// If received payload is more than requested Range, the server does not support Range
		if (ret < RECEIVE_RANGE)
			break;
	}

	dev_disconnect();

#ifdef __RECVLARGEBUF__
	free(GlBuf);
#endif
	return total;
#endif
}

