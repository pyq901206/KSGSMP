/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年2月23日
|  说明: 没有RTC,时间模块需要不断的写入配置表
|  修改:
|	
******************************************************************/
#ifndef __TIMEMANAGE_OUT_H__
#define __TIMEMANAGE_OUT_H__
#include "common.h"


#ifdef __cplusplus
extern "C" {
#endif 


typedef struct _Time_T{
	int timeZone;//当前时区
	int weekday;
	TimeYMD_T curTime;//当前时间
}Time_T;

typedef struct _Timeval_T{
	long sec;
	long usec;
}Timeval_T;

typedef struct _Ntp_T
{
	ENUM_ENABLED_E dhcpNtp;//NtpDHcp 默认为10.1.1.1 DHCP关闭
	ENUM_ENABLED_E ntpEnabled;//Ntp Enable
	unsigned short ntpInterval;//Ntp 调整时间间隔
	char ntpSer[64];//Ntp地址
}Ntp_T;

typedef struct _TimeConf_T
{
	int timeZone;
	int currTime;//当前秒数
	Ntp_T  ntp;
}TimeConf_T;



_OUT_ int GetCurTimeUTC(TimeYMD_T *tv);//获取相当于UTC的时间
_OUT_ int SetCurTimeUTC(TimeYMD_T *tv);//设置相对于UTC的时间
_OUT_ int GetLocalTime(Time_T *tv);//获取本地时间,经过了时间换算
_OUT_ int SetLocalTime(Time_T *tv);//设置本地时间,经过了时间换算
_OUT_ int GetTimeZone();//返回 Time_Tz
_OUT_ int GetTimeOfDay(Timeval_T *tv);//精确到微秒 相对于UTC时间
_OUT_ int TimeSec2UTCYMD(long sec,TimeYMD_T *tm);//相当于gm_time()
_OUT_ int TimeSec2LocalYMD(long sec,Time_T *tm);//相当于local_time()
_OUT_ long TimeUTCYMD2Sec(TimeYMD_T *timeYMD);//UTC的年月日转秒数
_OUT_ long TimeLocalYMD2Sec(TimeYMD_T *timet);//本地的年月日转秒数
_OUT_ int TimeUTC2LocalYMD(TimeYMD_T utcT,Time_T *localT);//UTC时间转本地时间接口
_OUT_ int TimeLocal2UTCYMD(TimeYMD_T localT,TimeYMD_T *utcT);//本地时间转UTC时间接口
_OUT_ int SetLocalTimeBySTR(int *timeZone,char *zone);
_OUT_ int GetLocalTimeBySTR(int timeZone,char *zone);
_OUT_ int GetSystemTime(TimeConf_T *info);
_OUT_ int GetUTCTime(char *timeT);
#ifdef __cplusplus
}
#endif

#endif
