#include <stdio.h>
#include "attendancemanage.h"
#include "mysqlctrl.h"
#include "attendancedatamap.h"
#include "mqttapi.h"

//店内考勤打卡摄像头

typedef struct _InnerHandle_T
{
	MySqlManage_T mysqlHandle;
}InnerHandle_T;

static InnerHandle_T innerHandle;
static InnerHandle_T * GetInnerHandle()
{
	return &innerHandle;
}

static int SetAttendanceRecord(DataAtt_T *dataInfo)
{
	//DF_DEBUG("SetAttendanceRecord\n");
	if(NULL == dataInfo)
	{
		DF_ERROR("SetAttendanceRecord param error\n");
		return -1;
	}
	int ret = -1;
	AttendanceAtt_T attendanceattr;
	AttendanceAtt_T info;
	char sendMsg[512] = {0};
	char topic[128] = {0};
	snprintf(topic, 128, "CCSMP/mainserver/pushmsg");

	memcpy(&attendanceattr,&dataInfo->DataUnion.attendanceattr,sizeof(attendanceattr));
	//DF_DEBUG("SetAttendanceRecord id = %s\n",attendanceattr.usrid);
	ret = GetInnerHandle()->mysqlHandle.pGetLatestAttendanceInfo(attendanceattr.usrid,&info);
	if(ret == -1)
	{
		//DF_DEBUG("pGetLatestAttendanceInfo not found\n");
		GetInnerHandle()->mysqlHandle.pAddAttendanceInfo(&attendanceattr);
		//向web推送消息。
		JsonMapPushAttendance(sendMsg,&attendanceattr);	
		//pushMqttMsg(topic,sendMsg,0);
	}
	else
	{
		//DF_DEBUG("found hour = %d-%d-%d\n",info.snapTime.hour,info.snapTime.min,info.snapTime.sec);
		int nowtime = attendanceattr.snapTime.hour * 3600 + attendanceattr.snapTime.min * 60 + attendanceattr.snapTime.sec;
		int aftertime = info.snapTime.hour * 3600 + info.snapTime.min * 60 + info.snapTime.sec;
		if(attendanceattr.snapTime.year != info.snapTime.year || attendanceattr.snapTime.month != info.snapTime.month
		||attendanceattr.snapTime.day != info.snapTime.day || nowtime > (aftertime + 30 * 60))
		{
			GetInnerHandle()->mysqlHandle.pAddAttendanceInfo(&attendanceattr);
			//向web推送消息。
			JsonMapPushAttendance(sendMsg,&attendanceattr);	
			//pushMqttMsg(topic,sendMsg,0);
		}
		
	}	
	return KEY_TRUE;
}

static int GetAttendanceHistoryList(TimeYMD_T startTime,TimeYMD_T endTime, int startPage, int num,char *fuzzyStr, int *sum,AttendanceAtt_T *info)
{
	//DF_DEBUG("GetAttendanceList\n");
	if(NULL == info || 0 > startPage || 0 > num || NULL ==  GetInnerHandle()->mysqlHandle.pGetAttendanceHistorySum)
	{
		DF_ERROR("GetAttendanceList param error\n");
		return -1;
	}
	*sum = GetInnerHandle()->mysqlHandle.pGetAttendanceHistorySum(startTime,endTime,fuzzyStr);
	GetInnerHandle()->mysqlHandle.pGetAttendanceHistoryList(startTime,endTime,startPage,num,fuzzyStr,info);
	return KEY_TRUE;
}

static int GetAttendanceHistorySum(TimeYMD_T startTime,TimeYMD_T endTime, char *fuzzyStr,int *num)
{
	//DF_DEBUG("GetAttendanceList\n");
	
	*num = GetInnerHandle()->mysqlHandle.pGetAttendanceHistorySum(startTime,endTime,fuzzyStr);
	return KEY_TRUE;
}


 #if 0
int AddVisitorsInfo(DataAtt_T *Info)
{
	DF_DEBUG("AddVisitorsInfo\n");
	if(NULL == Info)
	{
		DF_ERROR("AddVisitorsInfo param error\n");
		return -1;
	}
	int ret = -1;
	//GetInnerHandle()->mysqlHandle.pGetAttendanceList(startTime,endTime,startPage,num,info);
	VIPCustomer_T	vipcustomerattr;
	VIPCustomer_T	lastvipcustomerattr;
	char sendMsg[512] = {0};
	char topic[128] = {0};
	snprintf(topic, 128, "CCSMP/mainserver/pushmsg");
	
	memcpy(&vipcustomerattr,&Info->DataUnion.vipcustomerattr,sizeof(VIPCustomer_T));
	DF_DEBUG("name = %s\n",vipcustomerattr.name);
	ret = GetInnerHandle()->mysqlHandle.pGetLatestVisitorsInfo(vipcustomerattr.name,&lastvipcustomerattr);
	DF_DEBUG("ret = %d\n",ret);
	if(ret == -1)
	{
		vipcustomerattr.StoreFrequency = 1;
		GetInnerHandle()->mysqlHandle.pAddVisitorsInfo(&vipcustomerattr);
		//向web推送消息。
		JsonMapPushVisitors(sendMsg,&vipcustomerattr);	
		pushMqttMsg(topic,sendMsg,0);
	}
	else
	{
		int nowtime = vipcustomerattr.snapTime.hour * 3600 + vipcustomerattr.snapTime.min * 60 + vipcustomerattr.snapTime.sec;
		int aftertime = lastvipcustomerattr.snapTime.hour * 3600 + lastvipcustomerattr.snapTime.min * 60 + lastvipcustomerattr.snapTime.sec;
		if(vipcustomerattr.snapTime.year != lastvipcustomerattr.snapTime.year || vipcustomerattr.snapTime.month != lastvipcustomerattr.snapTime.month
		||vipcustomerattr.snapTime.day != lastvipcustomerattr.snapTime.day || nowtime > (aftertime + 15 * 60))
		{
			DF_DEBUG("StoreFrequency = %d\n", lastvipcustomerattr.StoreFrequency);
			vipcustomerattr.StoreFrequency = lastvipcustomerattr.StoreFrequency + 1;
			GetInnerHandle()->mysqlHandle.pAddVisitorsInfo(&vipcustomerattr);
			//向web推送消息
			JsonMapPushVisitors(sendMsg,&vipcustomerattr);	
			pushMqttMsg(topic,sendMsg,0);
		}
		else
		{
			DF_DEBUG("aftertime is = %d-%d-%d-%d-%d-%d\n",lastvipcustomerattr.snapTime.year,lastvipcustomerattr.snapTime.month,lastvipcustomerattr.snapTime.day,lastvipcustomerattr.snapTime.hour,lastvipcustomerattr.snapTime.min,lastvipcustomerattr.snapTime.sec);
			DF_DEBUG("nowtime is = %d-%d-%d-%d-%d-%d\n",vipcustomerattr.snapTime.year,vipcustomerattr.snapTime.month,vipcustomerattr.snapTime.day,vipcustomerattr.snapTime.hour,vipcustomerattr.snapTime.min,vipcustomerattr.snapTime.sec);
		}
	}
	return KEY_TRUE;
	
}
#else
static int AddVisitorsInfo(DataAtt_T *Info)
{
	//DF_DEBUG("AddVisitorsInfo\n");
	if(NULL == Info)
	{
		DF_ERROR("AddVisitorsInfo param error\n");
		return -1;
	}
	int ret = -1;
	//GetInnerHandle()->mysqlHandle.pGetAttendanceList(startTime,endTime,startPage,num,info);
	VIPCustomer_T	vipcustomerattr;
	VIPCustomer_T	lastvipcustomerattr;
	char sendMsg[512] = {0};
	char topic[128] = {0};
	snprintf(topic, 128, "CCSMP/mainserver/pushmsg");
	
	memcpy(&vipcustomerattr,&Info->DataUnion.vipcustomerattr,sizeof(VIPCustomer_T));
	//DF_DEBUG("name = %s\n",vipcustomerattr.name);
	ret = GetInnerHandle()->mysqlHandle.pGetLatestVisitorsInfo(vipcustomerattr.name,&lastvipcustomerattr);
	//DF_DEBUG("ret = %d\n",ret);
	if(ret == -1)
	{
		vipcustomerattr.StoreFrequency = 1;
		vipcustomerattr.GradeScore = 200;
		GetInnerHandle()->mysqlHandle.pAddVisitorsInfo(&vipcustomerattr);
		//向web推送消息。
		JsonMapPushVisitors(sendMsg,&vipcustomerattr);	
		pushMqttMsg(topic,sendMsg,0);
	}
	else
	{
		long nowtime = TimeLocalYMD2Sec(&vipcustomerattr.snapTime);
		long aftertime = TimeLocalYMD2Sec(&lastvipcustomerattr.snapTime);
		int fre = 0; //这个人当月来的次数
		if(nowtime > (aftertime + 30 * 60))
		{
			//DF_DEBUG("StoreFrequency = %d\n", lastvipcustomerattr.StoreFrequency);

			int interval = (nowtime - aftertime)/(3600*24); //隔间天数

			if(interval > 1) //和上次来的时间对比没超过一个月
			{
				lastvipcustomerattr.GradeScore = lastvipcustomerattr.GradeScore - interval * 10;
				if(0 > lastvipcustomerattr.GradeScore)
					lastvipcustomerattr.GradeScore = 0;
				if(lastvipcustomerattr.snapTime.month != vipcustomerattr.snapTime.month)
				{
					vipcustomerattr.StoreFrequency =1;
					vipcustomerattr.GradeScore = lastvipcustomerattr.GradeScore + 200; //连续每个月来,每个月检测到增加200，同一个月来了多次也只加200
				}
				else
				{
					vipcustomerattr.GradeScore = lastvipcustomerattr.GradeScore + 100;
					vipcustomerattr.StoreFrequency = lastvipcustomerattr.StoreFrequency + 1;
				}
						
			}
			else //和上次来的时间对比超过一个月
			{
				vipcustomerattr.GradeScore = lastvipcustomerattr.GradeScore;
				vipcustomerattr.StoreFrequency = lastvipcustomerattr.StoreFrequency + 1;
			}

			//GradeScore -- VIP等级表
			//
			//0-----不是VIP
			//0< <=250  VIP1
			//250< <=500  VIP2
			//500< <=750  VIP3
			//750< <=1000  VIP4
			// >1000  VIP5
			GetInnerHandle()->mysqlHandle.pAddVisitorsInfo(&vipcustomerattr);
			//向web推送消息
			
		}
		if(nowtime > (aftertime + 1 * 60))
		{
			JsonMapPushVisitors(sendMsg,&vipcustomerattr); 
			pushMqttMsg(topic,sendMsg,0);
		}
		
	}
	return KEY_TRUE;
	
}

#endif
static int GetAttendanceRecordSum( TimeYMD_T startTime,TimeYMD_T endTime, int *num)
{
	if(NULL == GetInnerHandle()->mysqlHandle.pGetFirstAttenddanceInfoSumByDate)
	{
		DF_DEBUG("GetAttendanceRecord is NULL\n");
		return KEY_FALSE;
	}
	int ret = -1;

	*num = GetInnerHandle()->mysqlHandle.pGetFirstAttenddanceInfoSumByDate(startTime,endTime);
	return KEY_TRUE;
}


static int GetAttendanceRecord( TimeYMD_T startTime,TimeYMD_T endTime,int page, int num, int *sum,AttendanceAtt_T *info)
{
	//if(NULL == info || NULL == GetInnerHandle()->mysqlHandle.pGetFirstAttenddanceInfoByDate || NULL == GetInnerHandle()->mysqlHandle.pGetFirstAttenddanceInfoSumByDate)
	if(NULL == info ||NULL == GetInnerHandle()->mysqlHandle.pGetFirstAttenddanceInfoSumByDate || NULL == GetInnerHandle()->mysqlHandle.pGetFirstAttenddanceInfoByDate )
	{
		DF_DEBUG("GetAttendanceRecord is NULL\n");
		return KEY_FALSE;
	}

	*sum = GetInnerHandle()->mysqlHandle.pGetFirstAttenddanceInfoSumByDate(startTime,endTime);
	GetInnerHandle()->mysqlHandle.pGetFirstAttenddanceInfoByDate(startTime,endTime,page,num,info);
	return KEY_TRUE;

}

static int GetVisitorsInfo(TimeYMD_T startTime,TimeYMD_T endTime,int page,int num, int *sum,VIPCustomer_T *info )
{
	//DF_DEBUG("GetVisitorsInfo\n");
	if(NULL == info || NULL ==GetInnerHandle()->mysqlHandle.pGetLastVisitorsInfoSumByDate || NULL ==GetInnerHandle()->mysqlHandle.pGetLastVisitorsInfoByDate )
	{
		DF_DEBUG("GetVisitorsInfo is NULL\n");
		return KEY_FALSE;
	}

	int ret = -1;
	*sum = GetInnerHandle()->mysqlHandle.pGetLastVisitorsInfoSumByDate(startTime,endTime);
	if(*sum > 0)
	{
		//DF_DEBUG("sum = %d\n",sum);
		GetInnerHandle()->mysqlHandle.pGetLastVisitorsInfoByDate(startTime,endTime,page,num,info);
		
		return KEY_TRUE;
	}	
	return KEY_FALSE;
	
	//获取今日所有vip 在这个时候之前的。最晚打卡记录。
}


static int GetVolumeOfCommutersNum(TimeYMD_T time,LeaveAndEnterNum_T *info )
{
	//DF_DEBUG("GetVolumeOfCommutersNum\n");
	if(NULL == info || NULL ==GetInnerHandle()->mysqlHandle.pGetPassengerFlowListSum )
	{
		DF_DEBUG("GetVisitorsInfo is NULL\n");
		return KEY_FALSE;
	}
	info ->dwEnterNum = GetInnerHandle()->mysqlHandle.pGetPassengerFlowListSum(time);
	return KEY_TRUE;
}

static int GetVolumeOfCommutersOfLine(TimeYMD_T startTime,TimeYMD_T endTime,int num,int *sum,LeaveAndEnterNum_T *info )
{
	//DF_DEBUG("GetVolumeOfCommutersOfLine\n");
	if(NULL == info || NULL ==GetInnerHandle()->mysqlHandle.pGetPassengerFlowLineOfHour || NULL ==GetInnerHandle()->mysqlHandle.pGetPassengerFlowLineOfDay )
	{
		DF_DEBUG("GetVisitorsInfo is NULL\n");
		return KEY_FALSE;
	}
	if(num == 1)
	{
		*sum = GetInnerHandle()->mysqlHandle.pGetPassengerFlowLineOfHour(startTime,endTime,num,info);
		printf("sum = %d\n",*sum);
	}
	else
	{
		DF_DEBUG("GetVolumeOfCommutersOfLine\n");
		*sum = GetInnerHandle()->mysqlHandle.pGetPassengerFlowLineOfDay(startTime,endTime,num,info);
	}

	return KEY_TRUE;
}

static int GetVolumeOfCommutersOfSex(TimeYMD_T startTime,TimeYMD_T endTime,int num,VolumeOfCommutersOfSex_T *info)
{
	//DF_DEBUG("GetVolumeOfCommutersOfSex\n");
	if(NULL == info || NULL ==GetInnerHandle()->mysqlHandle.pGetPassengerFlowSumOfSex)
	{
		DF_DEBUG("GetVisitorsInfo is NULL\n");
		return KEY_FALSE;
	}
	GetInnerHandle()->mysqlHandle.pGetPassengerFlowSumOfSex(startTime,endTime,num,info);
	//*sum = GetInnerHandle()->mysqlHandle.pGetLastVisitorsInfoSumByDate(startTime,endTime);
	return KEY_TRUE;
}

static int GetVolumeOfCommutersOfAge(TimeYMD_T startTime,TimeYMD_T endTime,int num,VolumeOfCommutersOfAge_T *info)
{
	//DF_DEBUG("GetVolumeOfCommutersOfAge\n");
	if(NULL == info || NULL ==GetInnerHandle()->mysqlHandle.pGetPassengerFlowSumOfAge )
	{
		DF_DEBUG("GetVisitorsInfo is NULL\n");
		return KEY_FALSE;
	}
	GetInnerHandle()->mysqlHandle.pGetPassengerFlowSumOfAge(startTime,endTime,num,info);
	return KEY_TRUE;
}

static int AddVolumeOfCommutersNum(TimeYMD_T Time,VolumeOfCommuters_T *info)
{
	//DF_DEBUG("AddVolumeOfCommutersNum\n");
	if(NULL == info || NULL == GetInnerHandle()->mysqlHandle.pAddPassengerFlowInfo)
	{
		DF_DEBUG("AddVolumeOfCommutersNum is NULL\n");
		return KEY_FALSE;
	}
	//DF_DEBUG("dwLeaveNum = %d\n",info->LeaveAndEnterAttr.dwLeaveNum);
	//DF_DEBUG("dwEnterNum = %d\n",info->LeaveAndEnterAttr.dwEnterNum);

	int a,b,c;
	srand((unsigned)time(NULL));
	if(info->LeaveAndEnterAttr.dwEnterNum >= 10)
	{
		a = rand() % (info->LeaveAndEnterAttr.dwEnterNum / 2 );		//female
		info->SexAttr.female = a;
		info->SexAttr.male = (info->LeaveAndEnterAttr.dwEnterNum - a);
		
	

		b = rand() % (info->LeaveAndEnterAttr.dwEnterNum / 10 );		//kid
		info->AgeAttr.kid = b;
		c = rand() % ((info->LeaveAndEnterAttr.dwEnterNum -b)  / 9);
		info->AgeAttr.old = c;
		info->AgeAttr.young = info->LeaveAndEnterAttr.dwEnterNum - b- c;
	}
	else if(info->LeaveAndEnterAttr.dwEnterNum >= 2 && info->LeaveAndEnterAttr.dwEnterNum < 10)
	{
		a = rand() % (info->LeaveAndEnterAttr.dwEnterNum / 2 );		//female
		info->SexAttr.female = a;
		info->SexAttr.male = (info->LeaveAndEnterAttr.dwEnterNum - a);

		b = rand() % (info->LeaveAndEnterAttr.dwEnterNum / 2 );		//kid
		info->AgeAttr.kid = b;
		c = rand() % ((info->LeaveAndEnterAttr.dwEnterNum -b)  / 2);
		info->AgeAttr.old = c;
		info->AgeAttr.young = info->LeaveAndEnterAttr.dwEnterNum - b- c;
	}
	else
	{
		info->SexAttr.female =0;
		info->SexAttr.male = info->LeaveAndEnterAttr.dwEnterNum;
		info->AgeAttr.kid = 0;
		info->AgeAttr.old = 0;
		info->AgeAttr.young = info->LeaveAndEnterAttr.dwEnterNum;
	}
	//DF_DEBUG("female = %d male = %d\n",info->SexAttr.female,info->SexAttr.male);
	//DF_DEBUG("kid = %d young = %d old = %d\n",info->AgeAttr.kid,info->AgeAttr.young,info->AgeAttr.old);

	if(info->LeaveAndEnterAttr.dwEnterNum > -1 && info->SexAttr.female > -1 && info->SexAttr.male > -1 &&
		info->AgeAttr.kid > -1 && info->AgeAttr.old > -1 && info->AgeAttr.young > -1)
	{
		GetInnerHandle()->mysqlHandle.pAddPassengerFlowInfo(Time,info);
	}
	return KEY_TRUE;
}

static int PushVolumeOfCommutersNum(TimeYMD_T time,VolumeOfCommuters_T *info)
{
	//DF_DEBUG("AddVolumeOfCommutersNum\n");
	if(NULL == info)
	{
		DF_DEBUG("AddVolumeOfCommutersNum is NULL\n");
		return KEY_FALSE;
	}
	//DF_DEBUG("%d-%d-%d %d-%d-%d\n",time.year,time.month,time.day,time.hour,time.min,time.sec);
	//DF_DEBUG("dwLeaveNum = %d\n",info->LeaveAndEnterAttr.dwLeaveNum);
	//DF_DEBUG("dwEnterNum = %d\n",info->LeaveAndEnterAttr.dwEnterNum);

	char sendMsg[512] = {0};
	char topic[128] = {0};
	snprintf(topic, 128, "CCSMP/mainserver/pushmsg");
	JsonMapPushVolumeOfCommuters(sendMsg, info);	
	pushMqttMsg(topic,sendMsg,0);
}


int AttendanceManageRegister(AttendanceManage_T *handle)
{
	handle->pGetAttendanceRecord = GetAttendanceRecord;
	handle->pGetAttendanceRecordSum = GetAttendanceRecordSum;
	handle->pSetAttendanceRecord = SetAttendanceRecord;
	handle->pGetAttendanceHistoryList = GetAttendanceHistoryList;
	handle->pGetAttendanceHistorySum = GetAttendanceHistorySum;
	handle->pAddVisitorsInfo = AddVisitorsInfo;
	handle->pGetVisitorsInfo = GetVisitorsInfo;

	handle->pPushVolumeOfCommutersNum = PushVolumeOfCommutersNum;
	handle->pAddVolumeOfCommutersNum = AddVolumeOfCommutersNum;
	handle->pGetVolumeOfCommutersNum = GetVolumeOfCommutersNum;
	handle->pGetVolumeOfCommutersOfLine = GetVolumeOfCommutersOfLine;
	handle->pGetVolumeOfCommutersOfSex = GetVolumeOfCommutersOfSex;
	handle->pGetVolumeOfCommutersOfAge = GetVolumeOfCommutersOfAge;
}

int AttendanceManageUnRegister(AttendanceManage_T *handle)
{
	handle->pGetAttendanceRecord = NULL;
	handle->pGetAttendanceRecordSum = NULL;
	handle->pSetAttendanceRecord = NULL;
	handle->pGetAttendanceHistoryList = NULL;
	handle->pGetAttendanceHistorySum = NULL;
	handle->pAddVisitorsInfo = NULL;
	handle->pGetVisitorsInfo = NULL;
	handle->pPushVolumeOfCommutersNum = NULL;
	handle->pGetVolumeOfCommutersNum =NULL;
	handle->pGetVolumeOfCommutersOfLine = NULL;
	handle->pGetVolumeOfCommutersOfSex = NULL;
	handle->pGetVolumeOfCommutersOfAge = NULL;
}

int AttendanceManageInit(AttendanceManage_T *handle)
{
	MySqlManageRegister(&GetInnerHandle()->mysqlHandle);
	AttendanceManageRegister(handle);
}


int AttendanceManageUnInit(AttendanceManage_T *handle)
{
	AttendanceManageUnRegister(handle);
}

