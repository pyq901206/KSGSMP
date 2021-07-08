/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年2月22日
|  说明:
|  修改:
|	
******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "publickey.h"


static PublicKey_T g_publicKey;
#define MICROSOFTAZUREKEY "tDxh/B35tYmtlKFxUbAL5t2oBm0vM7wo5/ME5tmgqVftRs2+8MsLFHW08jUdwCbzBd8kYSs02Umz8EjSX7j2BQ=="

int GetPublicKey(PublicKey_T *key)
{
	memcpy(key->publicKey, MICROSOFTAZUREKEY, sizeof(key->publicKey));
	memcpy(key->accountName, "gaozhichina", sizeof(key->accountName));
	memcpy(key->endpointSuffix, "gaozhichina.blob.core.windows.net", sizeof(key->endpointSuffix));
	return KEY_TRUE;
}

int SetPublicKey(PublicKey_T *key)
{
	memcpy(&g_publicKey,key->publicKey,sizeof(PublicKey_T));
	return KEY_TRUE;
}

_OUT_ int PublicKeyRegist(PublicKeyManage_T *handle)
{
	if(NULL == handle){
		return -1;
	}
	handle->pGetPublicKey = GetPublicKey;
	handle->pSetPublicKey = SetPublicKey;
	return KEY_TRUE;
}

_OUT_ int PublicKeyUnRegist(PublicKeyManage_T *handle)
{
	if(NULL == handle){
		return -1;
	}
	handle->pGetPublicKey = NULL;
	handle->pSetPublicKey = NULL;
	return KEY_TRUE;
}

_OUT_ int PublicKeyInit(PublicKeyManage_T *handle)
{
	PublicKeyRegist(handle);
	return KEY_TRUE;
}

_OUT_ int PublicKeyUnInit(PublicKeyManage_T *handle)
{
	PublicKeyUnRegist(handle);
	return KEY_TRUE;
}
