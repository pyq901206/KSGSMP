#ifndef __BASE64_H__
#define __BASE64_H__

extern void base64_bits_to_64(unsigned char *out, const unsigned char *in, int inlen);
extern int base64_64_to_bits(char *out, const char *in);
extern int base64Encode(char* result, char const* origSigned, unsigned origLength) ;
extern int base64Decode(char* in, unsigned char* out);
#endif