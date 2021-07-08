#ifndef __HKAPI_H__
#define __HKAPI_H__

#include "common.h"
#include "dataanalysisout.h"
#include "mediaqueueout.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NET_DVR_MAX_LEN
#define NET_DVR_MAX_LEN 64
#endif

#ifndef NET_IPC_MAX_NUM
#define NET_IPC_MAX_NUM 64
#endif

#ifndef NET_NVR_MAX_NUM
#define NET_NVR_MAX_NUM 48
#endif

#ifndef NET_FPID_NUM
#define NET_FPID_NUM 128
#endif


typedef struct _DvrOpt_T{
	int  wChannel;
	int  wQueueID;
	char sDvrSerialNumber[NET_DVR_MAX_LEN];
}DvrOpt_T;

typedef struct _HkDeviceParam_T{
	int  sOnline;
	int  sDeviceType; //设备类型
	int  sChannel;	  //通道号
	int  sQueueID;	  //通道对于的queueId
    char sDeviceName[NET_DVR_MAX_LEN];	  //通道名称
    char sDeviceAddress[NET_DVR_MAX_LEN]; //ip地址
}HkDeviceParam_T;   //IPC结构体


typedef struct _HkDeviceInfo_T{
    int  wPort;
	int  wChannel;		//数字通道数
	int  wStartDChan;	//起始通道号
	long  lUserID;
    char sUserName[NET_DVR_MAX_LEN];
    char sPassword[NET_DVR_MAX_LEN];
    char sDeviceAddress[NET_DVR_MAX_LEN];
	char sDvrName[NET_DVR_MAX_LEN];
	char sDvrSerialNumber[NET_DVR_MAX_LEN];
	char IWorkersFPID[NET_FPID_NUM];
	char ICustomerFPID[NET_FPID_NUM];
}HkDeviceInfo_T;	//DVR结构体

typedef struct _HKDVRParam_T{
	HkDeviceInfo_T  HkDvrInfo;   				 //DVR信息
	HkDeviceParam_T HkSdkParam[NET_IPC_MAX_NUM]; //IPC信息
}HKDVRParam_T;	


typedef struct _DvrParam_T
{
	char name[NET_DVR_MAX_LEN];
	char serialnumber[NET_DVR_MAX_LEN];
}DvrParam_T;

typedef struct _IpcamParam_T
{
	int channel;
	int devicetype;
	char name[NET_DVR_MAX_LEN];
	char serialnumber[NET_DVR_MAX_LEN];
}IpcamParam_T;

typedef int (*DvrOptF)(DvrOpt_T *param);
typedef int (*CreateQueueCbFun)(int queueSize);
typedef int (*DisdroyQueueCbFun)(int queueId);
typedef int (*WriteVideoRawDataCbFun)(int queueID,char *data,int len,QueueVideoInputInfo_T param,Enum_MediaType mediaType);
typedef int (*WriteAudioRawDataCbFun)(int queueID,char *data,int len,QueueAudioInputInfo_T param,Enum_MediaType mediaType);

typedef struct _HksdkManageHandle_T{
    DvrOptF addCameraCallBack;
    DvrOptF delCameraCallBack;
	
	CreateQueueCbFun  createQueneFromMediaQuenecb;
	DisdroyQueueCbFun disdroyQueneFromMediaQuenecb;

	WriteVideoRawDataCbFun writeVideoRawDatacb;
	WriteAudioRawDataCbFun writeAudioRawDatacb;
	
	int (*pHksdkDeviceAdd)(HKDVRParam_T *devInfo);
	int (*pHksdkDeviceDel)(HKDVRParam_T *devInfo);
	int (*pHksdkDeviceListGet)(HKDVRParam_T *dvrParam);
	int (*pHksdkDeviceListSet)(HKDVRParam_T *dvrParam);
	int (*pHKsdkDeviceAlarmSet)(DeviceAlarmParam_T *alarmParam);
	int (*pHKsdkDeviceAlarmGet)(DeviceAlarmParam_T *alarmParam);
}HksdkManageHandle_T;

_OUT_ int HksdkManageRegistHandle(HksdkManageHandle_T *handle);
_OUT_ int HksdkManageUnRegistHandle(HksdkManageHandle_T *handle);
_OUT_ int HksdkApiInit(HksdkManageHandle_T *handle);
_OUT_ int HksdkApiUnInit(HksdkManageHandle_T *handle);


#ifdef __cplusplus
}
#endif

#endif

