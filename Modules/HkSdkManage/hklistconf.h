#ifndef _HKLIST_CONF_H_
#define _HKLIST_CONF_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "httpapi.h"
#include "configmanage.h"

#ifndef D_CONFDVRLIST_STR 
#define D_CONFDVRLIST_STR	"ConfDvrList"
#endif

#ifndef D_CONFDEVVICEALARMLEAVE_STR
#define D_CONFDEVVICEALARMLEAVE_STR "ConfDeviceAlarmLeave"
#endif


int HkDvrListConfigGet(HKDVRParam_T * HkDvrListParam);
int HkDvrListConfigSet(HKDVRParam_T * HkDvrListParam);
int HkDvrListConfigAdd(HKDVRParam_T * HkDvrListParam);
int HkDvrListConfigDel(HKDVRParam_T * HkDvrListParam);

int HkDeviceAlarmConfigGet(DeviceAlarmParam_T *info);
int HkDeviceAlarmConfigSet(DeviceAlarmParam_T *info);

#ifdef __cplusplus
}
#endif

#endif
