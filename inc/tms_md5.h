#ifndef _tms__MD5__
#define _tms__MD5__

#define _TMS_S11 7
#define _TMS_S12 12
#define _TMS_S13 17
#define _TMS_S14 22
#define _TMS_S21 5
#define _TMS_S22 9
#define _TMS_S23 14
#define _TMS_S24 20
#define _TMS_S31 4
#define _TMS_S32 11
#define _TMS_S33 16
#define _TMS_S34 23
#define _TMS_S41 6
#define _TMS_S42 10
#define _TMS_S43 15
#define _TMS_S44 21

/* F, G, H and I are basic MD5 functions.
*/
#define _TMS_F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define _TMS_G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define _TMS_H(x, y, z) ((x) ^ (y) ^ (z))
#define _TMS_I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits.
*/
#define _TMS_ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
Rotation is separate from addition to prevent recomputation.
*/

#define _TMS_FF(a, b, c, d, x, s, ac) { \
 (a) += _TMS_F ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = _TMS_ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
}
#define _TMS_GG(a, b, c, d, x, s, ac) { \
 (a) += _TMS_G ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = _TMS_ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
}
#define _TMS_HH(a, b, c, d, x, s, ac) { \
 (a) += _TMS_H ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = _TMS_ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
}
#define _TMS_II(a, b, c, d, x, s, ac) { \
 (a) += _TMS_I ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = _TMS_ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
}

/* POINTER defines a generic pointer type */
typedef unsigned char * POINTER; 
  
/* UINT2 defines a two byte word */
//typedef unsigned short int UINT2; 
  
/* UINT4 defines a four byte word */
typedef unsigned long int UINT4; 
  
/* MD5 context. */
typedef struct { 
	UINT4 state[4]; 		  /* state (ABCD) */
	UINT4 count[2]; 		  /* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64]; /* input buffer */
}_tms_MD5_CTX;
  
void _tms_MD5Init (_tms_MD5_CTX *context); 
void _tms_MD5InitUpdate (_tms_MD5_CTX *context, unsigned char *input, unsigned int inputLen); 
void _tms_MD5InitUpdaterString(_tms_MD5_CTX *context,const char *string); 
int _tms_MD5InitFileUpdateFile (_tms_MD5_CTX *context,char *filename); 
void _tms_MD5InitFinal (unsigned char digest[16], _tms_MD5_CTX *context); 
int _tms_MD5InitFile (char *filename,unsigned char digest[16]); 

void _tms_MDString (char *string,unsigned char digest[16]);
int _tms_MD5File (char *filename,unsigned char digest[16]);
void _tms_MD5Final(unsigned char digest[16], _tms_MD5_CTX *context);
void _tms_MD5Update(_tms_MD5_CTX *context, unsigned char *input, unsigned int inputLen);

void _tms_MD5Transform (UINT4 state[4], unsigned char block[64]);
void _tms_MD5_memset (POINTER output, int value, unsigned int len);
void _tms_MD5_memcpy (POINTER output, POINTER input, unsigned int len);
void _tms_Encode (unsigned char *output, UINT4 *input, unsigned int len);
void _tms_Decode (UINT4 *output, unsigned char *input, unsigned int len);

#endif
