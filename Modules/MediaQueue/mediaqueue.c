#include "common.h"
#include "mediaqueue.h"
#include <unistd.h>

#define QUEUE_USE_MAX 4
#define QUEUE_CHN_MAX 201

typedef struct _QueueHandle_T{
	int 	 queueID;
	int 	 enabled;
	char 	*pWrite;
	char	*pRead[QUEUE_USE_MAX];//读指针
	char 	*dataBuff;
	int		 dataBuffSize;//BUFF的最大大小
	pthread_mutex_t    selfMutex;//自锁 读指针在读时 写指针全部失效
}QueueHandle_T;

static QueueHandle_T queueHandle[QUEUE_CHN_MAX];

/************************************
功能：打印	Queue中数据
返回值
*************************************/
int printfBuff(char *buff,int len)
{
	if(buff == NULL){
		return KEY_FALSE;
	}
	
	int i = 0;
	//printf("\r\n");
	for(i = 0;i < len;i++){
		printf("%x ",buff[i]);
	}

	printf("\r\n");
	return KEY_TRUE;
}

int CopyDataOnQueue(QueueHandle_T *handle,char *pRead,int readLen,char *buff,int bufflen)
{
	if(readLen > bufflen){
		return KEY_FALSE;
	}

	//求写指针与读指针的距离
	int rwlen = handle->pWrite - pRead;
	if(rwlen  < 0){
		rwlen = handle->dataBuffSize + rwlen;
	}
	//判断可读写数据距离在合法范围内
	if(rwlen < readLen){
//		DF_ERROR("rwlen < len");
		return KEY_FALSE;
	}
	
	int cplen = handle->dataBuffSize - (pRead - handle->dataBuff); 
	if(cplen >= readLen){
		memcpy(buff,pRead,readLen);
	}else{
		memcpy(buff,pRead,cplen);
		memcpy(buff+cplen,handle->dataBuff,readLen - cplen);
	}
	return KEY_TRUE;
}

int GetQueueID() 
{
	int i = 0;
	QueueHandle_T *handle = NULL;
	for(i = 1;i < QUEUE_CHN_MAX;i++){
		handle = &queueHandle[i];
		MUTEX_LOCK(&handle->selfMutex);
		if(ENUM_ENABLED_FALSE == handle->enabled){
			MUTEX_UNLOCK(&handle->selfMutex);
			return i;
		}
		MUTEX_UNLOCK(&handle->selfMutex);
	}
	return -1;
}

/************************************
功能:从当前帧找到下一个视频I帧
参数1: 当前帧地址
参数2: buff起始地址
参数3: buff最大大小
注意: 本函数是没有加锁的,因为一般是
在操作读写指针的时候做平移,所以不加锁
************************************/
#if 0
char *FindNextIframe(char *curW,char *pCur,char *buff,int buffSize)
{
	char frameHead[512] = {0};
	char *buffEnd = buff+buffSize;
	if(pCur == NULL || buff == NULL){
		DF_ERROR("pCur buff is 	NULL");
		return NULL;
	}
	//DF_DEBUG("1");
//	DF_DEBUG("-----------------1");
//	printfBuff(pCur,sizeof(MeidaVideo_T));
//	DF_DEBUG("-----------------1");
	int synclen = sizeof(MeidaHead_T);
	MeidaHead_T *head = NULL;
	while(1){
		memset(frameHead,0,512);
		int cplen = buffSize - (pCur - buff); 
		if(cplen >= synclen){
			memcpy(frameHead,pCur,synclen);
		}else{
			memcpy(frameHead,pCur,cplen);
			memcpy(frameHead+cplen,buff,synclen - cplen);
		}
		//printfBuff(pCur,sizeof(MeidaVideo_T)+16);

		int len;
		head = (MeidaHead_T *)pCur;//判断视音频
		if(head->sysncHead0 != D_SYNCDATA_HEAD0 || head->sysncHead1 != D_SYNCDATA_HEAD1){
			DF_DEBUG("sysncHead Error");
			return NULL;
		}

		if(MediaType_H264 ==  head->mediaType){
			memset(frameHead,0,512);
			int videoHeadLen = sizeof(MeidaVideo_T);
			if(cplen >= videoHeadLen){
				memcpy(frameHead,pCur,videoHeadLen);
			}else{
				memcpy(frameHead,pCur,cplen);
				memcpy(frameHead+cplen,buff,videoHeadLen - cplen);
			}
			MeidaVideo_T *pVideo = (MeidaVideo_T *)frameHead;//获取视频参数
			len = pVideo->datalen +  sizeof(MeidaVideo_T);
		}else if(MediaType_G711A ==  head->mediaType){
			int audioHeadLen = sizeof(MeidaAudio_T);
			if(cplen >= audioHeadLen){
				memcpy(frameHead,pCur,audioHeadLen);
			}else{
				memcpy(frameHead,pCur,cplen);
				memcpy(frameHead+cplen,buff,audioHeadLen - cplen);
			}
			MeidaAudio_T *pAudio = (MeidaAudio_T *)frameHead;//获取音频参数
			len = pAudio->datalen + sizeof(MeidaAudio_T);
		} 
		if(pCur + len > buffEnd){
			pCur = buff + (len -( buffEnd - pCur));
		}else{
			pCur += len;
		}

		head = (MeidaHead_T *)pCur;//判断视音频
		if(MediaType_H264 ==  head->mediaType){
			MeidaVideo_T *pVideo = (MeidaVideo_T *)pCur;//获取视频参数
			if(API_FRAME_TYPE_P != pVideo->info.frametype){
				//DF_DEBUG("API_FRAME_TYPE_I");
				break;
			}//else{
				//DF_DEBUG("API_FRAME_TYPE_P");
			//}
		}
	}
	//DF_DEBUG("1");
	return pCur;
}
#else
char *FindNextIframe(char *curW,char *pCur,char *buff,int buffSize)
{
	char frameHead[512] = {0};
	char *buffEnd = buff+buffSize;
	if(pCur == NULL || buff == NULL){
		DF_ERROR("pCur buff is 	NULL");
		return NULL;
	}
	QueueHandle_T handle = {0};
	handle.dataBuff = buff;
	handle.dataBuffSize = buffSize;
	handle.pWrite = curW;


	int synclen = sizeof(MeidaHead_T);
	MeidaHead_T *head = NULL;
	//DF_DEBUG("FindNextIframe start...");
	while(1){
		memset(frameHead,0,512);
		int cplen = buffSize - (pCur - buff); 
		int len;
		int synclen = sizeof(MeidaHead_T);
		memset(frameHead,0,512);
		//求协议头
		if(KEY_FALSE == CopyDataOnQueue(&handle,pCur,synclen,frameHead,512)){
			DF_ERROR("ReadData MeidaHead_T error");
			return NULL;
		}
		
		head = (MeidaHead_T *)frameHead;
		//head = (MeidaHead_T *)pCur;//判断视音频
		if(head->sysncHead0 != D_SYNCDATA_HEAD0 || head->sysncHead1 != D_SYNCDATA_HEAD1){
			printfBuff(pCur,sizeof(MeidaHead_T));
			DF_DEBUG("sysncHead Error");
			return NULL;
		}

		//求数据长度
		if(MediaType_H264 ==  head->mediaType  || MediaType_H265 == head->mediaType){
			memset(frameHead,0,512);
			int videoHeadLen = sizeof(MeidaVideo_T);
			if(KEY_FALSE == CopyDataOnQueue(&handle,pCur,videoHeadLen,frameHead,512)){
				DF_ERROR("ReadData MeidaVideo_T error");
				return NULL;
			}
			MeidaVideo_T *pVideo = (MeidaVideo_T *)frameHead;//获取视频参数
			len = pVideo->datalen +  sizeof(MeidaVideo_T);
		}else if(MediaType_G711A ==  head->mediaType){
			memset(frameHead,0,512);
			int audioHeadLen = sizeof(MeidaAudio_T);
			if(KEY_FALSE == CopyDataOnQueue(&handle,pCur,audioHeadLen,frameHead,512)){
				DF_ERROR("ReadData MeidaVideo_T error");
				return NULL;
			}
			MeidaAudio_T *pAudio = (MeidaAudio_T *)frameHead;//获取音频参数
			len = pAudio->datalen + sizeof(MeidaAudio_T);
		}
		
		//DF_DEBUG("pCur = %p,len = %d,buffEnd = %p,buff = %p",pCur,len,buffEnd,buff);
		if(pCur + len > buffEnd){
			pCur = buff + (len -( buffEnd - pCur));
			//DF_DEBUG("111 pCur = %p,len = %d",pCur,len);
		}else{
			pCur += len;
			//DF_DEBUG("222 pCur = %p,len = %d",pCur,len);
		}
		
		memset(frameHead,0,512);
		//求协议头
		if(KEY_FALSE == CopyDataOnQueue(&handle,pCur,synclen,frameHead,512)){
			DF_ERROR("ReadData MeidaHead_T error");
			return NULL;
		}
		
		head = (MeidaHead_T *)frameHead;
		//head = (MeidaHead_T *)pCur;//判断视音频
		if(head->sysncHead0 != D_SYNCDATA_HEAD0 || head->sysncHead1 != D_SYNCDATA_HEAD1){
			printfBuff(pCur,sizeof(MeidaHead_T));
			DF_DEBUG("sysncHead Error");
			return NULL;
		}

		if(MediaType_H264 ==  head->mediaType || MediaType_H265 == head->mediaType){
			memset(frameHead,0,512);
			int videoHeadLen = sizeof(MeidaVideo_T);
			if(KEY_FALSE == CopyDataOnQueue(&handle,pCur,videoHeadLen,frameHead,512)){
				DF_ERROR("ReadData MeidaVideo_T error");
				return NULL;
			}
			MeidaVideo_T *pVideo = (MeidaVideo_T *)frameHead;//获取视频参数
			if(API_FRAME_TYPE_P != pVideo->info.frametype){
			//	DF_DEBUG("API_FRAME_TYPE_I");
			//	DF_DEBUG("move pCur = %p",pCur);
				break;
			}
			else{
			//	DF_DEBUG("API_FRAME_TYPE_P");
			}
		}
	}
	//DF_DEBUG("1");
	return pCur;
}

#endif
/************************************
功能:移动读指针到安全位置
参数1: 当前读指针
参数2: 当前写指针
参数3: 写长度
问?: Queue起始位置
问?: Queue大小
注意: 本函数是没有加锁的,因为一般是
在操作读写指针的时候做平移,所以不加锁

诀窍:读指针所在的I帧不能被 写指针覆盖掉,
如果当前写的帧特别大,那么会覆盖下一个I帧,
所以I帧必须超过写指针+写长度 的距离
************************************/
char *MoveReadPoint(char *curR,char *curW,int writeLen,char *buff,int buffSize)
{
	char *pR = NULL; 
	//没有写超过QUEUE大小
	if(curW + writeLen <= buff + buffSize){
		//DF_DEBUG("Normal QUEUE");
		for(;;){
			pR = FindNextIframe(curW,curR,buff, buffSize);
			if(NULL != pR){
				if(pR > curW && pR <= curW + writeLen){
					curR = pR;
					//DF_DEBUG("curW = %p,pR = %p,writeLen=%d",curW,pR,writeLen);
					continue;
				}
			//	DF_DEBUG("--1--");
			}
			//DF_DEBUG("--2--");
			break;
		}
	}else{
		//超过QUEUE大小
		//DF_DEBUG("More than QUEUE");
		int dis = curW + writeLen - (buff + buffSize);
		for(;;){
			pR = FindNextIframe(curW,curR,buff, buffSize);
			if(NULL != pR){
				if(pR > curW && pR <= buff + buffSize){
					curR = pR;
					continue;
				}
				if(pR <= buff + dis){
					curR = pR;
					continue;
				}
			}
			
			break;
		}
	}
	if(NULL == pR){
		printfBuff(curR,20);
		assert(0);
	}
	return pR;
}



	

/************************************
功能：向mediaQueue写入数据
参数1：数据指针
参数2：数据长度
返回值
*************************************/
int WriteData(int queueID,char *data, int len)
{
	QueueHandle_T *handle = &queueHandle[queueID];
	MUTEX_LOCK(&handle->selfMutex);
	int writeLen = handle->dataBuffSize - (handle->pWrite - handle->dataBuff);
	if(ENUM_ENABLED_FALSE == handle->enabled || NULL == handle->pWrite){
		MUTEX_UNLOCK(&handle->selfMutex);
		return KEY_FALSE;		
	}
	if(writeLen >= len){
		//先要平移read的位置
		{
			int i = 0;
			for(i = 0; i < QUEUE_USE_MAX;i++){
				if(handle->pRead[i] > handle->pWrite && handle->pRead[i] < handle->pWrite+len){
					//DF_DEBUG("TEST 1");
					handle->pRead[i] = MoveReadPoint(handle->pRead[i],handle->pWrite,len,handle->dataBuff,handle->dataBuffSize);
					if (NULL == handle->pRead[i])
					{
						MUTEX_UNLOCK(&handle->selfMutex);	
						return KEY_FALSE;
					}
				}
			}
		}
		//直接写
		memcpy(handle->pWrite,data,len);
	}else{

		{
			int i = 0;
			for(i = 0; i < QUEUE_USE_MAX;i++){
				//读指针大于写指针小于末尾指针 被写指针覆盖掉了 直接挪到写指针位置
				if(handle->pRead[i] > handle->pWrite && handle->pRead[i] <= (handle->dataBuff + handle->dataBuffSize)){
					DF_DEBUG("TEST 2");
					handle->pRead[i] = MoveReadPoint(handle->pRead[i],handle->pWrite,len,handle->dataBuff,handle->dataBuffSize);
					if (NULL == handle->pRead[i])
					{
						MUTEX_UNLOCK(&handle->selfMutex);	
						return KEY_FALSE;
					}
				}
			}

			//读指针大于0 小于 最后要写入的位置 直接覆盖
			for(i = 0; i < QUEUE_USE_MAX;i++){
				if(handle->pRead[i] >= handle->dataBuff && handle->pRead[i] <= (handle->dataBuff + len - writeLen)){
					DF_DEBUG("TEST 3");
					handle->pRead[i] = MoveReadPoint(handle->pRead[i],handle->pWrite,len,handle->dataBuff,handle->dataBuffSize);
					if (NULL == handle->pRead[i])
					{
						MUTEX_UNLOCK(&handle->selfMutex);	
						return KEY_FALSE;
					}
				}
			}
		}
		//写到buff的末尾
		memcpy(handle->pWrite,data,writeLen);
		//buff从头继续写没写完的
		memcpy(handle->dataBuff,data+writeLen,len - writeLen);
	}
	handle->pWrite = handle->dataBuff + ((handle->pWrite - handle->dataBuff) + len)%handle->dataBuffSize;//写指针置位
	MUTEX_UNLOCK(&handle->selfMutex);	
	return KEY_TRUE;
}

/************************************
功能：获取数据
参数1：用户ID
参数2：QueueID
参数3: 获取数据buff
参数4：buff大小
参数5：返回数据长度
返回值: 
*************************************/
_OUT_ int ReadData(int registerID,int queueID,char *buff,int bufflen,int *datalen)
{
	QueueHandle_T *handle = &queueHandle[queueID];
	MUTEX_LOCK(&handle->selfMutex);
	char *pRead = handle->pRead[registerID];
	int len = 0;
	if(ENUM_ENABLED_FALSE == handle->enabled || NULL == pRead){
		MUTEX_UNLOCK(&handle->selfMutex);
		if(NULL == pRead){
			DF_ERROR("pRead NULL");
		}else{
		DF_ERROR("ReadData ENUM_ENABLED_FALSE");
		}
	//	assert(0);
		return KEY_FALSE;
	}

	char frameHead[512] = {0};
	int synclen = sizeof(MeidaHead_T);
	memset(frameHead,0,512);

	//求协议头
	if(KEY_FALSE == CopyDataOnQueue(handle,pRead,synclen,frameHead,512)){
		MUTEX_UNLOCK(&handle->selfMutex);
		//DF_ERROR("ReadData MeidaHead_T error");
		return KEY_FALSE;
	}
	MeidaHead_T *head = (MeidaHead_T *)frameHead;
	if(head->sysncHead0 != D_SYNCDATA_HEAD0 || head->sysncHead1 != D_SYNCDATA_HEAD1){
		DF_DEBUG("sysncHead Error");
		MUTEX_UNLOCK(&handle->selfMutex);
		return KEY_FALSE;
	}
	//求数据长度
	if(MediaType_H264 ==  head->mediaType ||  MediaType_H265 == head->mediaType){
		memset(frameHead,0,512);
		int videoHeadLen = sizeof(MeidaVideo_T);
		if(KEY_FALSE == CopyDataOnQueue(handle,pRead,videoHeadLen,frameHead,512)){
			MUTEX_UNLOCK(&handle->selfMutex);
			DF_ERROR("ReadData MeidaVideo_T error");
			return KEY_FALSE;
		}

		MeidaVideo_T *pVideo = (MeidaVideo_T *)frameHead;//获取视频参数
		len = pVideo->datalen +  sizeof(MeidaVideo_T);
	}else if(MediaType_G711A ==  head->mediaType){
		memset(frameHead,0,512);
		int audioHeadLen = sizeof(MeidaAudio_T);
		if(KEY_FALSE == CopyDataOnQueue(handle,pRead,audioHeadLen,frameHead,512)){
			MUTEX_UNLOCK(&handle->selfMutex);
			DF_ERROR("ReadData MeidaVideo_T error");
			return KEY_FALSE;
		}
		MeidaAudio_T *pAudio = (MeidaAudio_T *)frameHead;//获取音频参数
		len = pAudio->datalen + sizeof(MeidaAudio_T);
	} 
	//拷贝数据
	if(KEY_FALSE == CopyDataOnQueue(handle,pRead,len,buff,bufflen)){
		MUTEX_UNLOCK(&handle->selfMutex);
		DF_ERROR("ReadData read data error");
		return KEY_FALSE;
	}
	//偏移读指针
	if(pRead + len > (handle->dataBuff+handle->dataBuffSize)){
		handle->pRead[registerID] = handle->dataBuff + (pRead + len  - (handle->dataBuff+handle->dataBuffSize));
	}else{
		handle->pRead[registerID] = pRead + len;
	}
	MUTEX_UNLOCK(&handle->selfMutex);
	*datalen = len;
	return KEY_TRUE;
}


/************************************
功能：创建MediaQueue
参数1：QueueID
参数2: 队列的大小
返回值
*************************************/

_OUT_ int CreateQueue(int queueSize)
{
	int queueID = GetQueueID();
	if(queueID == -1){
		DF_ERROR("CreateQueue fail");
		return -1;
	}
	QueueHandle_T *handle = &queueHandle[queueID];
	MUTEX_LOCK(&handle->selfMutex);
	if(ENUM_ENABLED_TRUE == handle->enabled){
		MUTEX_UNLOCK(&handle->selfMutex);
		DF_ERROR("CreateQueue fail");
		return -1;		
	}
	handle->dataBuff = (char *)malloc(queueSize);
	if(NULL == handle->dataBuff){
		MUTEX_UNLOCK(&handle->selfMutex);
		DF_ERROR("CreateQueue malloc  fail");
		return -1;
	}
	memset(handle->dataBuff,0,queueSize);
	handle->enabled = ENUM_ENABLED_TRUE;
	handle->dataBuffSize = queueSize;
	handle->pWrite = handle->dataBuff;
	
	int i = 0;
	for(i = 0; i < QUEUE_USE_MAX;i++){
		handle->pRead[i] = NULL;//后面用了NULL 作为是否可以使用该指针的判断条件
	}
	MUTEX_UNLOCK(&handle->selfMutex);
	DF_DEBUG("CreateQueue Success %d !", queueID);
	return queueID;
}

/************************************
功能：注销Queue
参数1：QueueID
返回值
*************************************/
_OUT_ int DisdroyQueue(int queueID)
{
	QueueHandle_T *handle = &queueHandle[queueID];
	if (NULL == handle)
	{
		return KEY_FALSE;
	}
	
	MUTEX_LOCK(&handle->selfMutex);
	if(ENUM_ENABLED_FALSE == handle->enabled){
		MUTEX_UNLOCK(&handle->selfMutex);
		return KEY_FALSE;		
	}
	handle->enabled = ENUM_ENABLED_FALSE;
	handle->pWrite = NULL;
	int i = 0;
	for(i = 0; i < QUEUE_USE_MAX;i++){
		handle->pRead[i] = NULL;
	}
	if (NULL != handle->dataBuff)
	{
		free(handle->dataBuff);
		handle->dataBuff = NULL;
	}
	handle->dataBuffSize = 0;
	MUTEX_UNLOCK(&handle->selfMutex);
	return KEY_TRUE;
}



/************************************
功能：写视频raw数据
返回值
*************************************/
static int WriteVideoRawData(int queueID,char *data,int len,QueueVideoInputInfo_T param,Enum_MediaType mediaType)
{
	static int	stat = KEY_FALSE;
	static char *queueBuf[QUEUE_CHN_MAX];
	if(KEY_FALSE == stat){
		int i = 0;
		for(i = 0;i < QUEUE_CHN_MAX; i++){
			queueBuf[i] = NULL;
		}
		stat = KEY_TRUE;
	}
	
	char *buff = queueBuf[queueID]; 
	if(buff == NULL){
		queueBuf[queueID] = (char *)malloc(400*1024);
		buff = queueBuf[queueID];
	}
	memset(buff,0,400*1024);
	MeidaVideo_T *video = (MeidaVideo_T *)buff;
	video->head.sysncHead0 = D_SYNCDATA_HEAD0;
	video->head.sysncHead1 = D_SYNCDATA_HEAD1;
	video->head.mediaType = mediaType;
	memcpy(&video->info,&param,sizeof(QueueVideoInputInfo_T));
	video->datalen = len;
	memcpy(buff+sizeof(MeidaVideo_T),data,len);
	WriteData(queueID,buff,sizeof(MeidaVideo_T)+len);
	//printfBuff(buff,sizeof(MeidaVideo_T)+16);
	return KEY_TRUE;
}

/************************************
功能：写AudioRaw数据
返回值
*************************************/
static int WriteAudioRawData(int queueID,char *data,int len,QueueAudioInputInfo_T param,Enum_MediaType mediaType)
{
	static int	stat = KEY_FALSE;
	static char *queueBuf[QUEUE_CHN_MAX];
	if(KEY_FALSE == stat){
		int i = 0;
		for(i = 0;i < QUEUE_CHN_MAX; i++){
			queueBuf[i] = NULL;
		}
		stat = KEY_TRUE;
	}
	
	char *buff = queueBuf[queueID]; 
	if(buff == NULL){
		queueBuf[queueID] = (char *)malloc(320);
		buff = queueBuf[queueID];
	}
	memset(buff,0,320);
	MeidaAudio_T *audio = (MeidaAudio_T *)buff;
	audio->head.sysncHead0 = D_SYNCDATA_HEAD0;
	audio->head.sysncHead1 = D_SYNCDATA_HEAD1;
	audio->head.mediaType = mediaType;
	memcpy(&audio->info,&param,sizeof(QueueAudioInputInfo_T));
	audio->datalen = len;
	memcpy(buff+sizeof(MeidaAudio_T),data,len);
	WriteData(queueID,buff,sizeof(MeidaAudio_T)+len);
	//printfBuff(buff,sizeof(MeidaVideo_T)+16);
	return KEY_TRUE;
}

/************************************
功能：注册读的用户ID
参数1：参数2：QueueID
返回值： 
失败： -1 
成功： registerID
*************************************/
_OUT_ int MediaQueueRegistReadHandle(MediaQueueRegistHandle_T *handle)
{
	if(NULL == handle || handle->queueID < 0 || handle->queueID > QUEUE_CHN_MAX){
		return -1;
	}
	
	int registerID = KEY_FALSE;
	int queueID = handle->queueID;
	int i = 0;
	QueueHandle_T *qHandle = &queueHandle[queueID];
	MUTEX_LOCK(&qHandle->selfMutex);
	if(ENUM_ENABLED_FALSE == qHandle->enabled){
		MUTEX_UNLOCK(&qHandle->selfMutex);
		return KEY_FALSE;		
	}
	
	for(i = 0; i < QUEUE_USE_MAX;i++){
		if(NULL == qHandle->pRead[i]){
			registerID = i;
			break;
		}
	}
	qHandle->pRead[registerID] = qHandle->pWrite;
	handle->registerID = registerID;
	MUTEX_UNLOCK(&qHandle->selfMutex);
	handle->pReadData = ReadData;
	return registerID;
}

/************************************
功能：注销读的用户ID
参数1：参数2：QueueID
返回值：
失败： -1 
成功： registerID
*************************************/
_OUT_ int MediaQueueUnRegistReadHandle(MediaQueueRegistHandle_T *handle)
{
	if(NULL == handle || handle->queueID < 0 || handle->queueID > QUEUE_CHN_MAX){
		return -1;
	}	
	int queueID = handle->queueID;
	int registerID = handle->registerID;
	QueueHandle_T *qhandle = &queueHandle[queueID];
	MUTEX_LOCK(&qhandle->selfMutex);
	qhandle->pRead[registerID] = NULL;
	MUTEX_UNLOCK(&qhandle->selfMutex);
	handle->pReadData = NULL;
	return KEY_TRUE;
}

/************************************
功能：QueueInit
参数1：
返回值
*************************************/
_OUT_ int MediaQueueInit(MediaQueueHandle_T *handle)
{	

	if(NULL == handle){
		return KEY_FALSE;
	}
	memset(handle,0,sizeof(MediaQueueHandle_T));
	int i = 0; 
	for( i =0;i < QUEUE_CHN_MAX;i++){
		memset(&queueHandle[i],0,sizeof(QueueHandle_T));
		queueHandle[i].enabled = ENUM_ENABLED_FALSE;
		MUTEX_INIT(&queueHandle[i].selfMutex);
		queueHandle[i].queueID = i;
	}

	handle->pCreateQueue = CreateQueue;
	handle->pDisdroyQueue = DisdroyQueue;
	handle->pWriteVideoRawData = WriteVideoRawData;	
	handle->pWriteAudioRawData = WriteAudioRawData;	
	return KEY_TRUE;
}

/************************************
功能：反初始化
返回值
*************************************/
_OUT_ int MediaQueueUnInit(MediaQueueHandle_T *handle)
{
	int i = 0;
	for( i =0;i < QUEUE_CHN_MAX;i++){
		DisdroyQueue(i);
	}
	handle->pCreateQueue = NULL;
	handle->pDisdroyQueue = NULL;
	handle->pWriteVideoRawData = NULL;	
	handle->pWriteAudioRawData = NULL;	
	return KEY_TRUE;	
}

#if 0
///////////////////////////////////TesT
int runflag;

void *SndStreamTask(void *args)
{
	avi_t *avifile = NULL;
	char *buffer = NULL;
	int  framelen = 0;
	int chn = (int)args;
	
	int mark = 0;
	_printd("SndStreamTask is Running");
	buffer = (char *)malloc (sizeof(char )* 350*1024);
	avifile = AVI_open_input_file("./720.avi", 1);
	if (NULL == avifile){ _printd("file not find [%s]", "./720.avi"); getchar();}
	AVI_seek_start(avifile);
	int i = 0;
	while (runflag){
//		DF_DEBUG("write start");
		framelen = AVI_read_frame(avifile, buffer);
		if (framelen <= 0) {AVI_seek_start(avifile);i++;DF_DEBUG("fileCnt = %d",i);continue;}
	//	_printd("read file len = %10d", framelen);
		if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00 && buffer[3] == 0x01 && buffer[4] == 0x67){
			mark = 1;
		//	printf ("Find I idr frame");
		}
		if (mark == 0 ) continue;
		//DF_DEBUG("1 framelen =%d",framelen);
		QueueVideoInputInfo_T param;
		param.frametype = API_FRAME_TYPE_P;
		if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00 && buffer[3] == 0x01 && buffer[4] == 0x67){
			param.frametype = API_FRAME_TYPE_I;
		}
		param.bitrate = 1024;
		param.fps  = 15;
		param.reslution.width = 1270;
		param.reslution.height = 720;
		WriteVideoRawData(chn,buffer,framelen,param,MediaType_H264);
		usleep(16);
	}
}

void *readTest(void *args)
{
	char *buff = malloc(350*1024);
	memset(buff,0,350*1024);
	int registerID = (int)args;
	int dataLen = 0;
	DF_DEBUG("readTest start");
	while (runflag){
		//DF_ERROR("222")
		if(KEY_TRUE == ReadData(registerID, 1, buff,350*1024, &dataLen) &&  0 != dataLen){
			//DF_DEBUG("dataLen = %d",dataLen);
		}
		//DF_ERROR("222")
		usleep(2);
	}
}


int main()
{
	runflag = 1;
	MediaQueueInit();
	CreateQueue(100*1024*4);
	usleep(1000*100);
	CreateQueue(100*1024*15*10);
	usleep(1000*100);
	CreateQueue(100*1024*15*10);
	usleep(1000*100);
	CreateQueue(100*1024*15*10);
	usleep(1000*100);
	
	pthread_t pthid;
	pthread_create(&pthid, NULL, SndStreamTask, (void *)1);
	usleep(1000*1000*2);

	int regisID0 = RegistReadQueueHandle(1);

#if 1	
	//读测试
	
	DF_DEBUG("regisID = %d",regisID0);
	pthread_t pthid0;
	pthread_create(&pthid0, NULL, readTest, (void *)regisID0);
	usleep(1000*1000*2);


	pthread_t pthid1;

	int regisID1 = RegistReadQueueHandle(1);
	pthread_create(&pthid1, NULL, readTest, (void *)regisID1);
	usleep(1000*1000*2);


	int regisID2 = RegistReadQueueHandle(1);
	DF_DEBUG("regisID = %d",regisID2);
	pthread_t pthid2;
	pthread_create(&pthid2, NULL, readTest, (void *)regisID2);
	usleep(1000*1000*2);


	int regisID3 = RegistReadQueueHandle(1);
	DF_DEBUG("regisID = %d",regisID3);
	pthread_t pthid3;
	pthread_create(&pthid3, NULL, readTest, (void *)regisID3);
	usleep(1000*1000*2);
#endif



	pthread_create(&pthid1, NULL, SndStreamTask, (void *)2);
	usleep(1000*1000*2);
#if 1	
	//读测试
	int regisID4 = RegistReadQueueHandle(2);
	
	
	DF_DEBUG("regisID = %d",regisID0);
	pthread_create(&pthid0, NULL, readTest, (void *)regisID0);
	usleep(1000*1000*2);



	regisID0 = RegistReadQueueHandle(2);
	pthread_create(&pthid0, NULL, readTest, (void *)regisID0);
	usleep(1000*1000*2);


	regisID0 = RegistReadQueueHandle(2);
	DF_DEBUG("regisID = %d",regisID2);
	pthread_create(&pthid0, NULL, readTest, (void *)regisID0);
	usleep(1000*1000*2);

	regisID0 = RegistReadQueueHandle(2);
	DF_DEBUG("regisID = %d",regisID3);
	pthread_create(&pthid0, NULL, readTest, (void *)regisID0);
	usleep(1000*1000*2);
#endif

	getchar();
	runflag = 0;
	usleep(1000*1000*2);
	MediaQueueUnInit();
	return 0;
}
#endif
