/*******************************************************************
|  Copyright(c) 2015-2016 Graceport Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0
|  作者: FootMan [FootMan@graceport.cn]
|  日期: 2016年6月6日
|  说明:
|
|  版本: 1.1
|  作者:
|  日期:
|  说明:
******************************************************************/
#include "userdefine.h"
#include <iconv.h>

//@fun:bwteen two points  find a string from front to back
//@return if had. return the position ,else return NULL;  
char *usr_str_btweenpoint(char  *pfont,char  *pbehind,char  *str)
{
	int i = 0;
	int j = 0;
	int strlenth = 0; 
	if(	NULL == str						||  
		NULL == pfont					|| 
	   	NULL == pbehind					||  
	   	strlen(str) > (size_t)(pbehind - pfont)	||
	   	pbehind - pfont <= 0){
		return NULL;
	}
	
	strlenth = strlen(str);	
	for(i = 0; i + strlenth <=  pbehind - pfont; i++){
		for(j = 0; str[j] == pfont[i+j]; j++){
			if(j == strlenth - 1){
				return pfont+i;
			}
		}
	}
	
	return NULL;
}
//从 pfont 开始 查找 str 直到 pbehind 为止
char *usr_struntilstr_(char  *pfont,char  * pbehind,char  *str)
{
	char *pe;
	pe = strstr(pfont,pbehind);
	if(NULL == pe){
		return NULL;
	}
	return usr_str_btweenpoint(pfont,pe,str);
}
//在两个指针之间某字符串出现的次数
int usr_strnum_btweenpoint(char  *pfont,char  *pbehind,char  *str)
{
	char *pf =  NULL;
	int  count = 0;
	for(pf = pfont;;count++){
		pf = usr_str_btweenpoint(pf,pbehind,str);
		if(NULL == pf) 
			break;
		pf = pf+strlen(str);
	}

	return count;
}

int usr_strncasecmp(char *str1,char *str2,int len)
{
	int i = 0;
	if(	NULL == str1 ||
		NULL == str2 || 
		(int)strlen(str1) < len || 
		(int)strlen(str2) < len){
		return -1;
	}
	
	for(i = 0; i < len; i++){
		if(str1[i] ==  str2[i]){
			continue;
		}else if(str1[i] >= 65 && str1[i] <= 90 && str2[i] >= 97 && str2[i] <= 122){
			if(str1[i]+32 == str2[i]){
				continue; 
			}
		}else if(str2[i] >= 65 && str2[i] <= 90 && str1[i] >= 97 && str1[i] <= 122){
			if(str2[i]+32 == str1[i]){
				continue; 
			}
		}//全部转成小写判断
		return -1;
	}
	return 0; 
}

int usr_strcasecmp(char *str1,char *str2)
{
	int len = 0;
	if( (len = strlen(str1)) != strlen(str2)){
		return -1;
	} 
	return usr_strncasecmp(str1,str2,len);
}

//从指针开始往前查找该字符串,直到找到为止。
//慎重使用,必须知道有该字符,否则段错误

char* usr_strrstr_nohead(char  *p,char* str)
{
	if(NULL == p || NULL == str)
		return NULL; 
	for(;;){
		if(*p == str[0])
			if(0 == usr_strncasecmp(p,str,strlen(str)))
				break;
		p--;	
	}
	return p;
}


char* usr_strrstr_btweenpoint(char  *phead,char  *pend,char* str)
{
	if(NULL == pend || NULL == str || NULL == phead || pend <= phead)
		return NULL; 
	for(;;){
		//USER_DEBUG("%c %c",*pend,str[0]);
		if(*pend == str[0])
			if(0 == usr_strncasecmp(pend,str,strlen(str)))
				break;
		if(pend == phead)
			return NULL;
		pend--;
	}	
	return pend;
}
//获取两个字串之间的值 遇到空格 回车 TAB 结束 
int  usr_getvalue_btweenpoint(char  *phead,char  *pend, char  *out)
{
	int s_flag = 0;
	int e_flag = 0;
	int i = 0;
	char *pe = NULL;
	if(	NULL == phead ||
	 	NULL == pend  ||
	 	NULL == out   ||
	 	phead > pend
		)
		return -1;
	
	if(	pend == phead &&
		*phead != ' '  &&
	   	*phead != '	' &&
	   	*phead != '\r' &&
		*phead != '\n'){
		out[0] = phead[0];
		return 0;
	}
	//USER_DEBUG("%s",phead);
	for(;phead < pend;phead++){
		//USER_DEBUG("%c",*phead);
		if(*phead != ' '  &&
		   *phead != '	' &&
		   *phead != '\r' &&
		   *phead != '\n'){
		   s_flag = 1;
		   break;
		}
	}
	//USER_DEBUG("s_flag = [%d],*phead = [%c]",s_flag,*phead);
	
	for(i = 0;i < pend - phead;i++){
		if(phead[i] == ' '  ||
		   phead[i] == '	' ||
		   phead[i] == '\r' ||
		   phead[i] == '\n'){
		  
		   break;
		 //  e_flag = 1;
		}
		out[i] = phead[i];
		//USER_DEBUG("%c",phead[i]);
	}
	/*for(;phead < pend;pend--){
		if(*pend != ' '  &&
		   *pend != '	' &&
		   *pend != '\r' &&
		   *pend != '\n'){
		   e_flag = 1;
		   break; 
		}
	}*/
	/*
	USER_DEBUG("e_flag = [%d],*pend = [%c]",e_flag,*pend);
	if(1 == s_flag && 1 == e_flag){
		memcpy(out,phead,pend - phead + 1);
		USER_DEBUG("%d",(pend - phead + 1));
	}else if((1 == s_flag && 0 == e_flag) ||
			 (0 == s_flag && 1 == e_flag) ){
		*out = *phead;
	}else{
		//NULL
	}*/
	return 0;
}

//str:abcdefg x="0.789" hijklmn y="0.2356"

float usr_getfloatvalue_instr(char  *ptr,char  *str)
{
	char *ps = NULL;
	char tmp[32]= {0};
	char rule[128] = {0};
	sprintf(rule,"%s\"%[^\"]",str);
	if(NULL != (ps = strstr(ptr,str))){
		sscanf(ps,rule,tmp);//此处正则表达式
		return (float)atof(tmp);
	}
	return 0;
}

int usr_printf_btweenpoint(char  *ps,char  *pe)
{
	char *tmp;
	tmp = (char *)malloc(pe-ps+1);
	memset(tmp,0,pe-ps+1); 
	memcpy(tmp,ps,pe-ps+1);
	printf("%s",tmp);
	free(tmp);
	return 0;
}

int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen)
{
	iconv_t cd;
	size_t iret = -1; 
	int rc;
	char **pin = &inbuf;
	char **pout = &outbuf;
	cd = iconv_open(to_charset,from_charset);
	if (cd==0) return -1;
	memset(outbuf, 0, outlen);
	iret = iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);
	if (iret == -1) return -1;
	iconv_close(cd);
	return 0;
}
//UNICODE码转为GB2312码

int u2g(char *inbuf,int inlen,char *outbuf,int outlen)
{
    return code_convert("utf-8","gb2312",inbuf,inlen,outbuf,outlen);
}
//GB2312码转为UNICODE码

int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
	return code_convert("gb2312","utf-8",inbuf,inlen,outbuf,outlen);
}

//iInLen的长度不包括\0，应该用strlen。返回值是处理后的sOut长度
int Gb2312ToUtf8(char *sOut, int iMaxOutLen, const char *sIn, int iInLen)
{
    char *pIn = (char *)sIn;
    char *pOut = sOut;
    size_t ret;
    size_t iLeftLen=iMaxOutLen;
    iconv_t cd;

    cd = iconv_open("utf-8", "gb2312");
    if (cd == (iconv_t) - 1)
    {
        return -1;
    }
    size_t iSrcLen=iInLen;
    ret = iconv(cd, &pIn,&iSrcLen, &pOut,&iLeftLen);
    if (ret == (size_t) - 1)
    {
        iconv_close(cd);
        return -1;
    }

    iconv_close(cd);
    return (iMaxOutLen - iLeftLen);
}

//iInLen的长度不包括\0，应该用strlen。返回值是处理后的sOut长度
int Utf8ToGb2312(char *sOut, int iMaxOutLen, const char *sIn, int iInLen)
{
    char *pIn = (char *)sIn;
    char *pOut = sOut;
    size_t ret;
    size_t iLeftLen=iMaxOutLen;
    iconv_t cd;

    cd = iconv_open("gb2312", "utf-8");

    if (cd == (iconv_t) - 1)
    {
        return -1;
    }
	
    size_t iSrcLen=iInLen;
    ret = iconv(cd, &pIn,&iSrcLen, &pOut,&iLeftLen);
	
    if (ret == (size_t) - 1)
    {
        iconv_close(cd);
        return -1;
    }

    iconv_close(cd);

    return (iMaxOutLen - iLeftLen);
}


