#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "dataanalysisout.h"
#ifndef __VEHICLEDATAANALYSISMANAGE_H__
#define __VEHICLEDATAANALYSISMANAGE_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _DataAnalysisManage_T{		
	int (*pAddVehicleInfo)(DataAtt_T *dataInfo); //添加抓拍信息到存储
	int (*pGetVehiclesTerritory)(char *plateNo); //数据库获取归属地
	//拐入率列表数据
	int (*pGetEntranceVehiclesCurList)(int num, StationVehiclesInfo_T *VehiclesInfo);
	//临街车辆分析
	int (*pGetFrontageVehiclesSum)(TimeYMD_T startTime, TimeYMD_T endTime, StreetTypeAttr *streetAttr);   //获取临街车辆x时间段内车辆总数	
	//进站率分析
	int (*pGetEntranceVehiclesSum)(TimeYMD_T startTime, TimeYMD_T endTime, ProgressRate_T *progressAttr);
	//站内车辆详细数据!
	int (*pGetEntranceVehiclesList)(int page, int num, StationVehicles_T stationAttr, StationVehiclesInfo_T *VehiclesInfo, int *total);
	//站内车辆历史记录数据!
	int (*pGetEntranceHistoryList)(int page, int num, int *totalno, char *location, char *plateno, StationHistoryInfo_T *historyInfo);
	//站内单个车辆加油趋势
	int (*pGetEntranceHistoryCount)(TimeYMD_T startTime, TimeYMD_T endTime, char *plateNo, int *data);
	//站内车牌区域分析
	int (*pGetEntrancePlateAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, char *location, PlaceAttribution_T *PlaceAttr);
	//站内加油趋势分析
	int (*pGetEntranceServiceAnalysis)(TimeYMD_T Time, int days, int *data);
	//站内加油（汽油，柴油）车辆数量分析
	int (*pGetRefuelingTypeAnalysis)(TimeYMD_T Time1, TimeYMD_T Time2, RefuelingType_T *Refueling);
	//出站车辆分析
	int (*pGetOutboundVehiclesAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, int count, ProgressRate_T *Progress, ProgressRate_T *Progress1);
	//停留时间占比分析
	int (*pGetResidenceTimeAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, ResidenceTime_T *Residence);
	//异常车辆信息列表
	int (*pGetAbnormalVehicleList)(int page, int num, int *total, TimeYMD_T startTime, TimeYMD_T endTime, AbnormalVehicleInfo_T *Abnormal);
	//加油效率分析
	int (*pGetRefuelingEfficiencyAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, int count, Refueling_T *refueling);
	//车辆轨迹分析
	int (*pGetVehicleTrajectoryAnalysis)(int page, int num, int *totalno, StationVehiclesInfo_T *VehiclesInfo, TimeYMD_T timestart, TimeYMD_T timeend);
	//车辆轨迹路线
	int (*pGetVehicleTrajectoryRoute)(char *tokenid, DeviceTrajectory_T *Trajectory);
	//洗车占比分析
	int (*pGetProportionOfCarWashing)(TimeYMD_T startTime, TimeYMD_T endTime, DeviceWashStatistics_T *WashStatistics);
	//车辆行为占比分析
	int (*pGetProportionOfVehicleBehavior)(TimeYMD_T startTime, TimeYMD_T endTime, DeviceWashProportion_T *ProportionInfo);
	//智能告警列表数据
	int (*pGetIntelligentAlarm)(TimeYMD_T start, TimeYMD_T end, int page, int num, int type, int *total, DeviceAlarmInfo_T *alarmList);
	//告警占比数据
	int (*pGetAlarmProportionAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, DeviceAlarmCount_T *AlarmCount);
	//告警时段数据分析
	int (*pGetAlarmPeriodAnalysis)(TimeYMD_T startTime, TimeYMD_T endTime, int count, DeviceAlarmCount_T *AlarmCount);
}DataAnalysisManage_T;

typedef int (*HighTempFunc)(DeviceAlarm_T *info); //高温回调
typedef struct _HighTemperatureDetectionRegistHandle_T{	
	HighTempFunc highTempFun;   //alarm 结果抛送 请给所有的注册用户抛送
}HighTemperatureDetectionRegistHandle_T;

int VehicleDataAnalysisManageInit(DataAnalysisManage_T *handle);
int VehicleDataAnalysisManageUnInit(DataAnalysisManage_T *handle);
int VehicleDataAnalysisManageRegister(DataAnalysisManage_T *handle);
int VehicleDataAnalysisManageUnRegister(DataAnalysisManage_T *handle);
int HighTemperatureDetectionRegister(HighTemperatureDetectionRegistHandle_T *registerHandle);
int HighTemperatureDetectionUnRegister(HighTemperatureDetectionRegistHandle_T *registerHandle);

#ifdef __cplusplus
}
#endif

#endif

