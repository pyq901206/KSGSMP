#include "vehicledataanalysis.h"
#include "deviceinfo.h"
#include "timemanage.h"
#include "handlemanage.h"
#include "mysqlctrl.h"
#include "curl.h"
#include "cjson.h"
#include "base64.h"
#include "jsonmap.h"
#include "mqttapi.h"

static ENUM_ENABLED_E gInnerExitFlag = ENUM_ENABLED_FALSE;
typedef struct _InnerHandle_T
{
	HandleManage_T listHead;
	DeviceManageHandle_T innerDeviceHandle;
	MySqlManage_T innerSqlHandle;
	HandleManage_T highTempDetectionHead;
}InnerHandle_T;
static InnerHandle_T gInnerHandle;
static InnerHandle_T *GetInnerHandle()
{
	return &gInnerHandle;
}

typedef struct _VehicleCount_T
{
	int FrontageTotal;
	int EntranceTotal;
}VehicleCount_T, *PVehicleCount_T;
static VehicleCount_T VehicleCount = {0};

//内部缓存链表
typedef struct _DataAttList_T
{
	DataAtt_T dataAtt;
	pthread_mutex_t 	selfMutex;
}DataAttList_T;

static int GetPlateColorByType(char *data, int type)
{
	switch(type)
	{
		case yellow:
			sprintf(data, "%s", "黄色");
			break;
		case blue:
			sprintf(data, "%s", "蓝色");
			break;
		case green:
			sprintf(data, "%s", "绿色");
			break;
		default:
			sprintf(data, "%s", "其他");
			break;
	}
	
	return KEY_TRUE;
}

static int GetVehiclesByType(char *data, int type)
{
	switch(type)
	{
		case largeBus:
			sprintf(data, "%s", "大客车");
			break;
		case truck:
			sprintf(data, "%s", "货车");
			break;
		case vehicle:
			sprintf(data, "%s", "轿车");
			break;
		case van:
			sprintf(data, "%s", "面包车");
			break;
		default:
			sprintf(data, "%s", "其他");
			break;
	}
	
	return KEY_TRUE;
}

static int GetDaysByTime(int year, int month)
{
	int days = 0;
	switch(month)
	{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			days = 31;
			break;
		case 4:
		case 6:
		case 9:
		case 11:
			days = 30;
		default:
			if(year % 4 == 0 && year % 100 != 100 || year % 400 == 0)
				days = 29;
			else
				days = 28;
	}
	return days;
}

static int PushTurnInRateMqttMsg(int count, DeviceEntrance_T *entranceInfo)
{
	if(NULL == entranceInfo){
		return KEY_FALSE;
	}
	
	//推送实时消息到web
	char sendMsg[512] = {0};
	char cartype[64] = {0};
	GetVehiclesByType(cartype, entranceInfo->vehicleType);
	JsonMapPushDeviceEntrancesMake(sendMsg, entranceInfo, cartype, count);
	pushMqttMsg(MAIN_SERVER_PUSH_TOPIC, sendMsg, 0);
	return KEY_TRUE;
}

static int PushRateProgressMqttMsg() 
{
	char sendMsg[512] = {0};
	VehicleCount_T *fd = &VehicleCount;	
	JsonMapPushDeviceRateProgressMake(sendMsg, fd->FrontageTotal, fd->EntranceTotal);
	pushMqttMsg(MAIN_SERVER_PUSH_TOPIC, sendMsg, 0);
	return KEY_TRUE;
}

void *ThreadRateOfProgress(void *argv)
{
	Time_T tv = {0};
	static int day = 0;
	VehicleCount_T *fd = &VehicleCount;	

	GetLocalTime(&tv);
	day = tv.curTime.day;
	while(gInnerExitFlag)
	{
		GetLocalTime(&tv);
		if(tv.curTime.day != day)
		{
			fd->EntranceTotal = 0;
			fd->FrontageTotal = 0;
		}
		else
		{
			usleep(1*1000*1000);
		}
		day = tv.curTime.day;
	}
	return NULL;
}

static int highTempDetectionLoopDoFunc(void *handle,int argc,void *arg[])
{
	DeviceAlarm_T *gDeviceAlarm = NULL;
	if(handle == NULL || 0 == argc)
	{
		return KEY_FALSE;
	}
	gDeviceAlarm = (DeviceAlarm_T *)arg[0];
	HighTemperatureDetectionRegistHandle_T *curHandle = (HighTemperatureDetectionRegistHandle_T *)handle;
	if(NULL == curHandle->highTempFun){
		return KEY_FALSE;
	}
	curHandle->highTempFun(gDeviceAlarm);
	return KEY_TRUE;
}

//缓存数据同步到数据库，避免频繁写数据库
static void *MemoryDataToDatabasePro(void *argv)
{
	void *handle = NULL;
	int iret = -1, num = 0;
	long time1, time2;
	DataAttList_T *listInfo = NULL;
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	VehicleCount_T *fd = &VehicleCount;	

	for (;gInnerExitFlag;)
	{	
		HandleManage_T *listHead = &GetInnerHandle()->listHead;
		if (NULL == listHead || NULL == mysqlHandle)
		{
			usleep(1 * 1000 * 1000);
			continue;
		}
		for (;gInnerExitFlag; )
		{
			//获取内存数据
			handle = HandleManageGetNextHandle(listHead);
			if (NULL == handle)
			{
				usleep(2 * 1000 * 1000);
				break;
			}
						
			listInfo = (DataAttList_T *)handle;
			//存入数据库 
			MUTEX_LOCK(&listInfo->selfMutex);
			DF_DEBUG("-----------type:%d", listInfo->dataAtt.type);
			if(listInfo->dataAtt.type == DEVICE_TYPE_Frontage)
			{
				DeviceStreet_T info = {0};
				iret = mysqlHandle->pGetLatestFrontageInfo(listInfo->dataAtt.DataUnion.deviceStreet.plateNo, &info);
				if(KEY_TRUE == iret) //数据库已经记录
				{
					time1 = TimeLocalYMD2Sec(&listInfo->dataAtt.DataUnion.deviceStreet.time);
					time2 = TimeLocalYMD2Sec(&info.time);
					if(time1 - time2 > 30 * 60) //时间超过半小时,重新记录
					{
						fd->FrontageTotal++;
						PushRateProgressMqttMsg();
						iret = mysqlHandle->pAddFrontageInfo(&listInfo->dataAtt.DataUnion.deviceStreet);
					}
					else//更新抓拍时间
					{
						iret = mysqlHandle->pSetFrontageInfo(info.snapToken, &listInfo->dataAtt.DataUnion.deviceStreet);
					}
				}
				else 
				{
					fd->FrontageTotal++;
					PushRateProgressMqttMsg();
					iret = mysqlHandle->pAddFrontageInfo(&listInfo->dataAtt.DataUnion.deviceStreet);
				}
			}
			else if(listInfo->dataAtt.type == DEVICE_TYPE_Entrance)
			{
				if(strlen(listInfo->dataAtt.DataUnion.deviceEntrance.plateNo) > 0)
				{				
					DeviceEntrance_T info = {0};
					//DF_DEBUG("plateNo: %s", listInfo->dataAtt.DataUnion.deviceEntrance.plateNo);
					iret = mysqlHandle->pGetLatestEntranceInfo(listInfo->dataAtt.DataUnion.deviceEntrance.plateNo, &info);
					if(KEY_TRUE == iret)  //数据库已经记录
					{
						time1 = TimeLocalYMD2Sec(&listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart);
						time2 = TimeLocalYMD2Sec(&info.vehicleStart);
						if(time1 - time2 > 60 * 60) //时间超过1小时,重新记录
						{
							//进站
							fd->EntranceTotal++;
							//查找当前月份车辆加油次数
							int days = 0;
							TimeYMD_T time1 = {0};
							TimeYMD_T time2 = {0};
							VehicleSearchInfo_T serchInfo = {0};
							
							serchInfo.vehicleType = -1;
							serchInfo.type = VehicleSearch_TYPE_Fuzzy;
							memcpy(&serchInfo.fuzzyStr, &listInfo->dataAtt.DataUnion.deviceEntrance.plateNo, 128);
							days = GetDaysByTime(listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart.year, listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart.month);
							memcpy(&time1, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, sizeof(TimeYMD_T));
							memcpy(&time2, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, sizeof(TimeYMD_T));
							time1.day = 1;
							time1.hour= 0;
							time1.min = 0;
							time1.sec = 0;
							
							time2.day = days;
							time2.hour= 23;
							time2.min = 59;
							time2.sec = 59;
							iret = mysqlHandle->pAddEntranceInfo(&listInfo->dataAtt.DataUnion.deviceEntrance);
							//推送mqtt实时消息
							num  = mysqlHandle->pGetEntranceListSumBySerchType2(time1, time2, serchInfo);
							PushTurnInRateMqttMsg(num, &listInfo->dataAtt.DataUnion.deviceEntrance);
							PushRateProgressMqttMsg();
							
							//添加进车辆轨迹表
							DeviceTrajectory_T traj = {0};
							memcpy(&traj.snapToken, &listInfo->dataAtt.DataUnion.deviceEntrance.snapToken, 128);
							memcpy(&traj.plateNo, &listInfo->dataAtt.DataUnion.deviceEntrance.plateNo, 128);
							memcpy(&traj.arrivalstartdate, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, 128);
							memcpy(&traj.arrivalenddate, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, 128);
							memcpy(&traj.washstartdate, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, 128);
							memcpy(&traj.washenddate, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, 128);
							iret = mysqlHandle->pAddTrajectoryInfo(&traj);
						}			
						else if(0 == memcmp(&info.vehicleStart, &info.vehicleEnd, sizeof(TimeYMD_T)))
						{
							if(time1 - time2 < 5 * 60) //小于5分钟内
							{
								//设置更新结束时间
								iret = mysqlHandle->pSetEntranceInfo(info.snapToken, &listInfo->dataAtt.DataUnion.deviceEntrance);
								//车辆轨迹表更新
								DeviceTrajectory_T traj = {0};
								iret = mysqlHandle->pGetTrajectoryInfo(info.snapToken, &traj);
								memcpy(&traj.arrivalenddate, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleEnd, sizeof(TimeYMD_T));
								iret = mysqlHandle->pUpdateTrajectoryInfo(info.snapToken, &traj);
							}
						}
						else 
						{
							time1 = TimeLocalYMD2Sec(&listInfo->dataAtt.DataUnion.deviceEntrance.vehicleEnd);
							time2 = TimeLocalYMD2Sec(&info.vehicleEnd);
							if(time1 - time2 > 5 * 60) //5分钟内重复抓到，代表拥堵，设置更新结束时间
							{
								//设置更新结束时间 
								iret = mysqlHandle->pSetEntranceInfo(info.snapToken, &listInfo->dataAtt.DataUnion.deviceEntrance);
								//车辆轨迹表更新
								DeviceTrajectory_T traj = {0};
								iret = mysqlHandle->pGetTrajectoryInfo(info.snapToken, &traj);
								memcpy(&traj.arrivalenddate, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleEnd, sizeof(TimeYMD_T));
								iret = mysqlHandle->pUpdateTrajectoryInfo(info.snapToken, &traj);
							}
						}
					}
					else
					{
						fd->EntranceTotal++;
						#if 0
						//vip表更新
						DeviceVipCar_T vipCarInfo = {0};
						vipCarInfo.conut += 1;
						memcpy(&vipCarInfo.vehicle, &listInfo->dataAtt.DataUnion.deviceEntrance, sizeof(DeviceEntrance_T));
						memcpy(&vipCarInfo.time, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, sizeof(TimeYMD_T));
						#endif
						//查找当前月份车辆加油次数
						int days = 0;
						TimeYMD_T time1 = {0};
						TimeYMD_T time2 = {0};
						VehicleSearchInfo_T serchInfo = {0};
						
						serchInfo.vehicleType = -1;
						serchInfo.type = VehicleSearch_TYPE_Fuzzy;
						memcpy(&serchInfo.fuzzyStr, &listInfo->dataAtt.DataUnion.deviceEntrance.plateNo, 128);
						days = GetDaysByTime(listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart.year, listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart.month);
						memcpy(&time1, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, sizeof(TimeYMD_T));
						memcpy(&time2, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, sizeof(TimeYMD_T));
						time1.day = 1;
						time1.hour= 0;
						time1.min = 0;
						time1.sec = 0;
						
						time2.day = days;
						time2.hour= 23;
						time2.min = 59;
						time2.sec = 59;
						//添加到进站表
						iret = mysqlHandle->pAddEntranceInfo(&listInfo->dataAtt.DataUnion.deviceEntrance);
						//推送mqtt实时消息
						num  = mysqlHandle->pGetEntranceListSumBySerchType2(time1, time2, serchInfo);
						PushTurnInRateMqttMsg(num, &listInfo->dataAtt.DataUnion.deviceEntrance);
						PushRateProgressMqttMsg();
						//添加进车辆轨迹表
						DeviceTrajectory_T traj = {0};
						memcpy(&traj.snapToken, &listInfo->dataAtt.DataUnion.deviceEntrance.snapToken, 128);
						memcpy(&traj.plateNo, &listInfo->dataAtt.DataUnion.deviceEntrance.plateNo, 128);
						memcpy(&traj.arrivalstartdate, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, 128);
						memcpy(&traj.arrivalenddate, &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, 128);
						memcpy(&traj.washstartdate,  &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, 128);
						memcpy(&traj.washenddate,    &listInfo->dataAtt.DataUnion.deviceEntrance.vehicleStart, 128);
						iret = mysqlHandle->pAddTrajectoryInfo(&traj);
					}
				}
			}
			else if(listInfo->dataAtt.type == DEVICE_TYPE_Wash)
			{
				if(strlen(listInfo->dataAtt.DataUnion.deviceWash.plateNo) > 0)
				{
					DeviceWash_T info = {0};
					iret = mysqlHandle->pGetLastWashCarInfo(listInfo->dataAtt.DataUnion.deviceWash.plateNo, &info);
					if(KEY_TRUE == iret) //数据库已经记录
					{
						time1 = TimeLocalYMD2Sec(&listInfo->dataAtt.DataUnion.deviceWash.vehicleStart);
						time2 = TimeLocalYMD2Sec(&info.vehicleStart);
						if(time1 - time2 > 60 * 60) //时间超过1小时,重新记录
						{
							iret = mysqlHandle->pAddWashCarInfo(&listInfo->dataAtt.DataUnion.deviceWash);
							//添加进车辆轨迹表
							DeviceTrajectory_T traj = {0};
							memcpy(&traj.snapToken, &listInfo->dataAtt.DataUnion.deviceWash.snapToken, 128);
							memcpy(&traj.plateNo, &listInfo->dataAtt.DataUnion.deviceWash.plateNo, 128);
							memcpy(&traj.washstartdate, &listInfo->dataAtt.DataUnion.deviceWash.vehicleStart, 128);
							iret = mysqlHandle->pAddTrajectoryInfo(&traj);
						}
						else//更新结束时间
						{
							iret = mysqlHandle->pSetWashCarInfo(info.snapToken, &listInfo->dataAtt.DataUnion.deviceWash);
							//车辆轨迹表更新
							DeviceTrajectory_T traj = {0};
							iret = mysqlHandle->pGetTrajectoryInfo(info.snapToken, &traj);
							memcpy(&traj.washenddate, &listInfo->dataAtt.DataUnion.deviceWash.vehicleEnd, sizeof(TimeYMD_T));
							iret = mysqlHandle->pUpdateTrajectoryInfo(info.snapToken, &traj);
						}
					}
					else //数据库未记录，加入数据库
					{
						iret = mysqlHandle->pAddWashCarInfo(&listInfo->dataAtt.DataUnion.deviceWash);
						//添加进车辆轨迹表
						DeviceTrajectory_T traj = {0};
						memcpy(&traj.snapToken, &listInfo->dataAtt.DataUnion.deviceWash.snapToken, 128);
						memcpy(&traj.plateNo, &listInfo->dataAtt.DataUnion.deviceWash.plateNo, 128);
						memcpy(&traj.washstartdate, &listInfo->dataAtt.DataUnion.deviceWash.vehicleStart, 128);
						iret = mysqlHandle->pAddTrajectoryInfo(&traj);
					}
				}
			}
			else if(listInfo->dataAtt.type == DEVICE_TYPE_Island)
			{
				DeviceOilLand_T info = {0};
				iret = mysqlHandle->pGetLastIsLandInfo(listInfo->dataAtt.DataUnion.deviceOliLand.plateNo, &info);
				if(KEY_TRUE == iret) //数据库已经记录
				{
					time1 = TimeLocalYMD2Sec(&listInfo->dataAtt.DataUnion.deviceOliLand.snaptime);
					time2 = TimeLocalYMD2Sec(&info.snaptime);
					if(time1 - time2 > 8 * 60) //时间超过8分钟,重新记录
					{
						iret = mysqlHandle->pAddIsLandInfo(&listInfo->dataAtt.DataUnion.deviceOliLand);
					}
					else//更新结束时间
					{
						iret = mysqlHandle->pSetIsLandInfo(info.snapToken, &listInfo->dataAtt.DataUnion.deviceOliLand);
					}
				}
				else //数据库未记录，加入数据库
				{
					iret = mysqlHandle->pAddIsLandInfo(&listInfo->dataAtt.DataUnion.deviceOliLand);
				}
			}
			else if(listInfo->dataAtt.type == DEVICE_TYPE_Alarm)
			{
				iret = mysqlHandle->pAddAlarmInfo(&listInfo->dataAtt.DataUnion.deviceAlarm);
				//高温回调接口
				int argc = 1;
				void *arg[1];
				arg[0] = (void *)&listInfo->dataAtt.DataUnion.deviceAlarm;
				HandleManagePostLoop(&GetInnerHandle()->highTempDetectionHead, highTempDetectionLoopDoFunc, argc, arg);
			}
			//删除内存数据			
			HandleManageDelHandle(listHead, handle);
			MUTEX_UNLOCK(&listInfo->selfMutex);
			MUTEX_DESTROY(&listInfo->selfMutex);
			free(handle);
			usleep(5 * 1000 * 1000);
		}
	}
	return NULL;
}

static int MemoryDataToDatabase()
{
	pthread_t taskID;	
	if (pthread_create(&taskID, NULL, MemoryDataToDatabasePro, (void *)NULL)){
		DF_ERROR("pthread_create MqttStart  is err \n");
		assert(0);
	}
	pthread_detach(taskID);
	return KEY_TRUE;
}

static int AddVehicleInfo(DataAtt_T *dataInfo)
{
	DataAttList_T *listInfo = NULL;
	HandleManage_T *head = &GetInnerHandle()->listHead;
	if (NULL == head)
	{
		return KEY_FALSE;
	}
	listInfo = (DataAttList_T *)malloc(sizeof(DataAttList_T));
	memset(listInfo, 0, sizeof(DataAttList_T));
	MUTEX_INIT(&listInfo->selfMutex);
	MUTEX_LOCK(&listInfo->selfMutex);
	memcpy(&listInfo->dataAtt, dataInfo, sizeof(DataAtt_T));
	HandleManageAddHandle(head, (void *)listInfo);
	MUTEX_UNLOCK(&listInfo->selfMutex);
	//DF_DEBUG("AddVehicleInfo OK.");
	return KEY_TRUE;
}

static int GetVehiclesTerritory(char *plateNo)
{
	if(NULL == plateNo){
		return KEY_FALSE;
	}
	
	int  idNumber = 0;
	char buf1[5]  = {0};
	char buf2[5]  = {0};
	strncpy(buf1, plateNo, 3);
	strncpy(buf2, plateNo + 3, 1);

	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}
	
	mysqlHandle->pGetPlatNoAreaId(buf1, buf2, &idNumber);
	return idNumber;
}

static int GetEntranceVehiclesCurList(int num, StationVehiclesInfo_T *VehiclesInfo)
{
	int iret = 0, i = 0, count = 0;
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(num < 0 || NULL == VehiclesInfo)
	{
		return KEY_FALSE;
	}
	
	TimeYMD_T Starttime = {0}; 
	TimeYMD_T endtime = {0}; 
	Time_T tv = {0};
	GetLocalTime(&tv);

	memcpy(&Starttime, &tv.curTime, sizeof(TimeYMD_T));
	Starttime.hour = 0;
	Starttime.min  = 0;
	Starttime.sec  = 0;
	memcpy(&endtime, &tv.curTime, sizeof(TimeYMD_T));
	DeviceEntrance_T *info = (DeviceEntrance_T *)malloc(sizeof(DeviceEntrance_T)*num);
	memset(info, 0, sizeof(sizeof(DeviceEntrance_T)*num));
	VehicleSearchInfo_T serchInfo = {0};
	serchInfo.type = VehicleSearch_TYPE_NULL;
	serchInfo.vehicleType = -1;
	count = mysqlHandle->pGetEntranceListBySerchType2(1, num, Starttime, endtime, serchInfo, info);
	for(i = 0; i < count; i++)
	{
		memset(&VehiclesInfo[i], 0, sizeof(StationVehiclesInfo_T));
		VehiclesInfo[i].refuelNo = info[i].stay;
		memcpy(&VehiclesInfo[i].time, &info[i].vehicleStart, sizeof(TimeYMD_T));
		memcpy(&VehiclesInfo[i].plateNo, &info[i].plateNo, 128);
		GetVehiclesByType(VehiclesInfo[i].carType, (int)info[i].vehicleType);
	}
	free(info);
	return count;
}

//临街车辆分析
static int GetFrontageVehiclesSum(TimeYMD_T startTime, TimeYMD_T endTime, StreetTypeAttr *streetAttr)
{
	int iret = 0, i = 0;
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	streetAttr->vehickeType.VanCount   = mysqlHandle->pGetFrontageListSumBySearchtype(startTime, endTime, van, -1);
	streetAttr->vehickeType.BusCount   = mysqlHandle->pGetFrontageListSumBySearchtype(startTime, endTime, largeBus, -1);
	streetAttr->vehickeType.TruckCount = mysqlHandle->pGetFrontageListSumBySearchtype(startTime, endTime, truck, -1);
	streetAttr->vehickeType.CarCount   = mysqlHandle->pGetFrontageListSumBySearchtype(startTime, endTime, vehicle, -1);
	streetAttr->vehickeType.OtherCount = mysqlHandle->pGetFrontageListSumBySearchtype(startTime, endTime, vehicletype_Null, -1);

	streetAttr->plateType.BlueCount    = mysqlHandle->pGetFrontageListSumBySearchtype(startTime, endTime, -1, blue);
	streetAttr->plateType.GreenCount   = mysqlHandle->pGetFrontageListSumBySearchtype(startTime, endTime, -1, green);
	streetAttr->plateType.YellowCount  = mysqlHandle->pGetFrontageListSumBySearchtype(startTime, endTime, -1, yellow);
	streetAttr->plateType.OtherCount   = mysqlHandle->pGetFrontageListSumBySearchtype(startTime, endTime, -1, platecolor_null);
	return KEY_TRUE;
}

//进站率分析
static int GetEntranceVehiclesSum(TimeYMD_T startTime, TimeYMD_T endTime, ProgressRate_T *progressAttr)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(NULL == progressAttr){
		return KEY_FALSE;
	}
	
	progressAttr->IncomingCount = mysqlHandle->pGetEntranceListSumByTime(startTime, endTime);
	progressAttr->PassingCount  = mysqlHandle->pGetFrontageListSumByTime(startTime, endTime);
	return KEY_TRUE;
}

//站内车辆数据查询
static int GetEntranceVehiclesList(int page, int num, StationVehicles_T stationAttr, StationVehiclesInfo_T *VehiclesInfo, int *totalnum)
{
	if(NULL == VehiclesInfo || num <= 0){
		return KEY_FALSE; 
	}

	int total = 0, iret = 0, count = 0;
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	int i = 0;
	DeviceEntrance_T *info = (DeviceEntrance_T *)malloc(sizeof(DeviceEntrance_T)*num);
	VehicleSearchInfo_T serchInfo = {0};
	serchInfo.vehicleType = -1;
	serchInfo.type = VehicleSearch_TYPE_NULL;
	if(strlen(stationAttr.fuzzyStr) > 0)
	{
		memcpy(serchInfo.fuzzyStr, stationAttr.fuzzyStr, 128);
		serchInfo.type |= VehicleSearch_TYPE_Fuzzy;
	}
	
	if(stationAttr.vehicleType != vehicletype_all)
	{
		serchInfo.vehicleType = stationAttr.vehicleType;
		serchInfo.type |= VehicleSearch_TYPE_Vehicle;
	}

	if(stationAttr.situationType == Situation_TYPE_Increment)
	{
		serchInfo.type |= VehicleSearch_TYPE_Increment;
	}
	else if(stationAttr.situationType == Situation_TYPE_Losses)
	{
		serchInfo.type |= VehicleSearch_TYPE_Losses;
	}
	total = mysqlHandle->pGetEntranceListSumBySerchType2(stationAttr.vehicleStart, stationAttr.vehicleEnd, serchInfo);
	if(total <= 0){
		return KEY_TRUE;
	}
	DF_DEBUG("total:%d", total);
	count = mysqlHandle->pGetEntranceListBySerchType2(page, num, stationAttr.vehicleStart, stationAttr.vehicleEnd, serchInfo, info);
	*totalnum = total;
	DF_DEBUG("count: %d  total:%d", count, total);
	for(i = 0; i < count; i++)
	{
		memset(&VehiclesInfo[i], 0, sizeof(StationVehiclesInfo_T));
		VehiclesInfo[i].refuelNo = info[i].stay;
		memcpy(&VehiclesInfo[i].time, &info[i].vehicleStart, sizeof(TimeYMD_T));
		memcpy(&VehiclesInfo[i].picPath, &info[i].vehicleImagePath, 128);
		memcpy(&VehiclesInfo[i].plateNo, &info[i].plateNo, 128);
		GetPlateColorByType(VehiclesInfo[i].plateClor, (int)info[i].plateColor);
		GetVehiclesByType(VehiclesInfo[i].carType, (int)info[i].vehicleType);
		char province[32] = {0};
		iret = mysqlHandle->pGetPlatNoAreaById(info[i].vehicleTerr, province, VehiclesInfo[i].territory);
	}
	free(info);
	return count;
}

//车辆历史记录
static int GetEntranceHistoryList(int page, int num, int *totalno, char *location, char *plateno, StationHistoryInfo_T *historyInfo)
{
	if(NULL == location || NULL == historyInfo || num <= 0){
		return KEY_FALSE;
	}
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	int i = 0, iret = 0, count = 0, total = 0, mm = 0;
	TimeYMD_T start = {0};
	TimeYMD_T end = {0};
	Time_T tv = {0};
	VehicleSearchInfo_T searchTypeInfo;

	start.year  = 1970;
	start.month = 1;
	start.day   = 1;
	GetLocalTime(&tv);
	memcpy(&end, &tv.curTime, sizeof(TimeYMD_T));
	searchTypeInfo.type = VehicleSearch_TYPE_Fuzzy;
	searchTypeInfo.vehicleType = vehicletype_all;
	memcpy(searchTypeInfo.fuzzyStr, plateno, 128);
	DeviceEntrance_T *info = (DeviceEntrance_T *)malloc(sizeof(DeviceEntrance_T)*num);
	memset(info, 0, sizeof(DeviceEntrance_T)*num);
	total = mysqlHandle->pGetEntranceListSumBySerchType(start, end, searchTypeInfo);
	if(total <= 0){
		return KEY_FALSE;
	}
	*totalno = total;
	count = mysqlHandle->pGetEntranceListBySerchType(page, num, start, end, searchTypeInfo, info);
	for(i = 0; i < num; i++)
	{
		memcpy(historyInfo[i].plateNo, info[i].plateNo, 128);
		memcpy(&historyInfo[i].time, &info[i].vehicleStart, sizeof(TimeYMD_T));
		GetPlateColorByType(historyInfo[i].plateClor, (int)info[i].plateColor);
		GetVehiclesByType(historyInfo[i].carType, (int)info[i].vehicleType);

		char province[32] = {0};
		char city[32] = {0};
		mysqlHandle->pGetPlatNoAreaById(info[i].vehicleTerr, province, city);
		if(!strcmp(location, province))
		{
			sprintf(historyInfo[i].territory, "%s", "省内");
		}
		else
		{
			sprintf(historyInfo[i].territory, "%s", "省外");
		}
	}
	free(info);
	return count;
}

//站内单台车辆加油趋势
int GetEntranceHistoryCount(TimeYMD_T startTime, TimeYMD_T endTime, char *plateNo, int *data)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle || NULL == plateNo || NULL == data){
		return KEY_FALSE;
	}
	
	int i =0;
	TimeYMD_T time = {0};
	VehicleSearchInfo_T searchTypeInfo = {0};
	searchTypeInfo.type = VehicleSearch_TYPE_Fuzzy;
	searchTypeInfo.vehicleType = vehicletype_all;
	memcpy(searchTypeInfo.fuzzyStr, plateNo, 128);
	
	for(i = 0; i < 12; i++)
	{	
		memset(&time, 0, sizeof(TimeYMD_T));
		memcpy(&time, &startTime, sizeof(TimeYMD_T));
		time.day = 31;
		data[i] = mysqlHandle->pGetEntranceListSumBySerchType(startTime, time, searchTypeInfo);
		if(startTime.month == 12)
		{
			startTime.year++;
			startTime.month = 1;
		}
		else
		{
			startTime.month++;
		}
	}
	return KEY_TRUE;
}

//站内车牌区域分析
static int GetEntrancePlateAnalysis(TimeYMD_T startTime, TimeYMD_T endTime, char *location, PlaceAttribution_T *PlaceAttr)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle || NULL == location){
		return KEY_FALSE;
	}
	
	PlaceAttr->ProvinceCount = mysqlHandle->pGetEntranceListSumByLocation(startTime, endTime, location);
	if(PlaceAttr->ProvinceCount < 0)
	{
		return KEY_FALSE;
	}

	int num = mysqlHandle->pGetEntranceListSumByTime(startTime, endTime);
	if(num < 0)
	{
		return KEY_FALSE;
	}
	PlaceAttr->OutProvinceCount = num - PlaceAttr->ProvinceCount;
	return KEY_TRUE;
}

//站内加油趋势
static int GetEntranceServiceAnalysis(TimeYMD_T Time, int days, int *data)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle || NULL == data){
		return KEY_FALSE;
	}

	int i = 0;
	TimeYMD_T start = {0};
	TimeYMD_T end = {0};
	
	start.year = Time.year;
	start.month= Time.month;
	start.day  = 1;

	end.year = Time.year;
	end.month= Time.month;
	end.day = 1;
	end.hour= 23;
	end.min = 59;
	end.sec = 59;

	for(i = 0; i < days; i++)
	{
		//DF_DEBUG("%04d-%02d-%02d %02d:%02d:%02d", start.year, start.month, start.day, start.hour, start.min, start.sec);
		//DF_DEBUG("%04d-%02d-%02d %02d:%02d:%02d", end.year, end.month, end.day, end.hour, end.min, end.sec);
		data[i] = mysqlHandle->pGetEntranceListSumByTime(start, end);
		start.day++;
		end.day++;
	}
	return KEY_TRUE;
}

static int GetRefuelingTypeAnalysis(TimeYMD_T Time1, TimeYMD_T Time2, RefuelingType_T *Refueling)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(NULL == Refueling)
	{
		return KEY_FALSE;
	}

	int total = 0;
	VehicleSearchInfo_T searchTypeInfo = {0};
	searchTypeInfo.type = VehicleSearch_TYPE_Vehicle;
	searchTypeInfo.vehicleType = truck;
	Refueling->DieselOilCount = mysqlHandle->pGetEntranceListSumBySerchType(Time1, Time2, searchTypeInfo);

	searchTypeInfo.vehicleType = largeBus;
	Refueling->DieselOilCount += mysqlHandle->pGetEntranceListSumBySerchType(Time1, Time2, searchTypeInfo);

	searchTypeInfo.type = VehicleSearch_TYPE_NULL;
	searchTypeInfo.vehicleType = vehicletype_all;
	total = mysqlHandle->pGetEntranceListSumBySerchType(Time1, Time2, searchTypeInfo);
	Refueling->GasolineCount = total - Refueling->DieselOilCount;

	return KEY_TRUE;
}

static int GetOutboundVehiclesAnalysis(TimeYMD_T startTime, TimeYMD_T endTime, int count, ProgressRate_T *Progress, ProgressRate_T *Progress1)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(NULL == Progress || count < 0 || NULL == Progress1)
	{
		return KEY_FALSE;
	}

	TimeYMD_T time = {0};
	memset(&time, 0, sizeof(TimeYMD_T));
	memcpy(&time, &endTime, sizeof(TimeYMD_T));
	time.hour = 23;
	time.min  = 59;
	time.sec  = 59;
	Progress1->IncomingCount = mysqlHandle->pGetEntranceListSumByTime(startTime, time);
	Progress1->PassingCount  = mysqlHandle->pGetFrontageListSumByTime(startTime, time);

	int days = 0, i = 0;
	memset(&time, 0, sizeof(TimeYMD_T));
	if(startTime.year == endTime.year && startTime.month == endTime.month && startTime.day == endTime.day)
	{
		for(i = 0; i < count; i++)
		{
			if(startTime.hour == 23)
			{
				endTime.hour = 23;
				endTime.min  = 59;
				endTime.sec  = 59;
			}
			else
			{
				endTime.hour++;
			}
			Progress[i].PassingCount = mysqlHandle->pGetFrontageListSumByTime(startTime, endTime);
			Progress[i].IncomingCount= mysqlHandle->pGetEntranceListSumByTime(startTime, endTime);
			startTime.hour++;
		}
	}
	else
	{
		for(i = 0; i < count; i++)
		{
			memset(&time, 0, sizeof(TimeYMD_T));
			memcpy(&time, &startTime, sizeof(TimeYMD_T));
			days = GetDaysByTime(startTime.year, startTime.month);
			startTime.hour = 0;
			startTime.min  = 0;
			startTime.sec  = 0;

			time.hour = 23;
			time.min = 59;
			time.sec = 59;
			
			Progress[i].PassingCount = mysqlHandle->pGetFrontageListSumByTime(startTime, time);
			Progress[i].IncomingCount= mysqlHandle->pGetEntranceListSumByTime(startTime, time);
			if(startTime.day != days)
			{
				startTime.day++;
			}
			else if(startTime.day == days && startTime.month != 12)
			{
				startTime.month++;
				startTime.day = 1;
			}
			else if(startTime.day == days && startTime.month == 12)
			{
				startTime.year++;
				startTime.month = 1;
				startTime.day = 1;
			}
		}
	}
	return KEY_TRUE;
}

static int GetResidenceTimeAnalysis(TimeYMD_T startTime, TimeYMD_T endTime, ResidenceTime_T *Residence)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}
	
	if(NULL == Residence)
	{
		return KEY_FALSE;
	}
	
	Residence->count1 = mysqlHandle->pGetEntranceResidenceByTime(startTime, endTime, RESIDENCE_MIN1);
	Residence->count2 = mysqlHandle->pGetEntranceResidenceByTime(startTime, endTime, RESIDENCE_MIN2);
	Residence->count3 = mysqlHandle->pGetEntranceResidenceByTime(startTime, endTime, RESIDENCE_MIN3);
	return KEY_TRUE;
}

static int GetAbnormalVehicleList(int page, int num, int *total, TimeYMD_T startTime, TimeYMD_T endTime, AbnormalVehicleInfo_T *Abnormal)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	int iret = 0, count = 0, i = 0;
	if(Abnormal == NULL || NULL == total || num < 0)
	{
		return KEY_FALSE;
	}

	DeviceEntranceMysql_T *info = (DeviceEntranceMysql_T *)malloc(sizeof(DeviceEntranceMysql_T)*num);
	*total = mysqlHandle->pGetEntranceResidenceByTime(startTime, endTime, RESIDENCE_MIN3);
	 count = mysqlHandle->pGetEntranceResidenceListInfo(page, num, startTime, endTime, info);
	for(i = 0; i < count; i ++)
	{
	 	Abnormal[i].staytime = info[i].staytime;
	 	memcpy(&Abnormal[i].time, &info[i].vehicleStart, sizeof(TimeYMD_T));
	 	memcpy(&Abnormal[i].plateNo, &info[i].plateNo, sizeof(TimeYMD_T));
		GetVehiclesByType(Abnormal[i].carType, (int)info[i].vehicleType);
		char city[32] = {0};
		char province[32] = {0};
		mysqlHandle->pGetPlatNoAreaById(info[i].vehicleTerr, province, city);
		memcpy(&Abnormal[i].territory, &city, 32);
	}
	free(info);
	return count;
}

static int GetRefuelingEfficiencyAnalysis(TimeYMD_T startTime, TimeYMD_T endTime, int count, Refueling_T *refueling)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}
	
	if(NULL == refueling || count <= 0)
	{
		return KEY_FALSE;
	}

	int days = 0, i = 0;
	TimeYMD_T time = {0};
	if(startTime.year == endTime.year && startTime.month == endTime.month && startTime.day == endTime.day)
	{
		for(i = 0; i < count; i++)
		{
			if(startTime.hour == 23)
			{
				endTime.hour = 23;
				endTime.min  = 59;
				endTime.sec  = 59;
			}
			else
			{
				endTime.hour++;
			}
			mysqlHandle->pGetEntranceRefuelingEfficiency(startTime, endTime, &refueling[i]);
 			startTime.hour++;
		}
	}
	else
	{
		for(i = 0; i < count; i++)
		{
			memset(&time, 0, sizeof(TimeYMD_T));
			memcpy(&time, &startTime, sizeof(TimeYMD_T));
			days = GetDaysByTime(startTime.year, startTime.month);
			startTime.hour = 0;
			startTime.min  = 0;
			startTime.sec  = 0;

			time.hour = 23;
			time.min = 59;
			time.sec = 59;

			mysqlHandle->pGetEntranceRefuelingEfficiency(startTime, time, &refueling[i]);
			if(startTime.day != days)
			{
				startTime.day++;
			}
			else if(startTime.day == days && startTime.month != 12)
			{
				startTime.month++;
				startTime.day = 1;
			}
			else if(startTime.day == days && startTime.month == 12)
			{
				startTime.year++;
				startTime.month = 1;
				startTime.day = 1;
			}
		}
	}
	return KEY_TRUE;
}

static int GetVehicleTrajectoryAnalysis(int page, int num, int *totalno, StationVehiclesInfo_T *VehiclesInfo, TimeYMD_T timestart, TimeYMD_T timeend)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(num < 0 || NULL == totalno || NULL == VehiclesInfo)
	{
		return KEY_FALSE;
	}

	int total = 0, count = 0, i = 0;
	total = mysqlHandle->pGetEntranceListSumByTime(timestart, timeend);
	if(total < 0){
		return KEY_FALSE;
	}
	*totalno = total;
	DeviceEntrance_T *info = (DeviceEntrance_T *)malloc(sizeof(DeviceEntrance_T)*num);
	memset(info, 0, sizeof(DeviceEntrance_T)*num);
	VehicleSearchInfo_T serchInfo = {0};
	serchInfo.type = VehicleSearch_TYPE_NULL;
	serchInfo.vehicleType = vehicletype_all;
	count = mysqlHandle->pGetEntranceListBySerchType(page, num, timestart, timeend, serchInfo, info);
	for(i = 0; i < num; i++)
	{
		memcpy(VehiclesInfo[i].snapToken, info[i].snapToken, 128);
		memcpy(VehiclesInfo[i].plateNo, info[i].plateNo, 128);
		memcpy(VehiclesInfo[i].picPath, info[i].vehicleImagePath, 128);
		memcpy(&VehiclesInfo[i].time, &info[i].vehicleStart, sizeof(TimeYMD_T));		
		GetPlateColorByType(VehiclesInfo[i].plateClor, (int)info[i].plateColor);
		GetVehiclesByType(VehiclesInfo[i].carType, (int)info[i].vehicleType);
		char province[128] = {0};
		char city[128] = {0};
		mysqlHandle->pGetPlatNoAreaById(info[i].vehicleTerr, province, city);
		memcpy(VehiclesInfo[i].territory, city, 128);
	}
	free(info);
	return count;
}

static int GetProportionOfVehicleBehavior(TimeYMD_T startTime, TimeYMD_T endTime, DeviceWashProportion_T *ProportionInfo)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(NULL == ProportionInfo)
	{
		return KEY_FALSE;
	}

	ProportionInfo->oilcount  = mysqlHandle->pGetaddWithoutWashNum(startTime, endTime);
	ProportionInfo->washcount = mysqlHandle->pGetWashWithoutaddNum(startTime, endTime);
	ProportionInfo->meanwhilecount = mysqlHandle->pGetaddAndwashNum(startTime, endTime);
	return KEY_TRUE;
}

static int GetProportionOfCarWashing(TimeYMD_T startTime, TimeYMD_T endTime, DeviceWashStatistics_T *WashStatistics)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(NULL == WashStatistics)
	{
		return KEY_FALSE;
	}
	
	WashStatistics->washwithOutaddnum = mysqlHandle->pGetWashWithoutaddNum(startTime, endTime);
	WashStatistics->addwithOutwashnum = mysqlHandle->pGetaddWithoutWashNum(startTime, endTime);
	WashStatistics->addandwashnum	  = mysqlHandle->pGetaddAndwashNum(startTime, endTime);
	WashStatistics->washandaddnum	  = mysqlHandle->pGetwashAndaddNum(startTime, endTime);

	WashStatistics->washcount 		  = mysqlHandle->pGetWashCarListSumByTime(startTime, endTime);
	WashStatistics->totalcount        = mysqlHandle->pGetEntranceListSumByTime(startTime, endTime);
	return KEY_TRUE;
}

static int GetVehicleTrajectoryRoute(char *tokenid, DeviceTrajectory_T *Trajectory)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(NULL == tokenid || NULL == Trajectory)
	{
		return KEY_FALSE;
	}
	
	mysqlHandle->pGetTrajectoryInfo(tokenid, Trajectory);
	return KEY_TRUE;
}

static int GetIntelligentAlarm(TimeYMD_T start, TimeYMD_T end, int page, int num, int type, int *total, DeviceAlarmInfo_T *alarmList)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(NULL == alarmList || NULL == total || num < 0 || page < 0)
	{
		return KEY_FALSE;
	}
	int iret = -1, i = 0;
	HksdkManageHandle_T manage = {0};
	DeviceAlarmParam_T alarmParam = {0};
	HksdkManageRegistHandle(&manage);
	iret = manage.pHKsdkDeviceAlarmGet(&alarmParam);
	if(iret != iret)
	{
		return KEY_FALSE;
	}

	//DF_DEBUG("hightemp: %d  smoke: %d  type: %d", alarmParam.hightempLev, alarmParam.smokeLev, type);
	int totalno = 0, count = 0;
	AlarmLevelSearchType_E Leveltype;
	Leveltype = type;
	if(type == alarmParam.hightempLev && type == alarmParam.smokeLev)
	{
		Leveltype = AlarmLevelSearchType_TYPE_HighTemp | AlarmLevelSearchType_TYPE_Smoking;
	}
	else if(type == alarmParam.hightempLev && type != alarmParam.smokeLev)
	{
		Leveltype = AlarmLevelSearchType_TYPE_HighTemp;
	}
	else if(type != alarmParam.hightempLev && type == alarmParam.smokeLev)
	{
		Leveltype = AlarmLevelSearchType_TYPE_Smoking;
	}
	else if(type == AlarmLevelSearchType_TYPE_NULL)
	{
		Leveltype = AlarmLevelSearchType_TYPE_HighTemp | AlarmLevelSearchType_TYPE_Smoking;
	}
	else
	{
		return KEY_TRUE;
	}
	
	totalno = mysqlHandle->pGetAlarmListSumBySearchType(start, end, Leveltype);
	if(total <= 0)
	{
		return KEY_FALSE;
	}
	//DF_DEBUG("totalno: %d  Leveltype: %d", totalno, Leveltype);
	*total = totalno;
	DeviceAlarm_T *info = (DeviceAlarm_T *)malloc(sizeof(DeviceAlarm_T)*num);
	memset(info,0,sizeof(DeviceAlarm_T)*num);
	count = mysqlHandle->pGetAlarmListBySearchType(page, num, start, end, Leveltype, info);
	for(i = 0; i < count; i++)
	{
		alarmList[i].alrmtype = info[i].alarmType;
		if(info[i].alarmType == ALARM_SMOKE)
		{
			alarmList[i].alarmlev = alarmParam.smokeLev;
		}
		else
		{
			alarmList[i].alarmlev = alarmParam.hightempLev;
		}

	    sprintf(alarmList[i].time, "%04d-%02d-%02d %02d:%02d:%02d", info[i].time.year, info[i].time.month, 
			info[i].time.day, info[i].time.hour, info[i].time.min, info[i].time.sec);
		memcpy(&alarmList[i].alarmdata, &info[i].alarmPlace, 128);
	}
	free(info);
	HksdkManageUnRegistHandle(&manage);
	return count;
}

static int GetAlarmProportionAnalysis(TimeYMD_T startTime, TimeYMD_T endTime, DeviceAlarmCount_T *AlarmCount)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(NULL == AlarmCount){
		return KEY_FALSE;
	}
	
	AlarmLevelSearchType_E Leveltype;
	Leveltype = AlarmLevelSearchType_TYPE_HighTemp;
	AlarmCount->hightemcount = mysqlHandle->pGetAlarmListSumBySearchType(startTime, endTime, Leveltype);

	Leveltype = AlarmLevelSearchType_TYPE_Smoking;
	AlarmCount->smokecount = mysqlHandle->pGetAlarmListSumBySearchType(startTime, endTime, Leveltype);

	return KEY_TRUE;
}

static int GetAlarmPeriodAnalysis(TimeYMD_T startTime, TimeYMD_T endTime, int count, DeviceAlarmCount_T *AlarmCount)
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	if(NULL == AlarmCount){
		return KEY_FALSE;
	}
	
	int days = 0, i = 0;
	TimeYMD_T time = {0};
	AlarmLevelSearchType_E Leveltype;
	if(startTime.year == endTime.year && startTime.month == endTime.month && startTime.day == endTime.day)
	{
		for(i = 0; i < count; i++)
		{
			if(startTime.hour == 23)
			{
				endTime.hour = 23;
				endTime.min  = 59;
				endTime.sec  = 59;
			}
			else
			{
				endTime.hour++;
			}
			
			Leveltype = AlarmLevelSearchType_TYPE_HighTemp;
			AlarmCount[i].hightemcount = mysqlHandle->pGetAlarmListSumBySearchType(startTime, endTime, Leveltype);

			Leveltype = AlarmLevelSearchType_TYPE_Smoking;
			AlarmCount[i].smokecount   = mysqlHandle->pGetAlarmListSumBySearchType(startTime, endTime, Leveltype);
 			startTime.hour++;
			DF_DEBUG("hightemcount:%d  smokecount:%d", AlarmCount[i].hightemcount, AlarmCount[i].smokecount);
		}
	}
	else
	{
		for(i = 0; i < count; i++)
		{
			memset(&time, 0, sizeof(TimeYMD_T));
			memcpy(&time, &startTime, sizeof(TimeYMD_T));
			days = GetDaysByTime(startTime.year, startTime.month);
			startTime.hour = 0;
			startTime.min  = 0;
			startTime.sec  = 0;

			time.hour = 23;
			time.min  = 59;
			time.sec  = 59;
			
			Leveltype = AlarmLevelSearchType_TYPE_HighTemp;
			AlarmCount[i].hightemcount = mysqlHandle->pGetAlarmListSumBySearchType(startTime, time, Leveltype);
			
			Leveltype = AlarmLevelSearchType_TYPE_Smoking;
			AlarmCount[i].smokecount   = mysqlHandle->pGetAlarmListSumBySearchType(startTime, time, Leveltype);
			if(startTime.day != days)
			{
				startTime.day++;
			}
			else if(startTime.day == days && startTime.month != 12)
			{
				startTime.month++;
				startTime.day = 1;
			}
			else if(startTime.day == days && startTime.month == 12)
			{
				startTime.year++;
				startTime.month = 1;
				startTime.day = 1;
			}
		}
	}
	return KEY_TRUE;
}

static int VehicleParamInit()
{
	MySqlManage_T *mysqlHandle = &GetInnerHandle()->innerSqlHandle;
	if(NULL == mysqlHandle){
		return KEY_FALSE;
	}

	int iret = 0;
	Time_T tv = {0};
	TimeYMD_T start = {0};
	pthread_t threadId;
	VehicleCount_T *fd = &VehicleCount;	

	GetLocalTime(&tv);
	start.year = tv.curTime.year;
	start.month= tv.curTime.month;
	start.day  = tv.curTime.day;
	fd->EntranceTotal = mysqlHandle->pGetEntranceListSumByTime(start, tv.curTime);
	fd->FrontageTotal = mysqlHandle->pGetFrontageListSumByTime(start, tv.curTime);
	iret = pthread_create(&threadId, NULL, ThreadRateOfProgress, NULL);
	if(iret < 0)
	{
		return KEY_FALSE;
	}
	return KEY_TRUE;
}

int HighTemperatureDetectionRegister(HighTemperatureDetectionRegistHandle_T *registerHandle)
{
	if (NULL == registerHandle){
		return KEY_FALSE;
	}
	if (KEY_FALSE == HandleManageAddHandle(&GetInnerHandle()->highTempDetectionHead, (void *)registerHandle)){
		DF_DEBUG("HighTemperatureDetectionRegister add handle fail");
		return KEY_FALSE;
	}
	return KEY_TRUE;
}

int HighTemperatureDetectionUnRegister(HighTemperatureDetectionRegistHandle_T *registerHandle)
{
	if (NULL == registerHandle){
		return KEY_FALSE;
	}
	if (KEY_FALSE == HandleManageDelHandle(&GetInnerHandle()->highTempDetectionHead, (void *)registerHandle)){
		DF_DEBUG("HighTemperatureDetectionRegister HandleManageDelHandle fail");
		return KEY_FALSE;
	}
	
	if (NULL != registerHandle)
	{
		free(registerHandle);
	}
	return KEY_TRUE;
}

int VehicleDataAnalysisManageRegister(DataAnalysisManage_T *handle)
{
	if (NULL == handle){
		return KEY_FALSE;
	}
	
	handle->pAddVehicleInfo = AddVehicleInfo;
	handle->pGetVehiclesTerritory    = GetVehiclesTerritory;

	handle->pGetEntranceVehiclesCurList = GetEntranceVehiclesCurList;
	handle->pGetFrontageVehiclesSum  = GetFrontageVehiclesSum;
	handle->pGetEntranceVehiclesSum  = GetEntranceVehiclesSum;
	handle->pGetEntranceVehiclesList = GetEntranceVehiclesList;
	handle->pGetEntranceHistoryList  = GetEntranceHistoryList;
	handle->pGetEntranceHistoryCount = GetEntranceHistoryCount;
	handle->pGetEntrancePlateAnalysis= GetEntrancePlateAnalysis;
	handle->pGetEntranceServiceAnalysis = GetEntranceServiceAnalysis;
	handle->pGetRefuelingTypeAnalysis= GetRefuelingTypeAnalysis;
	handle->pGetOutboundVehiclesAnalysis = GetOutboundVehiclesAnalysis;
	handle->pGetResidenceTimeAnalysis = GetResidenceTimeAnalysis;
	handle->pGetAbnormalVehicleList = GetAbnormalVehicleList;
	handle->pGetRefuelingEfficiencyAnalysis = GetRefuelingEfficiencyAnalysis;
	handle->pGetVehicleTrajectoryAnalysis = GetVehicleTrajectoryAnalysis;
	handle->pGetProportionOfVehicleBehavior = GetProportionOfVehicleBehavior;
	handle->pGetProportionOfCarWashing = GetProportionOfCarWashing;
	handle->pGetVehicleTrajectoryRoute = GetVehicleTrajectoryRoute;


	handle->pGetIntelligentAlarm = GetIntelligentAlarm;
	handle->pGetAlarmProportionAnalysis = GetAlarmProportionAnalysis;
	handle->pGetAlarmPeriodAnalysis = GetAlarmPeriodAnalysis;
	return KEY_TRUE;
}

int VehicleDataAnalysisManageUnRegister(DataAnalysisManage_T *handle)
{
	if (NULL == handle){
		return KEY_FALSE;
	}
	
	handle->pAddVehicleInfo = NULL;
	handle->pGetVehiclesTerritory    = NULL;
	
	handle->pGetEntranceVehiclesCurList = NULL;
	handle->pGetFrontageVehiclesSum  = NULL;
	handle->pGetEntranceVehiclesSum  = NULL;
	handle->pGetEntranceVehiclesList = NULL;
	handle->pGetEntranceHistoryList  = NULL;
	handle->pGetEntranceHistoryCount = NULL;
	handle->pGetEntrancePlateAnalysis= NULL;
	handle->pGetEntranceServiceAnalysis = NULL;
	handle->pGetRefuelingTypeAnalysis= NULL;
	handle->pGetOutboundVehiclesAnalysis = NULL;
	handle->pGetResidenceTimeAnalysis = NULL;
	handle->pGetAbnormalVehicleList = NULL;
	handle->pGetRefuelingEfficiencyAnalysis = NULL;
	handle->pGetVehicleTrajectoryAnalysis = NULL;
	handle->pGetProportionOfVehicleBehavior = NULL;
	handle->pGetProportionOfCarWashing = NULL;

	handle->pGetIntelligentAlarm = NULL;
	handle->pGetAlarmProportionAnalysis = NULL;
	handle->pGetAlarmPeriodAnalysis = NULL;
	return KEY_TRUE;
}

int VehicleDataAnalysisManageInit(DataAnalysisManage_T *handle)
{
	InnerHandle_T *innerHandle = GetInnerHandle();
	gInnerExitFlag = ENUM_ENABLED_TRUE;
	HandleManageInitHead(&innerHandle->listHead);
	HandleManageInitHead(&innerHandle->highTempDetectionHead);
	DeiveManageRegister(&innerHandle->innerDeviceHandle);
	MySqlManageRegister(&innerHandle->innerSqlHandle);
	VehicleDataAnalysisManageRegister(handle);
	VehicleParamInit();

	//内存缓存数据存入数据库
	MemoryDataToDatabase();
	return KEY_TRUE;
}

int VehicleDataAnalysisManageUnInit(DataAnalysisManage_T *handle)
{
	InnerHandle_T *innerHandle = GetInnerHandle();
	gInnerExitFlag = ENUM_ENABLED_FALSE;
	VehicleDataAnalysisManageUnRegister(handle);
	DeiveManageUnRegister(&innerHandle->innerDeviceHandle);
	MySqlManageUnRegister(&innerHandle->innerSqlHandle);
	return KEY_TRUE;
}


