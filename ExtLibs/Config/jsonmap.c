/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年4月14日
|  说明:
******************************************************************/
#include "jsonmap.h"
#include "base64.h"

int GetGradeLevel(int gradescore)
{
	if(0 == gradescore)
	{
		return 0;
	}
	else if(0 < gradescore < 250)
	{
		return 1;
	}
	else if(249 < gradescore < 500)
	{
		return 2;
	}
	else if(499 < gradescore < 750)
	{
		return 3;
	}
	else if(749 < gradescore < 1000)
	{
		return 4;
	}
	else if(gradescore > 1000)
	{
		return 5;
	}
	else
	{
		return 0;
	}
	return 0;
}


//本地管理
cJSON* JsonMapDeviceInfoMake(DeviceInfo_T *info)
{
	if(NULL == info){
		return NULL;
	}
	cJSON *confDevice = cJSON_CreateObject();
	cJSON_AddStringToObject(confDevice, "manufacturer",info->facturer);
	cJSON_AddStringToObject(confDevice, "model",info->model);
	cJSON_AddStringToObject(confDevice, "uuid",info->uuid);
	cJSON_AddStringToObject(confDevice, "serialNumber",info->serialNumber);
	cJSON_AddStringToObject(confDevice, "softwarever",info->softwareVer);
	cJSON_AddStringToObject(confDevice, "name",info->name);
	cJSON_AddStringToObject(confDevice, "country",info->country);
	cJSON_AddStringToObject(confDevice, "city",info->city);
	cJSON_AddNumberToObject(confDevice, "devType",info->devType);
	cJSON_AddNumberToObject(confDevice, "coverday",info->coverDay);
	cJSON_AddNumberToObject(confDevice, "pullinterval",info->pullInterval);
	cJSON_AddNumberToObject(confDevice, "facethreshold",info->faceThreshold);   
	return confDevice;
}

int JsonMapDeviceInfoPares(cJSON *conf, DeviceInfo_T *info)
{
	if(NULL == conf || NULL == info){
		return KEY_FALSE;
	}
	
	DeviceInfo_T tempInfo = { 0 };
	int ret = -1, devType = -1;
	memset(&tempInfo, 0, sizeof(tempInfo));
	ret = cJSON_GetObjectItemValueString(conf,"manufacturer",tempInfo.facturer,sizeof(tempInfo.facturer));
	if(KEY_FALSE != ret) {
		snprintf(info->facturer, sizeof(tempInfo.facturer),"%s",tempInfo.facturer);
	}
	ret = cJSON_GetObjectItemValueString(conf,"model",tempInfo.model,sizeof(tempInfo.model));
	if(KEY_FALSE != ret) {
		snprintf(info->model, sizeof(tempInfo.model),"%s",tempInfo.model);
	}
	ret = cJSON_GetObjectItemValueString(conf,"serialNumber",tempInfo.serialNumber,sizeof(tempInfo.serialNumber));
	if(KEY_FALSE != ret) {
		snprintf(info->serialNumber, sizeof(tempInfo.serialNumber),"%s",tempInfo.serialNumber);
	}
	ret = cJSON_GetObjectItemValueString(conf,"softwarever",tempInfo.softwareVer,sizeof(tempInfo.softwareVer));
	if(KEY_FALSE != ret) {
		snprintf(info->softwareVer, sizeof(tempInfo.softwareVer),"%s",tempInfo.softwareVer);
	}
	ret = cJSON_GetObjectItemValueString(conf,"name",tempInfo.name,sizeof(tempInfo.name));
	if(KEY_FALSE != ret) {
		snprintf(info->name, sizeof(tempInfo.name),"%s",tempInfo.name);
	}
	ret = cJSON_GetObjectItemValueString(conf,"city",tempInfo.city,sizeof(tempInfo.city));
	if(KEY_FALSE != ret) {
		snprintf(info->city, sizeof(tempInfo.city),"%s",tempInfo.city);
	}
	ret = cJSON_GetObjectItemValueString(conf,"country",tempInfo.country,sizeof(tempInfo.country));
	if(KEY_FALSE != ret) {
		snprintf(info->country, sizeof(tempInfo.country),"%s",tempInfo.country);
	}
	ret = cJSON_GetObjectItemValueString(conf,"uuid",tempInfo.uuid,sizeof(tempInfo.uuid));
	if(KEY_FALSE != ret) {
		snprintf(info->uuid, sizeof(tempInfo.uuid),"%s",tempInfo.uuid);
	}
	devType = cJSON_GetObjectItemValueInt(conf,"devType");
	if(KEY_FALSE < devType) {
		info->devType = devType;
	}
	int coverday = cJSON_GetObjectItemValueInt(conf,"coverday");
	if(KEY_FALSE < coverday) {
		info->coverDay = coverday;
	}
	int pullinterval = cJSON_GetObjectItemValueInt(conf,"pullinterval");
	if(KEY_FALSE < pullinterval) {
		info->pullInterval = pullinterval;
	}
	int facethreshold = cJSON_GetObjectItemValueInt(conf,"facethreshold");
	if(KEY_FALSE < facethreshold) {
		info->faceThreshold = facethreshold;
	}
	return KEY_TRUE;
}

/**************
用户管理
****************/
cJSON* JsonMapUsrInfoMake(UsrInfo_T *info,int num)
{
 	return NULL;
}

int JsonMapUsrInfoPares(cJSON *data,UsrInfo_T *info)
{
	int userLevel = -1;
	cJSON_GetObjectItemValueString(data,"username",info->registInfo.usrname,sizeof(info->registInfo.usrname));
	cJSON_GetObjectItemValueString(data,"password",info->registInfo.passWord,sizeof(info->registInfo.passWord));
	cJSON_GetObjectItemValueString(data,"countrycode",info->registInfo.countrycode,sizeof(info->registInfo.countrycode));
	userLevel = cJSON_GetObjectItemValueInt(data,"level");
	if (-1 != userLevel)
	{
		info->registInfo.userLevel = userLevel;
	}
	return KEY_TRUE;
}

int JsonMapUsrPasswdPares(cJSON *data,char *username,char *oldPassword,char *newPassword)
{
	cJSON_GetObjectItemValueString(data,"username", username, 128);
	cJSON_GetObjectItemValueString(data,"oldpassword", oldPassword, 128);
	cJSON_GetObjectItemValueString(data,"newpassword", newPassword, 128);
	return KEY_TRUE;
}

cJSON *JsonMapGetUserInfoMake(UsrInfo_T info)
{
	cJSON *data = cJSON_CreateObject();
	cJSON_AddStringToObject(data,"countrycode", info.registInfo.countrycode);
	cJSON_AddStringToObject(data,"nickname", info.registInfo.customname);
	cJSON_AddStringToObject(data,"username", info.registInfo.usrname);
	return data;
}

int JsonMapCameraInfoMake(cJSON *respData, char *deviceID, DeviceStorageInfo_T *deviceInfo, char *accountName, char *endpointSuffix, bool overduestatus)
{
	if (NULL == respData || NULL == deviceInfo){
		return KEY_FALSE;
	}
	cJSON *data = cJSON_CreateObject();
	cJSON_AddItemToObject(respData,"data",data);
	cJSON_AddStringToObject(data, "cameraid", deviceID);
	cJSON_AddNumberToObject(data, "deviceonline", deviceInfo->deviceInfo.onlineStatus);
	cJSON_AddStringToObject(data, "devicename", deviceInfo->deviceInfo.deviceName);
	cJSON_AddNumberToObject(data, "devicemodel", deviceInfo->deviceInfo.deviceModel);
	return KEY_TRUE;
}

int JsonMapUsrInfoListMake(cJSON *root, int num, UsrInfo_T *info)
{
	int item = 0;
	cJSON  *pJsonArry,*pJsonsub;
	cJSON *pJsonRoot;
	pJsonRoot = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "data", pJsonRoot);
	pJsonArry = cJSON_CreateArray();
	cJSON_AddItemToObject(pJsonRoot, "usernamelist", pJsonArry);
	for (item = 0; item < num; item++)
	{
		pJsonsub = cJSON_CreateObject();
		cJSON_AddItemToArray(pJsonArry, pJsonsub);
		cJSON_AddStringToObject(pJsonsub, "username", info[item].registInfo.usrname);
	}
	return 0;	
}

int JsonMapCameraInfoListMake(cJSON *root, int num, DeviceStorageInfo_T *deviceInfo, char *accountName, char *endpointSuffix)
{
	int item = 0;
	cJSON  *pJsonArry,*pJsonsub;
	cJSON *pJsonRoot;
	pJsonRoot = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "data", pJsonRoot);
	pJsonArry = cJSON_CreateArray();
	cJSON_AddItemToObject(pJsonRoot, "camerainfolist", pJsonArry);
	for (item = 0; item < num; item++)
	{
		pJsonsub = cJSON_CreateObject();
		cJSON_AddItemToArray(pJsonArry, pJsonsub);
		cJSON_AddStringToObject(pJsonsub, "cameraid", deviceInfo[item].deviceInfo.deviceID);
		cJSON_AddNumberToObject(pJsonsub, "devicemodel", deviceInfo[item].deviceInfo.deviceModel);
	}
	return 0;	
}

int JsonMapLoginOutMake(char *msg)
{
	cJSON *pJsonRoot = NULL, *pJsonData = NULL;
	pJsonRoot = cJSON_CreateObject();
	pJsonData = cJSON_CreateObject();
	cJSON_AddStringToObject(pJsonRoot, "cmd", "loginoutmainserver");	
	cJSON_AddStringToObject(pJsonRoot, "method", "response");	
	cJSON_AddNumberToObject(pJsonRoot, "statuscode", 200);
	char *out = cJSON_PrintUnformatted(pJsonRoot);
	sprintf(msg,"%s",out);
	free(out);
	cJSON_Delete(pJsonRoot);
}


/********
时间
*********/
cJSON* JsonMapTimeManageConfigMake(TimeConf_T *info)
{
	if(NULL == info){
		return NULL;
	}
	cJSON *confTime = cJSON_CreateObject();
	//cJSON_AddNumberToObject(confTime, "timeZone", info->timeZone);	//时区
	//cJSON_AddNumberToObject(confTime, "currTime", info->currTime);	//当前时间
	char *zone = GetTimeZoneString(info->timeZone);
	if(zone != NULL)
	{
		cJSON_AddStringToObject(confTime,"timeZone",zone);
	}
	cJSON_AddNumberToObject(confTime, "currTime", info->currTime);
	return confTime;
}

int JsonMapTimeManageConfigPares(cJSON *conf,TimeConf_T *info)
{
	if(NULL == conf || NULL == info){
		return KEY_FALSE;
	}
	char tmp[128] = {0};
	cJSON_GetObjectItemValueString(conf,"timeZone",tmp,sizeof(tmp));
	info->timeZone = GetTimeZoneIndex(tmp);
	info->currTime = cJSON_GetObjectItemValueInt(conf,"currTime");
	return KEY_TRUE;
}

int JsonMapCameraInfoArraryPares(cJSON *data, char deviceId[][128])
{
	int num = 0;
	int i = 0;
	cJSON *item = NULL;
	cJSON *cameraList = cJSON_GetObjectItem(data, "cameralist");
	num = cJSON_GetArraySize(cameraList);
	for (i = 0; i < num; i++)
	{
		item = cJSON_GetArrayItem(cameraList, i);
		if (NULL != item)
		{
			cJSON_GetObjectItemValueString(item, "cameraid", deviceId[i], 128);
		}
	}
	return num;
}

int JsonMapGetDeviceInfoList(cJSON *root, DeviceStorageInfo_T *deviceList, int num)
{
	int item = 0;
	cJSON  *pJsonArry,*pJsonsub;
	cJSON *pJsonRoot;
	pJsonRoot = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "data", pJsonRoot);
	pJsonArry = cJSON_CreateArray();
	cJSON_AddItemToObject(pJsonRoot, "devicelist", pJsonArry);
	for (item = 0; item < num; item++)
	{
		pJsonsub = cJSON_CreateObject();
		cJSON_AddItemToArray(pJsonArry, pJsonsub);
		cJSON_AddStringToObject(pJsonsub, "deviceid", deviceList[item].deviceInfo.deviceID);
		cJSON_AddStringToObject(pJsonsub, "devicename", deviceList[item].deviceInfo.deviceName);
		cJSON_AddStringToObject(pJsonsub, "devicesoftwarever", deviceList[item].deviceInfo.deviceSoftwareVer);	
		cJSON_AddStringToObject(pJsonsub, "deviceipaddr", deviceList[item].deviceInfo.deviceIpaddr); 
		cJSON_AddNumberToObject(pJsonsub, "devicestatus", deviceList[item].deviceInfo.onlineStatus); 
	}
	return KEY_TRUE;
}


//告警配置
cJSON* JsonMapHkDeviceAlarmInfoMake(DeviceAlarmParam_T *info)
{
	if(info == NULL)
	{
		return NULL;
	}
	cJSON *alarm = cJSON_CreateObject();
	cJSON_AddNumberToObject(alarm, "smoke", info->smokeLev);
	cJSON_AddNumberToObject(alarm, "hightemp", info->hightempLev);
	return alarm;
}

int JsonMapDeviceAlarmInfoPares(cJSON *conf, DeviceAlarmParam_T *info)
{
	if(info == NULL || NULL == conf)
	{
		return KEY_FALSE;
	}
	info->smokeLev = cJSON_GetObjectItemValueInt(conf, "smoke");
	info->hightempLev = cJSON_GetObjectItemValueInt(conf, "hightemp");
	return KEY_TRUE;
}

//DVR设备
cJSON* JsonMapDvrInfoMake(HKDVRParam_T * Param)
{
	int item = 0;
	cJSON *confDvr   = cJSON_CreateObject();
	cJSON_AddNumberToObject(confDvr, "channels", Param->HkDvrInfo.wChannel);
	cJSON_AddNumberToObject(confDvr, "port", Param->HkDvrInfo.wPort);
	cJSON_AddStringToObject(confDvr, "Dvrname", Param->HkDvrInfo.sDvrName);
	cJSON_AddStringToObject(confDvr, "IPaddress", Param->HkDvrInfo.sDeviceAddress);
	cJSON_AddStringToObject(confDvr, "username", Param->HkDvrInfo.sUserName);
	cJSON_AddStringToObject(confDvr, "password", Param->HkDvrInfo.sPassword);
	cJSON_AddStringToObject(confDvr, "serialnumber", Param->HkDvrInfo.sDvrSerialNumber);
	cJSON *IPCArray = cJSON_CreateArray();
	cJSON_AddItemToObject(confDvr,"IPClist",IPCArray);
	for (item = 0; item < Param->HkDvrInfo.wChannel; item++){
		cJSON *IPCitem = cJSON_CreateObject();
		cJSON_AddItemToObject(IPCArray,"IPClist",IPCitem);
		cJSON_AddNumberToObject(IPCitem, "queueid",    Param->HkSdkParam[item].sQueueID);
		cJSON_AddNumberToObject(IPCitem, "channel",    Param->HkSdkParam[item].sChannel);
		cJSON_AddNumberToObject(IPCitem, "devicetype", Param->HkSdkParam[item].sDeviceType);
		cJSON_AddStringToObject(IPCitem, "devicename", Param->HkSdkParam[item].sDeviceName);
		cJSON_AddStringToObject(IPCitem, "deviceIP",   Param->HkSdkParam[item].sDeviceAddress);
	}
	return confDvr;
}

cJSON* JsonMapDvrListInfoMake(HKDVRParam_T * Param, int Dvrnum)
{
	int i,j;
	cJSON *confDvr   = cJSON_CreateObject();
	
	cJSON *Dvrlist = cJSON_CreateArray();
	cJSON_AddItemToObject(confDvr,"Dvrlist",Dvrlist);
	for (i = 0; i < Dvrnum; i++){
		cJSON *Dvritem   = cJSON_CreateObject();
		cJSON_AddItemToObject(Dvrlist,"Dvrlist",Dvritem);
		cJSON_AddNumberToObject(Dvritem, "port", Param[i].HkDvrInfo.wPort);
		cJSON_AddNumberToObject(Dvritem, "channels", Param[i].HkDvrInfo.wChannel);
		cJSON_AddStringToObject(Dvritem, "Dvrname", Param[i].HkDvrInfo.sDvrName);
		cJSON_AddStringToObject(Dvritem, "IPaddress", Param[i].HkDvrInfo.sDeviceAddress);
		cJSON_AddStringToObject(Dvritem, "username", Param[i].HkDvrInfo.sUserName);
		cJSON_AddStringToObject(Dvritem, "password", Param[i].HkDvrInfo.sPassword);
		cJSON_AddStringToObject(Dvritem, "serialnumber", Param[i].HkDvrInfo.sDvrSerialNumber);
		
		cJSON *IPCArray = cJSON_CreateArray();
		cJSON_AddItemToObject(Dvritem,"IPClist",IPCArray);
		for (j = 0; j < Param[i].HkDvrInfo.wChannel; j++){
			cJSON *IPCitem = cJSON_CreateObject();
			cJSON_AddItemToObject(IPCArray,"IPClist",IPCitem);
			cJSON_AddNumberToObject(IPCitem, "queueid", Param[i].HkSdkParam[j].sQueueID);
			cJSON_AddNumberToObject(IPCitem, "channel", Param[i].HkSdkParam[j].sChannel);
			cJSON_AddNumberToObject(IPCitem, "devicetype", Param[i].HkSdkParam[j].sDeviceType);
			cJSON_AddStringToObject(IPCitem, "devicename", Param[i].HkSdkParam[j].sDeviceName);
			cJSON_AddStringToObject(IPCitem, "deviceIP", Param[i].HkSdkParam[j].sDeviceAddress);
		}
	}
	
	return confDvr;
}

int JsonMapDvrInfoPares(cJSON * conf,HKDVRParam_T * Param)
{
	if(NULL == conf || NULL == Param){
		return KEY_FALSE;
	}
	int i,j ,IPCnum = 0,Dvrnum = 0;
	cJSON *DvrArray = cJSON_GetObjectItem(conf, "Dvrlist");
	if(DvrArray == NULL){
		DF_DEBUG("DvrArray == NULL");
		return KEY_FALSE;
	}
	Dvrnum = cJSON_GetArraySize(DvrArray);
	cJSON *Dvritem = NULL;
	for(i = 0 ; i < Dvrnum;i++){
		Dvritem = cJSON_GetArrayItem(DvrArray, i);
		if (NULL != Dvritem)
		{
			Param[i].HkDvrInfo.wPort = cJSON_GetObjectItemValueInt(Dvritem,"port");
			Param[i].HkDvrInfo.wChannel = cJSON_GetObjectItemValueInt(Dvritem,"channels");
			cJSON_GetObjectItemValueString(Dvritem,"Dvrname",Param[i].HkDvrInfo.sDvrName,sizeof(Param[i].HkDvrInfo.sDvrName));
			cJSON_GetObjectItemValueString(Dvritem,"IPaddress",Param[i].HkDvrInfo.sDeviceAddress,sizeof(Param[i].HkDvrInfo.sDeviceAddress));
			cJSON_GetObjectItemValueString(Dvritem,"username",Param[i].HkDvrInfo.sUserName,sizeof(Param[i].HkDvrInfo.sUserName));
			cJSON_GetObjectItemValueString(Dvritem,"password",Param[i].HkDvrInfo.sPassword,sizeof(Param[i].HkDvrInfo.sPassword));
			cJSON_GetObjectItemValueString(Dvritem,"serialnumber",Param[i].HkDvrInfo.sDvrSerialNumber,sizeof(Param[i].HkDvrInfo.sDvrSerialNumber));
			cJSON *IPCArray = cJSON_GetObjectItem(Dvritem, "IPClist");
			if(IPCArray == NULL){
				continue;
			}
			IPCnum = cJSON_GetArraySize(IPCArray);
			cJSON *IPCitem = NULL;
			for(j = 0 ; j < IPCnum;j++){
				IPCitem = cJSON_GetArrayItem(IPCArray, j);
				if (NULL != IPCitem)
				{
					Param[i].HkSdkParam[j].sQueueID = cJSON_GetObjectItemValueInt(IPCitem,"queueid");
					Param[i].HkSdkParam[j].sChannel = cJSON_GetObjectItemValueInt(IPCitem,"channel");
					Param[i].HkSdkParam[j].sDeviceType =  cJSON_GetObjectItemValueInt(IPCitem,"devicetype");
					cJSON_GetObjectItemValueString(IPCitem,"devicename",Param[i].HkSdkParam[j].sDeviceName,sizeof(Param[i].HkSdkParam[j].sDeviceName));
					cJSON_GetObjectItemValueString(IPCitem,"deviceIP",Param[i].HkSdkParam[j].sDeviceAddress,sizeof(Param[i].HkSdkParam[j].sDeviceAddress));
				}
			}
		}
	}
	return KEY_TRUE;
}

//告警
int JsonMapGetIntelligentAlarm(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend, int *page, int *num, int *type)
{
	if(NULL == data || NULL == page || NULL == num || NULL == type || NULL == timestart || NULL == timeend)
	{
		return KEY_FALSE;
	}
	
	char timebuf[128] = {0};
	memset(&timebuf, 0, 128);
	cJSON_GetObjectItemValueString(data, "starttime", timebuf, 128);
	if(strlen(timebuf) > 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", &timestart->year, &timestart->month, &timestart->day, &timestart->hour, &timestart->min, &timestart->sec);
	}
	else
	{
		timestart->year = 1970;
		timestart->month= 1;
		timestart->day  = 1;
		timestart->hour = 0;
		timestart->min  = 0;
		timestart->sec  = 0;
	}

	memset(&timebuf, 0, 128);
	cJSON_GetObjectItemValueString(data, "endtime", timebuf, 128);
	if(strlen(timebuf) > 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", &timeend->year, &timeend->month, &timeend->day, &timeend->hour, &timeend->min, &timeend->sec);
	}
	else
	{
		Time_T tv = {0};
		GetLocalTime(&tv);
		memcpy(timeend, &tv.curTime, sizeof(TimeYMD_T));
	}
	*page = cJSON_GetObjectItemValueInt(data, "page");
	*num  = cJSON_GetObjectItemValueInt(data, "num");
	*type = cJSON_GetObjectItemValueInt(data, "type");
	return KEY_TRUE;
}

cJSON *JsonMapGetIntelligentAlarmMake(DeviceAlarmInfo_T    *alarmList, int number, int total)
{
	if(NULL == alarmList)
	{
		return NULL;
	}
	int i = 0;
	cJSON *alarm = cJSON_CreateObject();
	cJSON_AddNumberToObject(alarm, "total",  total);
	
	cJSON *alarmlist = cJSON_CreateArray();
	cJSON_AddItemToObject(alarm,"alarmlist", alarmlist);
	for(i = 0; i < number; i++)
	{
		cJSON *alarmitem = cJSON_CreateObject();
		cJSON_AddItemToObject(alarmlist,"alarmlist",alarmitem);
		cJSON_AddNumberToObject(alarmitem, "type",alarmList[i].alrmtype);
		cJSON_AddNumberToObject(alarmitem, "leave",alarmList[i].alarmlev);
		cJSON_AddStringToObject(alarmitem, "time",alarmList[i].time);
		cJSON_AddStringToObject(alarmitem, "place",alarmList[i].alarmdata);
	}
	return alarm;
}

int JsonMapGetAlarmProportionAnalysis(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend)
{
	if(NULL == timestart || NULL == timeend || data == NULL)
	{
		return KEY_FALSE;
	}

	char time[64] = {0};
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "starttime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timestart->year, &timestart->month, &timestart->day);
	timestart->hour = 0;
	timestart->min  = 0;
	timestart->sec  = 0;

	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timeend->year, &timeend->month, &timeend->day);
	timeend->hour = 23;
	timeend->min  = 59;
	timeend->sec  = 59;
	return KEY_TRUE;
}

cJSON *JsonMapGetAlarmProportionAnalysisMake(DeviceAlarmCount_T *AlarmCount)
{
	if(NULL == AlarmCount)
	{
		return NULL;
	}
	
	cJSON *alarm = cJSON_CreateObject();
	cJSON_AddNumberToObject(alarm, "smokecount",AlarmCount->smokecount);
	cJSON_AddNumberToObject(alarm, "hightemcount",AlarmCount->hightemcount);
	return alarm;
}

int JsonMapGetAlarmPeriodAnalysis(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend, int *count)
{
	if(NULL == timestart || NULL == timeend || NULL == data || NULL == count)
	{
		return KEY_FALSE;
	}

	char time[64] = {0};
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "starttime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timestart->year, &timestart->month, &timestart->day);
	

	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timeend->year, &timeend->month, &timeend->day);

	*count = cJSON_GetObjectItemValueInt(data, "number");
	return KEY_TRUE;
}

cJSON *JsonMapGetAlarmPeriodAnalysisMake(int count, DeviceAlarmCount_T *AlarmCount)
{
	if(NULL == AlarmCount){
		return NULL;
	}
	
	int i = 0;
	cJSON *alarm = cJSON_CreateObject();
	cJSON *alarmlist = cJSON_CreateArray();
	cJSON_AddItemToObject(alarm,"alarmlist", alarmlist);
	for(i = 0; i < count; i++)
	{
		cJSON *alarmitem = cJSON_CreateObject();
		cJSON_AddItemToArray(alarmlist, alarmitem);
		cJSON_AddNumberToObject(alarmitem, "smokecount",AlarmCount[i].smokecount);
		cJSON_AddNumberToObject(alarmitem, "hightemcount",AlarmCount[i].hightemcount);
	}
	return alarm;
}

//超脑设备
int JsonMapsdkDeviceAddInfoPares(cJSON *conf, HKDVRParam_T *Param)
{
	if(NULL == conf || NULL == Param){
		return KEY_FALSE;
	}
	
	Param->HkDvrInfo.wPort = cJSON_GetObjectItemValueInt(conf, "port");
	cJSON_GetObjectItemValueString(conf, "dvrname", Param->HkDvrInfo.sDvrName, NET_DVR_MAX_LEN);
	cJSON_GetObjectItemValueString(conf, "username", Param->HkDvrInfo.sUserName, NET_DVR_MAX_LEN);
	cJSON_GetObjectItemValueString(conf, "password", Param->HkDvrInfo.sPassword, NET_DVR_MAX_LEN);
	cJSON_GetObjectItemValueString(conf, "ipaddress", Param->HkDvrInfo.sDeviceAddress, NET_DVR_MAX_LEN);
	return KEY_TRUE;
}

int JsonMapsdkDeviceDelInfoPares(cJSON *conf, HKDVRParam_T *Param)
{
	if(NULL == conf || NULL == Param){
		return KEY_FALSE;
	}
	
	cJSON_GetObjectItemValueString(conf, "sdvrserialnumber", Param->HkDvrInfo.sDvrSerialNumber, NET_DVR_MAX_LEN);
	return KEY_TRUE;
}

cJSON* JsonMapGetsdkDeviceList(HKDVRParam_T *dvrParam, int num)
{
	if(NULL == dvrParam || num <= 0){
		return NULL;
	}

	int i = 0, j = 0;
	cJSON *confDvr = cJSON_CreateObject();
	cJSON *Dvrlist = cJSON_CreateArray();
	cJSON_AddItemToObject(confDvr,"Dvrlist",Dvrlist);
	for (i = 0; i < num; i++){
		cJSON *Dvritem   = cJSON_CreateObject();
		cJSON_AddItemToObject(Dvrlist,"Dvrlist",	 	 Dvritem);
		cJSON_AddNumberToObject(Dvritem, "port",	 	 dvrParam[i].HkDvrInfo.wPort);
		cJSON_AddNumberToObject(Dvritem, "channels", 	 dvrParam[i].HkDvrInfo.wChannel);
		cJSON_AddStringToObject(Dvritem, "Dvrname",	 	 dvrParam[i].HkDvrInfo.sDvrName);
		cJSON_AddStringToObject(Dvritem, "IPaddress",	 dvrParam[i].HkDvrInfo.sDeviceAddress);
		cJSON_AddStringToObject(Dvritem, "username", 	 dvrParam[i].HkDvrInfo.sUserName);
		cJSON_AddStringToObject(Dvritem, "password", 	 dvrParam[i].HkDvrInfo.sPassword);
		cJSON_AddStringToObject(Dvritem, "serialnumber", dvrParam[i].HkDvrInfo.sDvrSerialNumber);
		
		cJSON *IPCArray = cJSON_CreateArray();
		cJSON_AddItemToObject(Dvritem,"IPClist",IPCArray);
		for (j = 0; j < dvrParam[i].HkDvrInfo.wChannel; j++){
			if(strlen(dvrParam[i].HkSdkParam[j].sDeviceAddress) > 0)
			{
				cJSON *IPCitem = cJSON_CreateObject();
				cJSON_AddItemToObject(IPCArray,"IPClist",IPCitem);
				cJSON_AddNumberToObject(IPCitem, "online",     dvrParam[i].HkSdkParam[j].sOnline);
				cJSON_AddNumberToObject(IPCitem, "channel",    dvrParam[i].HkSdkParam[j].sChannel);
				cJSON_AddNumberToObject(IPCitem, "devicetype", dvrParam[i].HkSdkParam[j].sDeviceType);
				cJSON_AddStringToObject(IPCitem, "devicename", dvrParam[i].HkSdkParam[j].sDeviceName);
				cJSON_AddStringToObject(IPCitem, "deviceIP",   dvrParam[i].HkSdkParam[j].sDeviceAddress);
			}
		}
	}
	
	return confDvr;
}

int JsonMapsdkDeviceSetInfoPares(cJSON *conf, DvrParam_T *Info)
{
	if(NULL == conf || NULL == Info){
		return KEY_FALSE;
	}

	cJSON_GetObjectItemValueString(conf, "dvrname",  Info->name, NET_DVR_MAX_LEN);	
	cJSON_GetObjectItemValueString(conf, "serialnumber",  Info->serialnumber, NET_DVR_MAX_LEN);
	return KEY_TRUE;
}

int JsonMapsdkIpcamSetInfoPares(cJSON *conf, IpcamParam_T *Info)
{
	if(NULL == conf || NULL == Info){
		return KEY_FALSE;
	}
	
	Info->channel = cJSON_GetObjectItemValueInt(conf, "channel");
	Info->devicetype = cJSON_GetObjectItemValueInt(conf, "devicetype");
	cJSON_GetObjectItemValueString(conf, "devicename",  Info->name, NET_DVR_MAX_LEN);	
	cJSON_GetObjectItemValueString(conf, "serialnumber",  Info->serialnumber, NET_DVR_MAX_LEN);
	return KEY_TRUE;
}

cJSON *JsonMapsdkDeviceAlarmGetMake(DeviceAlarmParam_T *alarmParam)
{
	if(NULL == alarmParam)
	{
		return NULL;
	}
	
	cJSON *alarm = cJSON_CreateObject();
	cJSON_AddNumberToObject(alarm, "smoke",  alarmParam->smokeLev);
	cJSON_AddNumberToObject(alarm, "hightemp",  alarmParam->hightempLev);
	return alarm;
}

int JsonMapsdkDeviceAlarmSetPares(cJSON *data, DeviceAlarmParam_T *alarmParam)
{
	if(NULL == data || NULL == alarmParam)
	{
		return KEY_FALSE;
	}
	alarmParam->smokeLev = cJSON_GetObjectItemValueInt(data, "smoke");
	alarmParam->hightempLev = cJSON_GetObjectItemValueInt(data, "hightemp");
	return KEY_TRUE;
}

int JsonMapGetEntranceVehiclesCurList(cJSON *data, int *num)
{
	if(NULL == data || NULL == num)
	{
		return KEY_FALSE;
	}
	*num = cJSON_GetObjectItemValueInt(data, "num");
	return KEY_TRUE;
}

cJSON *JsonMapGetEntranceVehiclesCurListMake(int num, StationVehiclesInfo_T *VehiclesInfo)
{
	if(num <= 0 || NULL == VehiclesInfo)
	{
		return NULL;
	}
	
	int i= 0;
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON *Vehicleslist = cJSON_CreateArray();
	cJSON_AddItemToObject(Vehicles,"vehicleslist",Vehicleslist);
	for(i = 0; i < num; i++)
	{
		cJSON *Vehiclesitem = cJSON_CreateObject();
		cJSON_AddItemToObject(Vehicleslist,"vehicleslist",Vehiclesitem);
		cJSON_AddStringToObject(Vehiclesitem, "plateno",  VehiclesInfo[i].plateNo);
		cJSON_AddStringToObject(Vehiclesitem, "cartype",  VehiclesInfo[i].carType);
		char time[64] = {0};
		memset(time, 0, 64);
		snprintf(time, 64, "%04d-%02d-%02d %02d:%02d:%02d", VehiclesInfo[i].time.year, VehiclesInfo[i].time.month,
			VehiclesInfo[i].time.day, VehiclesInfo[i].time.hour, VehiclesInfo[i].time.min, VehiclesInfo[i].time.sec);
		cJSON_AddStringToObject(Vehiclesitem, "time",  time);
		cJSON_AddNumberToObject(Vehiclesitem, "refueno",  VehiclesInfo[i].refuelNo);
	}
	return Vehicles;
}

int JsonMapGetFrontageVehiclesSumPares(cJSON *data, TimeYMD_T *startTime, TimeYMD_T *endTime)
{
	if(NULL == data || NULL == startTime || NULL == endTime)
	{
		return KEY_FALSE;
	}

	char timebuf[128] = {0};
	cJSON_GetObjectItemValueString(data, "starttime",  timebuf, 128);
	sscanf(timebuf, "%04d-%02d-%02d", &startTime->year, &startTime->month, &startTime->day);
	startTime->hour = 0;
	startTime->min	= 0;
	startTime->sec	= 0;

	memset(timebuf, 0, 128);
	cJSON_GetObjectItemValueString(data, "endtime",  timebuf, 128);
	sscanf(timebuf, "%04d-%02d-%02d", &endTime->year, &endTime->month, &endTime->day);
	endTime->hour = 23;
	endTime->min  = 59;
	endTime->sec  = 59;
	return KEY_TRUE;
}

cJSON *JsonMapGetFrontageVehiclesSumMake(StreetTypeAttr *streetAttr)
{
	if(NULL == streetAttr)
	{
		return NULL;
	}
	
	cJSON *street = cJSON_CreateObject();
	cJSON_AddNumberToObject(street, "vancount",  streetAttr->vehickeType.VanCount);
	cJSON_AddNumberToObject(street, "buscount",  streetAttr->vehickeType.BusCount);
	cJSON_AddNumberToObject(street, "tructcount",streetAttr->vehickeType.TruckCount);
	cJSON_AddNumberToObject(street, "carcount",  streetAttr->vehickeType.CarCount);
	cJSON_AddNumberToObject(street, "othercount",streetAttr->vehickeType.OtherCount);
	
	cJSON_AddNumberToObject(street, "bluecount",   streetAttr->plateType.BlueCount);
	cJSON_AddNumberToObject(street, "yellowcount", streetAttr->plateType.YellowCount);
	cJSON_AddNumberToObject(street, "greecount",   streetAttr->plateType.GreenCount);
	cJSON_AddNumberToObject(street, "othercount",  streetAttr->plateType.OtherCount);
	return street;
}

int JsonMapGetEntranceVehiclesSumPares(cJSON *data, TimeYMD_T *startTime, TimeYMD_T *endTime)
{
	if(NULL == data || NULL == startTime || NULL == endTime)
	{
		return KEY_FALSE;
	}

	char timebuf[128] = {0};
	cJSON_GetObjectItemValueString(data, "starttime",  timebuf, 128);
	sscanf(timebuf, "%04d-%02d-%02d", &startTime->year, &startTime->month, &startTime->day);
	startTime->hour = 0; 
	startTime->min  = 0;
	startTime->sec  = 0;

	memset(timebuf, 0, 128);
	cJSON_GetObjectItemValueString(data, "endtime",  timebuf, 128);
	sscanf(timebuf, "%04d-%02d-%02d", &endTime->year, &endTime->month, &endTime->day);
	endTime->hour = 23;
	endTime->min  = 59;
	endTime->sec  = 59;
	return KEY_TRUE;
}

cJSON *JsonMapGetEntranceVehiclesSumMake(ProgressRate_T *progressAttr)
{
	if(NULL == progressAttr)
	{
		return NULL;
	}
	
	cJSON *progress = cJSON_CreateObject();
	cJSON_AddNumberToObject(progress, "passingcount",  progressAttr->PassingCount);
	cJSON_AddNumberToObject(progress, "incominfcount", progressAttr->IncomingCount);
	return progress;
}

int JsonMapGetEntranceVehiclesListPares(cJSON *data, StationVehicles_T *Vehicles, int *page, int *num)
{
	if(NULL == data || NULL == Vehicles)
	{
		return KEY_FALSE;
	}
	
	char timebuf[128] = {0};
	*page = cJSON_GetObjectItemValueInt(data, "page");
	*num  = cJSON_GetObjectItemValueInt(data, "num");
	Vehicles->situationType = (VechicleSituation_E)cJSON_GetObjectItemValueInt(data, "situationtype");
	Vehicles->vehicleType = (Vehicle_Type_E)cJSON_GetObjectItemValueInt(data, "vehicletype");
	cJSON_GetObjectItemValueString(data, "fuzzystr",  Vehicles->fuzzyStr, 128);
	cJSON_GetObjectItemValueString(data, "starttime", timebuf, 128);
	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", &Vehicles->vehicleStart.year, &Vehicles->vehicleStart.month, &Vehicles->vehicleStart.day, \
			&Vehicles->vehicleStart.hour, &Vehicles->vehicleStart.min, &Vehicles->vehicleStart.sec);
	}
	else
	{
		Vehicles->vehicleStart.year = 1970;
		Vehicles->vehicleStart.month = 1;
		Vehicles->vehicleStart.day = 1;
	}
	
	memset(timebuf, 0, 128);
	cJSON_GetObjectItemValueString(data, "endtime", timebuf, 128);
	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", &Vehicles->vehicleEnd.year, &Vehicles->vehicleEnd.month, &Vehicles->vehicleEnd.day, \
			&Vehicles->vehicleEnd.hour, &Vehicles->vehicleEnd.min, &Vehicles->vehicleEnd.sec);
	}
	else
	{
		Time_T tv = {0};
		GetLocalTime(&tv);
		memcpy(&Vehicles->vehicleEnd, &tv.curTime, sizeof(TimeYMD_T));
	}
	return KEY_TRUE;
}

cJSON *JsonMapGetEntranceVehiclesListMake(StationVehiclesInfo_T *VehiclesInfo, int num, int total)
{
	if(NULL == VehiclesInfo)
	{
		return NULL;
	}

	int i = 0;
	char time[128] = {0};
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "total",  total);
	cJSON *Vehicleslist = cJSON_CreateArray();
	cJSON_AddItemToObject(Vehicles,"devicelist",Vehicleslist);
	for(i = 0; i < num; i++)
	{
		cJSON *IPCitem = cJSON_CreateObject();
		cJSON_AddItemToObject(Vehicleslist,"devicelist",IPCitem);
		cJSON_AddNumberToObject(IPCitem, "refueno",	 VehiclesInfo[i].refuelNo);
		cJSON_AddStringToObject(IPCitem, "plateno",  VehiclesInfo[i].plateNo);
		cJSON_AddStringToObject(IPCitem, "picpath",  VehiclesInfo[i].picPath);
		cJSON_AddStringToObject(IPCitem, "territory",VehiclesInfo[i].territory);
		cJSON_AddStringToObject(IPCitem, "plateclor",VehiclesInfo[i].plateClor);
		cJSON_AddStringToObject(IPCitem, "cartype",  VehiclesInfo[i].carType);

		sprintf(time, "%04d-%02d-%02d %02d:%02d", VehiclesInfo[i].time.year, VehiclesInfo[i].time.month, \
			VehiclesInfo[i].time.day, VehiclesInfo[i].time.hour, VehiclesInfo[i].time.min);
		cJSON_AddStringToObject(IPCitem, "time", time);
	}
	return Vehicles;
}

int JsonMapGetEntranceHistoryListPares(cJSON *data, int *page, int *num, char *location, char *plateNo)
{
	if(NULL == data || NULL == location )
	{
		return KEY_FALSE;
	}

	*page = cJSON_GetObjectItemValueInt(data, "page");
	*num  = cJSON_GetObjectItemValueInt(data, "num");
	cJSON_GetObjectItemValueString(data, "location",  location, 128);
	cJSON_GetObjectItemValueString(data, "plateno",  plateNo, 128);
	return KEY_TRUE;
}

cJSON *JsonMapGetEntranceHistoryListMake(StationHistoryInfo_T *historyInfo, int num, int total)
{
	if(NULL == historyInfo || num <= 0)
	{
		return NULL;
	}
	
	int i = 0;
	char time[128] = {0};
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "total", total);
	cJSON *Vehicleslist = cJSON_CreateArray();
	cJSON_AddItemToObject(Vehicles,"devicelist",Vehicleslist);
	for(i = 0; i < num; i++)
	{
		cJSON *IPCitem = cJSON_CreateObject();
		cJSON_AddItemToObject(Vehicleslist,"devicelist",IPCitem);
		cJSON_AddStringToObject(IPCitem, "plateno",   historyInfo[i].plateNo);
		cJSON_AddStringToObject(IPCitem, "territory", historyInfo[i].territory);
		cJSON_AddStringToObject(IPCitem, "plateclor", historyInfo[i].plateClor);
		cJSON_AddStringToObject(IPCitem, "cartype",   historyInfo[i].carType);

		sprintf(time, "%04d-%02d-%02d %02d:%02d", historyInfo[i].time.year, historyInfo[i].time.month, \
			historyInfo[i].time.day, historyInfo[i].time.hour, historyInfo[i].time.min);
		cJSON_AddStringToObject(IPCitem, "time", time);
	}
	return Vehicles;
}

int JsonMapPushDeviceEntrancesMake(char *msg, DeviceEntrance_T *info, char *cartype, int count)
{
	if(NULL == msg || NULL == info || NULL == cartype)
	{
		return KEY_FALSE;
	}
	
	char *out = NULL;
	char time[64] = {0};
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddStringToObject(Vehicles, "cmd", "pushdeviceentrances");
	
	cJSON *data = cJSON_CreateObject();
	cJSON_AddItemToObject(Vehicles,"data",data);
	cJSON_AddStringToObject(data, "plateno", info->plateNo);
	cJSON_AddStringToObject(data, "cartype", cartype);
	snprintf(time, 64, "%04d-%02d-%02d %02d:%02d:%02d", info->vehicleStart.year, info->vehicleStart.month, info->vehicleStart.day, 
		info->vehicleStart.hour, info->vehicleStart.min, info->vehicleStart.sec);
	cJSON_AddStringToObject(data, "time",  time);
	cJSON_AddNumberToObject(data, "refueno", count);
	out = cJSON_Print(Vehicles);
	memcpy(msg, out, strlen(out));
	free(out);
	return KEY_TRUE;
}

int JsonMapPushDeviceRateProgressMake(char *msg, int FrontageTotal, int EntranceTotal)
{
	if(NULL == msg)
	{
		return KEY_FALSE;
	}

	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddStringToObject(Vehicles, "cmd", "getentrancevehiclessum");

	cJSON *data = cJSON_CreateObject();
	cJSON_AddItemToObject(Vehicles,"data",data);
	cJSON_AddNumberToObject(data, "passingcount",  FrontageTotal);
	cJSON_AddNumberToObject(data, "incominfcount", EntranceTotal);
	
	char *out = cJSON_Print(Vehicles);
	memcpy(msg, out, strlen(out));
	free(out);
	return KEY_TRUE;
}

int JsonMapPushAlarmMake(char *msg, DeviceAlarm_T	*deviceAlarm)
{
	if(NULL == msg || NULL == deviceAlarm)
	{
		return KEY_FALSE;
	}
	cJSON *alarm = cJSON_CreateObject();
	cJSON_AddStringToObject(alarm, "cmd", "pushalarmmessage");
	
	cJSON *data = cJSON_CreateObject();
	cJSON_AddItemToObject(alarm,"data",data);
	cJSON_AddNumberToObject(data, "alarmtype", deviceAlarm->alarmType);
	char time[64] = {0};
	snprintf(time, 64, "%04d-%02d-%02d %02d:%02d:%02d", deviceAlarm->time.year, deviceAlarm->time.month,
		deviceAlarm->time.day, deviceAlarm->time.hour, deviceAlarm->time.min, deviceAlarm->time.sec);
	cJSON_AddStringToObject(data, "time",  time);
	cJSON_AddStringToObject(data, "place", deviceAlarm->alarmPlace);
	
	char *out = cJSON_Print(alarm);
	memcpy(msg, out, strlen(out));
	free(out);
	return KEY_TRUE;
}

int JsonMapGetEntranceServiceAnalysis(cJSON *data, TimeYMD_T *time)
{
	if(NULL == data || NULL == time)
	{
		return KEY_FALSE;
	}

	char date[64] = {0};
	cJSON_GetObjectItemValueString(data, "time",  date, 64);
	sscanf(date, "%04d-%02d-%02d", &time->year, &time->month, &time->day);
	
	return KEY_TRUE;
}

cJSON *JsonMapEntranceServiceAnalysisMake(int *data, int days)
{
	if(NULL == data)
	{
		return NULL;
	}
	
	int i = 0;
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON *Vehicleslist = cJSON_CreateArray();
	cJSON_AddItemToObject(Vehicles,"vehiclecount",Vehicleslist);
	for(i = 0; i < days; i++)
	{
		cJSON_AddItemToArray(Vehicleslist,cJSON_CreateNumber(data[i]));
	}

	return Vehicles;
}

int JsonMapGetEntrancePlateAnalysis(cJSON *data, TimeYMD_T *time1, TimeYMD_T *time2, char *location)
{
	if(NULL == data || NULL == time1 || NULL == time2 || NULL == location)
	{
		return KEY_FALSE;
	}

	char date[64] = {0};
	cJSON_GetObjectItemValueString(data, "starttime", date, 64);
	if(strlen(date) > 0)
	{
		sscanf(date, "%04d-%02d-%02d", &time1->year, &time1->month, &time1->day);
		time1->hour = 0;
		time1->min  = 0;
		time1->sec  = 0;
	}
	else
	{
		time1->year = 1970;
		time1->month= 1;
		time1->day  = 1;
		time1->hour = 0;
		time1->min  = 0;
		time1->sec  = 0;
	}

	memset(date, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", date, 64);
	if(strlen(date) > 0)
	{
		sscanf(date, "%04d-%02d-%02d", &time2->year, &time2->month, &time2->day);
		time2->hour = 23;
		time2->min  = 59;
		time2->sec  = 59;
	}
	else
	{
		Time_T tv = {0};
		GetLocalTime(&tv);
		memcpy(time2, &tv.curTime, sizeof(TimeYMD_T));
	}
	cJSON_GetObjectItemValueString(data, "location", location, 64);
	return KEY_TRUE;
}

cJSON *JsonMapEntrancePlateAnalysisMake(PlaceAttribution_T PlaceAttr)
{
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "Province", PlaceAttr.ProvinceCount);
	cJSON_AddNumberToObject(Vehicles, "OutProvince", PlaceAttr.OutProvinceCount);
	return Vehicles;
}

int JsonMapGetEntranceHistoryCount(cJSON *data, TimeYMD_T *time1, TimeYMD_T *time2, char *plateNo)
{
	if(NULL == data || NULL == time1 || NULL == time2)
	{
		return KEY_FALSE;
	}
	
	char date[64] = {0};
	cJSON_GetObjectItemValueString(data, "starttime", date, 64);
	sscanf(date, "%04d-%02d-%02d", &time1->year, &time1->month, &time1->day);
	time1->hour = 0;
	time1->min  = 0;
	time1->sec  = 0;
	
	memset(date, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", date, 64);
	sscanf(date, "%04d-%02d-%02d", &time2->year, &time2->month, &time2->day);
	time2->hour = 23;
	time2->min  = 59;
	time2->sec  = 59;

	cJSON_GetObjectItemValueString(data, "plateno", plateNo, 64);
	return KEY_TRUE;
}

cJSON *JsonMapEntranceHistoryCountMake(int *data)
{
	if(NULL == data)
	{
		return NULL;
	}
	
	int i = 0;
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON *Vehicleslist = cJSON_CreateArray();
	cJSON_AddItemToObject(Vehicles,"vehiclecount",Vehicleslist);
	for(i = 0; i < 12; i++)
	{
		cJSON_AddItemToArray(Vehicleslist,cJSON_CreateNumber(data[i]));
	}
	return Vehicles;
}

int JsonMapRefuelingTypeAnalysis(cJSON *data, TimeYMD_T *time1, TimeYMD_T *time2)
{
	if(NULL == data || NULL == time1 || NULL == time2)
	{
		return KEY_FALSE;
	}
	
	char date[64] = {0};
	cJSON_GetObjectItemValueString(data, "starttime", date, 64);
	sscanf(date, "%04d-%02d-%02d", &time1->year, &time1->month, &time1->day);
	time1->hour = 0;
	time1->min  = 0;
	time1->sec  = 0;
	
	memset(date, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", date, 64);
	sscanf(date, "%04d-%02d-%02d", &time2->year, &time2->month, &time2->day);
	time2->hour = 23;
	time2->min  = 59;
	time2->sec  = 59;
	
	return KEY_TRUE;
}

cJSON *JsonMapRefuelingTypeAnalysisMake(RefuelingType_T *Refueling)
{
	if(NULL == Refueling)
	{
		return NULL;
	}

	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "gasoline", Refueling->GasolineCount);
	cJSON_AddNumberToObject(Vehicles, "dieseloil", Refueling->DieselOilCount);
	return Vehicles;
}

int JsonMapOutboundVehiclesAnalysis(cJSON *data, TimeYMD_T *time1start, TimeYMD_T *time1end, TimeYMD_T *time2start, TimeYMD_T *time2end, int *days)
{
	if(NULL == data || NULL == time1start || NULL == time1end || NULL == time2start || NULL == time2end)
	{
		return KEY_FALSE;
	}
	char time[64] = {0};

	*days = cJSON_GetObjectItemValueInt(data, "number");
		
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "starttime1", time, 64);
	sscanf(time, "%04d-%02d-%02d", &time1start->year, &time1start->month, &time1start->day);

	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime1", time, 64);
	sscanf(time, "%04d-%02d-%02d", &time1end->year, &time1end->month, &time1end->day);

	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "starttime2", time, 64);
	sscanf(time, "%04d-%02d-%02d", &time2start->year, &time2start->month, &time2start->day);

	
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime2", time, 64);
	sscanf(time, "%04d-%02d-%02d", &time2end->year, &time2end->month, &time2end->day);
	
	return KEY_TRUE;
}

cJSON *JsonMapOutboundVehiclesAnalysisMake(int count, ProgressRate_T *Progress1, ProgressRate_T *Progress2, ProgressRate_T *Progress1total, ProgressRate_T *Progress2total)
{
	if(NULL == Progress1 || NULL == Progress2)
	{
		return NULL;
	}
	
	int i = 0;
	cJSON *pJsonsub = NULL;
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "pass1total", Progress1total->PassingCount);
	cJSON_AddNumberToObject(Vehicles, "int1total", Progress1total->IncomingCount);
	cJSON_AddNumberToObject(Vehicles, "pass2total", Progress2total->PassingCount);
	cJSON_AddNumberToObject(Vehicles, "int2total", Progress2total->IncomingCount);

	cJSON *Vehicleslist = cJSON_CreateArray();
	cJSON_AddItemToObject(Vehicles,"vehiclecount",Vehicleslist);
	for(i = 0; i < count; i++)
	{
		pJsonsub = cJSON_CreateObject();
		cJSON_AddItemToArray(Vehicleslist, pJsonsub);
		cJSON_AddNumberToObject(pJsonsub, "passingcount1", Progress1[i].PassingCount);
		cJSON_AddNumberToObject(pJsonsub, "incomingcount1", Progress1[i].IncomingCount);
		cJSON_AddNumberToObject(pJsonsub, "passingcount2", Progress2[i].PassingCount);
		cJSON_AddNumberToObject(pJsonsub, "incomingcount2", Progress2[i].IncomingCount);
	}
	return Vehicles;
}

int JsonMapResidenceTimeAnalysis(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend)
{
	if(NULL == data || NULL == timestart || NULL == timeend)
	{
		return KEY_FALSE;
	}

	char time[64] = {0};
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "starttime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timestart->year, &timestart->month, &timestart->day);
	timestart->hour = 0;
	timestart->min  = 0;
	timestart->sec  = 0;

	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timeend->year, &timeend->month, &timeend->day);
	timeend->hour  = 23;
	timeend->min   = 59;
	timeend->sec   = 59;
	
	return KEY_TRUE;
}


cJSON *JsonMapResidenceTimeAnalysisMake(ResidenceTime_T *Residence)
{
	if(NULL == Residence)
	{
		return NULL;
	}
	
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "count1", Residence->count1);
	cJSON_AddNumberToObject(Vehicles, "count2", Residence->count2);
	cJSON_AddNumberToObject(Vehicles, "count3", Residence->count3);
	return Vehicles;
}

int JsonMapAbnormalVehicleList(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend, int *page, int *num)
{
	if(NULL == data || NULL == timestart || NULL == timeend)
	{
		return KEY_FALSE;
	}

	char time[64] = {0};
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "starttime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timestart->year, &timestart->month, &timestart->day);
	timestart->hour = 0;
	timestart->min  = 0;
	timestart->sec  = 0;

	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timeend->year, &timeend->month, &timeend->day);
	timeend->hour  = 23;
	timeend->min   = 59;
	timeend->sec   = 59;

	*page = cJSON_GetObjectItemValueInt(data, "page");
	*num  = cJSON_GetObjectItemValueInt(data, "num");
	return KEY_TRUE;
}

cJSON *JsonMapAbnormalVehicleListMake(AbnormalVehicleInfo_T *Abnormal, int number, int total)
{
	if(NULL == Abnormal || total <= 0)
	{
		return NULL;
	}
	DF_DEBUG("number: %d	total: %d", number, total);
	int i = 0;
	char date[64] = {0};
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "total",  total);
	cJSON *Vehicleslist = cJSON_CreateArray();
	cJSON_AddItemToObject(Vehicles,"vehiclelist",Vehicleslist);
	for(i = 0; i < number; i++)
	{
		cJSON *IPCitem = cJSON_CreateObject();
		cJSON_AddItemToObject(Vehicleslist,"vehiclelist",IPCitem);
		cJSON_AddNumberToObject(IPCitem, "staytime", Abnormal[i].staytime);
		cJSON_AddStringToObject(IPCitem, "plateno",  Abnormal[i].plateNo);
		cJSON_AddStringToObject(IPCitem, "territory",Abnormal[i].territory);
		cJSON_AddStringToObject(IPCitem, "cartype",  Abnormal[i].carType);
		memset(&date, 0, 64);
		snprintf(date, 64, "%04d-%02d-%02d %02d:%02d:%02d", Abnormal[i].time.year, Abnormal[i].time.month,
			Abnormal[i].time.day, Abnormal[i].time.hour, Abnormal[i].time.min, Abnormal[i].time.sec);
		cJSON_AddStringToObject(IPCitem, "time", date);
		DF_DEBUG("date: %s  %s", date,  Abnormal[i].plateNo);
	}
	return Vehicles;
}

int JsonMapRefuelingEfficiencyAnalysis(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend, int *days)
{
	if(NULL == data || NULL == timestart || NULL == timeend)
	{
		return KEY_FALSE;
	}
	
	*days = cJSON_GetObjectItemValueInt(data, "number");
	
	char time[64] = {0};
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "starttime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timestart->year, &timestart->month, &timestart->day);

	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timeend->year, &timeend->month, &timeend->day);
	
	return KEY_TRUE;
}

cJSON *JsonMapRefuelingEfficiencyAnalysisMake(Refueling_T *Residence, int count)
{
	if(NULL == Residence || count <= 0)
	{
		return NULL;
	}
	
	int i = 0;
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON *Vehicleslist = cJSON_CreateArray();
	cJSON_AddItemToObject(Vehicles,"vehiclecount",Vehicleslist);
	for(i = 0; i < count; i++)
	{
		cJSON *pJsonsub = cJSON_CreateObject();
		cJSON_AddItemToArray(Vehicleslist, pJsonsub);
		cJSON_AddNumberToObject(pJsonsub, "total", Residence[i].total);
		cJSON_AddNumberToObject(pJsonsub, "average", Residence[i].average);
	}
	return Vehicles;
}

int JsonMapVehicleTrajectoryAnalysis(cJSON *data, int *page, int *num, TimeYMD_T *timestart, TimeYMD_T *timeend)
{
	if(NULL == data || NULL == page || NULL == num)
	{	
		return KEY_FALSE;
	}

	*page = cJSON_GetObjectItemValueInt(data, "page");
	*num  = cJSON_GetObjectItemValueInt(data, "num");
	
	char time[64] = {0};
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "starttime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timestart->year, &timestart->month, &timestart->day);
	timestart->hour = 0;
	timestart->min  = 0;
	timestart->sec  = 0;

	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timeend->year, &timeend->month, &timeend->day);
	timeend->hour = 23;
	timeend->min  = 59;
	timeend->sec  = 59;
	return KEY_TRUE;
}

cJSON *JsonMapVehicleTrajectoryAnalysisMake(StationVehiclesInfo_T *VehiclesInfo, int number, int total)
{
	if(NULL == VehiclesInfo)
	{
		return NULL;
	}

	int  i = 0;
	char time[64] = {0};
	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "total",  total);
	cJSON *Vehicleslist = cJSON_CreateArray();
	cJSON_AddItemToObject(Vehicles,"vehiclelist",Vehicleslist);
	for(i = 0; i < number; i++)
	{
		cJSON *IPCitem = cJSON_CreateObject();
		cJSON_AddItemToObject(Vehicleslist,"vehiclelist",IPCitem);
		cJSON_AddStringToObject(IPCitem, "tokenid",  VehiclesInfo[i].snapToken);
		cJSON_AddStringToObject(IPCitem, "plateno",  VehiclesInfo[i].plateNo);
		cJSON_AddStringToObject(IPCitem, "picpath",  VehiclesInfo[i].picPath);
		cJSON_AddStringToObject(IPCitem, "territory",VehiclesInfo[i].territory);
		cJSON_AddStringToObject(IPCitem, "plateclor",VehiclesInfo[i].plateClor);
		cJSON_AddStringToObject(IPCitem, "cartype",  VehiclesInfo[i].carType);
		memset(time, 0, 64);
		sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d", VehiclesInfo[i].time.year, VehiclesInfo[i].time.month,
			VehiclesInfo[i].time.day,VehiclesInfo[i].time.hour,VehiclesInfo[i].time.min,VehiclesInfo[i].time.sec);
		DF_DEBUG("time: %s", time);
		cJSON_AddStringToObject(IPCitem, "time",  	time);
	}
	return Vehicles;
}

int JsonMapVehicleTrajectoryRoute(cJSON *data, char *plateNo)
{
	if(NULL == data || NULL == plateNo)
	{
		return KEY_FALSE;
	}
	cJSON_GetObjectItemValueString(data, "tokenid", plateNo, 64);
	return KEY_TRUE;
}

cJSON *JsonMapVehicleTrajectoryRouteMake(DeviceTrajectory_T *Trajectory)
{
	cJSON *Vehicles = cJSON_CreateObject();
	char time[64] = {0};

	memset(&time, 0, 64);
	snprintf(time, 64, "%04d-%02d-%02d %02d:%02d:%02d", Trajectory->arrivalstartdate.year, Trajectory->arrivalstartdate.month,
		Trajectory->arrivalstartdate.day, Trajectory->arrivalstartdate.hour, Trajectory->arrivalstartdate.min, Trajectory->arrivalstartdate.sec);
	cJSON_AddStringToObject(Vehicles, "arrivalstartdate",time);
	
	memset(&time, 0, 64);
	snprintf(time, 64, "%04d-%02d-%02d %02d:%02d:%02d", Trajectory->arrivalenddate.year, Trajectory->arrivalenddate.month,
		Trajectory->arrivalenddate.day, Trajectory->arrivalenddate.hour, Trajectory->arrivalenddate.min, Trajectory->arrivalenddate.sec);
	cJSON_AddStringToObject(Vehicles, "arrivalenddate", time);

	if( Trajectory->arrivalstartdate.year  == Trajectory->washstartdate.year && 
		Trajectory->arrivalstartdate.month == Trajectory->washstartdate.month &&
		Trajectory->arrivalstartdate.day   == Trajectory->washstartdate.day &&
		Trajectory->arrivalstartdate.hour  == Trajectory->washstartdate.hour &&
		Trajectory->arrivalstartdate.min   == Trajectory->washstartdate.min &&
		Trajectory->arrivalstartdate.sec   == Trajectory->washstartdate.sec)
	{
		memset(&time, 0, 64);
		cJSON_AddStringToObject(Vehicles, "washstartdate", time);
		cJSON_AddStringToObject(Vehicles, "washenddate",   time);
	}
	else
	{
		memset(&time, 0, 64);
		snprintf(time, 64, "%04d-%02d-%02d %02d:%02d:%02d", Trajectory->washstartdate.year, Trajectory->washstartdate.month,
			Trajectory->washstartdate.day, Trajectory->washstartdate.hour, Trajectory->washstartdate.min, Trajectory->washstartdate.sec);
		cJSON_AddStringToObject(Vehicles, "washstartdate",  time);

		memset(&time, 0, 64);
		snprintf(time, 64, "%04d-%02d-%02d %02d:%02d:%02d", Trajectory->washenddate.year, Trajectory->washenddate.month,
			Trajectory->washenddate.day, Trajectory->washenddate.hour, Trajectory->washenddate.min, Trajectory->washenddate.sec);
		cJSON_AddStringToObject(Vehicles, "washenddate",   time);
	}
	return Vehicles;
}

int JsonMapProportionOfVehicleBehavior(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend)
{
	if(NULL == data || NULL == timestart || NULL == timeend)
	{
		return KEY_FALSE;
	}
		
	char time[64] = {0};
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "starttime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timestart->year, &timestart->month, &timestart->day);
	timestart->hour = 0;
	timestart->min  = 0;
	timestart->sec  = 0;

	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timeend->year, &timeend->month, &timeend->day);
	timeend->hour = 23;
	timeend->min  = 59;
	timeend->sec  = 59;
	
	return KEY_TRUE;
}

cJSON *JsonMapProportionOfVehicleBehaviorMake(DeviceWashProportion_T *ProportionInfo)
{
	if(NULL == ProportionInfo)
	{
		return NULL;
	}

	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "washcount",  ProportionInfo->washcount);
	cJSON_AddNumberToObject(Vehicles, "oilcount",   ProportionInfo->oilcount);
	cJSON_AddNumberToObject(Vehicles, "meanwhilecount",  ProportionInfo->meanwhilecount);
	
	return Vehicles;
}

int JsonMapGetProportionOfCarWashing(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend)
{
	if(NULL == data || NULL == timestart || NULL == timeend)
	{
		return KEY_FALSE;
	}
		
	char time[64] = {0};
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "starttime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timestart->year, &timestart->month, &timestart->day);
	timestart->hour = 0;
	timestart->min  = 0;
	timestart->sec  = 0;
	
	
	memset(time, 0, 64);
	cJSON_GetObjectItemValueString(data, "endtime", time, 64);
	sscanf(time, "%04d-%02d-%02d", &timeend->year, &timeend->month, &timeend->day);
	timeend->hour = 23;
	timeend->min  = 59;
	timeend->sec  = 59;
	return KEY_TRUE;
}

cJSON *JsonMapProportionOfCarWashingMake(DeviceWashStatistics_T *WashStatistics)
{
	if(NULL == WashStatistics)
	{
		return NULL;
	}

	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "washcount",    WashStatistics->washcount);
	cJSON_AddNumberToObject(Vehicles, "totalcount",   WashStatistics->totalcount);

	cJSON_AddNumberToObject(Vehicles, "washwithoutaddnum",   WashStatistics->washwithOutaddnum);
	cJSON_AddNumberToObject(Vehicles, "addwithoutwashnum",   WashStatistics->addwithOutwashnum);
	cJSON_AddNumberToObject(Vehicles, "addandwashnum",    	 WashStatistics->addandwashnum);
	cJSON_AddNumberToObject(Vehicles, "washandaddnum",  	 WashStatistics->washandaddnum);
	return Vehicles;
}

cJSON *JsonMapGetVisitorsListMake(int sum,VIPCustomer_T *VisitorsInfo, int num)
{
	if(NULL == VisitorsInfo || num <= 0)
	{
		return NULL;
	}

	int i = 0;
	char time[128] = {0};
	cJSON *Attendance = cJSON_CreateObject();
	cJSON *Attendancelist = cJSON_CreateArray();
	cJSON_AddNumberToObject(Attendance, "sum", sum);
	cJSON_AddItemToObject(Attendance,"visitorslist",Attendancelist);
	#if 1
	for(i = 0; i < num; i++)
	{	
		if(strlen(VisitorsInfo[i].name) > 0)
		{
			cJSON *IPCitem = cJSON_CreateObject();
			cJSON_AddItemToObject(Attendancelist,"visitorslist",IPCitem);
			cJSON_AddNumberToObject(IPCitem,"gradescore",GetGradeLevel(VisitorsInfo[i].GradeScore));
			cJSON_AddNumberToObject(IPCitem,"storefrequency",VisitorsInfo[i].StoreFrequency);
			cJSON_AddStringToObject(IPCitem, "name", VisitorsInfo[i].name);
			sprintf(time, "%04d-%02d-%02d %02d:%02d", VisitorsInfo[i].snapTime.year, VisitorsInfo[i].snapTime.month, \
				VisitorsInfo[i].snapTime.day, VisitorsInfo[i].snapTime.hour, VisitorsInfo[i].snapTime.min);
			cJSON_AddStringToObject(IPCitem, "time", time);
			cJSON_AddStringToObject(IPCitem, "snaptoken", VisitorsInfo[i].snapToken);
			cJSON_AddStringToObject(IPCitem, "plateimagepath", VisitorsInfo[i].plateImagePath);
			cJSON_AddStringToObject(IPCitem, "snapimagepath", VisitorsInfo[i].snapImagePath);
		}
	}
	#endif
	return Attendance;
	
		
}

cJSON *JsonMapGetAttendanceSumMake(int num)
{
	if(0 > num)
	{
		return NULL;
	}

	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "num", num);
	return Vehicles;
}

cJSON *JsonMapGetAttendanceHistorySumMake( int num)
{
	if(0 > num)
	{
		return NULL;
	}

	cJSON *Vehicles = cJSON_CreateObject();
	cJSON_AddNumberToObject(Vehicles, "num", num);
	return Vehicles;
}


cJSON *JsonMapGetAttendanceHistoryListMake(int sum,AttendanceAtt_T *AttendanceInfo, int num)
{
	if(NULL == AttendanceInfo || num <= 0)
	{
		return NULL;
	}

	int i = 0;
	char time[128] = {0};
	cJSON *Attendance = cJSON_CreateObject();
	cJSON *Attendancelist = cJSON_CreateArray();
	cJSON_AddNumberToObject(Attendance, "sum", sum);
	cJSON_AddItemToObject(Attendance,"attendancelist",Attendancelist);
	for(i = 0; i < num; i++)
	{
		if(strlen(AttendanceInfo[i].name) > 0)
		{
			cJSON *IPCitem = cJSON_CreateObject();
			cJSON_AddItemToObject(Attendancelist,"attendancelist",IPCitem);
		
			cJSON_AddStringToObject(IPCitem, "name",AttendanceInfo[i].name);
			cJSON_AddStringToObject(IPCitem, "usrid",AttendanceInfo[i].usrid);
			cJSON_AddStringToObject(IPCitem, "phonenum",  AttendanceInfo[i].phonenum);
			sprintf(time, "%04d-%02d-%02d %02d:%02d", AttendanceInfo[i].snapTime.year, AttendanceInfo[i].snapTime.month, \
				AttendanceInfo[i].snapTime.day, AttendanceInfo[i].snapTime.hour, AttendanceInfo[i].snapTime.min);
			cJSON_AddStringToObject(IPCitem, "time", time);
			cJSON_AddStringToObject(IPCitem, "snaptoken", AttendanceInfo[i].snapToken);
			cJSON_AddStringToObject(IPCitem, "plateimagepath", AttendanceInfo[i].plateImagePath);
			cJSON_AddStringToObject(IPCitem, "snapimagepath", AttendanceInfo[i].snapImagePath);
		}
	}
	return Attendance;
}

int JsonMapGetAttendanceListSumPares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime,char *fuzzyStr)
{
	if(NULL == data )
	{
		return KEY_FALSE;
	}
	
	char timebuf[128] = {0};

	cJSON_GetObjectItemValueString(data, "fuzzystr",fuzzyStr, 128);
	cJSON_GetObjectItemValueString(data, "starttime",  timebuf, 128);
	sscanf(timebuf, "%04d-%02d-%02d", &starttime->year, &starttime->month, &starttime->day);
	starttime->hour = 0;
	starttime->min	= 0;
	starttime->sec	= 0;

	memset(timebuf, 0, 128);
	cJSON_GetObjectItemValueString(data, "endtime",  timebuf, 128);
	sscanf(timebuf, "%04d-%02d-%02d", &endtime->year, &endtime->month, &endtime->day);
	endtime->hour = 23;
	endtime->min  = 59;
	endtime->sec  = 59;
	return KEY_TRUE;
}


int JsonMapGetAttendanceListPares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime, int *page, int *num,char *fuzzyStr)
{
	if(NULL == data )
	{
		return KEY_FALSE;
	}
	
	char timebuf[128] = {0};

	*page = cJSON_GetObjectItemValueInt(data, "page");
	*num  = cJSON_GetObjectItemValueInt(data, "num");
	cJSON_GetObjectItemValueString(data, "fuzzystr",fuzzyStr, 128);
	cJSON_GetObjectItemValueString(data, "starttime",  timebuf, 128);
	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", &starttime->year, &starttime->month, &starttime->day, \
			&starttime->hour, &starttime->min, &starttime->sec);
	}
	else
	{
		starttime->year = 1970;
		starttime->month = 1;
		starttime->day = 1;
		starttime->hour =0;
		starttime->min =0;
		starttime->sec =0;
	}
	
	memset(timebuf, 0, 128);
	cJSON_GetObjectItemValueString(data, "endtime", timebuf, 128);
	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", &endtime->year, &endtime->month, &endtime->day, \
			&endtime->hour, &endtime->min, &endtime->sec);
	}
	else
	{
		Time_T tv = {0};
		GetLocalTime(&tv);
		memcpy(endtime, &tv.curTime, sizeof(TimeYMD_T));
	}
	return KEY_TRUE;
}

int JsonMapGetAttendanceSumPares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime)
{
	if(NULL == data )
	{
		return KEY_FALSE;
	}
	
	char timebuf[128] = {0};
	cJSON_GetObjectItemValueString(data, "starttime",  timebuf, 128);
	sscanf(timebuf, "%04d-%02d-%02d", &starttime->year, &starttime->month, &starttime->day);
	starttime->hour = 0;
	starttime->min	= 0;
	starttime->sec	= 0;

	memset(timebuf, 0, 128);
	cJSON_GetObjectItemValueString(data, "endtime",  timebuf, 128);
	sscanf(timebuf, "%04d-%02d-%02d", &endtime->year, &endtime->month, &endtime->day);
	endtime->hour = 23;
	endtime->min  = 59;
	endtime->sec  = 59;
	return KEY_TRUE;
}


int JsonMapGetAttendancePares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime,int *page, int *num)
{
	if(NULL == data )
	{
		return KEY_FALSE;
	}
	
	char timebuf[128] = {0};
	*page = cJSON_GetObjectItemValueInt(data, "page");
	*num  = cJSON_GetObjectItemValueInt(data, "num");
	cJSON_GetObjectItemValueString(data, "starttime",  timebuf, 128);

	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", &starttime->year, &starttime->month, &starttime->day, \
			&starttime->hour, &starttime->min, &starttime->sec);
	}
	else
	{
		starttime->year = 1970;
		starttime->month = 1;
		starttime->day = 1;
		starttime->hour =0;
		starttime->min =0;
		starttime->sec =0;
	}
	
	memset(timebuf, 0, 128);
	cJSON_GetObjectItemValueString(data, "endtime", timebuf, 128);
	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", &endtime->year, &endtime->month, &endtime->day, \
			&endtime->hour, &endtime->min, &endtime->sec);
	}
	else
	{
		Time_T tv = {0};
		GetLocalTime(&tv);
		memcpy(endtime, &tv.curTime, sizeof(TimeYMD_T));
	}

	return KEY_TRUE;
}

int JsonMapGetVisitorsPares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime,int *page,int *num)
{
	if(NULL == data )
	{
		return KEY_FALSE;
	}
	
	char timebuf[128] = {0};
	*num = cJSON_GetObjectItemValueInt(data, "num");
	*page = cJSON_GetObjectItemValueInt(data, "page");
	cJSON_GetObjectItemValueString(data, "starttime",  timebuf, 128);
	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", &starttime->year, &starttime->month, &starttime->day, \
			&starttime->hour, &starttime->min, &starttime->sec);
	}
	else
	{
		starttime->year = 1970;
		starttime->month = 1;
		starttime->day = 1;
		starttime->hour =0;
		starttime->min =0;
		starttime->sec =0;
	}
	
	memset(timebuf, 0, 128);
	cJSON_GetObjectItemValueString(data, "endtime", timebuf, 128);
	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", &endtime->year, &endtime->month, &endtime->day, \
			&endtime->hour, &endtime->min, &endtime->sec);
	}
	else
	{
		Time_T tv = {0};
		GetLocalTime(&tv);
		memcpy(endtime, &tv.curTime, sizeof(TimeYMD_T));
	}
	return KEY_TRUE;
}

cJSON *JsonMapGetVolumeOfCommutersNumMake(LeaveAndEnterNum_T *LeaveAndEnterInfo)
{
	if(NULL == LeaveAndEnterInfo)
	{
		return NULL;
	}

	int i = 0;
	char time[128] = {0};
	cJSON *Attendance = cJSON_CreateObject();
	cJSON *Attendancelist = cJSON_CreateArray();
	//cJSON_AddNumberToObject(Attendance, "leavenum", LeaveAndEnterInfo->dwLeaveNum);
	cJSON_AddNumberToObject(Attendance, "enternum", LeaveAndEnterInfo->dwEnterNum);	
	return Attendance;
}

int JsonMapGetVolumeOfCommutersNumInfoPares(cJSON *data, TimeYMD_T *time)
{
	if(NULL == data )
	{
		return KEY_FALSE;
	}
	
	char timebuf[128] = {0};
	cJSON_GetObjectItemValueString(data, "time", timebuf, 128);
	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d", &time->year, &time->month, &time->day);
		time->hour =0;
		time->min =0;
		time->sec =0;
	}
	else
	{
		time->year = 1970;
		time->month = 1;
		time->day = 1;
		time->hour =0;
		time->min =0;
		time->sec =0;
	}
	
	return KEY_TRUE;
}

cJSON *JsonMapGetVolumeOfCommutersOfLineMake(LeaveAndEnterNum_T *LeaveAndEnterInfo,int num)
{
	if(NULL == LeaveAndEnterInfo || num <= 0)
	{
		return NULL;
	}

	int i = 0;
	char time[128] = {0};
	cJSON *Attendance = cJSON_CreateObject();
	cJSON *Attendancelist = cJSON_CreateArray();
#if 1
	for(i = 0; i < num; i++)
		{
			//cJSON_AddStringToObject(IPCitem, "time", LeaveAndEnterInfo[i].time);	
			cJSON_AddNumberToObject(Attendance, LeaveAndEnterInfo[i].time, LeaveAndEnterInfo[i].dwEnterNum);	
		}
#else
	if(num > 0)
	{
		cJSON_AddItemToObject(Attendance,"commusterlist",Attendancelist);
		for(i = 0; i < num; i++)
		{
			cJSON *IPCitem = cJSON_CreateObject();
			cJSON_AddItemToObject(Attendancelist,"commusterlist",IPCitem);
			//cJSON_AddStringToObject(IPCitem, "time", LeaveAndEnterInfo[i].time);	
			cJSON_AddNumberToObject(IPCitem, LeaveAndEnterInfo[i].time, LeaveAndEnterInfo[i].dwEnterNum);	
		}
	}
#endif
	return Attendance;
}


cJSON *JsonMapGetVolumeOfCommutersOfSexMake(VolumeOfCommutersOfSex_T *info,int num)
{
	if(NULL == info || num <= 0)
	{
		return NULL;
	}

	int i = 0;
	char time[128] = {0};
	cJSON *Attendance = cJSON_CreateObject();
	cJSON *Attendancelist = cJSON_CreateArray();
	cJSON_AddNumberToObject(Attendance, "male", info->male);
	cJSON_AddNumberToObject(Attendance, "female", info->female);	
	return Attendance;
}

cJSON *JsonMapGetVolumeOfCommutersOfAgeMake(VolumeOfCommutersOfAge_T *info,int num)
{
	if(NULL == info || num <= 0)
	{
		return NULL;
	}

	int i = 0;
	char time[128] = {0};
	cJSON *Attendance = cJSON_CreateObject();
	cJSON *Attendancelist = cJSON_CreateArray();
	cJSON_AddNumberToObject(Attendance, "kid", info->kid);
	cJSON_AddNumberToObject(Attendance, "young",  info->young);
	cJSON_AddNumberToObject(Attendance, "old", info->old);
	return Attendance;
}


int JsonMapGetVolumeOfCommutersOfLineInfoPares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime,int *num)
{
	if(NULL == data )
	{
		return KEY_FALSE;
	}
	
	char timebuf[128] = {0};
	*num = cJSON_GetObjectItemValueInt(data, "num");
	cJSON_GetObjectItemValueString(data, "starttime", timebuf, 128);
	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d", &starttime->year, &starttime->month, &starttime->day);
		starttime->hour =0;
		starttime->min =0;
		starttime->sec =0;
	}
	else
	{
		starttime->year = 1970;
		starttime->month = 1;
		starttime->day = 1;
		starttime->hour =0;
		starttime->min =0;
		starttime->sec =0;
	}

	memset(timebuf,0,128);
	cJSON_GetObjectItemValueString(data, "endtime", timebuf, 128);
	if(strlen(timebuf)> 0)
	{
		sscanf(timebuf, "%04d-%02d-%02d", &endtime->year, &endtime->month, &endtime->day);
		endtime->hour =23;
		endtime->min =59;
		endtime->sec =59;
	}
	else
	{
		endtime->year = 1970;
		endtime->month = 1;
		endtime->day = 1;
		endtime->hour =0;
		endtime->min =0;
		endtime->sec =0;
	}
	
	return KEY_TRUE;
}


