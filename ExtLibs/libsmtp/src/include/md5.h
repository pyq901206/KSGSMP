#ifndef __MD5_H__
#define __MD5_H__

/* UINT4 defines a four byte word */
#if defined(__alpha)
typedef unsigned int UINT4;
#else
typedef unsigned long int UINT4;
#endif

/* MD5 context. */
typedef struct
{
    UINT4           state[4];  /* state (ABCD) */
    UINT4           count[2];  /* number of bits, modulo 2^64 (lsb first) */
    unsigned char   buffer[64]; /* input buffer */
} MD5_CTX;


void MD5Init(MD5_CTX * md5Ctx);
void MD5Update(unsigned char * input, MD5_CTX * md5Ctx, unsigned int inputLen);
void MD5Final(unsigned char digest[16],MD5_CTX * md5Ctx);




#endif   /*__MD5_H__*/
