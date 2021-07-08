

#ifndef __ADPAPIINNER_H__
#define __ADPAPIINNER_H__

#include "common.h"
#include "adpapi.h"
#include "handlemanage.h"
#include "mysqlctrl.h"
#include "hkManage.h"
#include "alarmmanage.h"
#ifdef __cplusplus
extern "C" {
#endif 

typedef struct _AdpUsrRegistInfo_T{
	AdpAPIHandle_T *apiHandle;
	UsrManageRegistHandle_T usrManageRegistHandle;
	UsrInfo_T usrInfo;
	HighTemperatureDetectionRegistHandle_T highTempHandle;
}AdpUsrRegistInfo_T;

typedef struct _AdpHandle_T{
	DeciceHandle_T    localHandle;
	MediaQueueHandle_T mediaQueueHandle;
	DeviceManageHandle_T deviceHandle;
	UsrManageHandle_T usrManageHandle;
	MsgManageHandle_T msgManageHandle;
	PublicKeyManage_T publicKeyHandle;
	HandleManage_T listUsrRegistHead;
	DataAnalysisManage_T dataAnalysisHead;
	FuelQuantityHandle_T fuelQuantityHandle;
	MySqlManage_T mysqlHandle; 
	HksdkManageHandle_T hksdkHandle;
	AlarmHandle_T alarmHandle;
	AttendanceManage_T attendancehandle;
}AdpHandle_T;

#ifdef __cplusplus
}
#endif 

#endif

