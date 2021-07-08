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
#ifndef __TIMEMANAGE_H__
#define __TIMEMANAGE_H__
#include "common.h"
#include "timemanageout.h"
#ifdef __cplusplus
extern "C" {
#endif 

typedef int (*TimeChangeFunc)();//告警结果回调定义

typedef struct _TimeManageHandle_T{
	TimeChangeFunc ChangeFun;
}TimeManageHandle_T;

#ifndef D_CONFNTPPARAM_STR
#define D_CONFNTPPARAM_STR "ConfNtp"
#endif

_OUT_ int SetNtpParam(Ntp_T *info);
_OUT_ int GetNtpParam(Ntp_T *info);
_OUT_ int TimeManageRegist(TimeManageHandle_T *handle);//为了时间修改保证会通知到对应的人
_OUT_ int TimeManageUnRegist(TimeManageHandle_T *handle);
_OUT_ int TimeManageInit();//初始化一次就好 需要从配置表获取当前时间
_OUT_ int TimeManageUnInit();
_OUT_ int TimeSec2YMD(long sec,TimeYMD_T *timet);
_OUT_ long TimeYMD2Sec(TimeYMD_T *timet);
_OUT_ int GetTimeZoneByGM(int *timeZone,char *zone);
_OUT_ char* GetTimeZoneString(int tzIndex);
_OUT_ int   GetTimeZoneIndex(char *cityName);
_OUT_ int GetUpTime(long *sec); 

#ifdef __cplusplus
}
#endif

#endif
