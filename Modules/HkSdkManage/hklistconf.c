#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jsonmap.h"
#include "hklistconf.h"

static int HkDvrListConfigDefault()
{
	int i ;
	HKDVRParam_T HkDvrListParam;
	memset(&HkDvrListParam,0,sizeof(HKDVRParam_T));
	#if 0
	HkDvrListParam.HkDvrInfo.wPort=0;
	HkDvrListParam.HkDvrInfo.wChannel=3;
	sprintf(HkDvrListParam.HkDvrInfo.sDeviceAddress,"192.168.1.1");
	sprintf(HkDvrListParam.HkDvrInfo.sUserName,"sUserName");
	sprintf(HkDvrListParam.HkDvrInfo.sPassword,"sPassword");
	sprintf(HkDvrListParam.HkDvrInfo.sDvrSerialNumber,"sDvrSerialNumber");
	
	for(i = 0;i<3;i++){
		HkDvrListParam.HkSdkParam.sOnline=0;
		HkDvrListParam.HkSdkParam.sChannel=i;
		sprintf(HkDvrListParam.HkSdkParam.sDeviceName,"Default_%d",i);
		sprintf(HkDvrListParam.HkSdkParam.sDeviceAddress,"192.168.1.1");
	}
	#endif
	cJSON *confDvrList = JsonMapDvrListInfoMake(&HkDvrListParam,0);
	if(NULL == confDvrList){
		return KEY_FALSE;
	}
	return ConfigManageSet(confDvrList,D_CONFDVRLIST_STR);
}

int HkDvrListConfigGet(HKDVRParam_T * HkDvrListParam)
{
	int s32Ret=KEY_FALSE;
    cJSON *HkDvr=NULL;
    HkDvr=ConfigManageGet(D_CONFDVRLIST_STR);
    if(NULL==HkDvr)
    {
        HkDvrListConfigDefault();
        HkDvr=ConfigManageGet(D_CONFDVRLIST_STR);
	}
    s32Ret=JsonMapDvrInfoPares(HkDvr, HkDvrListParam);
	cJSON_Delete(HkDvr);HkDvr=NULL;
	return s32Ret;
}

int HkDvrListConfigSet(HKDVRParam_T * HkDvrListParam)
{
	int s32Ret=KEY_FALSE;
	int i = 0;
	cJSON *HkDvr=NULL;
	cJSON *newitem=NULL;
	HKDVRParam_T HkDvrListInfo[64];
    HkDvr=ConfigManageGet(D_CONFDVRLIST_STR);
	JsonMapDvrInfoPares(HkDvr, HkDvrListInfo);
	newitem = JsonMapDvrInfoMake(HkDvrListParam);

	for(i = 0 ; i<NET_DVR_MAX_LEN; i++ ){
		if(0 == strcmp(HkDvrListInfo[i].HkDvrInfo.sDvrSerialNumber,HkDvrListParam->HkDvrInfo.sDvrSerialNumber)){
			break;
		}
	}
	
	cJSON *DvrArray = cJSON_GetObjectItem(HkDvr, "Dvrlist");
	cJSON_ReplaceItemInArray(DvrArray,i,newitem);
	return ConfigManageSet(HkDvr,D_CONFDVRLIST_STR);
}

int HkDvrListConfigAdd(HKDVRParam_T * HkDvrListParam)
{
	int i = 0;
	cJSON *HkDvr=NULL;
	cJSON *newitem=NULL;
	HKDVRParam_T HkDvrListInfo[NET_DVR_MAX_LEN];
	HkDvr=ConfigManageGet(D_CONFDVRLIST_STR);
	
	newitem = JsonMapDvrInfoMake(HkDvrListParam);
	cJSON *DvrArray = cJSON_GetObjectItem(HkDvr, "Dvrlist");
	JsonMapDvrInfoPares(HkDvr, HkDvrListInfo);
	for(i = 0 ; i<NET_DVR_MAX_LEN; i++ ){
		if(0 == strcmp(HkDvrListInfo[i].HkDvrInfo.sDvrSerialNumber,HkDvrListParam->HkDvrInfo.sDvrSerialNumber)){
			break;
		}
	}
	
	if(i >= NET_DVR_MAX_LEN){
		cJSON_AddItemToArray(DvrArray, newitem);
	}else{
		cJSON_ReplaceItemInArray(DvrArray,i,newitem);
	}
	
	return ConfigManageSet(HkDvr,D_CONFDVRLIST_STR);
}

int HkDvrListConfigDel(HKDVRParam_T * HkDvrListParam)
{
	cJSON *HkDvr=NULL;
	cJSON *newitem=NULL;
	HKDVRParam_T HkDvrListInfo[64];
	HkDvr=ConfigManageGet(D_CONFDVRLIST_STR);
	JsonMapDvrInfoPares(HkDvr, HkDvrListInfo);
	int i = 0;
	for(i = 0 ; i<NET_DVR_MAX_LEN; i++ ){
		if(0 == strcmp(HkDvrListInfo[i].HkDvrInfo.sDvrSerialNumber,HkDvrListParam->HkDvrInfo.sDvrSerialNumber)){
			break;
		}
	}
	JsonMapDvrInfoPares(HkDvr, HkDvrListInfo);
	cJSON *DvrArray = cJSON_GetObjectItem(HkDvr, "Dvrlist");
	cJSON_DeleteItemFromArray(DvrArray,i);
	return ConfigManageSet(HkDvr,D_CONFDVRLIST_STR);
}


static int HkDeviceAlarmConfigDefault()
{
	int i ;
	DeviceAlarmParam_T info = {0};
	memset(&info,0,sizeof(DeviceAlarmParam_T));
	info.smokeLev = 1;
	info.hightempLev = 1;
	cJSON *DeviceAlarm = JsonMapHkDeviceAlarmInfoMake(&info);
	if(NULL == DeviceAlarm){
		return KEY_FALSE;
	}
	return ConfigManageSet(DeviceAlarm,D_CONFDEVVICEALARMLEAVE_STR);
}


int HkDeviceAlarmConfigGet(DeviceAlarmParam_T *info)
{
	if(NULL == info)
	{
		return KEY_FALSE;
	}
	
	int s32Ret=KEY_FALSE;
    cJSON *Alarm=NULL;
    Alarm = ConfigManageGet(D_CONFDEVVICEALARMLEAVE_STR);
    if(NULL == Alarm)
    {
        HkDeviceAlarmConfigDefault();
        Alarm=ConfigManageGet(D_CONFDEVVICEALARMLEAVE_STR);
	}
	
    s32Ret = JsonMapDeviceAlarmInfoPares(Alarm, info);
	cJSON_Delete(Alarm);Alarm=NULL;
	return KEY_TRUE;
}

int HkDeviceAlarmConfigSet(DeviceAlarmParam_T *info)
{
	if(NULL == info)
	{
		return KEY_FALSE;
	}

	cJSON *Alarm = ConfigManageGet(D_CONFDEVVICEALARMLEAVE_STR);
	if(NULL != Alarm){
		cJSON_ReplaceItemInObject(Alarm, "smoke", cJSON_CreateNumber(info->smokeLev));
		cJSON_ReplaceItemInObject(Alarm, "hightemp", cJSON_CreateNumber(info->hightempLev));
		ConfigManageSet(Alarm,D_CONFDEVVICEALARMLEAVE_STR);
	}
	return KEY_FALSE;
}

