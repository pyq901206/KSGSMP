/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年10月22日
|  说明:
|  修改:
******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#ifndef __PUBLICKEYMANAGE_H__
#define __PUBLICKEYMANAGE_H__

typedef struct _PublicKey_T{
	char publicKey[512];
	char accountName[128];
	char endpointSuffix[128];
}PublicKey_T;

typedef struct _PublicKeyManage_T{
	int (*pGetPublicKey)(PublicKey_T *key);
	int (*pSetPublicKey)(PublicKey_T *key);
}PublicKeyManage_T;



_OUT_ int PublicKeyInit(PublicKeyManage_T *handle);
_OUT_ int PublicKeyUnInit(PublicKeyManage_T *handle);
_OUT_ int PublicKeyRegist(PublicKeyManage_T *handle);
_OUT_ int PublicKeyUnRegist(PublicKeyManage_T *handle);

#endif
