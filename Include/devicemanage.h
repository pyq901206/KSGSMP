#ifndef __DEVICEMANAGE_H__
#define __DEVICEMANAGE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define MAX_GET_TIMEOUT (5)

typedef enum _ENUM_DEVICEMODELS_E{
	DEVICEMODEL_G7 = 0,
	DEVICEMODEL_BM1,
}ENUM_DEVICEMODELS_E;
	
typedef struct _DeviceInfoParam_T{
	ENUM_DEVICEMODELS_E	deviceModel;	
	ENUM_STATUS_E		onlineStatus;
	char				deviceIpaddr[16];
	char				deviceID[DF_USERDEVICENUM_MAX];
	char				deviceName[DF_USERDEVICENUM_MAX];	
	char				deviceSoftwareVer[DF_USERDEVICENUM_MAX];	
}DeviceInfoParam_T;

//设备信息结构数据
typedef struct _DeviceStorageInfo_T{
	DeviceInfoParam_T	deviceInfo;
}DeviceStorageInfo_T;

typedef struct _DeviceManageHandle_T{
	int (*pGetDeviceInfo)(char *deviceID,DeviceStorageInfo_T *cloudInfo);
	int (*pSetDeviceInfo)(char *deviceID,DeviceStorageInfo_T *cloudInfo);
	int (*pAddDevice)(DeviceStorageInfo_T *info);
	int (*pDelDevice)(char *deviceID);
	int (*pGetDeviceInfoNum)(int *num);
	int (*pGetDeviceInfoList)(DeviceStorageInfo_T *deviceInfo);
}DeviceManageHandle_T;

_OUT_ int DeiveManageInit(DeviceManageHandle_T *handle);
_OUT_ int DeiveManageUnInit(DeviceManageHandle_T *handle);
_OUT_ int DeiveManageRegister(DeviceManageHandle_T *handle);
_OUT_ int DeiveManageUnRegister(DeviceManageHandle_T *handle);

#endif
