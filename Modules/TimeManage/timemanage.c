/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年2月22日
|  说明: 时间对外是有时区的,相当板端永远是0时区
|  修改: 2018年5月19日 添加NTP配置表
|	
******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "handlemanage.h"
#include "timemanage.h"
//#include "ntp.h"
#include "pthread.h"
#include "timemanageconfig.h"
#include <sys/prctl.h>
#include <sys/sysinfo.h>
//#include "netmanage.h"

#define TZ_PATH "/etc/TZ"

typedef struct _TimeManage_T{
	HandleManage_T listUsrRegistHead;
	TimeConf_T conf;
	ENUM_ENABLED_E 		timeTaskRun;
}TimeManage_T;

typedef struct
{
    int  index;  /*序号*/
    char* pszCityName;
    unsigned char u32STDSign;                         /*时区符号：0为"+",1为"-"*/
    unsigned char hour;
	unsigned char min;
	unsigned char sec;
} TZONE_T;

static int timesuc = 0;
static TZONE_T g_astruTZS[]=
    {
        { 1 ,  "Etc/GMT-12",            0,   12,  0,   	0    },  
        { 2 ,  "Pacific/Apia",          0,   11,  0,   	0    },      
        { 3 ,  "Pacific/Honolulu",      0,   10,  0,   	0    },       
        { 4 ,  "America/Anchorage",     0,   9,   0,   	0    },      
        { 5 ,  "America/Los_Angeles",   0,   8,   0,   	0    },      
        { 6 ,  "America/Denver",        0,   7,   0,   	0    },      
        { 7 ,  "America/Phoenix",       0,   7,   0,   	0    },      
		{ 8 ,  "America/Tegucigalpa",	0,	 7,   0,	0    },	   
        { 9 ,  "America/Winnipeg",      0,   6,   0,   	0    },      
        { 10,  "America/Chicago",       0,   6,   0,   	0    },      
        { 11,  "America/Costa_Rica",    0,   6,   0,   	0    },      
		{ 12,  "America/Mexico_City",	0,	 6,   0,	0    },	   
		{ 13,  "America/Bogota",		0,	 5,   0,	0    },	   
        { 14,  "America/New_York",      0,   5,   0,   	0    },      
		{ 15,  "America/Indianapolis",	0,	 5,   0,	0    },	   
		{ 16,  "America/Montreal",		0,	 4,   0,	0    },	   
        { 17,  "America/Caracas",       0,   4,   0,   	0    },      
		{ 18,  "America/Santiago",		0,	 4,   0,	0    },	   
        { 19,  "America/St_Johns",      0,   3,  30,   	0    },      
		{ 20,  "America/Sao_Paulo", 	0,	 3,   0,	0    },	   
        { 21,  "America/Buenos_Aires",  0,   3,   0,   	0    },      
		{ 22,  "America/Thule", 		0,	 3,   0,	0    },	   
        { 23,  "Atlantic/South_Georgia", 0,  2,	  0,    0	 },      
        { 24,  "Atlantic/Cape_Verde",   0,   1,   0,   	0    },      
        { 25,  "Atlantic/Azores",       0,   1,   0,   	0    },      
        { 26,  "Europe/Dublin",         0,   0,   0,   	0    },      
        { 27,  "Africa/Casablanca",     0,   0,   0,   	0    },      
        { 28,  "Europe/Amsterdam",      1,   1,   0,   	0    },      
        { 29,  "Europe/Belgrade",       1,   1,   0,   	0    },      
        { 30,  "Europe/Brussels",       1,   1,   0,   	0    },      
        { 31,  "Europe/Warsaw",         1,   1,   0,   	0    },      
        { 32,  "Africa/Lagos",          1,   1,   0,   	0    },      
        { 33,  "Europe/Bucharest",      1,   2,   0,   	0    },      
		{ 34,  "Africa/Harare", 		1,	 2,   0,	0    },	   
		{ 35,  "Europe/Helsinki",		1,	 2,   0,	0    },	   
        { 36,  "Africa/Cairo",          1,   2,   0,   	0    },      
		{ 37,  "Europe/Athens", 		1,	 2,   0,	0    },	   
        { 38,  "Asia/Jerusalem",        1,   2,   0,   	0    },      
        { 39,  "Asia/Baghdad",          1,   3,   0,   	0    },      
        { 40,  "Asia/Kuwait",           1,   3,   0,   	0    },      
        { 41,  "Europe/Moscow",         1,   3,   0,   	0    },      
        { 42,  "Africa/Nairobi",        1,   3,   0,   	0    },      
        { 43,  "Asia/Tehran",           1,   3,   30,  	0    },      
        { 44,  "Asia/Dubai",            1,   4,   0,   	0    },      
        { 45,  "Asia/Baku",             1,   4,   0,   	0    },      
        { 46,  "Asia/Kabul",            1,   4,   30,  	0    },      
        { 47,  "Asia/Yekaterinburg",    1,   5,   0,   	0    },      
        { 48,  "Asia/Karachi",          1,   5,   0,   	0    },      
        { 49,  "Asia/Calcutta",         1,   5,   30,  	0    },      
        { 50,  "Asia/Katmandu",         1,   5,   45,  	0    },      
        { 51,  "Asia/Almaty",           1,   6,   0,   	0    },      
        { 52,  "Asia/Dhaka",            1,   6,   0,   	0    },      
        { 53,  "Asia/Colombo",          1,   6,   0,   	0    },      
        { 54,  "Asia/Rangoon",          1,   6,   30, 	0    },      
		{ 55,  "Asia/Krasnoyarsk",		1,	 7,   0,	0    },	   
        { 56,  "Asia/Bangkok",          1,   7,   0,   	0    },      
        { 57,  "Asia/Hong_Kong",        1,   8,   0,   	0    },      
        { 58,  "Asia/Kuala_Lumpur",     1,   8,   0,   	0    },      
        { 59,  "Australia/Perth",       1,   8,   0,   	0    },      
        { 60,  "Asia/Taipei",           1,   8,   0,   	0    },      
		{ 61,  "Asia/Irkutsk",			1,	 8,   0,	0    },	   
        { 62,  "Asia/Tokyo",            1,   9,   0,   	0    },     
        { 63,  "Asia/Seoul",            1,   9,   0,   	0    },     
        { 64,  "Asia/Yakutsk",          1,   9,   0,   	0    },     
        { 65,  "Australia/Adelaide",    1,   9,   30,  	0    },     
		{ 66,  "Australia/Darwin",		1,	 9,   30,	0    },	  
		{ 67,  "Australia/Brisbane",    1,   10,  0,   	0    },      
		{ 68,  "Asia/Vladivostok",		1,	 10,  0,	0    },	   
        { 69,  "Pacific/Guam",          1,   10,  0,  	0    },      
        { 70,  "Australia/Hobart",      1,   10,  0,   	0    },      
		{ 71,  "Australia/Sydney",		1,	 10,  0,	0    },	   
        { 72,  "Asia/Magadan",          1,   11,  0,   	0    },      
        { 73,  "Pacific/Auckland",      1,   12,  0,   	0    },      
        { 74,  "Pacific/Fiji",          1,   12,  0,   	0    },      
        { 75,  "Pacific/Tongatapu",     1,   13,  0,   	0    },      
        { 0,   NULL,        			1,   8,   0,   	0    },      
    };

static TimeManage_T g_timeManage;


#if 0
int NtpParamSet(Ntp_T *info)
{
	cJSON *confntp = JsonMapNtpParamMake(info);
	if(NULL == confntp){
		return KEY_FALSE;
	}
	return ConfigManageSet(confntp,D_CONFNTPPARAM_STR);
}

int NtpParamGet(Ntp_T *info)
{
    int s32Ret=KEY_FALSE;
    cJSON *cjtmp=NULL;
    cjtmp=ConfigManageGet(D_CONFNTPPARAM_STR);
    if(NULL==cjtmp)
    {
        NtpParamDefault();
        cjtmp=ConfigManageGet(D_CONFNTPPARAM_STR);
	}
    s32Ret=JsonMapNtpParamPares(cjtmp, info);
	cJSON_Delete(cjtmp);cjtmp=NULL;
	return s32Ret;
}
#endif

static int FunSendTimeChange(void *handle,int argc,void *arg[])
{
	if(handle == NULL || 0 == argc) 
		return KEY_FALSE;
	
	TimeManageHandle_T *curHandle = (TimeManageHandle_T *)handle;
	if(NULL == curHandle->ChangeFun ){
		return KEY_FALSE;
	}
	curHandle->ChangeFun();
	return KEY_TRUE;
}

#if 0
static int SetDateTime(TimeYMD_T *tv)
{
	char out[64] = {0};
	sprintf(out,"date -s \"%04d-%02d-%02d %02d:%02d:%02d\"",tv->year,tv->month,tv->day,tv->hour,tv->min,tv->sec);
	DF_DEBUG("%s",out);
	system(out); 
	return KEY_TRUE;
}
#endif

//获取系统启动时间
_OUT_ int GetUpTime(long *sec) 
{
	struct sysinfo info;
	time_t cur_time = 0;
	time_t boot_time = 0;
	struct tm *ptm = NULL;
	if (sysinfo(&info)) 
	{
		DF_ERROR("Failed to get sysinfo");
		*sec = 0;
		return -1;
	}

	*sec = info.uptime;
	//DF_DEBUG("sec = %ld,info.uptime = %ld",*sec,info.uptime);

    return 0; 
}

_OUT_ int GetSystemTime(TimeConf_T *info)
{
	TimeManageConfigGet(info);
	return 0;
}

_OUT_ int GetTimeZone()
{
	TimeConf_T info;
	memset(&info,0,sizeof(Timeval_T));
	TimeManageConfigGet(&info);
	DF_DEBUG("%d",info.timeZone);
	return info.timeZone;
}

_OUT_ int GetCurTimeUTC(TimeYMD_T *tv)
{
	time_t timer;   
	struct tm t_tm;
	timer = time(NULL);
	gmtime_r(&timer,&t_tm);
	tv->year= t_tm.tm_year+1900;
	tv->month= t_tm.tm_mon+1;
	tv->day= t_tm.tm_mday;
	tv->hour= t_tm.tm_hour;
	tv->min = t_tm.tm_min;
	tv->sec = t_tm.tm_sec;
	return KEY_TRUE;
}

_OUT_ int GetLocalTime(Time_T *tv)
{
	time_t timer;   
	struct tm t_tm;
	timer = time(NULL);
	TimeConf_T info = {0};
	TimeManageConfigGet(&info);
	localtime_r(&timer,&t_tm); 
	tv->curTime.year= t_tm.tm_year+1900;
	tv->curTime.month= t_tm.tm_mon+1;
	tv->curTime.day= t_tm.tm_mday;
	tv->curTime.hour= t_tm.tm_hour;
	tv->curTime.min = t_tm.tm_min;
	tv->curTime.sec = t_tm.tm_sec;
	tv->weekday = t_tm.tm_wday; 
	tv->timeZone = info.timeZone;
	return KEY_TRUE;
}

_OUT_ char* GetTimeZoneString(int tzIndex)
{
	if(tzIndex > 0){
		return  g_astruTZS[tzIndex - 1].pszCityName;
	}else{
		return NULL;
	}
}

//直接去index，i从0开始操作不需要-1
_OUT_ int   GetTimeZoneIndex(char *cityName)
{
	int i = 0;
	for(i = 0;NULL != g_astruTZS[i].pszCityName;i++){
		if(0 == strcasecmp(cityName,g_astruTZS[i].pszCityName)){
			return g_astruTZS[i].index;
		}
	}
}

//时区index从1开始，所以要-1
_OUT_ int SetTimeZone(int tzIndex)
{
	char tz[16] = {0};
	FILE *fp = fopen(TZ_PATH, "w+"); 
    if (NULL == fp) /* 如果失败了 */
    {
        DF_ERROR("fopen:");
		return -1;
	}
	if(1 == g_astruTZS[tzIndex -1].u32STDSign){
		sprintf(tz,"STD-%d:%d:%d\n",g_astruTZS[tzIndex -1].hour,g_astruTZS[tzIndex - 1].min,g_astruTZS[tzIndex -1].sec);
	}else{
		sprintf(tz,"STD+%d:%d:%d\n",g_astruTZS[tzIndex - 1].hour,g_astruTZS[tzIndex - 1].min,g_astruTZS[tzIndex -1].sec);
	}
	fwrite(tz,1,strlen(tz),fp);
	fclose(fp);
	return 0;
}

_OUT_ int SetLocalTime(Time_T *tv)
{
	Timeval_T timenow = {0};
	long setSec = 0;
	GetTimeOfDay(&timenow);

	setSec = TimeLocalYMD2Sec(&tv->curTime);
	DF_DEBUG("timenow.sec = %ld,setSec = %ld!!!",timenow.sec,setSec);
	if(abs(timenow.sec - setSec) <= 2)
	{
		DF_DEBUG("set time equal now time!!!");
		return KEY_FALSE;
	}
	
	struct timeval stv;
	TimeConf_T info;
	struct tm t_tm;
	memset(&stv,0,sizeof(struct timeval));
	memset(&info,0,sizeof(TimeConf_T));
	memset(&t_tm,0,sizeof(struct tm));
	TimeManageConfigGet(&info);
	SetTimeZone(tv->timeZone);
	usleep(1000*20);
	t_tm.tm_year = tv->curTime.year - 1900;
	t_tm.tm_mon = tv->curTime.month- 1;
	t_tm.tm_mday = tv->curTime.day;
	t_tm.tm_hour = tv->curTime.hour;
	t_tm.tm_min = tv->curTime.min;
	t_tm.tm_sec = tv->curTime.sec;
	//DF_DEBUG("%04d%02d%02d%02d%02d%02d ",t_tm.tm_year,t_tm.tm_mon,t_tm.tm_mday,t_tm.tm_hour,t_tm.tm_min,t_tm.tm_sec);
	stv.tv_sec = mktime(&t_tm);
	settimeofday(&stv ,NULL);
	//SetDateTime(&(tv->curTime));
	info.currTime = stv.tv_sec;
	info.timeZone = tv->timeZone;
	DF_DEBUG("currTime = %d",info.currTime);
	HandleManagePostLoop(&g_timeManage.listUsrRegistHead,FunSendTimeChange,1,NULL);
	TimeManageConfigSet(&info);

	return KEY_TRUE;
}


_OUT_ int SetLocalTimeBySTR(int *timeZone,char *zone)
{
	*timeZone = GetTimeZoneIndex(zone);
	
	return KEY_TRUE;
}

_OUT_ int GetTimeZoneByGM(int *timeZone,char *zone)
{
	char sTDSign[2];
	unsigned char u32STDSign;                         /*时区符号：0为"+",1为"-"*/
    int hour;
	int min;
	
	sscanf(zone,"%01s%02d:%02d",sTDSign,&hour,&min);
	//DF_DEBUG("u32STDSign=%s,hour=%d,min=%d",sTDSign,hour,min);

	if(strcmp(sTDSign,"+") == 0)
	{
		u32STDSign = 0;
	}
	else if(strcmp(sTDSign,"-") == 0)
	{
		u32STDSign = 1;
	}
	else
	{
		DF_DEBUG("unknow char ,please input '+' or '-' ");
		return KEY_FALSE;
	}
	
	int i = 0;
	for(i = 0;NULL != g_astruTZS[i].pszCityName;i++){
		if((u32STDSign == g_astruTZS[i].u32STDSign) && (g_astruTZS[i].hour == hour) && (g_astruTZS[i].min == min))
		{
			*timeZone = g_astruTZS[i].index;
			//DF_DEBUG("timeZone:%d", *timeZone);
			return KEY_TRUE;
		}
	}
	
	return KEY_FALSE;
}

_OUT_ int GetLocalTimeBySTR(int timeZone,char *zone)
{
	char *zoneString = NULL;
	zoneString = GetTimeZoneString(timeZone);
	if(zoneString != NULL)
	{
		memcpy(zone,zoneString,strlen(zoneString));
		return KEY_TRUE;
	}
	
	return KEY_FALSE;
}

static long GetDvalueLocal2Utc()
{
	time_t timeTmp =  time(NULL);
	struct tm t_tmTmp;
	struct tm t_tmTmplocal;
	time_t timeLocal;
	time_t timeGm;
	
	localtime_r(&timeTmp,&t_tmTmplocal);
	timeLocal = mktime(&t_tmTmplocal);
	gmtime_r(&timeTmp,&t_tmTmp);
	timeGm = mktime(&t_tmTmp);
//	DF_DEBUG("timeLocal = %lu, timeGm = %lu,gop = %lu",timeLocal,timeGm,timeLocal - timeGm);
	return timeLocal - timeGm;
}

 _OUT_ int SetCurTimeUTC(TimeYMD_T *tv)
{
	Timeval_T timenow = {0};
	long setSec = 0;
	GetTimeOfDay(&timenow);

	setSec = TimeUTCYMD2Sec(tv);
	DF_DEBUG("timenow.sec = %ld,setSec = %ld!!!",timenow.sec,setSec);
	if(abs(timenow.sec - setSec) <= 2)
	{
		DF_DEBUG("set time equal now time!!!");
		return KEY_FALSE;
	}
	
 	struct timeval stv;
 	TimeConf_T info;
	 struct tm t_tm;
 	memset(&stv,0,sizeof(struct timeval));
 	memset(&info,0,sizeof(TimeConf_T));
	memset(&t_tm,0,sizeof(struct tm));
 	TimeManageConfigGet(&info);
 	t_tm.tm_year = tv->year - 1900;
 	t_tm.tm_mon = tv->month- 1;
 	t_tm.tm_mday = tv->day;
 	t_tm.tm_hour = tv->hour;
 	t_tm.tm_min = tv->min;
 	t_tm.tm_sec = tv->sec;
 	
	DF_DEBUG("tv_sec old = %lu",mktime(&t_tm));
	stv.tv_sec = mktime(&t_tm) + GetDvalueLocal2Utc();
	DF_DEBUG("tv_sec new = %lu",stv.tv_sec);
 	settimeofday(&stv ,NULL);
 	//SetDateTime(tv);
 	info.currTime = stv.tv_sec;
	HandleManagePostLoop(&g_timeManage.listUsrRegistHead,FunSendTimeChange,1,NULL);
 	TimeManageConfigSet(&info);
 	return KEY_TRUE;
}


_OUT_ int TimeSec2UTCYMD(long sec,TimeYMD_T *timeYMD)
{
	struct tm t_tm;
	time_t timer;   
	if(NULL == timeYMD){
		return KEY_FALSE;
	}
	timer = sec;
	gmtime_r(&timer,&t_tm);
	timeYMD->year= t_tm.tm_year+1900;
	timeYMD->month= t_tm.tm_mon+1;
	timeYMD->day= t_tm.tm_mday;
	timeYMD->hour= t_tm.tm_hour;
	timeYMD->min = t_tm.tm_min;
	timeYMD->sec = t_tm.tm_sec;
	return KEY_TRUE;
}

_OUT_ int TimeSec2LocalYMD(long sec,Time_T *timet)
{
	struct tm t_tm;
	time_t timer;   
	TimeConf_T info;
	memset(&info,0,sizeof(TimeConf_T));
	if(NULL == timet){
		return KEY_FALSE;
	}
	TimeManageConfigGet(&info);
	timer = sec;
	localtime_r(&timer,&t_tm);
	timet->curTime.year= t_tm.tm_year+1900;
	timet->curTime.month= t_tm.tm_mon+1;
	timet->curTime.day= t_tm.tm_mday;
	timet->curTime.hour= t_tm.tm_hour;
	timet->curTime.min = t_tm.tm_min;
	timet->curTime.sec = t_tm.tm_sec;
	timet->timeZone = info.timeZone;
	return KEY_TRUE;
}

_OUT_ long TimeUTCYMD2Sec(TimeYMD_T *timeYMD)
{
	struct tm t_tm;
	long timesec = 0;
	time_t mk_t,dvalue_t;
	
	memset(&t_tm,0,sizeof(struct tm));
	t_tm.tm_year = timeYMD->year - 1900;
	t_tm.tm_mon = timeYMD->month- 1;
	t_tm.tm_mday = timeYMD->day;
	t_tm.tm_hour = timeYMD->hour;
	t_tm.tm_min = timeYMD->min;
	t_tm.tm_sec = timeYMD->sec;
	mk_t = mktime(&t_tm);
	dvalue_t = GetDvalueLocal2Utc();
	timesec = mk_t  + dvalue_t;
	return timesec;
}


_OUT_ long TimeLocalYMD2Sec(TimeYMD_T *timet)
{
	long timesec = 0;
	struct tm t_tm;
	memset(&t_tm,0,sizeof(struct tm));
	t_tm.tm_year = timet->year - 1900;
	t_tm.tm_mon =  timet->month- 1;
	t_tm.tm_mday = timet->day;
	t_tm.tm_hour = timet->hour;
	t_tm.tm_min = timet->min;
	t_tm.tm_sec = timet->sec;
	timesec = mktime(&t_tm);
	return timesec;
}

_OUT_ int TimeUTC2LocalYMD(TimeYMD_T utcT,Time_T *localT)//UTC时间转本地时间接口
{
	long sec = 0;
	sec = TimeUTCYMD2Sec(&utcT);
	TimeSec2LocalYMD(sec,localT);
	return KEY_TRUE;
}

_OUT_ int TimeLocal2UTCYMD(TimeYMD_T localT,TimeYMD_T *utcT)//本地时间转UTC时间接口
{
	long sec = 0;
	sec = TimeLocalYMD2Sec(&localT);
	TimeSec2UTCYMD(sec,utcT);
	return KEY_TRUE;
}


//为了时间体系使用，UTC时间或者本地时间与秒值的转换只需要修改该函数即可(目前为本地时间)

//秒值转换为年月日时分秒类型
_OUT_ int TimeSec2YMD(long sec,TimeYMD_T *timet)
{
	struct tm t_tm;
	time_t timer;   
	if(NULL == timet){
		return KEY_FALSE;
	}
	timer = sec;
	localtime_r(&timer,&t_tm);
	timet->year= t_tm.tm_year+1900;
	timet->month= t_tm.tm_mon+1;
	timet->day= t_tm.tm_mday;
	timet->hour= t_tm.tm_hour;
	timet->min = t_tm.tm_min;
	timet->sec = t_tm.tm_sec;

	return KEY_TRUE;
}

//年月日时分秒类型转换为秒
_OUT_ long TimeYMD2Sec(TimeYMD_T *timet)
{
	long timesec = 0;
	struct tm t_tm;
	memset(&t_tm,0,sizeof(struct tm));
	t_tm.tm_year = timet->year - 1900;
	t_tm.tm_mon =  timet->month- 1;
	t_tm.tm_mday = timet->day;
	t_tm.tm_hour = timet->hour;
	t_tm.tm_min = timet->min;
	t_tm.tm_sec = timet->sec;
	timesec = mktime(&t_tm);
	return timesec;
}


_OUT_ int GetTimeOfDay(Timeval_T *tv)
{
	struct timeval stv;
	memset(tv,0,sizeof(Timeval_T));
	gettimeofday(&stv ,NULL);
	tv->sec = stv.tv_sec;
	tv->usec = stv.tv_usec;
	return KEY_TRUE;
}

_OUT_ int GetUTCTime(char *timeT)
{
	time_t timer;
	struct tm t_tm;	
	char *wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	char *wMouth[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	timer = time(NULL);
	gmtime_r(&timer,&t_tm);
	sprintf(timeT, "%s, %02d %s %d %02d:%02d:%02d GMT", wday[t_tm.tm_wday], t_tm.tm_mday, wMouth[t_tm.tm_mon], t_tm.tm_year + 1900, t_tm.tm_hour, t_tm.tm_min, t_tm.tm_sec);
	//printf("timeT = %s\n",timeT);

	return 0;
}

static int SetLocalTimeByConfig()
{
	struct timeval stv;
	TimeConf_T info;
	struct tm t_tm;
	Time_T timev = {0};
	memset(&stv,0,sizeof(struct timeval));
	memset(&info,0,sizeof(TimeConf_T));
	memset(&t_tm,0,sizeof(struct tm));
	
	TimeManageConfigGet(&info);
	SetTimeZone(info.timeZone);
	
	stv.tv_sec = info.currTime;
	settimeofday(&stv ,NULL);
	//SetDateTime(&(tv->curTime));
	//info.currTime = stv.tv_sec;
	//info.timeZone = info.timeZone;
	//DF_DEBUG("currTime = %d",info.currTime);
	//TimeManageConfigSet(&info);

	return KEY_TRUE;
}

static int NtpGetTimeEventFun(long sec)
{
	timesuc = 1;
	DF_DEBUG("sec = %ld",sec);
	TimeYMD_T cUtc;
	TimeSec2UTCYMD(sec, &cUtc);
	DF_DEBUG("cUtc = %d-%d-%d %d:%d:%d",cUtc.year,cUtc.month,cUtc.day,cUtc.hour,cUtc.min,cUtc.sec);
	SetCurTimeUTC(&cUtc);
	return 0;
}

#if 0
_OUT_ int GetNtpParam(Ntp_T *info)
{
	DF_DEBUG("ntpEnabled = %d",info->ntpEnabled);
	NtpParamGet(info);
	return 0;
}

_OUT_ int SetNtpParam(Ntp_T *info)
{
	if(info == NULL)
	{
		return KEY_FALSE;
	}
	NtpParamSet(info);
	return KEY_TRUE;
}

int NtpParamDefault()
{
	Ntp_T info = {0};
	info.ntpEnabled = 1;
	info.ntpInterval = 1000;
	sprintf(info.ntpSer,"%s","ntps1-0.cs.tu-berlin.de");
	SetNtpParam(&info);
	return KEY_TRUE;
}

void* NtpTask(void *arg)
{
	int ret= -1;
	Ntp_T info;
	Timeval_T tCur = {0};
	Timeval_T tOld = {0};
	NetInfoHandle_T wifiHandle = {0};
	memset(&wifiHandle,0,sizeof(wifiHandle));
	NetInfoRegistHandle(&wifiHandle);
	Time_T timer = {0};
	TimeConf_T conf = {0};
	
	prctl(PR_SET_NAME, (unsigned long)"NtpTask", 0,0,0);
	while(g_timeManage.timeTaskRun){
		GetLocalTime(&timer);

		TimeManageConfigGet(&conf);
		conf.timeZone = timer.timeZone;
		conf.currTime = TimeLocalYMD2Sec(&timer.curTime);
		TimeManageConfigSet(&conf);
		//DF_DEBUG("---conf.currTime = %d--- \n",conf.currTime);
		if(wifiHandle.pGetWifiStatus() == 0)
		{
			usleep(1000*1000);
			continue;
		}
		
		memset(&info,0,sizeof(Ntp_T));
		NtpParamGet(&info);
		GetTimeOfDay(&tCur);
		DF_DEBUG("info.ntpEnabled = %d,tCur.sec =%d,tOld.sec = %d",info.ntpEnabled,tCur.sec,tOld.sec);
		if(ENUM_ENABLED_TRUE == info.ntpEnabled && (tCur.sec - tOld.sec > 3600 || tOld.sec > tCur.sec)){	
			DF_DEBUG("NtpStart!!!!!,info.ntpSer = %s",info.ntpSer);
			timesuc = 0;
			while(g_timeManage.timeTaskRun && timesuc == 0 && info.ntpEnabled == ENUM_ENABLED_TRUE)
			{
				if(wifiHandle.pGetWifiStatus() == 0)
				{
					break;
				}
				ret = NtpStart(info.ntpSer,1, NtpGetTimeEventFun);
				DF_DEBUG("ret = %d",ret);
				usleep(10*1000*1000);
				NtpParamGet(&info);
			}
			tOld = tCur;
		}
		usleep(60*1000*1000);
	}
}

int NtpTaskStart()
{
	pthread_t nepTask;	
	g_timeManage.timeTaskRun = ENUM_ENABLED_TRUE;
	
	pthread_attr_t attr;
		
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&nepTask, &attr, NtpTask, NULL);
	pthread_attr_destroy (&attr);

	return KEY_TRUE;
}
#endif

_OUT_ int TimeManageRegist(TimeManageHandle_T *handle)
{
	HandleManageAddHandle(&g_timeManage.listUsrRegistHead,handle);
	return KEY_TRUE;
}

_OUT_ int TimeManageUnRegist(TimeManageHandle_T *handle)
{
	HandleManageDelHandle(&g_timeManage.listUsrRegistHead,handle);
	return KEY_TRUE;
}

_OUT_ int TimeManageInit()
{
//	Ntp_T ntpinfo = {0};
//	NtpParamGet(&ntpinfo);
	
	SetLocalTimeByConfig();
	memset(&g_timeManage,0,sizeof(g_timeManage));
	HandleManageInitHead(&g_timeManage.listUsrRegistHead);
//	NtpTaskStart();
	return KEY_TRUE;
}

_OUT_ int TimeManageUnInit()//保存一次时间
{
	TimeConf_T info = {0};
	TimeManageConfigGet(&info);
	TimeManageConfigSet(&info);
	
	memset(&g_timeManage,0,sizeof(g_timeManage));
	HandleManageInitHead(&g_timeManage.listUsrRegistHead);
	g_timeManage.timeTaskRun = ENUM_ENABLED_FALSE;

	DF_DEBUG("TimeManageUnInit success");
	return KEY_TRUE;
}


