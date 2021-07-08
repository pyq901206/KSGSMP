/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年4月17日
|  说明:
|  修改:
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
int SetTimerOut(TimerHandle_T *handle);//响应一次
int SetTimerOutStop(TimerHandle_T *handle);//在未响应之前停止
int SetInterval();//循环响应
int SetIntervalStop();//循环响应停止
int TimerInit();//定时器初始化
int TimerUnInit();//定时器模块注销
#endif
