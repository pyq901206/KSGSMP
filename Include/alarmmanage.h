#ifndef __ALARMMANAGE_H__
#define __ALARMMANAGE_H__

#include <pthread.h>
#include <sys/prctl.h>
#include "handlemanage.h"

#ifdef __cplusplus
extern "C" {
#endif




typedef enum _ENUM_ALARMTYPE_E
{
	ENUM_ALARMTYPE_DEFAULT = 0x00,
	ENUM_ALARMTYPE_TEMPERATURE,
	ENUM_ALARMTYPE_SMOKE,
}ENUM_ALARMTYPE_E;


typedef struct _AlarmInfo_T{
	ENUM_ALARMTYPE_E Alarmtype;
	TimeYMD_T Alarmtime;
	float temperature;
}AlarmInfo_T;

typedef int (*AlarmCB)(AlarmInfo_T *Alarminfo);

typedef struct _AlarmHandle_T{
	int (*pPushAlarm)(AlarmInfo_T *msg);
	AlarmCB AlarmFun;
}AlarmHandle_T;

_OUT_ int AlarmInit(AlarmHandle_T *handle);
_OUT_ int AlarmUnInit(AlarmHandle_T *handle);
_OUT_ int AlarmRegistHandle(AlarmHandle_T *handle);
_OUT_ int AlarmUnRegistHandle(AlarmHandle_T *handle);

#ifdef __cplusplus
}
#endif

#endif
