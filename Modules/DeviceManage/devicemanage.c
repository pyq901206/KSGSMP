#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "devicemanage.h"
#include "usrmanage.h"
#include "handlemanage.h"
#include "mysqlctrl.h"
#include "mqttapi.h"
#include "jsonmap.h"

typedef struct _DeviceListInner_T{
	HandleManage_T listHead;      //设备链表
	MySqlManage_T mysqlHandle;
}DeviceListInner_T;
static int deviceExitFlag = ENUM_ENABLED_FALSE;
static DeviceListInner_T g_deviceList;

static DeviceListInner_T *GetDeviceListHandle()
{
	return &g_deviceList;
}

//入链表
typedef struct _DeviceListInfo_T{
	DeviceStorageInfo_T deviceInfo;
	pthread_mutex_t 	selfMutex;
}DeviceListInfo_T;

static int GetUUIDNonce(char *uuid)
{
	char uuidNumber[8] = {0};
	if(NULL == uuid) return KEY_FALSE;
	char *ps = strstr(uuid,"-");
	if(NULL == ps){
		return KEY_FALSE;
	}
	ps = ps + 1;
	char *pe = strstr(ps,"-");
	if(NULL == pe){
		return KEY_FALSE;
	}
	memcpy(uuidNumber,ps,pe-ps);
	return atoi(uuidNumber);
}

static int GetDeviceInfoByUUID(char *uuid, DeviceListInfo_T *deviceInfo)
{
	void *result = NULL;
	if(NULL == deviceInfo || !strlen(uuid)){
		return KEY_FALSE;
	}
	HandleManageGetHandleByStr(&GetDeviceListHandle()->listHead, deviceInfo.deviceInfo.deviceID, uuid, result, DeviceListInfo_T);
	if(NULL == result){
		return KEY_NOTFOUND;
	}
	DeviceListInfo_T *device = (DeviceListInfo_T *)result;
	if (0 != strcmp(uuid, device->deviceInfo.deviceInfo.deviceID))
	{
		return KEY_NOTFOUND;
	}
	MUTEX_LOCK(&deviceInfo->selfMutex);
	memcpy(deviceInfo,device,sizeof(DeviceListInfo_T));
	MUTEX_UNLOCK(&deviceInfo->selfMutex);
	return KEY_TRUE;
}

static int GetDeviceInfo(char *deviceID, DeviceStorageInfo_T *info)
{
	int iret = KEY_FALSE;
	DeviceListInfo_T deviceInfo;
	memset(&deviceInfo,0,sizeof(DeviceListInfo_T));
	iret = GetDeviceInfoByUUID(deviceID, &deviceInfo);
	if (iret == KEY_TRUE)
	{
		memcpy(info, &deviceInfo.deviceInfo, sizeof(DeviceStorageInfo_T));
	}
	return iret;
}

static int SetDeviceInfo(char *deviceID, DeviceStorageInfo_T *info)
{
	int iret = KEY_FALSE;
	void *result = NULL;
	DeviceListInfo_T *deviceInfo;
	if (0 >= strlen(deviceID) || NULL == info){
		return KEY_FALSE;
	}
	HandleManageGetHandleByStr(&GetDeviceListHandle()->listHead, deviceInfo.deviceInfo.deviceID, deviceID, result, DeviceListInfo_T);
	if(NULL == result){
		return KEY_NOTFOUND;
	}
	
	deviceInfo = (DeviceListInfo_T *)result;
	MUTEX_LOCK(&deviceInfo->selfMutex);
	memcpy(&deviceInfo->deviceInfo, info, sizeof(DeviceStorageInfo_T));
	
	MySqlManage_T gHandle = GetDeviceListHandle()->mysqlHandle;		
	if (NULL != gHandle.pSetDeviceInfo)
	{
		iret = gHandle.pSetDeviceInfo(&deviceInfo->deviceInfo);
	}
	MUTEX_UNLOCK(&deviceInfo->selfMutex);
	return iret;
}

static int GetCameraInfoNum(int *num)
{
	*num = 0;
	DeviceListInfo_T *deviceInfo;
	HandleManage_T*	head = &GetDeviceListHandle()->listHead;
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head->next;cur != NULL;cur = cur->next)
	{
		deviceInfo = cur->handle;
		if (strlen(deviceInfo->deviceInfo.deviceInfo.deviceID))
		{
			(*num)++;
		}
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;	
}

static int GetCameraInfoList(DeviceStorageInfo_T *deviceInfo)
{
	int i = 0, num = 0;
	DeviceListInfo_T *deviceListInfo;
	HandleManage_T*	head = &GetDeviceListHandle()->listHead;
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	{
		for(cur = head->next;cur != NULL;cur = cur->next)
		{
			deviceListInfo = cur->handle;
			if (deviceListInfo)
			{
				memcpy(&deviceInfo[i], &deviceListInfo->deviceInfo, sizeof(DeviceStorageInfo_T));
				i++;
			}
		}
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;	
}

static int AddDevice(DeviceStorageInfo_T *info)
{
	int iret = KEY_FALSE;
	DeviceListInfo_T *deviceInfoList;
	void *result = NULL;
	if(NULL == info)
	{
		return iret;
	}
	
	if (!strlen(info->deviceInfo.deviceID))
	{
		return iret;
	}
	HandleManageGetHandleByStr(&GetDeviceListHandle()->listHead, deviceInfo.deviceInfo.deviceID, info->deviceInfo.deviceID, result, DeviceListInfo_T);
	if(NULL != result){
		DF_ERROR("The device already exists.");
		return KEY_FORBIDDEN;
	}
	deviceInfoList = (DeviceListInfo_T *)malloc(sizeof(DeviceListInfo_T));
	MUTEX_INIT(&deviceInfoList->selfMutex);
	MUTEX_LOCK(&deviceInfoList->selfMutex);
	memset(&deviceInfoList->deviceInfo.deviceInfo,0,sizeof(DeviceInfoParam_T));
	memcpy(&deviceInfoList->deviceInfo.deviceInfo, &info->deviceInfo, sizeof(DeviceInfoParam_T));
	HandleManageAddHandle(&GetDeviceListHandle()->listHead, (void *)deviceInfoList);
	MySqlManage_T gHandle = GetDeviceListHandle()->mysqlHandle;		
	if (NULL != gHandle.pAddDevice)
	{
		iret = gHandle.pAddDevice(info);
	}
	MUTEX_UNLOCK(&deviceInfoList->selfMutex);	
	return iret;
}

static int DelDevice(char *deviceID)
{
	int iret =  KEY_FALSE;
	DeviceListInfo_T *deviceInfo;
	void *result = NULL;
	if (32 < strlen(deviceID))
	{
		DF_ERROR("Invalid parameter.!!!");
		return KEY_FALSE;
	}
	if (!strlen(deviceID)){
		return KEY_NOTFOUND;
	}
	HandleManageGetHandleByStr(&GetDeviceListHandle()->listHead, deviceInfo.deviceInfo.deviceID, deviceID, result, DeviceListInfo_T);
	if (NULL == result){
		DF_DEBUG("The device no exists.");
		return KEY_NOTFOUND;
	}
	deviceInfo = (DeviceListInfo_T *)result;
	HandleManageDelHandle(&GetDeviceListHandle()->listHead, (void *)deviceInfo);
	MyFree(deviceInfo);
	MySqlManage_T gHandle = GetDeviceListHandle()->mysqlHandle;		
	if (NULL != gHandle.pDelDevice)
	{
		iret = gHandle.pDelDevice(deviceID);
	}
	return iret;
}

static int InitDeviceList()
{
	//数据从数据库同步到内存
	int i = 0, num = 0, j = 0;
	DeviceListInfo_T *deviceInfo = NULL;
	DeviceStorageInfo_T *deviceListInfo;
	HandleManageInitHead(&GetDeviceListHandle()->listHead);
	MySqlManage_T mysqlHandle = GetDeviceListHandle()->mysqlHandle;
	if (NULL == mysqlHandle.pGetDeviceInfoList || NULL == mysqlHandle.pGetDeviceInfoListNum){
		DF_DEBUG("Init Device List Fail.");
		return KEY_FALSE;
	}
	mysqlHandle.pGetDeviceInfoListNum(&num);
	if (0 >= num){
		if (num == 0)
		{
			return KEY_TRUE;
		}
		else
		{			
			DF_DEBUG("Init Device List Fail.");
			return KEY_FALSE;
		}
	}
	deviceListInfo = (DeviceStorageInfo_T *)malloc(sizeof(DeviceStorageInfo_T) * num);
	memset(deviceListInfo, 0, sizeof(DeviceStorageInfo_T) * num);
	if (KEY_TRUE != mysqlHandle.pGetDeviceInfoList(deviceListInfo)){
		free(deviceListInfo);
		deviceListInfo = NULL;
		return KEY_FALSE;
	}
	for (i = 0; i < num; i++)
	{
		deviceInfo = (DeviceListInfo_T *)malloc(sizeof(DeviceListInfo_T));
		memset(deviceInfo, 0, sizeof(DeviceListInfo_T));
		memset(&(deviceInfo->deviceInfo), 0, sizeof(DeviceStorageInfo_T));
		MUTEX_INIT(&deviceInfo->selfMutex);
		memcpy(&(deviceInfo->deviceInfo), &deviceListInfo[i], sizeof(DeviceStorageInfo_T));
		HandleManageAddHandle(&GetDeviceListHandle()->listHead, (void *)deviceInfo);
	}
	free(deviceListInfo);
	deviceListInfo = NULL;
	return KEY_TRUE;
}

static int UnInitDeviceList()
{
	HandleManage_T*	head = &GetDeviceListHandle()->listHead;
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head->next;cur != NULL;cur = cur->next){
		free(cur->handle);
		if(cur->pre != head && cur->pre != NULL){
			free(cur->pre);
		}
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;	
}

_OUT_ int DeiveManageRegister(DeviceManageHandle_T *handle)
{
	if (NULL == handle)
	{
		return KEY_FALSE;
	}
	handle->pAddDevice = AddDevice;
	handle->pDelDevice = DelDevice;
	handle->pGetDeviceInfo = GetDeviceInfo;
	handle->pSetDeviceInfo = SetDeviceInfo;
	handle->pGetDeviceInfoNum = GetCameraInfoNum;
	handle->pGetDeviceInfoList = GetCameraInfoList;
	return KEY_TRUE;
}

_OUT_ int DeiveManageUnRegister(DeviceManageHandle_T *handle)
{
	if (NULL == handle)
	{
		return KEY_FALSE;
	}
	handle->pAddDevice = NULL;
	handle->pDelDevice = NULL;
	handle->pGetDeviceInfo = NULL;
	handle->pSetDeviceInfo = NULL;
	handle->pGetDeviceInfoNum = NULL;
	handle->pGetDeviceInfoList = NULL;
	return KEY_TRUE;
}

_OUT_ int DeiveManageInit(DeviceManageHandle_T *handle)
{
	if(NULL == handle){
		return KEY_FALSE;
	}
	deviceExitFlag = ENUM_ENABLED_TRUE;
	DeviceListInner_T *gHandle =  GetDeviceListHandle();
	MySqlManageRegister(&gHandle->mysqlHandle);
	InitDeviceList();
	DeiveManageRegister(handle);
	return KEY_TRUE;
}

_OUT_ int DeiveManageUnInit(DeviceManageHandle_T *handle)
{
	if(NULL == handle){
		return KEY_FALSE;
	}
	deviceExitFlag = ENUM_ENABLED_FALSE;
	DeviceListInner_T *gHandle =  GetDeviceListHandle();
	MySqlManageUnRegister(&gHandle->mysqlHandle);
	DeiveManageUnRegister(handle);
	UnInitDeviceList();
	return KEY_TRUE;
}
