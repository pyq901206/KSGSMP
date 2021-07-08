#ifndef __ADP_API_H__
#define __ADP_API_H__

#ifdef __cplusplus
extern "C" {
#endif 
#include "usrmanage.h"
#include "devicemanage.h"
#include "common.h"
#include "publickey.h"
#include "messagemanage.h"
#include "timemanage.h"
#include "sysmanage.h"
#include "vehicledataanalysis.h"
#include "fuelquantity.h"
#include "humandataanalysis.h"
#include "deviceinfo.h"
#include "mediaqueue.h"
#include "hkManage.h"
#include "alarmmanage.h"
#include "attendancemanage.h"

#define INNER_USER_NAME "gzadmin@u"
#define INNER_USER_PASSWORD "gzadmin@p"

typedef struct _AdpAPIHandle_T{
	//基本属性
	int (*pGetBaseInfo)(DeviceInfo_T *info);
	int (*pSetBaseInfo)(DeviceInfo_T *info);
	//用户属性
	int	(*pGetUserRegistInfo)(int usrID, UserRegistInfo_T *info);
	int	(*pModifyPassword)(int usrID, char *username,char *oldPassword,char *newPassword);
	int (*pResetPassword)(char *username, char *countrycode, char *password);
	int (*pModifyUsrBindInfo)(UsrInfo_T *info, char *oldusername);
	int (*pFindUsr)(char *username);
	int (*pGetUsrInfoNum)(int *num); 
	int	(*pGetUsrInfoList)(UsrInfo_T *info);
	int	(*pModifyUsrInfo)(int usrID,UsrInfo_T *info);
	int (*pGetUsrInfo)(int usrID,UsrInfo_T *info);
	int (*pAddUsr)(int usrID,UsrInfo_T *info);
	int (*pCheckUsr)(int usrID,UsrInfo_T *info);
	int (*pDelUsr)(int usrID,UsrInfo_T *info);
	int (*pGetPublicKey)(PublicKey_T *key);
	int (*pSetPublicKey)(PublicKey_T *key);

	//设备属性
	int (*pGetDeviceInfo)(char *deviceID,DeviceStorageInfo_T *cloudInfo);
	int (*pSetDeviceInfo)(char *deviceID,DeviceStorageInfo_T *cloudInfo);
	int (*pGetDeviceInfoListNum)(int *num);
	int (*pGetDeviceInfoList)(DeviceStorageInfo_T *cloudInfo);
	int (*pAddDevice)(DeviceStorageInfo_T *info);
	int (*pDelDevice)(char *deviceID);

	//超脑设备
	int (*pHksdkDeviceAdd)(HKDVRParam_T *devInfo);
	int (*pHksdkDeviceDel)(HKDVRParam_T *devInfo);
	int (*pHksdkDeviceListGet)(HKDVRParam_T *dvrParam);
	int (*pHksdkDeviceListSet)(HKDVRParam_T *dvrParam);

	int (*pHKsdkDeviceAlarmSet)(DeviceAlarmParam_T *alarmParam);
	int (*pHKsdkDeviceAlarmGet)(DeviceAlarmParam_T *alarmParam);

	//数据模块分析属性	
	int (*pAddVehicleInfo)(DataAtt_T *dataInfo); //添加抓拍信息到存储
	int (*pGetEntranceVehiclesCurList)(int num, StationVehiclesInfo_T *VehiclesInfo);
	int (*pGetFrontageVehiclesSum)(TimeYMD_T startTime, TimeYMD_T endTime, StreetTypeAttr *streetAttr);   //获取临街车辆x时间段内车辆总数	
	int (*pGetEntranceVehiclesSum)(TimeYMD_T startTime, TimeYMD_T endTime, ProgressRate_T *progressAttr);
	int (*pGetEntranceVehiclesList)(int page, int num, StationVehicles_T stationAttr, StationVehiclesInfo_T *VehiclesInfo, int *totalnum);
	int (*pGetEntranceHistoryList)(int page, int num, int *totalno, char *location, char *plateno, StationHistoryInfo_T *historyInfo);
	int (*pGetEntrancePlateAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, char *location, PlaceAttribution_T *PlaceAttr);
	int (*pGetEntranceServiceAnalysis)(TimeYMD_T Time, int days, int *data);
	int (*pGetEntranceHistoryCount)(TimeYMD_T startTime, TimeYMD_T endTime, char *plateNo, int *data);
	int (*pGetRefuelingTypeAnalysis)(TimeYMD_T Time1, TimeYMD_T Time2, RefuelingType_T *Refueling);
	int (*pGetOutboundVehiclesAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, int count, ProgressRate_T *Progress, ProgressRate_T *Progress1);
	int (*pGetResidenceTimeAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, ResidenceTime_T *Residence);
	int (*pGetAbnormalVehicleList)(int page, int num, int *total, TimeYMD_T startTime, TimeYMD_T endTime, AbnormalVehicleInfo_T *Abnormal);
	int (*pGetRefuelingEfficiencyAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, int count, Refueling_T *refueling);
	int (*pGetVehicleTrajectoryAnalysis)(int page, int num, int *totalno, StationVehiclesInfo_T *VehiclesInfo, TimeYMD_T timestart, TimeYMD_T timeend);
	int (*pGetVehicleTrajectoryRoute)(char *plateNo, DeviceTrajectory_T *Trajectory);

	//洗车
	int (*pGetProportionOfVehicleBehavior)(TimeYMD_T startTime, TimeYMD_T endTime, DeviceWashProportion_T *ProportionInfo);
	int (*pGetProportionOfCarWashing)(TimeYMD_T startTime, TimeYMD_T endTime, DeviceWashStatistics_T *WashStatistics);

	//加满率属性
	int (*pAddStuckPipeData)(StuckPipeInfo_T *info, int sum);
	int (*pGetFuelingRangeProportionByDate)(TimeYMD_T date, int *lowPoint, int *mediumPoint, int *highPoint);
	int (*pGetFuelQuantityByType)(TimeYMD_T date, int *point92, int *point95, int *point98);
	int (*pGetOliGunSum)(TimeYMD_T date);
	int (*pGetOliGunFrequency)(TimeYMD_T date, OliGunInfo_T *info);
	//告警
	int (*pGetIntelligentAlarm)(TimeYMD_T start, TimeYMD_T end, int page, int num, int type, int *total, DeviceAlarmInfo_T *alarmList);
	int (*pGetAlarmProportionAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, DeviceAlarmCount_T *AlarmCount);
	int (*pGetAlarmPeriodAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, int count, DeviceAlarmCount_T *AlarmCount);

	int (*pGetAttendanceHistoryList)(TimeYMD_T startTime,TimeYMD_T endTime, int startPage, int num,char *fuzzyStr,int *sum, AttendanceAtt_T *info);
	int (*pGetAttendanceHistorySum)(TimeYMD_T startTime,TimeYMD_T endTime,char *fuzzyStr,int *num);
	int (*pGetAttendanceRecord)( TimeYMD_T startTime,TimeYMD_T endTime, int page,int num,  int *sum,AttendanceAtt_T *info);
	int (*pGetAttendanceRecordSum)( TimeYMD_T startTime,TimeYMD_T endTime,   int *num);
	int (*pGetVisitorsInfo)(TimeYMD_T startTime,TimeYMD_T endTime,int page,int num, int *sum,VIPCustomer_T *info );
	//客流
	int (*pGetVolumeOfCommutersNum)(TimeYMD_T time,LeaveAndEnterNum_T *info);
	int (*pGetVolumeOfCommutersOfLine)(TimeYMD_T startTime,TimeYMD_T endTime,int num,int *sum,LeaveAndEnterNum_T *info);
	int (*pGetVolumeOfCommutersOfSex)(TimeYMD_T startTime,TimeYMD_T endTime,int num,VolumeOfCommutersOfSex_T *info);
	int (*pGetVolumeOfCommutersOfAge)(TimeYMD_T startTime,TimeYMD_T endTime,int num,VolumeOfCommutersOfAge_T *info);
	//高温回调
	HighTempFunc highTempFun;
	//告警推送
	int (*pPushAlarmInfo)(AlarmInfo_T *msg);
	int usrID; //0x0000ffff 
	int sessionid;
	char usrName[128];
}AdpAPIHandle_T;


#define APIISNULL(cbFun) if(cbFun == NULL){DF_ERROR("Param Callback Function is NULL"); return;}

_OUT_ int AdpRegistHandle(AdpAPIHandle_T *handle,char *usrName,char *passwd);
_OUT_ int AdpUnRegistHandle(AdpAPIHandle_T *handle);
_OUT_ int AdpHandleInit();
_OUT_ int AdpHandleUnInit();


#ifdef __cplusplus
}
#endif

#endif
