#ifndef __DEVICEINFO_H__
#define __DEVICEINFO_H__
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif 

typedef enum _ENUM_DEVICEOPT_E{
	ENUM_DEVICEOPT_NULL = 0x00,
	ENUM_DEVICEOPT_HARDFACDEFAULT,
	ENUM_DEVICEOPT_SOFTFACDEFAULT,
	ENUM_DEVICEOPT_REBOOT,
}ENUM_DEVICEOPT_E;

typedef enum _ENUM_DEVTYPE_E{
	ENUM_DEVTYPE_IPC = 0x00,
	ENUM_DEVTYPE_NVR,
	ENUM_DEVTYPE_PlatFrom,
}ENUM_DEVTYPE_E;
	
typedef struct _RecordLocal_T
{
	int coverDay;    //循环覆盖天数
	int pullInterval; //同一个人脸拉取间隔
}RecordLocal_T;


typedef struct _DeviceInfo_T{
	char				uuid[32];//did
	char 				city[64];
	char 				country[64];
	char				facturer[64];   //制造商
	char				model[64];   //产品型号
	char				serialNumber[64];//设备序列号
	char				softwareVer[128];//软件版本
	char 				name[128];//名字
	ENUM_DEVTYPE_E		devType;//设备类型
	int coverDay;    	//循环覆盖天数
	int pullInterval; 	//同一个人脸拉取间隔
	int	faceThreshold;  //人脸识别灵敏度
}DeviceInfo_T;

typedef struct _DeciceHandle_T{
	int (*pGetDeviceInfo)(DeviceInfo_T *info);
	int (*pSetDeviceInfo)(DeviceInfo_T *info);
	int (*pSetDeviceOperation)(ENUM_DEVICEOPT_E operation);	
}DeciceHandle_T;

_OUT_ int  LocalInfoInit(DeciceHandle_T *handle);
_OUT_ int  LocalInfoUnInit(DeciceHandle_T *handle);
_OUT_ int  LocalInfoRegist(DeciceHandle_T *handle);
_OUT_ int  LocalInfoUnRegist(DeciceHandle_T *handle);

#ifdef __cplusplus
}
#endif

#endif
