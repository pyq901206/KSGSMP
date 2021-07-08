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
#include "deviceinfoconfig.h"
#include "jsonmap.h"

static int DevInfoConfigDefault()
{
	DeviceInfo_T info = {0};
	sprintf(info.facturer,"%s","GaoZhi");
	sprintf(info.model,"%s","AIMP v1.0");
	sprintf(info.serialNumber,"%s","aswQQx142zZs11Qa");
	sprintf(info.softwareVer,"%s","v1.0.1");
	sprintf(info.name,"%s","AIMP");
	sprintf(info.country,"%s","china");
	sprintf(info.city,"%s","changsha");
	info.devType = ENUM_DEVTYPE_PlatFrom;
	info.coverDay = 1;
	info.pullInterval = 5;
	info.faceThreshold = 70;
	DevInfoConfigSet(&info);
	return KEY_TRUE;
}

int DevInfoConfigSet(DeviceInfo_T *info)
{
	cJSON *confDevice = JsonMapDeviceInfoMake(info);
	if(NULL == confDevice){return KEY_FALSE;}
	return ConfigManageSet(confDevice,D_CONFDEVINFO_STR);
}

int DevInfoConfigGet(DeviceInfo_T *info)
{
	if(KEY_FALSE == JsonMapDeviceInfoPares(ConfigManageGet(D_CONFDEVINFO_STR),info)){
		DevInfoConfigDefault();
		return JsonMapDeviceInfoPares(ConfigManageGet(D_CONFDEVINFO_STR),info);
	}
	return KEY_TRUE;
}

