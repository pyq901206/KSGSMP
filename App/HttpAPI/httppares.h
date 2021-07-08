/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0
|  ����: ̷�� [tanfan0406@163.com]
|  ����: 2018��03��28��
|  ˵��:
|
|  �汾: 1.1
|  ����:
|  ����:
|  ˵��:
******************************************************************/
#ifndef __HTTPPARES_H__
#define __HTTPPARES_H__

#ifndef HTTPMSG_MAXLEN 
#define HTTPMSG_MAXLEN 1024*10
#endif

#ifndef HTTPMSG_SERVER
#define HTTPMSG_SERVER "GaoZhi/2.0"
#endif

#ifndef HTTP_XMLTAG
#define HTTP_XMLTAG	  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
#endif

typedef enum _HTTP_METHOD_E{
	HTTP_NULL = 0x00,
	HTTP_POST = 0X01,
	HTTP_GET,
}HTTP_METHOD_E;

typedef enum _HTTP_CONNECT_E{
	HTTP_CONNECT_NULL = 0x00,
	HTTP_CONNECT_CLOSE = 0X01,
	HTTP_CONNECT_KEEPALIVE,
}HTTP_CONNECT_E;

typedef struct _HttpReqHead_T{
	HTTP_METHOD_E	method;
	HTTP_CONNECT_E	connect;
	int 			contentLength;
	char 			host[64];//主机名
	char			url[128];
	char 			boundary[256];
}HttpReqHead_T;


int ParesHttpMsg(char *msg,int len,char *out,int *outlen);
int ParesPayPalMsg(char *msg,int len,char *out,int *outlen);
int CheckedDataComefromPayPal(char *data,int len);
#endif
