/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0 
|  ����: ̷�� [tanfan0406@163.com]
|  ����: 2018��2��23��
|  ˵��: û��RTC,ʱ��ģ����Ҫ���ϵ�д�����ñ�
|  �޸�:
|	
******************************************************************/
#ifndef __TIMEMANAGE_H__
#define __TIMEMANAGE_H__
#include "common.h"
#include "timemanageout.h"
#ifdef __cplusplus
extern "C" {
#endif 

typedef int (*TimeChangeFunc)();//�澯����ص�����

typedef struct _TimeManageHandle_T{
	TimeChangeFunc ChangeFun;
}TimeManageHandle_T;

#ifndef D_CONFNTPPARAM_STR
#define D_CONFNTPPARAM_STR "ConfNtp"
#endif

_OUT_ int SetNtpParam(Ntp_T *info);
_OUT_ int GetNtpParam(Ntp_T *info);
_OUT_ int TimeManageRegist(TimeManageHandle_T *handle);//Ϊ��ʱ���޸ı�֤��֪ͨ����Ӧ����
_OUT_ int TimeManageUnRegist(TimeManageHandle_T *handle);
_OUT_ int TimeManageInit();//��ʼ��һ�ξͺ� ��Ҫ�����ñ��ȡ��ǰʱ��
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
