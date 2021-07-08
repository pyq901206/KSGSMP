#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"

#define BAD     -1

static const char *BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const signed char base64val[] = { 
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD, 62, BAD,BAD,BAD, 63, 
    52, 53, 54, 55,  56, 57, 58, 59,  60, 61,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,  0,  1,  2,   3,  4,  5,  6,   7,  8,  9, 10,  11, 12, 13, 14, 
    15, 16, 17, 18,  19, 20, 21, 22,  23, 24, 25,BAD, BAD,BAD,BAD,BAD,
    BAD, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 36,  37, 38, 39, 40, 
    41, 42, 43, 44,  45, 46, 47, 48,  49, 50, 51,BAD, BAD,BAD,BAD,BAD
};

#define DECODE64(c)  (isascii(c) ? base64val[c] : BAD)
//base64±àÂë1
void base64_bits_to_64(unsigned char *out, const unsigned char *in, int inlen)
{
    for (; inlen >= 3; inlen -= 3)
    {   
        *out++ = BASE64_CHARS[in[0] >> 2]; 
        *out++ = BASE64_CHARS[((in[0] << 4) & 0x30) | (in[1] >> 4)];
        *out++ = BASE64_CHARS[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
        *out++ = BASE64_CHARS[in[2] & 0x3f];
        in += 3;
    }   

    if (inlen > 0)
    {   
        unsigned char fragment;

        *out++ = BASE64_CHARS[in[0] >> 2]; 
        fragment = (in[0] << 4) & 0x30;

        if (inlen > 1)
            fragment |= in[1] >> 4;

        *out++ = BASE64_CHARS[fragment];
        *out++ = (inlen < 2) ? '=' : BASE64_CHARS[(in[1] << 2) & 0x3c];
        *out++ = '=';
    }

    *out = '\0';
}

//base64±àÂë2
int	base64Encode(char* result, char const* origSigned, unsigned origLength) 
{
  unsigned char const* orig = (unsigned char const*)origSigned; // in case any input bytes have the MSB set
  if(orig == NULL) 
  	return -1;

  unsigned const numOrig24BitValues = origLength/3;
  int havePadding = origLength > numOrig24BitValues*3;
  int havePadding2 = origLength == numOrig24BitValues*3 + 2;
  unsigned const numResultBytes = 4*(numOrig24BitValues + havePadding);
  unsigned i;
  for (i = 0; i < numOrig24BitValues; ++i) {
	  result[4*i+0] = BASE64_CHARS[(orig[3*i]>>2)&0x3F];
	  result[4*i+1] = BASE64_CHARS[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
	  result[4*i+2] = BASE64_CHARS[((orig[3*i+1]<<2) | (orig[3*i+2]>>6))&0x3F];
	  result[4*i+3] = BASE64_CHARS[orig[3*i+2]&0x3F];
	}

  if (havePadding) {
	  result[4*i+0] = BASE64_CHARS[(orig[3*i]>>2)&0x3F];
	  if (havePadding2) {
		  result[4*i+1] = BASE64_CHARS[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
		  result[4*i+2] = BASE64_CHARS[(orig[3*i+1]<<2)&0x3F];
		} else {
		  result[4*i+1] = BASE64_CHARS[((orig[3*i]&0x3)<<4)&0x3F];
		  result[4*i+2] = '=';
		}
	  result[4*i+3] = '=';
	}

  result[numResultBytes] = '\0';
  return 0;
}

//base64½âÂë1
int base64_64_to_bits(char *out, const char *in)
{
    int len = 0;
    register unsigned char digit1, digit2, digit3, digit4;

    if (in[0] == '+' && in[1] == ' ')
        in += 2;
    if (*in == '\r')
        return(0);

    do {
        digit1 = in[0];
        if (DECODE64(digit1) == BAD)
            return(-1);
        digit2 = in[1];
        if (DECODE64(digit2) == BAD)
            return(-1);
        digit3 = in[2];
        if (digit3 != '=' && DECODE64(digit3) == BAD)
            return(-1);
        digit4 = in[3];
        if (digit4 != '=' && DECODE64(digit4) == BAD)
            return(-1);
        in += 4;
        *out++ = (DECODE64(digit1) << 2) | (DECODE64(digit2) >> 4);
        ++len;
        if (digit3 != '=')
        {
            *out++ = ((DECODE64(digit2) << 4) & 0xf0) | (DECODE64(digit3) >> 2);
            ++len;
            if (digit4 != '=')
            {
                *out++ = ((DECODE64(digit3) << 6) & 0xc0) | DECODE64(digit4);
                ++len;
            }
        }
    } while (*in && *in != '\r' && digit4 != '=');

    return (len);
}

//base64½âÂë2
int base64Decode(char* in, unsigned char* out) 
{
	int j, i;
	int k = 0;
	char base64DecodeTable[256];
	int const jMax = strlen(in) - 3;
	// in case "in" is not a multiple of 4 bytes (although it should be)
	memset(base64DecodeTable, 0, sizeof(base64DecodeTable));
	for (i = 0; i < 256; ++i) base64DecodeTable[i] = (char)0x80;
		// default value: invalid
	for (i = 'A'; i <= 'Z'; ++i) base64DecodeTable[i] = 0 + (i - 'A');
	for (i = 'a'; i <= 'z'; ++i) base64DecodeTable[i] = 26 + (i - 'a');
	for (i = '0'; i <= '9'; ++i) base64DecodeTable[i] = 52 + (i - '0');
	base64DecodeTable[(unsigned char)'+'] = 62;
	base64DecodeTable[(unsigned char)'/'] = 63;
	base64DecodeTable[(unsigned char)'='] = 0;
	
	for ( j = 0; j < jMax; j += 4) {
	  char inTmp[4], outTmp[4];
	  for (i = 0; i < 4; ++i) {
		  inTmp[i] = in[i+j];
		  outTmp[i] = base64DecodeTable[(unsigned char)inTmp[i]];
		  if ((outTmp[i]&0x80) != 0) outTmp[i] = 0; // pretend the input was 'A'
		}

	  out[k++] = (outTmp[0]<<2) | (outTmp[1]>>4);
	  out[k++] = (outTmp[1]<<4) | (outTmp[2]>>2);
	  out[k++] = (outTmp[2]<<6) | outTmp[3];
	}

	return k;
} 

