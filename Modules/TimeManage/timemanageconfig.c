/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0 
|  ����: ̷�� [tanfan0406@163.com]
|  ����: 2018��02��20��
|  ˵��: Ԥ��JSON��װ�ͽ����ӿ� ΪHTTP API ͸����׼��
|
******************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "timemanageconfig.h"
#include "jsonmap.h"


int  TimeManageConfigSet(TimeConf_T *info)
{
	cJSON *confTime = JsonMapTimeManageConfigMake(info);
	if(NULL == confTime){return KEY_FALSE;}
	return ConfigManageSet(confTime,D_CONFTIMECONF_STR);
}

static int TimeManageConfigDefault()
{
	TimeConf_T info = {0};
	info.currTime = 8192;
	info.timeZone = 57;
	TimeManageConfigSet(&info);
	return KEY_TRUE;
}

int  TimeManageConfigGet(TimeConf_T *info)
{
    int s32Ret=KEY_FALSE;
    cJSON *cjtmp=NULL;
    cjtmp=ConfigManageGet(D_CONFTIMECONF_STR);
    if(NULL==cjtmp)
    {
        TimeManageConfigDefault();
        cjtmp=ConfigManageGet(D_CONFTIMECONF_STR);
	}
    s32Ret=JsonMapTimeManageConfigPares(cjtmp, info);
	cJSON_Delete(cjtmp);cjtmp=NULL;
	return s32Ret;
}


