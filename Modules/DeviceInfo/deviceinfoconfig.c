/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年02月20日
|  说明: 预留JSON组装和解析接口 为HTTP API 透传做准备
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

