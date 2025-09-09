/*
 * sound.c
 *
 *  Created on: 2021Äê10ÔÂ15ÈÕ
 *      Author: vanstone
 */
#include "../inc/def.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

void AppPlayTip(char *tip)
{
	 PlaySound_Api((unsigned char *)tip, G_sys_param.sound_level*2-1, 0); // pakai ini kalo ubah volume suara di asino udh diimplementasiin
//	PlaySound_Api((unsigned char *)tip, 1, 0);
}

int PlayMP3File(char *audioFileName){
	int ret;
	char filePath[128];

	memset(filePath,0,sizeof(filePath));
	if(PMflag == PLAYMP3_ENGLISH)
		sprintf(filePath,"/ext/en/%s",audioFileName);
	else
		sprintf(filePath,"/ext/arabic/%s",audioFileName);
	strcat( filePath, ".mp3");
	MAINLOG_L1("MP3 Path is %s",filePath);

	while(1){
		ret = audioFilePlayPath_lib(filePath);
		MAINLOG_L1("audioFilePlayPath_lib:%s ret:%d ", filePath, ret);

		if (ret==0){
			break;
		}
		else if (ret==-1) {
			MAINLOG_L1("Play failed, please check if the file is damaged");
			break;
		}
		else if (ret==-2) {
			MAINLOG_L1("File [%s] is not present", filePath );
			break;
		}
		else if (ret==-3) {
			MAINLOG_L1("TSS is occupied");
			Delay_Api(100);
		}
	}
	return ret;
}

int PlayMP3Num(char *audioFileName){
	char buf[16];
	strcpy( buf, "_" );	// number files with prefix _
	strcat( buf, audioFileName );
	PlayMP3File( buf );
	return 0;
}


int PlayMP3Num9(char *audioFileName){
	// play a single digit directly.
	PlayMP3Num( audioFileName );
	return 0;
}


int PlayMP3Num99(char *audioFileName){
	char buf[4];
	char c;
	if ( strlen(audioFileName) >= 2 ) {
		if(PMflag == PLAYMP3_ENGLISH)
		{
			memset( buf, 0, 4 );
			// copy the last 2 digits to play
			strncpy( buf, audioFileName+strlen(audioFileName)-2 , 2  );
			if( buf[0] != '0' ) {
				if( buf[0] == '1' ) {
					// 11 ~ 19
					PlayMP3Num( buf );
				} else {
					// 20 ~ 99
					c = buf[1];
					buf[1] = '0';
					PlayMP3Num( buf ); // 20, 30, n0
					buf[0] = c;
					buf[1] = 0;
					PlayMP3Num9( buf ); // single number
				}
			} else {
				// 01 ~ 09
				PlayMP3Num9( buf+1 ); // single number
			}
		}
		else
		{
			memset( buf, 0, 4 );
			memcpy(buf, audioFileName, 2);
			if( buf[1] != '0' )
				PlayMP3Num9( buf+1 );

			if( buf[0] != '0' )
			{
				buf[1] = '0';
				PlayMP3Num( buf );
			}
		}
	} else {
		// single number to play
			PlayMP3Num9( audioFileName );
	}

	return 0;
}

int PlayMP3Num999(char *audioFileName){
	char *p = audioFileName;
	char buf[4];

	// remove 0 from the left
	while( (*p) == '0' && (*p) != 0 ) {
		++p;
	}
	if( strlen(p) > 3 )
		{
			MAINLOG_L1( "Invlid input: %s", p );
			return -1;
		}
	if ( strlen(p) == 3 ) {
		// 100 ~ 999
		memset( buf, 0, 4);
		strncpy( buf, p, 1  );
		PlayMP3Num( buf );
		PlayMP3Num( "100" );

		if( p[1] == '0' && p[2] == '0' ) {
			// n00, no more to play
			return 0;
		} else {
			// play and with remain digits
			if(PMflag == PLAYMP3_ENGLISH)
				PlayMP3File( "and" );
		}
		++p;
	}

	// remove 0 from the left
	while( (*p) == '0' && (*p) != 0 ) {
		++p;
	}

	if ( strlen(p) == 2 ) {
		// 123
			// play the last 2 digits
			PlayMP3Num99( p );
	} else if( strlen(p) ==1 ) {
		// play the single digit directly
			PlayMP3Num9( p );
	}
}


int PlayMP3Numbers(char *audioFileName){
	// 987,654,321
	char *p = audioFileName;
	char buf[12];

	// remove 0 from the left
	while( (*p) == '0' && (*p) != 0 ) {
		++p;
	}
	if( strlen( p ) > 9 ){
		// too big to play, skip
		MAINLOG_L1( "number is too large to play: %s", p );
		return -1;
	}

	// remove 0 from the left
	while( (*p) == '0' && (*p) != 0 ) {
		++p;
	}

	if(strlen(p) > 4)
		PMflag = PLAYMP3_ENGLISH;
	else
		PMflag = PLAYMP3_ARABIC; //only support maximum number 9,999

	if( strlen(p) <= 9 && strlen(p) > 6 )
	{
		// 999,---,---
		memset( buf, 0, 12);
		strncpy( buf, audioFileName, strlen(audioFileName)-6 );
		PlayMP3Num999( buf );
		PlayMP3File("_1000000");

		p += (strlen(audioFileName)-6); // move the p to the last 6 digits to play.
	}

	// remove 0 from the left
	while( (*p) == '0' && (*p) != 0 ) {
		++p;
	}
	if( strlen(p) <= 6 && strlen(p) > 3 ){
		// 999,---
		memset( buf, 0, 12);
		strncpy( buf, p, strlen(p)-3 );
		PlayMP3Num999( buf );
		PlayMP3File("_1000");

		p += (strlen(p)-3);
  }

	// remove 0 from the left
	while( (*p) == '0' && (*p) != 0 ) {
		++p;
	}
	if ( strlen(p) != 0 ) {
		memset( buf, 0, 12);
		strncpy( buf, p, strlen(p) );
		PlayMP3Num999( buf );
	}

	return 0;
}

void AppPlayTipviaMP3(char *tip)
{
	// read each word from the tip and play the mp3 file one by one.
	// actually, you could define any code to a work / sentence to play them.
	MAINLOG_L1( "playing: %s", tip );

	char mp3file[256];
	char* pCharStart = mp3file;
	char* pCharEnd = mp3file;
	char c;
	int ret = 0;

	int end = 0;
	int isnumber = 0;

	if( strlen( tip ) < 256 ) {
		strcpy( mp3file, tip);
	} else {
		strncpy( mp3file, tip, 255 );
		mp3file[255] = 0;
	}

	isnumber = 1;

	while( 0 == (end & 2) && (ret == 0) ){
		if( 0 == (*pCharEnd) ){
			// end of the input.
			end = 3;	// 2+1
			c = 0;
		} else if( ' ' == (*pCharEnd) ) {
			// space to split a word.
			end = 1;
		} else if( '.' == (*pCharEnd) ) {
			if( isnumber == 1 ) {
				end = 1;
			}
		} else if( (*pCharEnd) >= 'A' && (*pCharEnd) <= 'Z' ) {
			(*pCharEnd) = (*pCharEnd) + 'a' -'A';	// to lower case
			isnumber = 0;
		} else if( (*pCharEnd) >= '0' && (*pCharEnd) <= '9' ) {
//			isnumber = 1;

		} else {
			isnumber = 0;
		}

		if( 0 == end ){
			++pCharEnd;
		} else {
			c = (*pCharEnd);
			(*pCharEnd) = 0;
			if( isnumber == 1 ){
				//
				MAINLOG_L1( "got number: %s", pCharStart );
				ret = PlayMP3Numbers(pCharStart);
			} else {
				ret = PlayMP3File(pCharStart);

			}

			isnumber = 1;
		}

		if( 1 == end ) {
			(*pCharEnd) = c;
			++pCharEnd;

			pCharStart = pCharEnd;

			end = 0;
		}
	}
	if( ret != 0 ) {
		PlaySound_Api((unsigned char *)"Cannot play", G_sys_param.sound_level, 0);
		PlaySound_Api((unsigned char *)tip, G_sys_param.sound_level, 0);
	}

}

void PlayEnArabicAmt(char *amt)
{
	int iamt, i;
	u8 tmp[64];

	MAINLOG_L1( "PlayEnArabicAmt :%s ", amt);
	memset(tmp, 0, sizeof(tmp));
	for(i=0; i<strlen(amt); i++)
	{
		if(amt[i] != '.')
			tmp[i] = amt[i];
	}

	iamt = (int)AscToLong_Api(tmp, strlen(tmp));MAINLOG_L1( "iamt:%d",  iamt);
	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%d", iamt); MAINLOG_L1( "AppPlayTipviaMP3- Arabic Amount:%s",  tmp);
	AppPlayTipviaMP3( tmp);  Delay_Api(2000);

	MAINLOG_L1( "PlayArabicAmt- Dolor Amount:%s",  amt);
	AppPlayTip(amt);
}




