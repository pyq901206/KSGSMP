/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0 
|  ����: ̷�� [tanfan0406@163.com]
|  ����: 2018��4��17��
|  ˵��:
|  �޸�:
|	
******************************************************************/
#ifndef __TIMER_H__
#define __TIMER_H__

typedef int (*TimerOutCbFun)(void *arg);

typedef struct _TimerHandle_T{
	int sec;
	int msec; 
	TimerOutCbFun cbFun;
	void *arg;
	int timerIdx;
}TimerHandle_T;
int SetTimerOut(TimerHandle_T *handle);//��Ӧһ��
int SetTimerOutStop(TimerHandle_T *handle);//��δ��Ӧ֮ǰֹͣ
int SetInterval();//ѭ����Ӧ
int SetIntervalStop();//ѭ����Ӧֹͣ
int TimerInit();//��ʱ����ʼ��
int TimerUnInit();//��ʱ��ģ��ע��
#endif
