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
#ifndef __TIMEMANAGE_OUT_H__
#define __TIMEMANAGE_OUT_H__
#include "common.h"


#ifdef __cplusplus
extern "C" {
#endif 


typedef struct _Time_T{
	int timeZone;//��ǰʱ��
	int weekday;
	TimeYMD_T curTime;//��ǰʱ��
}Time_T;

typedef struct _Timeval_T{
	long sec;
	long usec;
}Timeval_T;

typedef struct _Ntp_T
{
	ENUM_ENABLED_E dhcpNtp;//NtpDHcp Ĭ��Ϊ10.1.1.1 DHCP�ر�
	ENUM_ENABLED_E ntpEnabled;//Ntp Enable
	unsigned short ntpInterval;//Ntp ����ʱ����
	char ntpSer[64];//Ntp��ַ
}Ntp_T;

typedef struct _TimeConf_T
{
	int timeZone;
	int currTime;//��ǰ����
	Ntp_T  ntp;
}TimeConf_T;



_OUT_ int GetCurTimeUTC(TimeYMD_T *tv);//��ȡ�൱��UTC��ʱ��
_OUT_ int SetCurTimeUTC(TimeYMD_T *tv);//���������UTC��ʱ��
_OUT_ int GetLocalTime(Time_T *tv);//��ȡ����ʱ��,������ʱ�任��
_OUT_ int SetLocalTime(Time_T *tv);//���ñ���ʱ��,������ʱ�任��
_OUT_ int GetTimeZone();//���� Time_Tz
_OUT_ int GetTimeOfDay(Timeval_T *tv);//��ȷ��΢�� �����UTCʱ��
_OUT_ int TimeSec2UTCYMD(long sec,TimeYMD_T *tm);//�൱��gm_time()
_OUT_ int TimeSec2LocalYMD(long sec,Time_T *tm);//�൱��local_time()
_OUT_ long TimeUTCYMD2Sec(TimeYMD_T *timeYMD);//UTC��������ת����
_OUT_ long TimeLocalYMD2Sec(TimeYMD_T *timet);//���ص�������ת����
_OUT_ int TimeUTC2LocalYMD(TimeYMD_T utcT,Time_T *localT);//UTCʱ��ת����ʱ��ӿ�
_OUT_ int TimeLocal2UTCYMD(TimeYMD_T localT,TimeYMD_T *utcT);//����ʱ��תUTCʱ��ӿ�
_OUT_ int SetLocalTimeBySTR(int *timeZone,char *zone);
_OUT_ int GetLocalTimeBySTR(int timeZone,char *zone);
_OUT_ int GetSystemTime(TimeConf_T *info);
_OUT_ int GetUTCTime(char *timeT);
#ifdef __cplusplus
}
#endif

#endif
