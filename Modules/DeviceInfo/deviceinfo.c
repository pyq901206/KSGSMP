#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "deviceinfoconfig.h"
#include "deviceinfo.h"

static int GetDeviceInfo(DeviceInfo_T *info)
{
	return DevInfoConfigGet(info);
}

static int SetDeviceInfo(DeviceInfo_T *info)
{
	if (info->coverDay > 7 || info->coverDay < 1)
	{
		return KEY_FALSE;
	}

	if (info->pullInterval > 10 || info->pullInterval < 1)
	{
		return KEY_FALSE;
	}

	if (info->faceThreshold > 100 || info->faceThreshold < 1)
	{
		return KEY_FALSE;
	}
	
	return DevInfoConfigSet(info);
}

static int SetDeviceOperation(ENUM_DEVICEOPT_E operation)
{
	switch(operation){
		case ENUM_DEVICEOPT_HARDFACDEFAULT:
			printf("this operation is HARDFACDEFAULT \r\n");
			break;
		case ENUM_DEVICEOPT_SOFTFACDEFAULT:
			printf("this operation is SOFTFACDEFAULT \r\n");
			break;
		default:
			printf("this operation not recongnition\r\n");
			break;
	}
	return KEY_TRUE;
}

int LocalInfoRegist(DeciceHandle_T *handle)
{
	if(NULL == handle){
		return -1;
	}
	handle->pGetDeviceInfo= GetDeviceInfo;
	handle->pSetDeviceInfo = SetDeviceInfo;
	handle->pSetDeviceOperation = SetDeviceOperation;
	return KEY_TRUE;
}

int LocalInfoUnRegist(DeciceHandle_T *handle)
{
	if(NULL == handle){
		return -1;
	}
	handle->pGetDeviceInfo= NULL;
	handle->pSetDeviceInfo = NULL;
	handle->pSetDeviceOperation = NULL;
	return KEY_TRUE;
}

_OUT_ int LocalInfoInit(DeciceHandle_T *handle)
{
	DeviceInfo_T info = {0};
	DevInfoConfigGet(&info);
	LocalInfoRegist(handle);
	DF_DEBUG("DeviceInfoInit success");
	return KEY_TRUE;
}

_OUT_ int  LocalInfoUnInit(DeciceHandle_T *handle)
{
	return KEY_TRUE;
}
