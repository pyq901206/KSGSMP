#ifndef __JSONMAP_H__
#define __JSONMAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "cjson.h"
#include "usrmanage.h"
#include "devicemanage.h"
#include "deviceinfo.h"
#include "publickey.h"
#include "timemanage.h"
#include "messagemanage.h"
#include "humandataanalysis.h"
#include "hkManage.h"
#include "dataanalysisout.h"

int GetGradeLevel(int gradescore);

cJSON* JsonMapDeviceInfoMake(DeviceInfo_T *info);
int JsonMapDeviceInfoPares(cJSON *conf,DeviceInfo_T *info);
cJSON* JsonMapUsrInfoMake(UsrInfo_T *info,int num);
int JsonMapUsrInfoPares(cJSON *conf,UsrInfo_T *info);
int JsonMapCameraInfoPares(cJSON *data, UserRegistInfo_T *registInfo, char *cameraId, int *payType, int *recType, int *coverRecDatas);
int JsonMapUsrPasswdPares(cJSON *data,char *username,char *oldPassword,char *newPassword);
cJSON *JsonMapGetUserInfoMake(UsrInfo_T info);

int JsonMapUsrInfoListMake(cJSON *root, int num, UsrInfo_T *info);
int JsonMapCameraInfoListMake(cJSON *root, int num, DeviceStorageInfo_T *deviceInfo, char *accountName, char *endpointSuffix);
int JsonMapGetCameraInfoListFromUserMake(cJSON *root, char *loginUsr, int num, DeviceStorageInfo_T *deviceInfo);
int JsonMapCameraInfoMake(cJSON *respData, char *deviceID, DeviceStorageInfo_T *deviceInfo, char *accountName, char *endpointSuffix, bool overduestatus);
int JsonMapNotifyOffLineMake(char *desttopic,char *msg);
int JsonMapLoginOutMake(char *msg);
int JsonMapBindCameraFailedMake(cJSON *root, char *existingUsers);
cJSON* JsonMapTimeManageConfigMake(TimeConf_T *info);
int JsonMapTimeManageConfigPares(cJSON *conf,TimeConf_T *info);
int JsonMapShareDevicePares(cJSON *data, char *did,char *shareUsr,char *curUsr);
int JsonMapUnShareDevicePares(cJSON *data, char *did,char *shareUsr,char *curUsr);
int JsonMapShareUserUnBindCameraPares(cJSON *data, char *shareUsr, char *did);
int JsonMapCameraInfoArraryPares(cJSON *data, char deviceId[][128]);
int JsonMapPushAlarmPares(cJSON *data, AlarmMsg_T *info);
int JsonMapGetDeviceInfoList(cJSON *root, DeviceStorageInfo_T *deviceList, int num);
int JsonMapPushAlarmMake(char *msg, DeviceAlarm_T	*deviceAlarm);

//DVR
cJSON* JsonMapDvrInfoMake(HKDVRParam_T * Param);
cJSON* JsonMapDvrListInfoMake(HKDVRParam_T * Param, int Dvrnum);
int JsonMapDvrInfoPares(cJSON * conf,HKDVRParam_T * Param);

//告警
cJSON *JsonMapsdkDeviceAlarmGetMake(DeviceAlarmParam_T *alarmParam);
int JsonMapsdkDeviceAlarmSetPares(cJSON *data, DeviceAlarmParam_T *alarmParam);
int JsonMapGetAlarmProportionAnalysis(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend);
cJSON *JsonMapGetAlarmProportionAnalysisMake(DeviceAlarmCount_T *AlarmCount);
int JsonMapGetAlarmPeriodAnalysis(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend, int *count);
cJSON *JsonMapGetAlarmPeriodAnalysisMake(int count, DeviceAlarmCount_T *AlarmCount);
int JsonMapGetIntelligentAlarm(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend, int *page, int *num, int *type);
cJSON* JsonMapHkDeviceAlarmInfoMake(DeviceAlarmParam_T *info);
int JsonMapDeviceAlarmInfoPares(cJSON *conf, DeviceAlarmParam_T *info);

//超脑设备
int JsonMapsdkDeviceAddInfoPares(cJSON *conf, HKDVRParam_T *Param);
int JsonMapsdkDeviceDelInfoPares(cJSON *conf, HKDVRParam_T *Param);
cJSON* JsonMapGetsdkDeviceList(HKDVRParam_T *dvrParam, int num);
int JsonMapsdkDeviceSetInfoPares(cJSON *conf, DvrParam_T *Info);
int JsonMapsdkIpcamSetInfoPares(cJSON *conf, IpcamParam_T *Info);
int JsonMapGetFrontageVehiclesSumPares(cJSON *data, TimeYMD_T *startTime, TimeYMD_T *endTime);
cJSON *JsonMapGetFrontageVehiclesSumMake(StreetTypeAttr *streetAttr);
int JsonMapGetEntranceVehiclesSumPares(cJSON *data, TimeYMD_T *startTime, TimeYMD_T *endTime);
cJSON *JsonMapGetEntranceVehiclesSumMake(ProgressRate_T *progressAttr);
int JsonMapGetEntranceVehiclesListPares(cJSON *data, StationVehicles_T *Vehicles, int *page, int *num);
cJSON *JsonMapGetEntranceVehiclesListMake(StationVehiclesInfo_T *VehiclesInfo, int num, int total);
int JsonMapGetEntranceHistoryListPares(cJSON *data, int *page, int *num, char *location, char *plateNo);
cJSON *JsonMapGetEntranceHistoryListMake(StationHistoryInfo_T *historyInfo, int num, int total);

int JsonMapGetEntranceVehiclesCurList(cJSON *data, int *num);
cJSON *JsonMapGetEntranceVehiclesCurListMake(int num, StationVehiclesInfo_T *VehiclesInfo);
int JsonMapPushDeviceEntrancesMake(char *msg, DeviceEntrance_T *info, char *cartype, int count);
int JsonMapPushDeviceRateProgressMake(char *msg, int FrontageTotal, int EntranceTotal);
cJSON *JsonMapEntranceServiceAnalysisMake(int *data, int days);
int JsonMapGetEntranceServiceAnalysis(cJSON *data, TimeYMD_T *time);
int JsonMapGetEntrancePlateAnalysis(cJSON *data, TimeYMD_T *time1, TimeYMD_T *time2, char *location);
cJSON *JsonMapEntrancePlateAnalysisMake(PlaceAttribution_T PlaceAttr);
int JsonMapGetEntranceHistoryCount(cJSON *data, TimeYMD_T *time1, TimeYMD_T *time2, char *plateNo);
cJSON *JsonMapEntranceHistoryCountMake(int *data);
int JsonMapRefuelingTypeAnalysis(cJSON *data, TimeYMD_T *time1, TimeYMD_T *time2);
cJSON *JsonMapRefuelingTypeAnalysisMake(RefuelingType_T *Refueling);
cJSON *JsonMapOutboundVehiclesAnalysisMake(int count, ProgressRate_T *Progress1, ProgressRate_T *Progress2, ProgressRate_T *Progress1total, ProgressRate_T *Progress2total);
int JsonMapResidenceTimeAnalysis(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend);
cJSON *JsonMapResidenceTimeAnalysisMake(ResidenceTime_T *Residence);
int JsonMapRefuelingEfficiencyAnalysis(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend, int *days);
cJSON *JsonMapRefuelingEfficiencyAnalysisMake(Refueling_T *Residence, int count);
int JsonMapVehicleTrajectoryAnalysis(cJSON *data, int *page, int *num, TimeYMD_T *timestart, TimeYMD_T *timeend);
cJSON *JsonMapVehicleTrajectoryAnalysisMake(StationVehiclesInfo_T *VehiclesInfo, int number, int total);
int JsonMapOutboundVehiclesAnalysis(cJSON *data, TimeYMD_T *time1start, TimeYMD_T *time1end, TimeYMD_T *time2start, TimeYMD_T *time2end, int *days);
cJSON *JsonMapGetIntelligentAlarmMake(DeviceAlarmInfo_T    *alarmList, int number, int total);
int JsonMapVehicleTrajectoryRoute(cJSON *data, char *plateNo);
cJSON *JsonMapVehicleTrajectoryRouteMake(DeviceTrajectory_T *Trajectory);

int JsonMapProportionOfVehicleBehavior(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend);
cJSON *JsonMapProportionOfVehicleBehaviorMake(DeviceWashProportion_T *ProportionInfo);
int JsonMapGetProportionOfCarWashing(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend);
cJSON *JsonMapProportionOfCarWashingMake(DeviceWashStatistics_T *WashStatistics);
int JsonMapAbnormalVehicleList(cJSON *data, TimeYMD_T *timestart, TimeYMD_T *timeend, int *page, int *num);
cJSON *JsonMapAbnormalVehicleListMake(AbnormalVehicleInfo_T *Abnormal, int number, int total);

//人脸
int JsonMapGetVisitorsPares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime,int *page,int *num);
int JsonMapGetAttendanceListSumPares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime,char *fuzzyStr);
int JsonMapGetAttendancePares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime,int *page, int *num);
int JsonMapGetAttendanceSumPares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime);
cJSON *JsonMapGetVisitorsListMake(int sum,VIPCustomer_T *VisitorsInfo, int num);
int JsonMapGetAttendanceListPares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime, int *page, int *num,char *fuzzyStr);
cJSON *JsonMapGetAttendanceHistoryListMake(int sum,AttendanceAtt_T *AttendanceInfo, int num);
cJSON *JsonMapGetAttendanceSumMake(int num);
cJSON *JsonMapGetAttendanceHistorySumMake( int num);
int JsonMapGetVolumeOfCommutersNumInfoPares(cJSON *data, TimeYMD_T *time);
cJSON *JsonMapGetVolumeOfCommutersNumMake(LeaveAndEnterNum_T *LeaveAndEnterInfo);
int JsonMapGetVolumeOfCommutersOfLineInfoPares(cJSON *data, TimeYMD_T *starttime,TimeYMD_T *endtime,int *num);
cJSON *JsonMapGetVolumeOfCommutersOfLineMake(LeaveAndEnterNum_T *LeaveAndEnterInfo,int num);
cJSON *JsonMapGetVolumeOfCommutersOfSexMake(VolumeOfCommutersOfSex_T *info,int num);
cJSON *JsonMapGetVolumeOfCommutersOfAgeMake(VolumeOfCommutersOfAge_T *info,int num);


#ifdef __cplusplus
}
#endif

#endif

