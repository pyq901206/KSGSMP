#ifndef _mysqlctrl_h
#define _mysqlctrl_h
#include "common.h"
#include "usrmanage.h"
#include "devicemanage.h"
#include "publickey.h"
#include "mysqlcli/mysql.h"
#include "dataanalysisout.h"
#include "fuelquantity.h"

#define MYSQL_BACKUP_TIME (1 * 24 * 3600) 
#define MYSQL_DB_NAME "gzsmartdb"
#define MYSQL_TABLE_USRINFO "usrinfo"
#define MYSQL_TABLE_CAMERA "deviceinfo"
#define MYSQL_TABLE_FRONTAGE "frontagedata"  	        //临街数据库
#define MYSQL_TABLE_ENTRANCE "entrancedata"  	        //入口数据库
#define MYSQL_TABLE_ALARM    "alarmdata"     	        //告警数据库
#define MYSQL_TABLE_VIPCar   "vipcardata"    	        //VIP车辆数据库
#define MYSQL_TABLE_WASHCar   "washcardata"    	        //洗车车辆数据库
#define MYSQL_TABLE_CARTRAJECTORY   "cartrajectory"    	//车辆轨迹数据库
#define MYSQL_TABLE_ISLAND    "islandcardata"    	    //加油车辆数据库
#define MYSQL_TABLE_FUELQUANTITY "fuelquantity"         //加满率
#define MYSQL_TABLE_ATTENDDANCE "attenddance"	        //无感考勤
#define MYSQL_TABLE_VISITORS "visitorsdata"             //访客记录
#define MYSQL_TABLE_PASSENGERFLOW "passengerflowdata"   //入店客流
#define MYSQL_TABLE_PLATNOAREA "platnoareaninfo"

//出入口
//.....唯一token 车牌号 车牌颜色 车牌所属地 车辆型号 车辆颜色 入口时间 出口时间 车牌图URL
//临街
//.....唯一token   车牌颜色 车辆型号 抓拍时间
//洗车
//.....唯一token   车牌号 入口时间 出口时间 
//异常告警记录
//.....唯一token 告警类型	告警时间 告警地点
//VIP车辆
//.....唯一token   车牌号 近期加油时间 加油频次


#define MYSQL_SERVER_ADDRESS	"127.0.0.1"//"mqtt.inewcamcloud.com"//"113.247.222.69"  //"10.67.3.58" //10.67.2.30
#define MYSQL_SERVER_USR		"root"
#define MYSQL_SERVER_PASSWD		"gaozhi2014" //"gaozhi123456"//"gAOzHI2018" //"123456"

typedef struct _MYSQL_HANDLE_T
{
	pthread_mutex_t 	selfMutex;
	MYSQL *handle;
}MysqlHandle_T;

typedef struct _MySqlManage_T{
	int (*pGetUsrInfoList)(UsrInfo_T *info);
	int (*pGetUsrInfoListNum)(int *num);
	int	(*pAddUsrInfo)(UsrInfo_T *info);
	int	(*pDelUsrInfo)(UsrInfo_T *info);
	int	(*pGetUsrInfo)(UsrInfo_T *info);
	int (*pSetUsrInfo)(UsrInfo_T *info);
	int (*pModifyUsrBindInfocb)(UsrInfo_T *info, char *oldusrname);
	int (*pAddDevice)(DeviceStorageInfo_T *info);
	int (*pDelDevice)(char *deviceID);
	int (*pGetDeviceInfo)(DeviceStorageInfo_T *info);
	int (*pSetDeviceInfo)(DeviceStorageInfo_T *info);		
	int (*pGetDeviceInfoListNum)(int *num);
	int (*pGetDeviceInfoList)(DeviceStorageInfo_T *info);
	//车辆数据
	//获取临街车辆x日数据（车辆类型、车辆归属地、车牌颜色）            //计算环比数据
	int (*pAddFrontageInfo)(DeviceStreet_T *info);
	int (*pGetFrontageListSum)();
	int (*pGetFrontageListSumByTime)(TimeYMD_T start, TimeYMD_T end);
	int (*pGetFrontageList)(int startPage, int num, DeviceStreet_T *info);
	int (*pGetFrontageListByTime)(int startPage, int num, TimeYMD_T start, TimeYMD_T end, DeviceStreet_T *info);
	int (*pGetLatestFrontageInfo)(char *plateNo, DeviceStreet_T *info);
	int (*pSetFrontageInfo)(char *token, DeviceStreet_T *info);
	int (*pGetFrontageListSumBySearchtype)(TimeYMD_T start, TimeYMD_T end, Vehicle_Type_E Vehicle_Type,Plate_Color_E Plate_Color);
	
	//获取进站车辆x日数据             //计算环比数据
	int (*pAddEntranceInfo)(DeviceEntrance_T *info);
	int (*pGetEntranceListSum)();
	int (*pGetEntranceListSumByTime)(TimeYMD_T start, TimeYMD_T end);
	int (*pGetEntranceListSumBySerchType)(TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo);
	int (*pGetEntranceListSumBySerchType2)(TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo);
	int (*pGetEntranceList)(int startPage, int num, DeviceEntrance_T *info);
	int (*pGetEntranceListByTime)(int startPage, int num, TimeYMD_T start, TimeYMD_T end, DeviceEntrance_T *info);
	int (*pGetEntranceListBySerchType)(int startPage, int num, TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo, DeviceEntrance_T *info);
	int (*pGetEntranceListBySerchType2)(int startPage, int num, TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo, DeviceEntrance_T *info);
	int (*pGetLatestEntranceInfo)(char *plateNo, DeviceEntrance_T *info);
	int (*pSetEntranceInfo)(char *token, DeviceEntrance_T *info);
	int (*pGetEntranceListSumByLocation)(TimeYMD_T start, TimeYMD_T end,char * location);
	int (*pGetEntranceResidenceByTime)(TimeYMD_T start, TimeYMD_T end, ResidenceType_T type);
	int (*pGetEntranceRefuelingEfficiency)(TimeYMD_T start, TimeYMD_T end, Refueling_T *refueling);
	int (*pGetEntranceResidenceListInfo)(int startPage, int num, TimeYMD_T start, TimeYMD_T end, DeviceEntranceMysql_T *info);

	//异常告警
	int (*pAddAlarmInfo)(DeviceAlarm_T *info);
	int (*pGetAlarmListSum)();
	int (*pGetAlarmList)(int startPage, int num, DeviceAlarm_T *info);
	int (*pGetAlarmListBySearchType)(int startPage, int num, TimeYMD_T start, TimeYMD_T end,AlarmLevelSearchType_E type,DeviceAlarm_T *info);
	int (*pGetAlarmListSumBySearchType)(TimeYMD_T start, TimeYMD_T end,AlarmLevelSearchType_E type);
	
	//VIP车辆
	int (*pAddVipCarInfo)(DeviceVipCar_T *info);
	int (*pGetVipCarListSum)();
	int (*pGetVipCarList)(int startPage, int num, DeviceVipCar_T *info);
	int (*pGetVipCarInfoByPlateNo)(char *platNo, DeviceVipCar_T *info);
	int (*pSetVipCarInfo)(char *token, DeviceVipCar_T *info);
	int (*pGetVipCarListBySerchType)(int startPage, int num, TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo,DeviceVipCar_T *info);
	int (*pGetVipCarListSumBySerchType)(TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo);

	//洗车表
	int (*pAddWashCarInfo)(DeviceWash_T *info);
	int (*pGetWashCarListSumByTime)(TimeYMD_T start, TimeYMD_T end);
	int (*pGetLastWashCarInfo)(char *plateNo, DeviceWash_T *info);
	int (*pSetWashCarInfo)(char *token, DeviceWash_T *info);

	//加油岛表
	int (*pAddIsLandInfo)(DeviceOilLand_T *info);
	int (*pGetIsLandListSumByTime)(TimeYMD_T start, TimeYMD_T end);
	int (*pGetLastIsLandInfo)(char *plateNo, DeviceOilLand_T *info);
	int (*pSetIsLandInfo)(char *token, DeviceOilLand_T *info);

	//车辆轨迹
	int (*pAddTrajectoryInfo)(DeviceTrajectory_T *info);
	int (*pGetTrajectoryInfo)(char *token, DeviceTrajectory_T *info);
	int (*pUpdateTrajectoryInfo)(char *token, DeviceTrajectory_T *info);
	int (*pGetaddWithoutWashNum)(TimeYMD_T startTime,TimeYMD_T endTime);
	int (*pGetWashWithoutaddNum)(TimeYMD_T startTime,TimeYMD_T endTime);
	int (*pGetaddAndwashNum)(TimeYMD_T startTime,TimeYMD_T endTime);
	int (*pGetwashAndaddNum)(TimeYMD_T startTime,TimeYMD_T endTime);

	//无感考勤
	int (*pAddAttendanceInfo)(AttendanceAtt_T *info);
	int (*pGetAttendanceHistorySum)(TimeYMD_T startTime,TimeYMD_T endTime, char *fuzzyStr);
	int (*pGetAttendanceHistoryList)(TimeYMD_T startTime,TimeYMD_T endTime, int startPage, int num,char *fuzzyStr, AttendanceAtt_T *info);
	int (*pGetLatestAttendanceInfo)(char *usrId, AttendanceAtt_T *info);
	int (*pGetFirstAttenddanceInfoByDate)(TimeYMD_T startTime, TimeYMD_T endTime,int page,int num, AttendanceAtt_T *info);
	int(*pGetFirstAttenddanceInfoSumByDate)(TimeYMD_T startTime, TimeYMD_T endTime);

	int (*pGetLastVisitorsInfoByDate)(TimeYMD_T startTime, TimeYMD_T endTime,int page,int num, VIPCustomer_T *info);
	int(*pGetLastVisitorsInfoSumByDate)(TimeYMD_T startTime, TimeYMD_T endTime);

	//访客记录
	int (*pAddVisitorsInfo)(VIPCustomer_T *info);	
	int (*pGetVisitorsListSum)();
	int (*pGetVisitorsList)(int startPage, int num, VIPCustomer_T *info);
	int (*pGetLatestVisitorsInfo)(char *name, VIPCustomer_T *info);
	//入店客流
	int (*pAddPassengerFlowInfo)(TimeYMD_T time, VolumeOfCommuters_T *info);	
	int (*pGetPassengerFlowListSum)(TimeYMD_T starttime);
	int (*pGetPassengerFlowList)(int startPage, int num, VolumeOfCommuters_T *info);
	int (*pGetPassengerFlowLineOfHour)(TimeYMD_T startTime,TimeYMD_T endTime,int num,LeaveAndEnterNum_T *info);
	int (*pGetPassengerFlowLineOfDay)(TimeYMD_T startTime,TimeYMD_T endTime,int num,LeaveAndEnterNum_T *info);
	int (*pGetPassengerFlowSumOfSex)(TimeYMD_T startTime,TimeYMD_T endTime,int num,VolumeOfCommutersOfSex_T *info);
	int (*pGetPassengerFlowSumOfAge)(TimeYMD_T startTime,TimeYMD_T endTime,int num,VolumeOfCommutersOfAge_T *info);
	//车牌信息查找
	int (*pGetPlatNoAreaId)(char * province_code, char *city_code,int * AreaId);
	int (*pGetPlatNoAreaById)(int AreaId,char * province_name,char * city_name);
	int (*pAddStuckPipeData)(StuckPipeInfo_T *info);
	int (*pDelStuckPipeDataByDate)(TimeYMD_T date);
	//油品分析
	int (*pGetFillRateSumByType)(TimeYMD_T date, FillRateType_E findType);
	//油品分析
	int (*pGetFuelQuantityByType)(TimeYMD_T date, FuelQuantityType_E findType, int *quantity);
	//提枪次数分析
	int (*pGetOliGunSum)(TimeYMD_T date);
	int (*pGetOliGunFrequency)(TimeYMD_T date, OliGunInfo_T *info);
}MySqlManage_T;

int MySqlManageInit(MySqlManage_T *mysqlHandle);
int MySqlManageUnInit(MySqlManage_T *mysqlHandle);
int MySqlManageRegister(MySqlManage_T *mysqlHandle);
int MySqlManageUnRegister(MySqlManage_T *mysqlHandle);


#endif


