#ifndef __RECORDCONFIG_H__
#define __RECORDCONFIG_H__

#include "configmanage.h"
#include "deviceinfo.h"

#ifndef D_CONFDEVINFO_STR
#define D_CONFDEVINFO_STR "ConfDevInfo"
#endif

int DevInfoConfigSet(DeviceInfo_T *info);
int DevInfoConfigGet(DeviceInfo_T *info);


#endif

