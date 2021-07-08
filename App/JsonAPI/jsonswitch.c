#include "common.h"
#include "author.h"
#include "jsonswitch.h"
#include "handlemanage.h"
#include "userdefine.h"
#include "jsonmap.h"
#include "base64.h"
//#include "jpeg_yuv.h"
typedef int (*HttpCmd)(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData);


typedef struct _HttpCmdList_T{
	char *cmd;
	HttpCmd func;
}HttpCmdList_T;

typedef struct _JsonApi_T{
	pthread_mutex_t 	selfMutex;
	HandleManage_T 		listHead;//注册表维护
	ENUM_ENABLED_E		initFlag;
	AdpAPIHandle_T		innerHandle;
}JsonApi_T;

static JsonApi_T g_JsonApi;

//解析CjsonHead
static int ParesJsonMsg(cJSON *data, JsonMsg_T *head)
{
	if(data == NULL || head == NULL){
		return KEY_FALSE;
	}
	cJSON_GetObjectItemValueString(data,"cmd",head->cmd, sizeof(head->cmd));
	cJSON_GetObjectItemValueString(data,"method",head->mothod, sizeof(head->mothod));
	cJSON_GetObjectItemValueString(data,"msgid",head->msgID, sizeof(head->msgID));
	cJSON_GetObjectItemValueString(data,"sessionid",head->ssionID, sizeof(head->ssionID));
	cJSON_GetObjectItemValueString(data,"srctopic",head->srctopic, sizeof(head->srctopic));
	cJSON_GetObjectItemValueString(data,"desttopic",head->desttopic, sizeof(head->desttopic));
	
	head->bodyData = cJSON_GetObjectItem(data,"data");
	cJSON_GetObjectItemValueString(head->bodyData,"cameraid",head->deviceId, sizeof(head->deviceId));
	return KEY_TRUE;
}

//生成CjsonHead
static int MakeJsonMsg(cJSON *data, JsonMsg_T *head)
{
	if(data == NULL || head == NULL){
		return KEY_FALSE;
	}
	cJSON_AddStringToObject(data,"cmd",head->cmd);
	cJSON_AddStringToObject(data,"method","response");
	cJSON_AddStringToObject(data,"msgid",head->msgID);
	return KEY_TRUE;
}

static int AdptUnRegist(int usrid)
{
	
}

//登录 2
static int FunLogin(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	if(handle == NULL){
		return KEY_FALSE;
	}
	char usrName[DF_MAXLEN_USRNAME + 1] = {0};
	char password[DF_MAXLEN_PASSWD + 1] = {0};
	int terminaltype = -1;
	char countrycode[16] = {0};
	char deviceId[DF_UUID_LEN] = {0};
	char usrID[32] = {0};
	cJSON_GetObjectItemValueString(data,"username",usrName, sizeof(usrName));
	cJSON_GetObjectItemValueString(data,"password",password, sizeof(password));
	
 	DF_DEBUG("%s:%s \n",usrName, password);	
	int iret = AdpRegistHandle(handle,usrName,password);
	if (KEY_TRUE != iret || 0 == handle->usrID)
	{
		DF_ERROR("AdpRegistHandle fault.");
		return iret;
	}
	
	MUTEX_LOCK(&g_JsonApi.selfMutex);
	if(KEY_FALSE == HandleManageAddHandle(&g_JsonApi.listHead,(void *)handle)){
		MUTEX_UNLOCK(&g_JsonApi.selfMutex);
		DF_ERROR("json api FunLogin add handle fail");
		return KEY_FALSE;
	}
	MUTEX_UNLOCK(&g_JsonApi.selfMutex);
	sprintf(usrID, "%d", handle->usrID);
	cJSON *resdata = cJSON_CreateObject();
	cJSON_AddItemToObject(respData,"data",resdata);
	cJSON_AddStringToObject(resdata, "sessionid", usrID);
	return KEY_TRUE;
}

//登出 3
static int FunLoginOut(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	MUTEX_LOCK(&g_JsonApi.selfMutex);
	AdpUnRegistHandle(handle);
	if(KEY_FALSE == HandleManageDelHandle(&g_JsonApi.listHead,(void *)handle)){
		MUTEX_UNLOCK(&g_JsonApi.selfMutex);
		DF_ERROR("json api del handle fail");
		return KEY_FALSE;
	}
	MUTEX_UNLOCK(&g_JsonApi.selfMutex);
	MyFree(handle);
	return KEY_TRUE;
}

static int FunGetBaseInfo(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	DeviceInfo_T baseInfo = {0};
	if(NULL == handle || NULL == handle->pGetBaseInfo){
		return KEY_FALSE;
	}
	iret = handle->pGetBaseInfo(&baseInfo);
	if (KEY_FALSE != iret)
	{
		cJSON_AddItemToObject(respData, "data", JsonMapDeviceInfoMake(&baseInfo));
	}
	return iret;
}

static int FunSetBaseInfo(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	DeviceInfo_T baseInfo = {0};
	if(NULL == handle || NULL == handle->pSetBaseInfo || NULL == handle->pGetBaseInfo){
		return KEY_FALSE;
	}
	iret = handle->pGetBaseInfo(&baseInfo);
	if (KEY_FALSE != iret)
	{
		JsonMapDeviceInfoPares(data, &baseInfo);
		iret = handle->pSetBaseInfo(&baseInfo);
	}
	return iret;
}

static int FunAddUser(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	UsrInfo_T info = {0};
	if(NULL == handle || NULL == data || NULL == handle->pAddUsr){
		return KEY_FALSE;
	}
	JsonMapUsrInfoPares(data, &info);
	if (0 == info.registInfo.userLevel)
	{
		info.registInfo.userLevel = ENUM_USRLEVEL_Administrator;
	}
	iret = handle->pAddUsr(handle->usrID, &info);
	return iret;
}

static int FunCheckUser(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	UsrInfo_T info = {0};
	if(NULL == handle || NULL == data || NULL == handle->pCheckUsr){
		return KEY_FALSE;
	}
	JsonMapUsrInfoPares(data, &info);
	iret = handle->pCheckUsr(handle->usrID, &info);
	return iret;
}

//修改用户密码
static int FunModifyUsrPasswd(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	UsrInfo_T info = {0};
	char username[128] = {0}, oldPassword[128] = {0}, newPassword[128] = {0};
	int iret = 0;
	if(NULL == handle || NULL == data || NULL == handle->pModifyPassword){
		return KEY_FALSE;
	}	
	JsonMapUsrPasswdPares(data, username, oldPassword, newPassword);
//	DF_DEBUG("%s %s %s",username,oldPassword,newPassword);
	iret = handle->pModifyPassword(handle->usrID, username, oldPassword, newPassword);
	return iret;
}

//重置密码，用户忘记密码 注册服务器通信
static int FunResetPassword(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	UsrInfo_T info = {0};
	if(NULL == handle || NULL == data || NULL == handle->pResetPassword)
	{
		return KEY_FALSE;
	}
	JsonMapUsrInfoPares(data, &info);
	iret = handle->pResetPassword(info.registInfo.usrname, info.registInfo.countrycode, info.registInfo.passWord);
	return iret;
}

static int FunAddDevice(AdpAPIHandle_T *handle,cJSON *data, cJSON *respData)
{
	int iret = -1;
	DeviceStorageInfo_T deviceInfo = {0};
	if (NULL == handle || NULL == handle->pAddDevice)
	{
		return KEY_FALSE;
	}
 	cJSON_GetObjectItemValueString(data, "devicename", deviceInfo.deviceInfo.deviceName, 128);
	cJSON_GetObjectItemValueString(data, "deviceid", deviceInfo.deviceInfo.deviceID, 128);
	iret = handle->pAddDevice(&deviceInfo);
	return iret;
}

static int FunDelDevice(AdpAPIHandle_T *handle,cJSON *data, cJSON *respData)
{
	int iret = -1;
	char deviceID[128] = {0};
	if (NULL == handle || NULL == handle->pDelDevice)
	{
		return KEY_FALSE;
	}
	cJSON_GetObjectItemValueString(data, "deviceid", deviceID, 128);
	iret = handle->pDelDevice(deviceID);
	if (iret == KEY_TRUE){
		cJSON *jsonData = cJSON_CreateObject();
    	cJSON_AddStringToObject(jsonData, "deviceid", deviceID);
		cJSON_AddItemToObject(respData, "data", jsonData);
	}
	return iret;
}

static int FunGetDeviceInfo(AdpAPIHandle_T *handle,cJSON *data, cJSON *respData)
{
	int iret = -1;
	DeviceStorageInfo_T deviceInfo = {0};
	if (NULL == handle || NULL == handle->pGetDeviceInfo)
	{
		return KEY_FALSE;
	}
	cJSON_GetObjectItemValueString(data, "deviceid", deviceInfo.deviceInfo.deviceID, 128);
	iret = handle->pGetDeviceInfo(deviceInfo.deviceInfo.deviceID, &deviceInfo);
	if (KEY_TRUE == iret)
	{
		cJSON *jsonData = cJSON_CreateObject();
		cJSON_AddStringToObject(jsonData, "deviceid", deviceInfo.deviceInfo.deviceID);	
 		cJSON_AddStringToObject(jsonData, "devicename", deviceInfo.deviceInfo.deviceName);	
		cJSON_AddStringToObject(jsonData, "devicesoftwarever", deviceInfo.deviceInfo.deviceSoftwareVer);	
		cJSON_AddStringToObject(jsonData, "deviceipaddr", deviceInfo.deviceInfo.deviceIpaddr); 
		cJSON_AddNumberToObject(jsonData, "devicestatus", deviceInfo.deviceInfo.onlineStatus); 
		cJSON_AddItemToObject(respData, "data", jsonData);
	}
	return iret;
}

static int FunSetDeviceInfo(AdpAPIHandle_T *handle,cJSON *data, cJSON *respData)
{
	int iret = -1;
	DeviceStorageInfo_T deviceInfo = {0};
	if (NULL == handle || NULL == handle->pSetDeviceInfo || NULL == handle->pGetDeviceInfo)
	{
		return KEY_FALSE;
	}
	cJSON_GetObjectItemValueString(data, "deviceid", deviceInfo.deviceInfo.deviceID, 128);
	iret = handle->pGetDeviceInfo(deviceInfo.deviceInfo.deviceID, &deviceInfo);
	if (KEY_TRUE == iret)
	{
		cJSON_GetObjectItemValueString(data, "devicename", deviceInfo.deviceInfo.deviceName, 128);
 		iret = handle->pSetDeviceInfo(deviceInfo.deviceInfo.deviceID, &deviceInfo);
	}
	return iret;
}

static int FunGetDeviceInfoList(AdpAPIHandle_T *handle,cJSON *data, cJSON *respData)
{
	int iret = -1;
	int num = 0;
	DeviceStorageInfo_T *deviceInfoList = NULL;
	if (NULL== handle || NULL == handle->pGetDeviceInfoList || NULL == handle->pGetDeviceInfoListNum)
	{
		return KEY_FALSE;
	}
	iret = handle->pGetDeviceInfoListNum(&num);
	if (0 >= num)
	{
		return KEY_NOTFOUND;
	}
	if (KEY_FALSE == iret)
	{
		return KEY_FALSE;
	}
	deviceInfoList = (DeviceStorageInfo_T *)malloc(sizeof(DeviceStorageInfo_T) * num);
	iret = handle->pGetDeviceInfoList(deviceInfoList);
	if (KEY_TRUE == iret){
		JsonMapGetDeviceInfoList(respData, deviceInfoList, num);
	}
	free(deviceInfoList);
	return iret;
}

//超脑
static int FunsdkDeviceAdd(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == handle->pHksdkDeviceAdd || NULL == data)
	{
		return KEY_FALSE;
	}
	
	HKDVRParam_T devInfo = {0};
	JsonMapsdkDeviceAddInfoPares(data, &devInfo);
	iret = handle->pHksdkDeviceAdd(&devInfo);
	return iret;
}

static int FunsdkDeviceDel(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == handle->pHksdkDeviceDel || NULL == data)
	{
		return KEY_FALSE;
	}
	
	HKDVRParam_T devInfo = {0};
	JsonMapsdkDeviceDelInfoPares(data, &devInfo);
	iret = handle->pHksdkDeviceDel(&devInfo);
	return iret;
}

static int FunsdkDeviceListGet(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1, num = -1;
	if(NULL == handle || NULL == handle->pHksdkDeviceListGet)
	{
		return KEY_FALSE;
	}

	HKDVRParam_T dvrParam[NET_NVR_MAX_NUM] = {0};
	num = handle->pHksdkDeviceListGet(dvrParam);
	if (num > 0){
		cJSON_AddItemToObject(respData,"data",JsonMapGetsdkDeviceList(dvrParam, num));
	}
	return KEY_TRUE;
}

static int FunsdkDeviceListSet(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1, num = 0, i = 0;
	if(NULL == handle || NULL == handle->pHksdkDeviceListSet || NULL == data)
	{
		return KEY_FALSE;
	}
	
	HKDVRParam_T *dvrParam = (HKDVRParam_T *)malloc(sizeof(HKDVRParam_T)*NET_NVR_MAX_NUM);
	num = handle->pHksdkDeviceListGet(dvrParam);

	DvrParam_T Info = {0};
	iret = JsonMapsdkDeviceSetInfoPares(data, &Info);
	if(iret == KEY_TRUE)
	{
		for(i = 0; i < num; i++)
		{
			if(!strcmp(dvrParam[i].HkDvrInfo.sDvrSerialNumber, Info.serialnumber))
			{
				memcpy(dvrParam[i].HkDvrInfo.sDvrName, Info.name, NET_DVR_MAX_LEN);
				iret = handle->pHksdkDeviceListSet(&dvrParam[i]);
				free(dvrParam);
				return iret;
			}
		}
	}
	free(dvrParam);
	return KEY_FALSE;
}

static int FunsdkDeviceipcamSet(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1, num = 0, i = 0, j = 0;
	if(NULL == handle || NULL == handle->pHksdkDeviceListSet || NULL == data)
	{
		return KEY_FALSE;
	}
	
	HKDVRParam_T *dvrParam = (HKDVRParam_T *)malloc(sizeof(HKDVRParam_T)*NET_NVR_MAX_NUM);
	num = handle->pHksdkDeviceListGet(dvrParam);

	IpcamParam_T ipcam = {0};
	iret = JsonMapsdkIpcamSetInfoPares(data, &ipcam);
	if(iret == KEY_TRUE)
	{
		for(i = 0; i < num; i++)
		{
			if(!strcmp(dvrParam[i].HkDvrInfo.sDvrSerialNumber, ipcam.serialnumber))
			{
				for(j = 0; j < dvrParam[i].HkDvrInfo.wChannel; j++)
				{
					if(dvrParam[i].HkSdkParam[j].sChannel == ipcam.channel)
					{
						dvrParam[i].HkSdkParam[j].sDeviceType = ipcam.devicetype;
						memcpy(dvrParam[i].HkSdkParam[j].sDeviceName, ipcam.name, NET_DVR_MAX_LEN);
						memcpy(dvrParam[i].HkDvrInfo.sDvrSerialNumber, ipcam.serialnumber, NET_DVR_MAX_LEN);
						iret = handle->pHksdkDeviceListSet(&dvrParam[i]);
						free(dvrParam);
						return iret;
					}
				}
			}
		}
	}
	free(dvrParam);
	return KEY_FALSE;
}

//告警
static int FunsdkDeviceAlarmGet(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == handle->pHKsdkDeviceAlarmGet)
	{
		return KEY_FALSE;
	}

	DeviceAlarmParam_T alarmParam = {0};
	iret = handle->pHKsdkDeviceAlarmGet(&alarmParam);
	if(iret != KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	cJSON_AddItemToObject(respData,"data",JsonMapsdkDeviceAlarmGetMake(&alarmParam));
	return KEY_TRUE;
}

static int FunsdkDeviceAlarmSet(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == handle->pHKsdkDeviceAlarmSet || NULL == data)
	{
		return KEY_FALSE;
	}
	
	DeviceAlarmParam_T alarmParam = {0};
	iret = JsonMapsdkDeviceAlarmSetPares(data, &alarmParam);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	iret = handle->pHKsdkDeviceAlarmSet(&alarmParam);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	return KEY_TRUE;
}

static int FunGetEntranceVehiclesCurList(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == handle->pGetEntranceVehiclesCurList || NULL == data)
	{
		return KEY_FALSE;
	}

	int num = 0, count = 0, number = 0;
	iret = JsonMapGetEntranceVehiclesCurList(data, &num);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	StationVehiclesInfo_T *VehiclesInfo = (StationVehiclesInfo_T *)malloc(sizeof(StationVehiclesInfo_T)*num);
	count = handle->pGetEntranceVehiclesCurList(num, VehiclesInfo);	
	number = (count < num) ? count : num;
	cJSON_AddItemToObject(respData,"data",JsonMapGetEntranceVehiclesCurListMake(number, VehiclesInfo));
	free(VehiclesInfo);
	return KEY_TRUE;
}

//车辆
static int FunGetFrontageVehiclesSum(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == handle->pGetFrontageVehiclesSum || NULL == data)
	{
		return KEY_FALSE;
	}
	
	TimeYMD_T startTime = {0}; 
	TimeYMD_T endTime   = {0};
	iret = JsonMapGetFrontageVehiclesSumPares(data, &startTime, &endTime);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	StreetTypeAttr streetAttr = {0};
	iret = handle->pGetFrontageVehiclesSum(startTime, endTime, &streetAttr);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapGetFrontageVehiclesSumMake(&streetAttr));
	return KEY_TRUE;
}

static int FunGetEntranceVehiclesSum(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetEntranceVehiclesSum)
	{
		return KEY_FALSE;
	}
	
	TimeYMD_T startTime = {0}; 
	TimeYMD_T endTime   = {0};
	iret = JsonMapGetEntranceVehiclesSumPares(data, &startTime, &endTime);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	ProgressRate_T progressAttr = {0};
	iret = handle->pGetEntranceVehiclesSum(startTime, endTime, &progressAttr);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapGetEntranceVehiclesSumMake(&progressAttr));
	return KEY_TRUE;
}

static int FunGetEntranceVehiclesList(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1, count = 0, number = 0;
	if(NULL == handle || NULL == data || NULL == handle->pGetEntranceVehiclesList)
	{
		return KEY_FALSE;
	}
	int page = 0, num = 0, total = 0;
	StationVehicles_T Vehicles = {0};
	iret = JsonMapGetEntranceVehiclesListPares(data, &Vehicles, &page, &num);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	StationVehiclesInfo_T *VehiclesInfo = (StationVehiclesInfo_T *)malloc(sizeof(StationVehiclesInfo_T)* num);
	memset(VehiclesInfo,0,sizeof(StationVehiclesInfo_T)* num);
	count = handle->pGetEntranceVehiclesList(page, num, Vehicles, VehiclesInfo, &total);
	number = (count < num) ? count : num;
	cJSON_AddItemToObject(respData,"data",JsonMapGetEntranceVehiclesListMake(VehiclesInfo, number, total));
	free(VehiclesInfo);
	return KEY_TRUE;
}

static int FunGetEntranceHistoryList(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1, count = 0;
	if(NULL == handle || NULL == data || NULL == handle->pGetEntranceHistoryList)
	{
		return KEY_FALSE;
	}

	int carnum = 0;
	int page = 0, num = 0, number = 0;
	char location[64] = {0};
	char plateNo[64] = {0};
	
	iret = JsonMapGetEntranceHistoryListPares(data, &page, &num, location, plateNo);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	StationHistoryInfo_T *historyInfo = (StationHistoryInfo_T *)malloc(sizeof(StationHistoryInfo_T)*num);
	memset(historyInfo,0,sizeof(StationHistoryInfo_T)* num);
	count = handle->pGetEntranceHistoryList(page, num, &carnum, location, plateNo, historyInfo);
	if(count < 0)
	{
		return KEY_FALSE;
	}
	number = (count < num) ? count : num;
	cJSON_AddItemToObject(respData,"data",JsonMapGetEntranceHistoryListMake(historyInfo, number, carnum));
	free(historyInfo);
	return KEY_TRUE;
}

static int FunGetEntranceServiceAnalysis(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetEntranceServiceAnalysis)
	{
		return KEY_FALSE;
	}
	
	TimeYMD_T time = {0};
	iret = JsonMapGetEntranceServiceAnalysis(data, &time);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	int day[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	if (time.year%4==0&&time.year%100!=0||time.year%400==0) 
	{
		day[1]++;
	}
	
	int date[31] = {0};
	memset(date, 0, sizeof(int)*31);
	iret = handle->pGetEntranceServiceAnalysis(time, day[time.month-1], date);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapEntranceServiceAnalysisMake(date, day[time.month-1]));
	return KEY_TRUE;
}

static int FunGetEntrancePlateAnalysis(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetEntrancePlateAnalysis)
	{
		return KEY_FALSE;
	}

	char provice[64] = {0};
	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	iret = JsonMapGetEntrancePlateAnalysis(data, &timestart, &timeend, provice);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	PlaceAttribution_T PlaceAttr = {0};
	iret = handle->pGetEntrancePlateAnalysis(timestart, timeend, provice, &PlaceAttr);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapEntrancePlateAnalysisMake(PlaceAttr));
	return KEY_TRUE;
}

static int FunGetEntranceHistoryCount(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetEntranceHistoryCount)
	{
		return KEY_FALSE;
	}
	
	char plateNo[64] = {0};
	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	
	iret = JsonMapGetEntranceHistoryCount(data, &timestart, &timeend, plateNo);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	int count[12] = {0};
	memset(count, 0, sizeof(int) * 12);
	iret = handle->pGetEntranceHistoryCount(timestart, timeend, plateNo, count);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapEntranceHistoryCountMake(count));
	return KEY_TRUE;
}

static int FunGetRefuelingTypeAnalysis(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetRefuelingTypeAnalysis)
	{
		return KEY_FALSE;
	}
	
	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	iret = JsonMapRefuelingTypeAnalysis(data, &timestart, &timeend);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	RefuelingType_T  Refueling = {0};
	iret = handle->pGetRefuelingTypeAnalysis(timestart, timeend, &Refueling);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapRefuelingTypeAnalysisMake(&Refueling));
	return KEY_TRUE;
}

static int FunGetOutboundVehiclesAnalysis(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetOutboundVehiclesAnalysis)
	{
		return KEY_FALSE;
	}
	
	int days = 0, count = 0;
	TimeYMD_T time1start = {0};
	TimeYMD_T time1end = {0};
	TimeYMD_T time2start = {0};
	TimeYMD_T time2end = {0};
	iret = JsonMapOutboundVehiclesAnalysis(data, &time1start, &time1end, &time2start, &time2end, &days);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}

	ProgressRate_T Progress1total = {0};
	ProgressRate_T Progress2total = {0};
	
	ProgressRate_T *Progress1 = NULL;
	ProgressRate_T *Progress2 = NULL;
	if(days == 1)
	{
		Progress1 = (ProgressRate_T *)malloc(sizeof(ProgressRate_T)*24);
		Progress2 = (ProgressRate_T *)malloc(sizeof(ProgressRate_T)*24);
		count = 24;
	}
	else
	{
		Progress1 = (ProgressRate_T *)malloc(sizeof(ProgressRate_T)*days);
		Progress2 = (ProgressRate_T *)malloc(sizeof(ProgressRate_T)*days);
		count = days;
	}
	iret = handle->pGetOutboundVehiclesAnalysis(time1start, time1end, count, Progress1, &Progress1total);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}

	iret = handle->pGetOutboundVehiclesAnalysis(time2start, time2end, count, Progress2, &Progress2total);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapOutboundVehiclesAnalysisMake(count, Progress1, Progress2, &Progress1total, &Progress2total));	
	free(Progress1);
	free(Progress2);
	return KEY_TRUE;
}

static int FunGetResidenceTimeAnalysis(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetOutboundVehiclesAnalysis)
	{
		return KEY_FALSE;
	}

	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	iret = JsonMapResidenceTimeAnalysis(data, &timestart, &timeend);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	ResidenceTime_T Residence = {0};
	iret = handle->pGetResidenceTimeAnalysis(timestart, timeend, &Residence);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapResidenceTimeAnalysisMake(&Residence));	
	return KEY_TRUE;
}

static int FunGetAbnormalVehicleList(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetAbnormalVehicleList)
	{
		return KEY_FALSE;
	}

	int page = 0, num = 0, count = 0, number = 0, total = 0;
	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	iret = JsonMapAbnormalVehicleList(data, &timestart, &timeend, &page, &num);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	AbnormalVehicleInfo_T *Abnormal = (AbnormalVehicleInfo_T *)malloc(sizeof(AbnormalVehicleInfo_T)*num);
	count = handle->pGetAbnormalVehicleList(page, num, &total, timestart, timeend, Abnormal);
	number = (count < num) ? count : num;
	DF_DEBUG("number: %d  %d %d",count, num, number);
	cJSON_AddItemToObject(respData,"data",JsonMapAbnormalVehicleListMake(Abnormal, number, total));
	free(Abnormal);
	return KEY_TRUE;
}

static int FunGetRefuelingEfficiencyAnalysis(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetRefuelingEfficiencyAnalysis)
	{
		return KEY_FALSE;
	}

	int days = 0, count = 0;
	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	
	iret = JsonMapRefuelingEfficiencyAnalysis(data, &timestart, &timeend, &days);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}

	Refueling_T *refueling = NULL;
	if(days == 1)
	{
		refueling = (Refueling_T *)malloc(sizeof(Refueling_T)*24);
		count = 24;
	}
	else
	{
		refueling = (Refueling_T *)malloc(sizeof(Refueling_T)*days);
		count = days;
	}

	iret = handle->pGetRefuelingEfficiencyAnalysis(timestart, timeend, count, refueling);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapRefuelingEfficiencyAnalysisMake(refueling, count));
	free(refueling);
	return KEY_TRUE;
}

static int FunGetVehicleTrajectoryAnalysis(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetVehicleTrajectoryAnalysis)
	{
		return KEY_FALSE;
	}

	int page = 0, num = 0, total = 0, count = 0, number = 0;
	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	iret = JsonMapVehicleTrajectoryAnalysis(data, &page, &num, &timestart, &timeend);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	StationVehiclesInfo_T *VehiclesInfo = (StationVehiclesInfo_T *)malloc(sizeof(StationVehiclesInfo_T)* num);
	memset(VehiclesInfo, 0, sizeof(StationVehiclesInfo_T)* num);
	count = handle->pGetVehicleTrajectoryAnalysis(page, num, &total, VehiclesInfo, timestart, timeend);
	number = (count < num) ? count : num;
	cJSON_AddItemToObject(respData,"data",JsonMapVehicleTrajectoryAnalysisMake(VehiclesInfo, number, total));
	free(VehiclesInfo);
	return KEY_TRUE;
}

static int FunGetVehicleTrajectoryRoute(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetVehicleTrajectoryRoute)
	{
		return KEY_FALSE;
	}

	char plateNo[64] = {0};
	iret = JsonMapVehicleTrajectoryRoute(data, plateNo);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	DeviceTrajectory_T Trajectory = {0};
	iret = handle->pGetVehicleTrajectoryRoute(plateNo, &Trajectory);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapVehicleTrajectoryRouteMake(&Trajectory));
	return KEY_TRUE;
}

static int FunGetProportionOfVehicleBehavior(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetProportionOfVehicleBehavior)
	{
		return KEY_FALSE;
	}
	
	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	iret = JsonMapProportionOfVehicleBehavior(data, &timestart, &timeend);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}

	DeviceWashProportion_T ProportionInfo = {0};
	iret = handle->pGetProportionOfVehicleBehavior(timestart, timeend, &ProportionInfo);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapProportionOfVehicleBehaviorMake(&ProportionInfo));
	return KEY_TRUE;
}

static int FunGetProportionOfCarWashing(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetProportionOfCarWashing)
	{
		return KEY_FALSE;
	}
	
	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	iret = JsonMapGetProportionOfCarWashing(data, &timestart, &timeend);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	DeviceWashStatistics_T WashStatistics = {0};
	iret = handle->pGetProportionOfCarWashing(timestart, timeend, &WashStatistics);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapProportionOfCarWashingMake(&WashStatistics));
	return KEY_TRUE;
}

static int FunGetIntelligentAlarm(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetIntelligentAlarm)
	{
		return KEY_FALSE;
	}

	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	int type = -1, page = 0, num = 0, total = 0, count = 0, number = 0;
	iret = JsonMapGetIntelligentAlarm(data, &timestart, &timeend, &page, &num, &type);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}

	DeviceAlarmInfo_T *alarmList = (DeviceAlarmInfo_T *)malloc(sizeof(DeviceAlarmInfo_T)*num);
	count = handle->pGetIntelligentAlarm(timestart, timeend, page, num, type, &total, alarmList);
	number = (count < num) ? count : num;
	cJSON_AddItemToObject(respData,"data",JsonMapGetIntelligentAlarmMake(alarmList, number, total));
	free(alarmList);
	return KEY_TRUE;
}

static int FunGetAlarmProportionAnalysis(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetAlarmProportionAnalysis)
	{
		return KEY_FALSE;
	}
	
	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	iret = JsonMapGetAlarmProportionAnalysis(data, &timestart, &timeend);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}

	DeviceAlarmCount_T AlarmCount = {0};
	iret = handle->pGetAlarmProportionAnalysis(timestart, timeend, &AlarmCount);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapGetAlarmProportionAnalysisMake(&AlarmCount));
	return KEY_TRUE;
}

static int FunGetAlarmPeriodAnalysis(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetAlarmPeriodAnalysis)
	{
		return KEY_FALSE;
	}

	int count = 0, number = 0;
	TimeYMD_T timestart = {0};
	TimeYMD_T timeend = {0};
	iret = JsonMapGetAlarmPeriodAnalysis(data, &timestart, &timeend, &count);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	DeviceAlarmCount_T *AlarmCount = NULL;
	if(count == 1)
	{
		number = 24;
	}
	else
	{
		number = count;
	}
	
	AlarmCount = (DeviceAlarmCount_T *)malloc(sizeof(DeviceAlarmCount_T)*number);
	memset(AlarmCount, 0, sizeof(DeviceAlarmCount_T)*count);
	iret = handle->pGetAlarmPeriodAnalysis(timestart, timeend, count, AlarmCount);
	if(iret !=  KEY_TRUE)
	{
		free(AlarmCount);
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapGetAlarmPeriodAnalysisMake(count, AlarmCount));
	free(AlarmCount);
	return KEY_TRUE;
}

static int FunGetAttendanceSum(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	DF_DEBUG("FunGetAttendance");
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetAttendanceRecord)
	{
		return KEY_FALSE;
	}
	
	int page = 0, num = 0;
	StationVehicles_T Vehicles = {0};
	TimeYMD_T starttime;
	TimeYMD_T endtime;
	iret = JsonMapGetAttendanceSumPares(data, &starttime,&endtime);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	iret = handle->pGetAttendanceRecordSum(starttime, endtime,&num);
	if(iret != KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapGetAttendanceSumMake(num));
	return KEY_TRUE;
}



static int FunGetAttendance(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	DF_DEBUG("FunGetAttendance");
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetAttendanceRecord)
	{
		return KEY_FALSE;
	}
	
	int page = 0, num = 0;
	StationVehicles_T Vehicles = {0};
	TimeYMD_T starttime;
	TimeYMD_T endtime;
	iret = JsonMapGetAttendancePares(data, &starttime,&endtime,&page,&num);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	AttendanceAtt_T *Attendanceinfo;
	Attendanceinfo = (AttendanceAtt_T *)malloc(sizeof(AttendanceAtt_T) * num);
	memset(Attendanceinfo,0,sizeof(AttendanceAtt_T) * num);
	int sum;
	iret = handle->pGetAttendanceRecord(starttime, endtime,page,num,&sum,Attendanceinfo);
	if(iret != KEY_TRUE)
	{
		free(Attendanceinfo);
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapGetAttendanceHistoryListMake(sum,Attendanceinfo, num));
	free(Attendanceinfo);
	return KEY_TRUE;
}

static int FunGetAttendanceHistorySum(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	DF_DEBUG("FunGetAttendanceList");
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetAttendanceHistorySum)
	{
		return KEY_FALSE;
	}
	
	int page = 0, num = 0;
	StationVehicles_T Vehicles = {0};
	TimeYMD_T starttime;
	TimeYMD_T endtime;
	char fuzzyStr[128] = {0};
	iret = JsonMapGetAttendanceListSumPares(data, &starttime, &endtime,fuzzyStr);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	iret = handle->pGetAttendanceHistorySum(starttime,endtime,fuzzyStr, &num);
	if(iret != KEY_TRUE)
	{
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapGetAttendanceHistorySumMake(num));
	return KEY_TRUE;
}


static int FunGetAttendanceHistoryList(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	DF_DEBUG("FunGetAttendanceList");
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetAttendanceHistoryList)
	{
		return KEY_FALSE;
	}
	
	int page = 0, num = 0;
	StationVehicles_T Vehicles = {0};
	TimeYMD_T starttime;
	TimeYMD_T endtime;
	char fuzzyStr[128] = {0};
	int sum;
	iret = JsonMapGetAttendanceListPares(data, &starttime, &endtime,&page, &num,fuzzyStr);
	if(iret !=  KEY_TRUE)
	{
		return KEY_FALSE;
	}
	
	AttendanceAtt_T *Attendanceinfo = (AttendanceAtt_T *)malloc(sizeof(AttendanceAtt_T) * num);
	memset(Attendanceinfo,0,sizeof(AttendanceAtt_T) * num);
	iret = handle->pGetAttendanceHistoryList(starttime,endtime,page, num, fuzzyStr,&sum,Attendanceinfo);
	if(iret != KEY_TRUE)
	{
		free(Attendanceinfo);
		return KEY_FALSE;
	}
	cJSON_AddItemToObject(respData,"data",JsonMapGetAttendanceHistoryListMake(sum,Attendanceinfo, num));
	free(Attendanceinfo);
	return KEY_TRUE;
}

static int FunGetVisitorsInfo(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	DF_DEBUG("FunGetVisirorsInfo");
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetVisitorsInfo)
	{
		return KEY_FALSE;
	}
	
	int page = 0, sum = 0,num = 0;
	TimeYMD_T starttime;
	TimeYMD_T endtime;
	iret = JsonMapGetVisitorsPares(data, &starttime,&endtime,&page,&num);
	if(iret !=KEY_TRUE)
	{
		return KEY_FALSE;
	}
	DF_DEBUG("start %d-%d-%d-%d-%d-%d\n",starttime.year,starttime.month,starttime.day,starttime.hour,starttime.min,starttime.sec);
	DF_DEBUG("end %d-%d-%d-%d-%d-%d\n",endtime.year,endtime.month,endtime.day,endtime.hour,endtime.min,endtime.sec);
	DF_DEBUG("num = %d\n",num);
	VIPCustomer_T *vipCustomerinfo;
	vipCustomerinfo = (VIPCustomer_T *)malloc(sizeof(VIPCustomer_T) *num );
	memset(vipCustomerinfo,0,sizeof(VIPCustomer_T) *num);

	iret = handle->pGetVisitorsInfo(starttime,endtime, page,num,&sum,vipCustomerinfo);
	if(iret != KEY_TRUE)
	{
		free(vipCustomerinfo);
		return KEY_FALSE;
	}

	cJSON_AddItemToObject(respData,"data",JsonMapGetVisitorsListMake(sum,vipCustomerinfo, num));
	free(vipCustomerinfo);
	return KEY_TRUE;
}

static int FunUploadOilStationData(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	int num = 0, i = 0;
	cJSON *Dvritem = NULL;
	cJSON *dataList = NULL;
	StuckPipeInfo_T *stuckPipeList = NULL;
	char type = cJSON_GetObjectItemValueInt(data, "uploadtype");
	dataList = cJSON_GetObjectItem(data, "datalist");
	if (NULL == dataList || NULL == handle->pAddStuckPipeData)
	{
		return KEY_FALSE;
	}
	num = cJSON_GetArraySize(dataList);
	if (FuelQuantityType_StuckPipe == type)
	{
		stuckPipeList = (StuckPipeInfo_T *)malloc(sizeof(StuckPipeInfo_T) * num);
		for(i = 0 ; i < num; i++)
		{
			Dvritem = cJSON_GetArrayItem(dataList, i);
			if (NULL != Dvritem)
			{
				char szOils[128] = {0}, szLPM[128] = {0}, szNumber[128] = {0};
				cJSON_GetObjectItemValueString(Dvritem, "name", stuckPipeList[i].stationName, 128);
				cJSON_GetObjectItemValueString(Dvritem, "Oils", szOils, 128);				
				cJSON_GetObjectItemValueString(Dvritem, "LPM", szLPM, 128);
				cJSON_GetObjectItemValueString(Dvritem, "number", szNumber, 128);
				stuckPipeList[i].oils = atoi(szOils);
				stuckPipeList[i].LPM = atof(szLPM);
				stuckPipeList[i].number = atoi(szNumber);
				char date[128] = {0};
				cJSON_GetObjectItemValueString(Dvritem, "date", date, 128);
				sscanf(date, "%04d-%02d-%02d %02d:%02d:%02d", &stuckPipeList[i].date.year, &stuckPipeList[i].date.month, &stuckPipeList[i].date.day, \
					&stuckPipeList[i].date.hour, &stuckPipeList[i].date.min, &stuckPipeList[i].date.sec);
			}
		}
		handle->pAddStuckPipeData(stuckPipeList, num);
		free(stuckPipeList);
	}
	else if (FuelQuantityType_PointSale == type)
	{

	}
	
	return KEY_TRUE;
}

static int FunGetOilStationData(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	TimeYMD_T tYmd = {0};
	char date[128] = {0};
	OliGunInfo_T *oliGunInfo = NULL;
	int lowPoint = 0, mediumPoint = 0, highPoint = 0;
	int point92 = 0, point95 = 0, point98 = 0;
	int oliGunSum  = 0;
	if (NULL == handle->pGetFuelingRangeProportionByDate ||
		NULL == handle->pGetFuelQuantityByType ||
		NULL == handle->pGetOliGunSum ||
		NULL == handle->pGetOliGunFrequency)
	{
		return KEY_FALSE;
	}
	cJSON *cJdate = cJSON_CreateObject();
	cJSON_AddItemToObject(respData, "data", cJdate);
	cJSON_GetObjectItemValueString(data, "date", date, 128);		
	sscanf(date, "%4d-%2d", &tYmd.year, &tYmd.month); 
	oliGunSum = handle->pGetOliGunSum(tYmd);
	if (0 < oliGunSum)
	{
		oliGunInfo = (OliGunInfo_T *)malloc(sizeof(OliGunInfo_T) * oliGunSum);
		memset(oliGunInfo, 0, sizeof(OliGunInfo_T) * oliGunSum);
	}
	else
	{
		return KEY_TRUE;
	}
	handle->pGetFuelingRangeProportionByDate(tYmd, &lowPoint, &mediumPoint, &highPoint);
	handle->pGetFuelQuantityByType(tYmd, &point92, &point95, &point98);
	handle->pGetOliGunFrequency(tYmd, oliGunInfo);
	cJSON *cJFillRate = cJSON_CreateObject();
	if (NULL != cJFillRate)
	{
		cJSON_AddNumberToObject(cJFillRate, "low", lowPoint);
		cJSON_AddNumberToObject(cJFillRate, "middle", mediumPoint);
		cJSON_AddNumberToObject(cJFillRate, "high", highPoint);
	}
	cJSON_AddItemToObject(cJdate, "fillrate", cJFillRate);
	cJSON *cJOliType = cJSON_CreateObject();
	if (NULL != cJOliType)
	{
		cJSON_AddNumberToObject(cJOliType, "92type", point92);
		cJSON_AddNumberToObject(cJOliType, "95type", point95);
		cJSON_AddNumberToObject(cJOliType, "98type", point98);
	}
	cJSON_AddItemToObject(cJdate, "olitype", cJOliType);

	
	cJSON *cJOilGun = cJSON_CreateArray();
	if (NULL != cJOilGun)
	{
		cJSON_AddItemToObject(cJdate, "oilgun", cJOilGun);
		int i = 0;
		for (i = 0; i < oliGunSum; i++)
		{
			cJSON *cJOilGunArr = cJSON_CreateObject();
			cJSON_AddItemToArray(cJOilGun, cJOilGunArr);
			cJSON_AddNumberToObject(cJOilGunArr, "number", oliGunInfo[i].number);
			cJSON_AddNumberToObject(cJOilGunArr, "frequency", oliGunInfo[i].frequency);
		}
	}

	cJSON *cJPaymentAnalysis = cJSON_CreateArray();
	if (NULL != cJPaymentAnalysis)
	{	
		cJSON_AddItemToObject(cJdate, "paymentanalysis", cJPaymentAnalysis);
		char arr[4][128] = {"加油卡","现金","APP","石化钱包"};
		int i = 0;
		for (i = 0; i < 4; i++)
		{
			cJSON *cJPaymentAnalysisArr = cJSON_CreateObject();
			cJSON_AddItemToArray(cJPaymentAnalysis, cJPaymentAnalysisArr);
			cJSON_AddStringToObject(cJPaymentAnalysisArr, "type", arr[i]);
			cJSON_AddNumberToObject(cJPaymentAnalysisArr, "paymentnumber", 705);
			cJSON_AddNumberToObject(cJPaymentAnalysisArr, "paymentamount", 5545);
		}
	}
	if (NULL != oliGunInfo)
	{
		free(oliGunInfo);
	}
	return KEY_TRUE;
}

static int FunGetVolumeOfCommutersNumInfo(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	DF_DEBUG("FunGetVolumeOfCommutersInfo");
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetVisitorsInfo)
	{
		return KEY_FALSE;
	}
	
	TimeYMD_T time;
	iret = JsonMapGetVolumeOfCommutersNumInfoPares(data, &time);
	if(iret !=KEY_TRUE)
	{
		return KEY_FALSE;
	}
	DF_DEBUG("start %d-%d-%d-%d-%d-%d\n",time.year,time.month,time.day,time.hour,time.min,time.sec);

	LeaveAndEnterNum_T *LeaveAndEnterinfo;
	LeaveAndEnterinfo = (LeaveAndEnterNum_T *)malloc(sizeof(LeaveAndEnterNum_T));


	iret = handle->pGetVolumeOfCommutersNum(time,LeaveAndEnterinfo);
	if(iret != KEY_TRUE)
	{
		free(LeaveAndEnterinfo);
		return KEY_FALSE;
	}

	cJSON_AddItemToObject(respData,"data",JsonMapGetVolumeOfCommutersNumMake(LeaveAndEnterinfo));
	free(LeaveAndEnterinfo);

	return KEY_TRUE;
}

static int FunGetVolumeOfCommutersOfLineInfo(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	DF_DEBUG("FunGetVolumeOfCommutersInfo");
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetVisitorsInfo)
	{
		return KEY_FALSE;
	}
	
	int page = 0, sum = 0,num = 0;
	TimeYMD_T starttime;
	TimeYMD_T endtime;
	int reporttype;
	iret = JsonMapGetVolumeOfCommutersOfLineInfoPares(data, &starttime,&endtime,&num);
	if(iret !=KEY_TRUE)
	{
		return KEY_FALSE;
	}
	DF_DEBUG("start %d-%d-%d-%d-%d-%d\n",starttime.year,starttime.month,starttime.day,starttime.hour,starttime.min,starttime.sec);
	DF_DEBUG("end %d-%d-%d-%d-%d-%d\n",endtime.year,endtime.month,endtime.day,endtime.hour,endtime.min,endtime.sec);
	DF_DEBUG("reporttype = %d\n",reporttype);
	LeaveAndEnterNum_T *LeaveAndEnterinfo;
	if(num > 0)
	{
		if(num == 1)
		{
			LeaveAndEnterinfo = (LeaveAndEnterNum_T *)malloc(sizeof(LeaveAndEnterNum_T) *24 );
			memset(LeaveAndEnterinfo,0,sizeof(LeaveAndEnterNum_T) *24);
		}
		else
		{
			LeaveAndEnterinfo = (LeaveAndEnterNum_T *)malloc(sizeof(LeaveAndEnterNum_T) *num );
			memset(LeaveAndEnterinfo,0,sizeof(LeaveAndEnterNum_T) *num);	
		}
	}

	iret = handle->pGetVolumeOfCommutersOfLine(starttime,endtime, num,&sum,LeaveAndEnterinfo);
	if(iret != KEY_TRUE)
	{
		free(LeaveAndEnterinfo);
		return KEY_FALSE;
	}

	cJSON_AddItemToObject(respData,"data",JsonMapGetVolumeOfCommutersOfLineMake(LeaveAndEnterinfo, sum));
	free(LeaveAndEnterinfo);

	return KEY_TRUE;
}
static int FunGetVolumeOfCommutersOfSexInfo(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	DF_DEBUG("FunGetVolumeOfCommutersInfo");
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetVisitorsInfo)
	{
		return KEY_FALSE;
	}
	
	int page = 0, sum = 0,num = 0;
	TimeYMD_T starttime;
	TimeYMD_T endtime;
	int reporttype;
	iret = JsonMapGetVolumeOfCommutersOfLineInfoPares(data, &starttime,&endtime,&num);
	if(iret !=KEY_TRUE)
	{
		return KEY_FALSE;
	}
	DF_DEBUG("start %d-%d-%d-%d-%d-%d\n",starttime.year,starttime.month,starttime.day,starttime.hour,starttime.min,starttime.sec);
	DF_DEBUG("end %d-%d-%d-%d-%d-%d\n",endtime.year,endtime.month,endtime.day,endtime.hour,endtime.min,endtime.sec);
	DF_DEBUG("num = %d\n",num);
	VolumeOfCommutersOfSex_T *VolumeOfCommutersOfSexinfo;

	VolumeOfCommutersOfSexinfo = (VolumeOfCommutersOfSex_T *)malloc(sizeof(VolumeOfCommutersOfSex_T)  );
	memset(VolumeOfCommutersOfSexinfo,0,sizeof(VolumeOfCommutersOfSex_T));

	iret = handle->pGetVolumeOfCommutersOfSex(starttime,endtime,num,VolumeOfCommutersOfSexinfo);
	if(iret != KEY_TRUE)
	{
		free(VolumeOfCommutersOfSexinfo);
		return KEY_FALSE;
	}

	cJSON_AddItemToObject(respData,"data",JsonMapGetVolumeOfCommutersOfSexMake(VolumeOfCommutersOfSexinfo, num));
	free(VolumeOfCommutersOfSexinfo);

	return KEY_TRUE;
}
static int FunGetVolumeOfCommutersOfAgeInfo(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	DF_DEBUG("FunGetVolumeOfCommutersInfo");
	int iret = -1;
	if(NULL == handle || NULL == data || NULL == handle->pGetVisitorsInfo)
	{
		return KEY_FALSE;
	}
	
	int page = 0, sum = 0,num = 0;
	TimeYMD_T starttime;
	TimeYMD_T endtime;
	int reporttype;
	iret = JsonMapGetVolumeOfCommutersOfLineInfoPares(data, &starttime,&endtime,&num);
	if(iret !=KEY_TRUE)
	{
		return KEY_FALSE;
	}
	DF_DEBUG("start %d-%d-%d-%d-%d-%d\n",starttime.year,starttime.month,starttime.day,starttime.hour,starttime.min,starttime.sec);
	DF_DEBUG("end %d-%d-%d-%d-%d-%d\n",endtime.year,endtime.month,endtime.day,endtime.hour,endtime.min,endtime.sec);
	DF_DEBUG("num = %d\n",num);
	VolumeOfCommutersOfAge_T *VolumeOfCommutersOfAgeinfo;
	
	VolumeOfCommutersOfAgeinfo = (VolumeOfCommutersOfAge_T *)malloc(sizeof(VolumeOfCommutersOfAge_T)  );
	memset(VolumeOfCommutersOfAgeinfo,0,sizeof(VolumeOfCommutersOfAge_T) );


	iret = handle->pGetVolumeOfCommutersOfAge(starttime,endtime,num,VolumeOfCommutersOfAgeinfo);
	if(iret != KEY_TRUE)
	{
		free(VolumeOfCommutersOfAgeinfo);
		return KEY_FALSE;
	}

	cJSON_AddItemToObject(respData,"data",JsonMapGetVolumeOfCommutersOfAgeMake(VolumeOfCommutersOfAgeinfo, num));
	free(VolumeOfCommutersOfAgeinfo);

	return KEY_TRUE;
}



static int GetImageFormat(unsigned char *image, int imageLen)
{
	int format = Format_NONE;
	if (NULL == image)
	{
		return Format_NONE;
	}
	if (0xff == image[0] && 0xd8 == image[1])
	{
		format = Format_JPEG;
	}
	
	if (0x89 == image[0] && 0x50 == image[1] && 0x4e == image[2] && 0x47 == image[3]
		&& 0x0d == image[4] && 0x0a == image[5] && 0x1a == image[6] && 0x0a == image[7])
	{
		format = Format_PNG;
	}

	if (0x47 == image[0] && 0x49 == image[1] && 0x46 == image[2])
	{
		format = Format_GIF;
	}
	return format;
}

int out(cJSON *item)
{
	char *out;
	out  = cJSON_PrintUnformatted(item);
	printf("%s",out);
	printf("\r\n");
	free(out);	
}

//...................
static int FunNoMothod(AdpAPIHandle_T *handle,cJSON *data,cJSON *respData)
{
	return KEY_CMDNOTFOUND;
}

static int JsonPrintUnformatted(cJSON *respData, char *response)
{
	if (NULL == respData || NULL == response)
	{
		return KEY_FALSE;
	}
	char *out = cJSON_PrintUnformatted(respData);
	sprintf(response,"%s",out);//JSON转换成字符串
	if(NULL!=out)
	{
		free(out);out=NULL;
	}
	return KEY_TRUE;
}

int JsonSwitch(char *requst, char *response, int buffLen)
{
	static HttpCmdList_T cmdList[] = {
		{"loginmainserver",FunLogin},	
		{"loginoutmainserver",FunLoginOut},	
		{"getbaseinfo", FunGetBaseInfo},
		{"setbaseinfo", FunSetBaseInfo},
		{"adduser",FunAddUser}, 
		{"registrationcheckuser", FunCheckUser},
		{"userresetpassword", FunResetPassword},
  		{"adddevice", FunAddDevice},
		{"deldevice", FunDelDevice},
		{"getdeviceinfo", FunGetDeviceInfo},
		{"setdeviceinfo", FunSetDeviceInfo},
		{"getdeviceinfolist", FunGetDeviceInfoList},

		//超脑信息
		{"sdkdeviceadd", 	 FunsdkDeviceAdd},
		{"sdkdevicedel", 	 FunsdkDeviceDel},
		{"sdkdevicelistget", FunsdkDeviceListGet},
		{"sdkdevicelistset", FunsdkDeviceListSet},
		{"sdkdeviceipcamset",FunsdkDeviceipcamSet},


		//告警级别
		{"sdkdevicealarmget", 	 FunsdkDeviceAlarmGet},
		{"sdkdevicealarmset",	 FunsdkDeviceAlarmSet},
			
		//车辆信息
		{"getentrancevehiclescurlist",	FunGetEntranceVehiclesCurList},
		{"getfrontagevehiclessum",  FunGetFrontageVehiclesSum},
		{"getentrancevehiclessum",  FunGetEntranceVehiclesSum},
		{"getentrancevehiclelist",  FunGetEntranceVehiclesList},
		{"getentrancehistorylist",  FunGetEntranceHistoryList},
		{"getentranceserviceanalysis",FunGetEntranceServiceAnalysis},
		{"getentranceplateanalysis",  FunGetEntrancePlateAnalysis},
		{"getentrancehistorycount",   FunGetEntranceHistoryCount},
		{"getrefuelingtypeanalysis",  FunGetRefuelingTypeAnalysis},
		{"getoutboundvehiclesanalysis",  FunGetOutboundVehiclesAnalysis},
		{"getresidencetimeanalysis",  FunGetResidenceTimeAnalysis},
		{"getabnormalvehiclelist",  FunGetAbnormalVehicleList},
		{"getrefuelingefficiencyanalysis",  FunGetRefuelingEfficiencyAnalysis},
		{"getvehicletrajectoryanalysis",	FunGetVehicleTrajectoryAnalysis},
		{"getvehicletrajectoryroute",	FunGetVehicleTrajectoryRoute},
		
		//洗车
		{"getproportionofvehiclebehavior",	FunGetProportionOfVehicleBehavior},
		{"getproportionofcarwashing",	FunGetProportionOfCarWashing},
    	//考勤
	
		{"getattendance", FunGetAttendance},								//获取今日所有打卡人最早打卡记录。
		{"getattendancedeta", FunGetAttendanceHistoryList},					//获取当前员工打卡记录。
		{"getattendancelisthistory", FunGetAttendanceHistoryList},			//获取所有人打卡历史记录。
		{"getvisitors", FunGetVisitorsInfo},								//获取所有vip进店次数。
		{"getvolumeofcommutersnum",FunGetVolumeOfCommutersNumInfo},			//获取今日进店客流量总数
		{"getvolumeofcommutersofline",FunGetVolumeOfCommutersOfLineInfo},	//获取客流量折线图
		{"getvolumeofcommutersofsex",FunGetVolumeOfCommutersOfSexInfo},		//获取客流量男女比例
		{"getvolumeofcommutersofage",FunGetVolumeOfCommutersOfAgeInfo},		//获取客流量年龄分布

		//告警
		{"getintelligentalarm",	FunGetIntelligentAlarm},
		{"getalarmproportionanalysis",	FunGetAlarmProportionAnalysis},		
		{"getalarmperiodanalysis",	FunGetAlarmPeriodAnalysis}, 	

		//加满率信息
		{"uploadoilstationdata", FunUploadOilStationData},
		{"getoilstationdata", FunGetOilStationData},
		{NULL,FunNoMothod}
	};
	AdpAPIHandle_T *handle = NULL;
	void *result = NULL;
	int i = 0;
	int iret = KEY_FALSE;
	cJSON *data = NULL;
	cJSON *bodydData = NULL;
	JsonMsg_T jsonMsg = {0};
	if(NULL == requst || 
	   NULL == response ||
	   NULL == (data = cJSON_Parse(requst))){
		 return KEY_FALSE;
	}
	
	cJSON *respData = cJSON_CreateObject();
	if(KEY_FALSE == ParesJsonMsg(data,&jsonMsg)){
		cJSON_Delete(data);
		cJSON_Delete(respData);
		return KEY_FALSE;
	}
	MakeJsonMsg(respData,&jsonMsg);
	for(i = 0;NULL != cmdList[i].cmd; i++){
		if(0 == STRCASECMP(jsonMsg.cmd, cmdList[i].cmd)){		
			if (0 != STRCASECMP(jsonMsg.cmd, "loginmainserver")){
				HandleManageGetHandleByInt(&g_JsonApi.listHead,usrID,atoi(jsonMsg.ssionID),result,AdpAPIHandle_T);
				handle = (AdpAPIHandle_T *)result;
				if(NULL == handle) {
					cJSON_AddNumberToObject(respData, "statuscode", KEY_UNAUTHORIZED);
					JsonPrintUnformatted(respData, response);
					cJSON_Delete(respData);
					cJSON_Delete(data);
					return KEY_UNAUTHORIZED;
				}
			}
			
			if (NULL == handle && 0 == STRCASECMP(jsonMsg.cmd, "loginmainserver")){
				handle = (AdpAPIHandle_T *)malloc(sizeof(AdpAPIHandle_T));
				memset(handle, 0, sizeof(AdpAPIHandle_T));
			}
			
			iret = cmdList[i].func(handle,jsonMsg.bodyData,respData);
			if(KEY_TRUE != iret){
				int statusCode = 0;
				if (KEY_NORESPONSE == iret)
				{
					cJSON_Delete(respData);
					cJSON_Delete(data);
					return iret;
				}
				else
				{
					if (KEY_FALSE == iret)
					{
						statusCode = 400;
					}
					else
					{
						statusCode = iret;
					}
				}
				cJSON_AddNumberToObject(respData,"statuscode", statusCode);
				JsonPrintUnformatted(respData, response);
				if (KEY_UNAUTHORIZED == iret || KEY_USERREPEATLOGIN == iret)
				{
					MyFree(handle);
				}
				cJSON_Delete(respData);
				cJSON_Delete(data);
				return iret;
			}		
			cJSON_AddNumberToObject(respData,"statuscode", 200);
			JsonPrintUnformatted(respData, response);
			cJSON_Delete(respData);
			cJSON_Delete(data);
			return iret;
		}
	}
	cJSON_Delete(respData);
	cJSON_Delete(data);
	return KEY_CMDNOTFOUND;
}

int JsonApiInit()
{
	MUTEX_INIT(&g_JsonApi.selfMutex);
	MUTEX_LOCK(&g_JsonApi.selfMutex);
	if(ENUM_ENABLED_TRUE == g_JsonApi.initFlag){
		g_JsonApi.initFlag = ENUM_ENABLED_TRUE;
	}
	HandleManageInitHead(&g_JsonApi.listHead);
	MUTEX_UNLOCK(&g_JsonApi.selfMutex);

	if (KEY_FALSE == AdpRegistHandle(&g_JsonApi.innerHandle, INNER_USER_NAME, INNER_USER_PASSWORD)){//请解析出来
		DF_ERROR("AdpRegistHandle fault");
		return KEY_UNAUTHORIZED;
	}
	return KEY_TRUE;
}
