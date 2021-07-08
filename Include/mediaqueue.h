

#ifndef __MEDIAQUEUE_H__
#define __MEDIAQUEUE_H__

#include "common.h"
#include "mediaqueueout.h"

#ifdef __cplusplus
extern "C" {
#endif 


typedef struct _MediaQueueHandle_T{
	int (*pCreateQueue)(int queueSize);
	int (*pDisdroyQueue)(int queueID);
	int (*pWriteVideoRawData)(int queueID,char *data,int len,QueueVideoInputInfo_T param,Enum_MediaType mediaType);
	int (*pWriteAudioRawData)(int queueID,char *data,int len,QueueAudioInputInfo_T param,Enum_MediaType mediaType);
}MediaQueueHandle_T;

typedef struct _MediaQueueRegister_T{
	int (*pReadData)(int registerID,int queueID,char *buff,int bufflen,int *datalen);
	int registerID;
	int queueID;
}MediaQueueRegistHandle_T;

_OUT_ int MediaQueueInit(MediaQueueHandle_T *handle);
_OUT_ int MediaQueueUnInit(MediaQueueHandle_T *handle);
_OUT_ int MediaQueueRegistReadHandle(MediaQueueRegistHandle_T *handle);
_OUT_ int MediaQueueUnRegistReadHandle(MediaQueueRegistHandle_T *handle);


#ifdef __cplusplus
}
#endif

#endif
