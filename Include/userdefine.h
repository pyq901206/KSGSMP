#ifndef __USER_DEFINE_H__
#define __USER_DEFINE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "string.h"
#include "stdlib.h"


#ifdef	WIN32

#ifndef	USER_DEBUG
#define USER_DEBUG
#endif

#else

#ifndef USER_DEBUG
#define USER_DEBUG(msg...) printf("[ %s, %d ]=> ",__FILE__,  __LINE__);printf(msg);printf("\r\n");
#endif

#endif

char *usr_str_btweenpoint(char *pfont,char  *pbehind,char  *str);
int usr_strncasecmp(char *str1,char *str2,int len);
int usr_strcasecmp(char *str1,char *str2);
char* usr_strrstr_nohead(char  *p,char* str);
char* usr_strrstr_btweenpoint(char  *phead,char  *pend,char* str);
int usr_strnum_btweenpoint(char  *pfont,char  *pbehind,char  *str);
int usr_getvalue_btweenpoint(char  *phead,char  *pend, char  *out);
int usr_printf_btweenpoint(char  *ps,char  *pe);
float usr_getfloatvalue_instr(char  *ptr,char  *str);

#define STR_BTPOINT(pfont,pbehind,str)		usr_str_btweenpoint(pfont,pbehind,str)
#define STRNCASECMP(str1,str2,len)			usr_strncasecmp(str1,str2,len)
#define STRCASECMP(str1,str2)				usr_strcasecmp(str1,str2)
#define STRRSTR_NOHEAD(p,str)				usr_strrstr_nohead(p,str)
#define STRRSTR_BTPOINT(pfont,pbehind,str)	usr_strrstr_btweenpoint(pfont,pbehind,str)
#define STRNUM_BTPOINT(pfont,pbehind,str)	usr_strnum_btweenpoint(pfont,pbehind,str)
#define STR_GETVALUE_BTPOINT(pfont,pbehind,out) usr_getvalue_btweenpoint(pfont,pbehind,out)
#define STR_GETFLOATVALUE_INSTR(ptr,str)		usr_getfloatvalue_instr(ptr,str)
//#define DEBUG_BTPOINT(fileName,lineNum,ps,pe);	usr_printf_btweenpoint(fileName,lineNum,ps,pe);
int u2g(char *inbuf,int inlen,char *outbuf,int outlen);
int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen);


int Gb2312ToUtf8(char *sOut, int iMaxOutLen, const char *sIn, int iInLen);
int Utf8ToGb2312(char *sOut, int iMaxOutLen, const char *sIn, int iInLen);

#ifdef	WIN32
#ifndef	DEBUG_BTPOINT
#define DEBUG_BTPOINT
#endif

#else

#ifndef	DEBUG_BTPOINT	
#define DEBUG_BTPOINT(ps,pe,msg...); printf("[ %s, %d ]=> ",__FILE__,  __LINE__);printf(msg);usr_printf_btweenpoint(ps,pe);printf("\r\n");
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif

