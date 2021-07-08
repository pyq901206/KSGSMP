#ifndef __MESSAGE_MANAGE_H__
#define __MESSAGE_MANAGE_H__
#include "common.h"
typedef struct _AlarmMsg_T
{
	E_ALARMTYPE type;
	TimeYMD_T date; 
}AlarmMsg_T;

typedef struct _MsgManageHandle_T{
	int (*pPushAlarmMsg)(char *deviceId, AlarmMsg_T *msg);
}MsgManageHandle_T;


_OUT_ int MsgManageRegist(MsgManageHandle_T *handle);
_OUT_ int MsgManageUnRegist(MsgManageHandle_T *handle);
_OUT_ int MsgManageInit(MsgManageHandle_T *handle);
_OUT_ int MsgManageUnInit(MsgManageHandle_T *handle);


#endif

