#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "adpapiinner.h"
#include "adpapi.h"
#include "adphandleinit.h"
#include "mysqlctrl.h"
#include "configmanage.h"

static AdpHandle_T g_handle;
static AdpHandle_T *GetAdpHandle()
{
	return &g_handle;
} 

static HandleManage_T *AdpGetListUsrRegistHead()
{
	AdpHandle_T *handle = GetAdpHandle();
	return &handle->listUsrRegistHead;
}

static AdpUsrRegistInfo_T *GetUsrRegistInfo(int usrID)
{
	void *resultHandle = NULL;
	HandleManageGetHandleByInt(AdpGetListUsrRegistHead(), apiHandle->usrID, usrID, resultHandle, AdpUsrRegistInfo_T);
	return (AdpUsrRegistInfo_T *)resultHandle;
}

static int Log_Init()
{
	int iLogLevel, iLogDirect, iLogMaxFileSize, iLogMaxFileCount;
	char pcLogFilePath[128], pcLogFilePrefix[128];
	int iRet = -1;
	snprintf(pcLogFilePath, 128, "%s", "log/");
	snprintf(pcLogFilePrefix, 128, "%s", "minimalmainserver");
	iLogMaxFileSize = 1024*1024;	
	iLogMaxFileCount = 5;
	iLogLevel = (LOG_ERR | LOG_WAR | LOG_NOT | LOG_DBG | LOG_LAST);			
	iLogDirect = LOG_TTY;
	
	iRet = LogInit(pcLogFilePath, pcLogFilePrefix, iLogMaxFileSize, \
				 iLogMaxFileCount, iLogLevel, iLogDirect, LOGPARAM_INIT);

	//printf("%d   %d", iLogLevel, iLogDirect);
	
	if(iRet) 
	{
		printf("LOG PARAMETER INIT ERROR!!\n");
	}
	else  
	{
		//DF_DEBUG("LOG DISPLAY START HERE!!");
	}
	
	return 0;	
}


static int MakeUsrID()
{
	void *resultHandle = NULL;
	static int usrID = 0;
	do
	{
		usrID++;
		usrID = usrID % 10000; //125
		HandleManageGetHandleByInt(AdpGetListUsrRegistHead(), apiHandle->usrID, usrID, resultHandle, AdpUsrRegistInfo_T);
	}while(resultHandle != NULL);
	return usrID;
}

static int ModifyUsrInfo(int usrID,UsrInfo_T *info)
{
	AdpUsrRegistInfo_T *usr = GetUsrRegistInfo(usrID);
	AdpHandle_T *adphandle = GetAdpHandle();	
	return adphandle->usrManageHandle.pModifyUsrInfo(&(usr->usrInfo),info);
}

static int AddUsr(int usrID,UsrInfo_T *info)
{
	AdpUsrRegistInfo_T *usr = GetUsrRegistInfo(usrID);
	AdpHandle_T *adphandle = GetAdpHandle();
	return adphandle->usrManageHandle.pAddUsr(&(usr->usrInfo),info);
}

static int CheckUsr(int usrID,UsrInfo_T *info)
{
	AdpUsrRegistInfo_T *usr = GetUsrRegistInfo(usrID);
	AdpHandle_T *adphandle = GetAdpHandle();
	return adphandle->usrManageHandle.pcheckUsr(&(usr->usrInfo),info);
}

static int DelUsr(int usrID,UsrInfo_T *info)
{
	AdpUsrRegistInfo_T *usr = GetUsrRegistInfo(usrID);
	AdpHandle_T *adphandle = GetAdpHandle();
	return adphandle->usrManageHandle.pDelUsr(&(usr->usrInfo),info);
}

static int GetUsrInfo(int usrID,UsrInfo_T *info)
{
	AdpUsrRegistInfo_T *usr = GetUsrRegistInfo(usrID);
	AdpHandle_T *adphandle = GetAdpHandle();
	return adphandle->usrManageHandle.pGetUsrInfo(&(usr->usrInfo),info);
}

static int ModifyPassword(int usrID, char *username,char *oldPassword,char *newPassword)
{
	AdpUsrRegistInfo_T *usr = GetUsrRegistInfo(usrID);
	AdpHandle_T *adphandle = GetAdpHandle();
	if (strcmp(usr->usrInfo.registInfo.usrname, username))
	{
		DF_DEBUG("User information mismatch!");
		return KEY_FALSE;
	}
	return adphandle->usrManageHandle.pModifyPassword(username, oldPassword, newPassword);
}

static int	GetUserRegistInfo(int usrID, UserRegistInfo_T *info)
{
	AdpUsrRegistInfo_T *usr = GetUsrRegistInfo(usrID);
	memcpy(info,&(usr->usrInfo.registInfo),sizeof(UserRegistInfo_T));
	return KEY_TRUE;
}

static int AdpRegisterhighTempHandle(AdpAPIHandle_T *handle,AdpUsrRegistInfo_T *usrRegistInfo)
{
	if (NULL != handle->highTempFun)
	{
		usrRegistInfo->highTempHandle.highTempFun = handle->highTempFun;
		if(KEY_FALSE == HighTemperatureDetectionRegister(&usrRegistInfo->highTempHandle)){
			DF_ERROR("HighTemperatureDetectionRegister ERRS");
		}
	}
	return KEY_TRUE;
}

static int AdpRegistManage(AdpAPIHandle_T *handle,AdpUsrRegistInfo_T *usrRegistInfo)
{
	UsrManageHandle_T *usrManageHandle = &GetAdpHandle()->usrManageHandle;
	DeviceManageHandle_T *deviceManageHandle = &GetAdpHandle()->deviceHandle;
	MsgManageHandle_T *MsgManageHandle = &GetAdpHandle()->msgManageHandle;
	DeciceHandle_T *baseHandle = &GetAdpHandle()->localHandle;
	DataAnalysisManage_T *dataAnalysisHandle = &GetAdpHandle()->dataAnalysisHead;
	AttendanceManage_T *attendancehandle = &GetAdpHandle()->attendancehandle;
	FuelQuantityHandle_T *fuelQuantityHandle = &GetAdpHandle()->fuelQuantityHandle;
	HksdkManageHandle_T *HksdkManagehandle = &GetAdpHandle()->hksdkHandle;
	UsrManageRegistHandle_T *usrRegistHandle = &(usrRegistInfo->usrManageRegistHandle);
	AlarmHandle_T *alarmHandle = &GetAdpHandle()->alarmHandle;
	UsrManageRegist(usrRegistHandle);
	switch(usrRegistInfo->usrInfo.registInfo.userLevel){
		case ENUM_USRLEVEL_ROOT:
			handle->pGetUsrInfoNum = usrManageHandle->pGetUsrInfoNum; 
			handle->pGetUsrInfoList = usrManageHandle->pGetUsrInfoList; 
		case ENUM_USRLEVEL_Administrator:
			handle->pAddUsr = AddUsr;
			handle->pCheckUsr = CheckUsr;
			handle->pDelUsr = DelUsr;
			handle->pGetUsrInfo = GetUsrInfo;
			
			handle->pAddDevice = deviceManageHandle->pAddDevice;
			handle->pDelDevice = deviceManageHandle->pDelDevice;
			handle->pGetDeviceInfo = deviceManageHandle->pGetDeviceInfo;
			handle->pSetDeviceInfo = deviceManageHandle->pSetDeviceInfo;
			handle->pGetDeviceInfoList = deviceManageHandle->pGetDeviceInfoList;
			handle->pGetDeviceInfoListNum = deviceManageHandle->pGetDeviceInfoNum;
		case ENUM_USRLEVEL_Operator:
			handle->pModifyUsrInfo = ModifyUsrInfo; 	
			handle->pModifyPassword =  ModifyPassword;
			handle->pResetPassword = usrManageHandle->pResetPassword;
			handle->pModifyUsrBindInfo = usrManageHandle->pModifyUsrBindInfo;
			handle->pFindUsr = usrManageHandle->pFindUsr;
			handle->pGetUserRegistInfo = GetUserRegistInfo;
			handle->pGetBaseInfo = baseHandle->pGetDeviceInfo;
			handle->pSetBaseInfo = baseHandle->pSetDeviceInfo;

			//超脑
			handle->pHksdkDeviceAdd = HksdkManagehandle->pHksdkDeviceAdd;
			handle->pHksdkDeviceDel = HksdkManagehandle->pHksdkDeviceDel;
			handle->pHksdkDeviceListGet = HksdkManagehandle->pHksdkDeviceListGet;
			handle->pHksdkDeviceListSet = HksdkManagehandle->pHksdkDeviceListSet;

			handle->pHKsdkDeviceAlarmGet = HksdkManagehandle->pHKsdkDeviceAlarmGet;
			handle->pHKsdkDeviceAlarmSet = HksdkManagehandle->pHKsdkDeviceAlarmSet;

			//车辆数据分析
			handle->pAddVehicleInfo = dataAnalysisHandle->pAddVehicleInfo;
			handle->pGetEntranceVehiclesCurList = dataAnalysisHandle->pGetEntranceVehiclesCurList;
			handle->pGetFrontageVehiclesSum  = dataAnalysisHandle->pGetFrontageVehiclesSum;
			handle->pGetEntranceVehiclesSum  = dataAnalysisHandle->pGetEntranceVehiclesSum;
			handle->pGetEntranceVehiclesList = dataAnalysisHandle->pGetEntranceVehiclesList;
			handle->pGetEntranceHistoryList  = dataAnalysisHandle->pGetEntranceHistoryList;
			handle->pGetEntrancePlateAnalysis= dataAnalysisHandle->pGetEntrancePlateAnalysis;
			handle->pGetEntranceServiceAnalysis = dataAnalysisHandle->pGetEntranceServiceAnalysis;
			handle->pGetEntranceHistoryCount = dataAnalysisHandle->pGetEntranceHistoryCount;
			handle->pGetRefuelingTypeAnalysis= dataAnalysisHandle->pGetRefuelingTypeAnalysis;
			handle->pGetOutboundVehiclesAnalysis = dataAnalysisHandle->pGetOutboundVehiclesAnalysis;
			handle->pGetResidenceTimeAnalysis = dataAnalysisHandle->pGetResidenceTimeAnalysis;
			handle->pGetAbnormalVehicleList = dataAnalysisHandle->pGetAbnormalVehicleList;
			handle->pGetRefuelingEfficiencyAnalysis = dataAnalysisHandle->pGetRefuelingEfficiencyAnalysis;
			handle->pGetVehicleTrajectoryAnalysis = dataAnalysisHandle->pGetVehicleTrajectoryAnalysis;
			handle->pGetVehicleTrajectoryRoute = dataAnalysisHandle->pGetVehicleTrajectoryRoute;

			//加满率属性
			handle->pAddStuckPipeData = fuelQuantityHandle->pAddStuckPipeData;
			handle->pGetFuelingRangeProportionByDate = fuelQuantityHandle->pGetFuelingRangeProportionByDate;
			handle->pGetFuelQuantityByType = fuelQuantityHandle->pGetFuelQuantityByType;
			handle->pGetOliGunSum = fuelQuantityHandle->pGetOliGunSum;
			handle->pGetOliGunFrequency = fuelQuantityHandle->pGetOliGunFrequency;
			//洗车
			handle->pGetProportionOfVehicleBehavior = dataAnalysisHandle->pGetProportionOfVehicleBehavior;
			handle->pGetProportionOfCarWashing = dataAnalysisHandle->pGetProportionOfCarWashing;
			
			handle->pGetAttendanceHistoryList = attendancehandle->pGetAttendanceHistoryList;
			handle->pGetAttendanceHistorySum = attendancehandle->pGetAttendanceHistorySum;
			handle->pGetAttendanceRecord = attendancehandle->pGetAttendanceRecord;
			handle->pGetAttendanceRecordSum = attendancehandle->pGetAttendanceRecordSum;
			handle->pGetVisitorsInfo = attendancehandle->pGetVisitorsInfo;
			handle->pGetVolumeOfCommutersNum = attendancehandle->pGetVolumeOfCommutersNum;
			handle->pGetVolumeOfCommutersOfLine = attendancehandle->pGetVolumeOfCommutersOfLine;
			handle->pGetVolumeOfCommutersOfSex = attendancehandle->pGetVolumeOfCommutersOfSex;
			handle->pGetVolumeOfCommutersOfAge = attendancehandle->pGetVolumeOfCommutersOfAge;

			//告警
			handle->pGetIntelligentAlarm = dataAnalysisHandle->pGetIntelligentAlarm;
			handle->pGetAlarmProportionAnalysis = dataAnalysisHandle->pGetAlarmProportionAnalysis;
			handle->pGetAlarmPeriodAnalysis = dataAnalysisHandle->pGetAlarmPeriodAnalysis;
			//高温注册
			AdpRegisterhighTempHandle(handle, usrRegistInfo);
			//告警推送
			handle->pPushAlarmInfo = alarmHandle->pPushAlarm;
			break;
	}
	return KEY_TRUE;
}

_OUT_ int AdpRegistHandle(AdpAPIHandle_T *handle,char *usrName,char *passwd)
{
	if(NULL == handle){
		return KEY_FALSE;
	}
	InitAdpAPIRegistHandleNoAuthor(handle);
	int userLevel = ENUM_USRLEVEL_Invalid;
	if(ENUM_USRLEVEL_Invalid == (userLevel = GetAdpHandle()->usrManageHandle.pAuthentication(usrName,passwd))){
		DF_ERROR("AdpRegistHandle Authentication Fail!!");
		return KEY_UNAUTHORIZED;
	}

	AdpUsrRegistInfo_T *usrRegistInfo = (AdpUsrRegistInfo_T *)malloc(sizeof(AdpUsrRegistInfo_T));
	memset(usrRegistInfo,0,sizeof(AdpUsrRegistInfo_T));
	snprintf(usrRegistInfo->usrInfo.registInfo.usrname,DF_MAXLEN_USRNAME + 1,"%s",usrName);
	snprintf(usrRegistInfo->usrInfo.registInfo.passWord,DF_MAXLEN_PASSWD + 1,"%s",passwd);
	usrRegistInfo->usrInfo.registInfo.userLevel = userLevel;

	AdpRegistManage(handle,usrRegistInfo);
	usrRegistInfo->usrManageRegistHandle.pGetUserRegistInfo(&(usrRegistInfo->usrInfo.registInfo));
	snprintf(handle->usrName,DF_MAXLEN_USRNAME + 1,"%s",usrName);
	handle->usrID = (usrRegistInfo->usrInfo.registInfo.userLevel << 16) | MakeUsrID();
	usrRegistInfo->apiHandle = handle;
	HandleManageAddHandle(AdpGetListUsrRegistHead(), (void *)usrRegistInfo);
	DF_DEBUG("AdpRegistHandle Success. usrID = [%d]",handle->usrID);
	return KEY_TRUE;
}

_OUT_ int AdpUnRegistHandle(AdpAPIHandle_T *handle)
{
	if(NULL == handle){
		DF_ERROR("handle is NULL");
		return KEY_FALSE;
	}
	AdpUsrRegistInfo_T *usr = GetUsrRegistInfo(handle->usrID);
	if(NULL == usr){
		DF_ERROR("usr is NULL");
		return KEY_FALSE;
	}
	HighTemperatureDetectionUnRegister(&usr->highTempHandle);
	//去除用户列表
	HandleManageDelHandle(AdpGetListUsrRegistHead(),usr);
	//资源销毁
	if (NULL != usr)
	{
		free(usr);
	}
	DF_DEBUG("AdpUnRegistHandle %d %x ",handle->usrID, handle);
	InitAdpAPIRegistHandleNoAuthor(handle);
	return KEY_TRUE;
}

_OUT_ int AdpHandleInit()
{
	AdpHandle_T *adphandle = GetAdpHandle();
	//Log 日志
	Log_Init();
	//配置表
	HandleManageInitHead(AdpGetListUsrRegistHead());
	if(KEY_NOTFOUNDCONF == ConfigManageInit()){
		DF_DEBUG("KEY_NOTFOUNDCONF");
	}
		
	//数据库模块初始化
	MySqlManageInit(&adphandle->mysqlHandle);	
	DF_DEBUG("MySqlManageInit succeed.");
	
	//时间模块初始化
	TimeManageInit();
	DF_DEBUG("TimeManageInit succeed.");
	adphandle->usrManageHandle.addUsrcb = adphandle->mysqlHandle.pAddUsrInfo;
	adphandle->usrManageHandle.delUsrcb = adphandle->mysqlHandle.pDelUsrInfo;
	adphandle->usrManageHandle.modifyUsrBindInfocb = adphandle->mysqlHandle.pModifyUsrBindInfocb;
	adphandle->usrManageHandle.getUsrInfocb = adphandle->mysqlHandle.pGetUsrInfo;
	adphandle->usrManageHandle.setUsrInfocb = adphandle->mysqlHandle.pSetUsrInfo;	
	adphandle->usrManageHandle.getUsrInfoListcb = adphandle->mysqlHandle.pGetUsrInfoList;
	adphandle->usrManageHandle.getUsrInfoListNumcb = adphandle->mysqlHandle.pGetUsrInfoListNum;
	
	//本地模块数据初始化
	 LocalInfoInit(&adphandle->localHandle);
	//用户模块初始化
	if (KEY_TRUE != UsrManageInit(&adphandle->usrManageHandle)){
		DF_DEBUG("UsrManageInit Fail.");
	}
	DF_DEBUG("UsrManageInit succeed.");
	//Mediaquene 模块初始化
	MediaQueueInit(&adphandle->mediaQueueHandle);
	DF_DEBUG("MediaQueueInit succeed.");
	//设备模块初始化
	if (KEY_TRUE != DeiveManageInit(&adphandle->deviceHandle))
	{
		DF_DEBUG("DeiveManageInit Fail.");
	}
	DF_DEBUG("DeiveManageInit succeed.");

	//系统模块初始化
	SystemManageInit();
	DF_DEBUG("SystemManageInit succeed.");
	
	//加密管理模块初始化	
 	PublicKeyInit(&adphandle->publicKeyHandle);
	DF_DEBUG("PublicKeyInit succeed.");
	
	//初始化设备管理模块
	adphandle->hksdkHandle.createQueneFromMediaQuenecb = adphandle->mediaQueueHandle.pCreateQueue;
	adphandle->hksdkHandle.disdroyQueneFromMediaQuenecb= adphandle->mediaQueueHandle.pDisdroyQueue;

	adphandle->hksdkHandle.writeAudioRawDatacb = adphandle->mediaQueueHandle.pWriteAudioRawData;
	adphandle->hksdkHandle.writeVideoRawDatacb = adphandle->mediaQueueHandle.pWriteVideoRawData;	
	HksdkApiInit(&adphandle->hksdkHandle);

	//数据模块（海康AI）初始化
	VehicleDataAnalysisManageInit(&adphandle->dataAnalysisHead);
	DF_DEBUG("DataAnalysisManageInit succeed.");

	AttendanceManageInit(&adphandle->attendancehandle);
	DF_DEBUG("AttendanceManageInit succeed.");
	//加满率分析模块
	FuelQuantityAnalysisManageInit(&adphandle->fuelQuantityHandle);
	DF_DEBUG("FuelQuantityAnalysisManageInit succeed.");

	//adphandle->alarmHandle.AlarmFun = HelloWorld;
	AlarmInit(&adphandle->alarmHandle);
	DF_DEBUG("AdpHandleInit Success");
	return KEY_TRUE;
}

_OUT_ int AdpHandleUnInit()
{
	AdpHandle_T *adphandle = GetAdpHandle();
	HksdkApiUnInit(&adphandle->hksdkHandle);
	MySqlManageUnInit(&adphandle->mysqlHandle);
	UsrManageUnInit(&adphandle->usrManageHandle);
	MediaQueueUnInit(&adphandle->mediaQueueHandle);
	PublicKeyUnInit(&adphandle->publicKeyHandle);
	DeiveManageUnInit(&adphandle->deviceHandle);
	SystemManageUnInit();
	VehicleDataAnalysisManageUnInit(&adphandle->dataAnalysisHead);	
	FuelQuantityAnalysisManageUnInit(&adphandle->fuelQuantityHandle);
	ConfigManageUnInit();
	LogDestroy();
	DF_DEBUG("AdpHandleUnInit");
	return KEY_TRUE;
}


