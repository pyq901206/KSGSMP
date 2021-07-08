#include "ttimer.h"
#include "HandleManage.h"

//毫秒级定时器一次跳变的刻度
#ifndef TIMER_RESMSEC
#define TIMER_RESMSEC 50
#endif

#define TIMER_MAX_CNT ((24 * 3600 * 1000)/TIMER_RESMSEC)
typedef struct _Timer_T{
	HandleManage_T listHead;
	HandleManage_T msgList;
	int cnt;
	pthread_mutex_t 	selfMutex;
	int run;//运行标志
}Timer_T;

static Timer_T g_timer;


static int makeIdx(int intervalEnable,int msec)
{
    int idx = g_timer.cnt;
	idx = (idx+msec/TIMER_RESMSEC)%TIMER_MAX_CNT;
	if(KEY_FALSE == intervalEnable){
		return idx;
	}
	return idx |=  0x10000000;
}

static TimerChecked(void *handle,int argc,void *arg[])
{
	int cnt = argc;
	TimerHandle_T *timerHandle = (TimerHandle_T *)handle;
	if(cnt == (timerHandle->timerIdx)& 0x0fffffff){
		HandleManageAddHandle(g_timer.msgList,handle);
	}

	if(0x10000000 == (timerHandle->timerIdx)& 0x10000000){
		timerHandle->timerIdx = makeIdx(KEY_TRUE,timerHandle->sec*1000+timerHandle->msec);
	}else{
		HandleManageDelHandle(g_timer.listHead,handle);
	}
	return KEY_TRUE;
}

void *TimerTask(void *arg){
    struct timeval tv;
    tv.tv_sec=TIMER_RESMSEC/1000;
    tv.tv_usec=(TIMER_RESMSEC%1000)*1000;
    do{
     	select(0,NULL,NULL,NULL,&tv);
		MUTEX_LOCK(&g_timer.selfMutex);
		g_timer.cnt = (g_timer.cnt++) % TIMER_MAX_CNT;
		MUTEX_UNLOCK(&g_timer.selfMutex);
		HandleManagePostLoop(&g_timer.listHead,TimerChecked,g_timer.cnt,NULL);
    }while(g_timer.run);
}

void *TimerMsgTask(void *arg)
{
	
	while(g_timer.run){
		TimerHandle_T *timerHandle = HandleManageGetNextHandle(&g_timer.msgList);
		if(NULL == timerHandle){
			SleepMs(TIMER_RESMSEC);//防止空跑
		}else{
			timerHandle->cbFun(timerHandle->arg);
			HandleManageDelHandle(timerHandle);
		}
	}
}

int SetTimerOut(TimerHandle_T *handle)
{
	MUTEX_LOCK(&g_timer.selfMutex);
	handle->timerIdx = makeIdx(KEY_FALSE,handle->sec*1000+handle->msec);
	HandleManageAddHandle(&(g_timer.listHead), handle);
	MUTEX_UNLOCK(&g_timer.selfMutex);
	return KEY_TRUE;
}

int SetTimerOutStop(TimerHandle_T *handle)
{
	MUTEX_LOCK(&g_timer.selfMutex);
	HandleManageDelHandle(&(g_timer.listHead), handle);
	MUTEX_UNLOCK(&g_timer.selfMutex);
	return KEY_TRUE;
}


int SetInterval(TimerHandle_T *handle)
{
	MUTEX_LOCK(&g_timer.selfMutex);
	handle->timerIdx = makeIdx(KEY_TRUE,handle->sec*1000+handle->msec);
	HandleManageAddHandle(&(g_timer.listHead), handle);
	MUTEX_UNLOCK(&g_timer.selfMutex);
	return KEY_TRUE;
}
int SetIntervalStop(TimerHandle_T *handle)
{
	MUTEX_LOCK(&g_timer.selfMutex);
	HandleManageDelHandle(&(g_timer.listHead), handle);
	MUTEX_UNLOCK(&g_timer.selfMutex);
	return KEY_TRUE;
}
int TimerInit()
{
	MUTEX_INIT(&g_timer.selfMutex);
	MUTEX_LOCK(&g_timer.selfMutex);
	HandleManageInitHead(&g_timer.listHead);
	HandleManageInitHead(&g_timer.msgList);
	g_timer.run = KEY_TRUE;
	MUTEX_UNLOCK(&g_timer.selfMutex);
	
	pthread_t timerTask;
	pthread_t timerMsgTask;
	if (pthread_create(&timerTask, NULL, TimerTask, NULL))
	{
		DF_ERROR("pthread_create timerTask is err \n");
	}
	pthread_detach(timerTask);


	if (pthread_create(&timerMsgTask, NULL, TimerMsgTask, NULL))
	{
		DF_ERROR("pthread_create timerMsgTask is err \n");
	}
	pthread_detach(timerMsgTask);
	return KEY_TRUE;
}
int TimerUnInit()
{
	MUTEX_LOCK(&g_timer.selfMutex);
	g_timer.run = KEY_FALSE;
	MUTEX_UNLOCK(&g_timer.selfMutex);
	return KEY_TRUE;
}
