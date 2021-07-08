#include "alarmmanage.h"
#include "timemanageout.h"
typedef struct _AlarmManage_T{
	pthread_mutex_t 	AlarmmngMutex;
    HandleManage_T 		listHead_AlarmCB;
}AlarmManage_T;

typedef struct _AlarmMsgList_T
{
	HandleManage_T headlist;	
	int msgnum;
	pthread_mutex_t pushmutex;
}AlarmMsgList_T;

static AlarmManage_T s_AlarmInnerHandle;
static  ENUM_ENABLED_E g_innerExit = ENUM_ENABLED_FALSE;
static AlarmMsgList_T AlarmList;
#define MAX_PUSHMSG_NUM 200

static	int AlarmMsgInput(AlarmInfo_T * info)
{
	//DF_DEBUG("+++++++++++++++++PushAlarmInput start+++++++++++++++");
	int ret;
	void*pTmpmsg=NULL;
	AlarmInfo_T * msginput=NULL;
	MUTEX_LOCK(&AlarmList.pushmutex);
	
	if(AlarmList.msgnum>MAX_PUSHMSG_NUM)
	{
		pTmpmsg=HandleManageGetNextHandle(&(AlarmList.headlist));
		if(pTmpmsg==NULL)
		{
			MUTEX_UNLOCK(&AlarmList.pushmutex);
			return KEY_FALSE;
		}
		ret=HandleManageDelHandle(&(AlarmList.headlist),pTmpmsg);
		if(ret)
		{
			MUTEX_UNLOCK(&AlarmList.pushmutex);
			return KEY_FALSE;
		}
		free(pTmpmsg);
		AlarmList.msgnum--;
	}
	msginput=(AlarmInfo_T*)malloc(sizeof(AlarmInfo_T));
	memset(msginput,0,sizeof(AlarmInfo_T));
	//DF_DEBUG("%d",sizeof(msginput));
	//DF_DEBUG("[%s]-[%s]\n",remotofilepath,uuid);
	memcpy(msginput,info,sizeof(AlarmInfo_T));
	ret=HandleManageAddHandle(&(AlarmList.headlist),(void*)msginput);
	if(ret)
	{
		free(msginput);
		MUTEX_UNLOCK(&AlarmList.pushmutex);
		return KEY_FALSE;
	}
	AlarmList.msgnum++;
	MUTEX_UNLOCK(&AlarmList.pushmutex);
	//DF_DEBUG("+++++++++++++++++PushAlarmInput end+++++++++++++++");
	return KEY_TRUE;
}

static AlarmInfo_T * AlarmMsgOut()
{

	int ret=0;
	AlarmInfo_T *msg=NULL;
	MUTEX_LOCK(&AlarmList.pushmutex);
	
	msg=(AlarmInfo_T *)HandleManageGetNextHandle(&(AlarmList.headlist));
	if(msg==NULL)
	{
		MUTEX_UNLOCK(&AlarmList.pushmutex);
		return NULL;
	}
	MUTEX_UNLOCK(&AlarmList.pushmutex);

	return msg;
}

static int AlarmMsgDel(AlarmInfo_T *msg)
{
	int ret;
	MUTEX_LOCK(&(AlarmList.pushmutex));
	ret=HandleManageDelHandle(&(AlarmList.headlist),msg);
	if(ret)
	{
		MUTEX_UNLOCK(&(AlarmList.pushmutex));
		DF_DEBUG("AlarmMsgDel is error\n");
		return KEY_FALSE;
	}
	free(msg);
	AlarmList.msgnum--;
	MUTEX_UNLOCK(&(AlarmList.pushmutex));
	
	return KEY_TRUE;
}

static int AlarmPushLoopDoFunc(void *handle,int argc,void *arg[])
{
	if(handle == NULL || 0 == argc) 
		return KEY_FALSE;
	AlarmHandle_T *curHandle = (AlarmHandle_T *)handle;
	AlarmInfo_T *info = (AlarmInfo_T *)arg[0];
	if(NULL == curHandle->AlarmFun || NULL == info){
		return KEY_FALSE;
	}

	curHandle->AlarmFun(info);
	return KEY_TRUE;
}

static void *AlarmManageProc(void *arg)
{
	prctl(PR_SET_NAME, (unsigned long)"AlarmManageProc", 0,0,0);
	int ret = KEY_FALSE;
	for(;g_innerExit;)
	{
		#if 0//for test
		Time_T tv = {0};
		AlarmInfo_T inbuff = {0};
		GetLocalTime(&tv);
		memcpy(&inbuff.Alarmtime,&tv.curTime,sizeof(TimeYMD_T));
		inbuff.Alarmtype = ENUM_ALARMTYPE_SMOKE;
		AlarmMsgInput(&inbuff);	
		#endif
		AlarmInfo_T *alarminfo = NULL;
		alarminfo = AlarmMsgOut();
		if(!alarminfo)
		{
			usleep(1*1000*1000);
			continue;
		}
	//	DF_DEBUG("type = %d,time = %d.%d.%d %d:%d:%d,temp = %f",alarminfo->Alarmtype,alarminfo->Alarmtime.year,alarminfo->Alarmtime.month, \
	//		alarminfo->Alarmtime.day,alarminfo->Alarmtime.hour,alarminfo->Alarmtime.min,alarminfo->Alarmtime.sec,alarminfo->temperature);
		int argc = 1;
		void *argv[1];
		argv[0]=alarminfo;
		ret = HandleManagePostLoop(&s_AlarmInnerHandle.listHead_AlarmCB,AlarmPushLoopDoFunc,argc,argv);
		if(KEY_TRUE == ret)
		{
			ret = AlarmMsgDel(alarminfo);
			if(ret){
				DF_DEBUG("AlarmMsgDel err!!!");
			}
		}
	}
	return NULL;
}

_OUT_ int AlarmRegistHandle(AlarmHandle_T *handle)
{
	if(NULL == handle){
		return -1;
	}
	handle->pPushAlarm = AlarmMsgInput;
	MUTEX_LOCK(&s_AlarmInnerHandle.AlarmmngMutex);
	if(KEY_FALSE == HandleManageAddHandle(&s_AlarmInnerHandle.listHead_AlarmCB,handle)){
		MUTEX_UNLOCK(&s_AlarmInnerHandle.AlarmmngMutex);
		DF_DEBUG("AlarmRegist add handle fail");
		return KEY_FALSE;
	}
	MUTEX_UNLOCK(&s_AlarmInnerHandle.AlarmmngMutex);
	
	return KEY_TRUE;
}

_OUT_ int AlarmUnRegistHandle(AlarmHandle_T *handle)
{
	if(NULL == handle){
		return -1;
	}
	handle->pPushAlarm = NULL;
	MUTEX_LOCK(&s_AlarmInnerHandle.AlarmmngMutex);
	if(KEY_FALSE == HandleManageDelHandle(&s_AlarmInnerHandle.listHead_AlarmCB,handle)){
		MUTEX_UNLOCK(&s_AlarmInnerHandle.AlarmmngMutex);
		DF_DEBUG("AlarmRegist add handle fail");
		return KEY_FALSE;
	}
	MUTEX_UNLOCK(&s_AlarmInnerHandle.AlarmmngMutex);
	
	return KEY_TRUE;
}

_OUT_ int AlarmInit(AlarmHandle_T *handle)
{
	AlarmRegistHandle(handle);
	pthread_t AlarmManageThread;
	g_innerExit = ENUM_ENABLED_TRUE;
	if (pthread_create(&AlarmManageThread, 0, AlarmManageProc, (void *)NULL))
	{
		DF_DEBUG("FtpPutUpThreadProc is err \n");
		g_innerExit = ENUM_ENABLED_FALSE;
		return KEY_FALSE;
	}
	pthread_detach(AlarmManageThread);

	return KEY_TRUE;
}

_OUT_ int AlarmUnInit(AlarmHandle_T *handle)
{
	g_innerExit = ENUM_ENABLED_FALSE;
	AlarmUnRegistHandle(handle);
	return KEY_TRUE;
}


