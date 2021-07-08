/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年03月28日
|  说明:
|
|  版本: 1.1
|  作者:
|  日期:
|  说明:
******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "tsocket.h"
#include "adpapi.h"
#include "jsonswitch.h"
#include "httppares.h"

typedef struct _HttpApi_T{
	NetWorkSerHandle_T netSer; 
	NetWorkSerHandle_T searchSer; 
	AdpAPIHandle_T innerAdpHandle;
}HttpApi_T; 

static HttpApi_T g_handle;

static HttpApi_T *GetHandle()
{
	return &g_handle;
}

static int InitHandle()
{
	memset(GetHandle(),0,sizeof(HttpApi_T));
	return 0;
}

static int GetEthName(char *ethIP,char *name)
{
	return KEY_FALSE;
}

static int GetNetCabInfo(NetCab_T *info)
{
	
	return KEY_TRUE;
}

typedef struct _HttpInfo_S{
	int ContentLen;
	int HeadLen;
	int IsHeadEnd;
	char *pHeadEnd;
}HttpDef_S;

static int HttpClose(int sockfd)
{
	if (0 < sockfd){
		close(sockfd);
	}
	return KEY_TRUE;
}

static int HttpChecked(NetCliInfo_T *cli)
{
	HttpDef_S *info;
	char *headEnd = NULL;
	char *ps;
	char *pe;
	char tmp[32];
	if(NULL == cli->usrdef){
		cli->usrdef = (void *)malloc(sizeof(HttpDef_S));
		memset(cli->usrdef,0,sizeof(HttpDef_S));
		info = (HttpDef_S *)cli->usrdef;
		info->IsHeadEnd = KEY_FALSE;
	}
	info = (HttpDef_S *)cli->usrdef;
	if(info->IsHeadEnd == KEY_FALSE){
		if(NULL == (headEnd =  strstr(cli->recvmsg,"\r\n\r\n"))){
			//DF_DEBUG("last one http head is not end");
			return KEY_FALSE;
		}
		headEnd += strlen("\r\n\r\n");
		//info->pHeadEnd = headEnd;
		info->HeadLen = headEnd - cli->recvmsg;
		info->IsHeadEnd = KEY_TRUE;		
	}
	
	if(0 == info->ContentLen){
		ps =  strstr(cli->recvmsg, "content-length:");
		if(NULL == ps){
			ps = strstr(cli->recvmsg, "Content-Length:");
			if(NULL == ps) {
				DF_DEBUG("can't find Content-Length and content-length:");
				return KEY_TRUE;
			}
		}
		pe =  strstr(ps,"\r\n");
		if(NULL == pe){
			return KEY_TRUE;
		}

		ps += strlen("Content-Length:");
		ps++;

		memset(tmp,0,sizeof(tmp));
		memcpy(tmp,ps,pe - ps);
		info->ContentLen = atoi(tmp);
//		DF_DEBUG("ContentLen = %d", info->ContentLen);
	}
	
//	DF_DEBUG("ContentLen = %d %d", info->ContentLen + info->HeadLen, cli->recvLen);
	if((info->ContentLen + info->HeadLen) <= cli->recvLen){
		return KEY_TRUE;
	}
	return KEY_FALSE;
}

static int Requst(NetCliInfo_T *cli)
{
	if(	0 == strlen(cli->fromIP) || 
		0 >= cli->recvLen || 
		0 >= cli->fromPort || 
		0 >= cli->recvSocket){
		DF_DEBUG("%s %d %d %d\r\n",cli->fromIP,cli->recvLen,cli->fromPort,cli->recvSocket);
		DF_DEBUG("HTTP API Requst param error\r\n");
		return KEY_FALSE;
	}
	char out[HTTPMSG_MAXLEN];
	int outLen = 0;
	memset(out,0,HTTPMSG_MAXLEN);
	DF_DEBUG("len = %d, msg =%s, socket = %d",cli->recvLen,cli->recvmsg,cli->recvSocket);
	ParesHttpMsg(cli->recvmsg,cli->recvLen,out,&outLen);
	//DF_DEBUG("outlen  = %d,out = %s",outLen,out);
	NetTcpSendMsg(cli->recvSocket,out,outLen);
	return KEY_TRUE;
}

int HttpApiStart(int port)
{
	NetWorkSerHandle_T *netHandle = &(GetHandle()->netSer);
	netHandle->readcb = Requst;
	netHandle->checkendcb = HttpChecked;
	netHandle->closecb = HttpClose;
	netHandle->serverport = port;
	NetCreateTcpSer(netHandle);
	AdpRegistHandle(&GetHandle()->innerAdpHandle, INNER_USER_NAME, INNER_USER_PASSWORD);
	return 0;
}

int HttpApiStop()
{
	NetWorkSerHandle_T *netHandle = &(GetHandle()->netSer);
	NetWorkSerHandle_T *mulicastHandle = &(GetHandle()->searchSer);
	NetCloseSerSocket(netHandle);
	AdpUnRegistHandle(&GetHandle()->innerAdpHandle);
	return 0;
}

int HttpApiInit()
{
	JsonApiInit();
	InitHandle();
	return 0;
}

int HttpApiUnInit()
{	
	return 0;
}

