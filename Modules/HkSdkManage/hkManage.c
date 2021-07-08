#include <time.h>
#include <iconv.h>
#include "userdefine.h"
#include "hkManage.h"
#include "handlemanage.h"
#include "hklistconf.h"
#include "hkDataprase.h"
#include "HCNetSDK.h"
#include "webs.h"
#include "mxml.h"
#include "mxml-private.h"
#include "curl.h"
#include "attendancemanage.h"
#include "mqttapi.h"

#define HK_DEBUG(msg...) printf("[%s,%d]=> ",__FILE__,__LINE__);printf(msg);printf("\r\n");
#define HK_ERROR(msg...) printf("[%s,%d]=>error: ",__FILE__,__LINE__);printf(msg);printf("\r\n");

#define OUTLEN 255
#define MAX_DEV 1
#define MAX_FRAME_LENTH 64
#define MAX_PACK_SIZE 5120 

#define GET_YEAR(_time_)        (((_time_)>>26) + 2000) 
#define GET_MONTH(_time_)       (((_time_)>>22) & 15)
#define GET_DAY(_time_)         (((_time_)>>17) & 31)
#define GET_HOUR(_time_)        (((_time_)>>12) & 31)
#define GET_MINUTE(_time_)     (((_time_)>>6) & 63)
#define GET_SECOND(_time_)     (((_time_)>>0) & 63) 

typedef struct tagCacheBuffder_T
{
	BYTE*  pCacheBuffer;
	int	   nCacheBufLenth;
	long   lTimeStamp;
	DWORD  dwFrameLenth;
}CacheBuffder;

//码流数据缓存结构体
typedef struct{
	int m_mFrameCacheLenth[NET_IPC_MAX_NUM];
	CacheBuffder m_pFrameCache[NET_IPC_MAX_NUM][MAX_FRAME_LENTH];
}FRAME_FD, *PFREAE_FD;
static FRAME_FD _frame_fd = {0};

//数据库
typedef struct _HKstore_Regist_T
{
	DataAnalysisManage_T DataAnalysishandle;
	AttendanceManage_T attendancehandle;
}HKstore_Regist_T;
static HKstore_Regist_T  gHKstoreRegist;
static HKstore_Regist_T *GetHKstoreRegistHandle()
{
	return &gHKstoreRegist;
}

//在线链表 存放登陆ID等
typedef struct _HkSdk_DeviceInfo_T
{
	int	 lUserID;
	int  lhandle;	//告警监听handle
	int  lChannel[NET_IPC_MAX_NUM];
	int  lqueueId[NET_IPC_MAX_NUM];
	int  lRealPlayHandle[NET_IPC_MAX_NUM];
	char lDvrSerialNumber[NET_DVR_MAX_LEN];
}HkSdk_DeviceInfo;

typedef struct _HkSdk_Regist_T
{
	HkSdk_DeviceInfo dvrInfo;
	HandleManage_T dvrlistHead;
}HkSdk_Regist_T;
static HkSdk_Regist_T  gHksdkRegist;
static HkSdk_Regist_T *GetHksdkRegistHandle()
{
	return &gHksdkRegist;
}

//对外链表包含dvr所有信息
typedef struct _HkSdk_Inner_T
{	
	HKDVRParam_T 	HkDvrInfo[NET_DVR_MAX_NUM];
	HandleManage_T  deviceListHead;   
}HkSdk_Inner_T;
static HkSdk_Inner_T gHkSdkInner;
static HkSdk_Inner_T *GetHkSdkHandle()
{
	return &gHkSdkInner;
}

//用户注册链表
typedef struct _HksdkUserRegist_T
{
	HandleManage_T  deviceListHead;   
}HksdkUserRegist_T;
static HksdkUserRegist_T gHksdkUserRegist;
static HksdkUserRegist_T *GetHksdkUserRegistHandle()
{
	return &gHksdkUserRegist;
}

typedef struct _Hksdk_Mediaqueue_T
{
	CreateQueueCbFun  createQueneFromMediaQuenecb;
	DisdroyQueueCbFun disdroyQueneFromMediaQuenecb;
	
	WriteVideoRawDataCbFun writeVideoRawDatacb;
	WriteAudioRawDataCbFun writeAudioRawDatacb;
}Hksdk_Mediaqueue_T;
static Hksdk_Mediaqueue_T gHKMediaqueueRegist;
static Hksdk_Mediaqueue_T *GetMediaqueueRegistHandle()
{
	return &gHKMediaqueueRegist;
}

typedef struct _MemoryStruct_T {
  char *memory;
  int size;
}MemoryStruct_T;
static int HkSdkInit();

BOOL GetH246FromPS(BYTE* pBuffer, int nBufLenth, BYTE** pH264, int& nH264Lenth, BOOL& bVideo)
{
	if(!pBuffer || nBufLenth <= 0)
	{
		return FALSE;
	}
	
	int   nHerderLen  = 0;
	BYTE *pH264Buffer = NULL;

	if(pBuffer && pBuffer[0]==0x00 && pBuffer[1]==0x00 && pBuffer[2]==0x01 && pBuffer[3]==0xE0)//E==视频数据(此处E0标识为视频)
	{
		bVideo = TRUE;
		nHerderLen  = 9 + (int)pBuffer[8];//9个为固定的数据包头长度，pBuffer[8]为填充头部分的长度
		pH264Buffer = pBuffer + nHerderLen;
		if(*pH264 == NULL)
		{
			*pH264 = (BYTE*)malloc(nBufLenth);
		}
		
		if (*pH264 && pH264Buffer && (nBufLenth-nHerderLen) > 0)
		{
			memcpy(*pH264, pH264Buffer, (nBufLenth-nHerderLen));
		}
		
		nH264Lenth = nBufLenth-nHerderLen;
		return TRUE;
	}
	else if(pBuffer && pBuffer[0]==0x00 && pBuffer[1]==0x00 && pBuffer[2]==0x01 && pBuffer[3]==0xBA)//视频流数据包 包头
	{
		 bVideo = TRUE;
		*pH264  = NULL;
		 nH264Lenth = 0;
		 return FALSE;
	}
	return FALSE;
}

int GetDeviceType(int channel, char *SerialNumber, HkDeviceParam_T *deviceInfo)
{
	int i = 0;
	void *resultHanlde = NULL;
	HandleManage_T *head = &GetHkSdkHandle()->deviceListHead;
	
	HandleManageGetHandleByStr(head, HkDvrInfo.sDvrSerialNumber, (char *)SerialNumber, resultHanlde, HKDVRParam_T);
	if(NULL != resultHanlde)
	{
		HKDVRParam_T *dvrparam = (HKDVRParam_T *)resultHanlde;
		for(i = 0; i < NET_IPC_MAX_NUM; i++)
		{
			//if(channel == dvrparam->HkSdkParam[i].sChannel + dvrparam->HkDvrInfo.wStartDChan)
			if(channel == dvrparam->HkSdkParam[i].sChannel)
			{
				deviceInfo->sDeviceType = (DeviceType_T)dvrparam->HkSdkParam[i].sDeviceType;
				memcpy(&deviceInfo->sDeviceName, &dvrparam->HkSdkParam[i].sDeviceName, NET_DVR_MAX_LEN);
				return KEY_TRUE;
			}
		}
	}
	return KEY_FALSE;
}

static int WriteH264DataToChace(int queueid, BYTE* pBuffer, int nBufSize, BOOL bIsKeyFrame, long lTimeStamp)
{
	if (!pBuffer || nBufSize <= 0 || lTimeStamp < 0 || queueid < 0)
	{
		return KEY_FALSE;
	}
	QueueVideoInputInfo_T param = {0};
	unsigned long long tempPts;
	tempPts = (unsigned long long)lTimeStamp / 1000;
	param.fps = 20;
	param.rcmode  = 0;
	param.bitrate = 1024;
	param.highPts = tempPts >> 32;
	param.lowPts  = tempPts;
	param.frametype = (bIsKeyFrame == TRUE)?API_FRAME_TYPE_I:API_FRAME_TYPE_P;
	param.reslution.width = 1280;
	param.reslution.height= 720;
	param.staty0 = MediaType_H264;
/*	if(NULL == fp)
	{
		fp = fopen("./mm.h264", "wb+");
	}
	if(queueid == 1)
	{
		fwrite((char *)pBuffer, nBufSize, 1, fp);
	}
*/
	GetMediaqueueRegistHandle()->writeVideoRawDatacb(queueid, (char *)pBuffer, nBufSize, param, MediaType_H264);
	return KEY_FALSE;
}

static int WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  int realsize = size * nmemb;
  MemoryStruct_T *mem = (MemoryStruct_T *)userp;
 
 // 注意这里根据每次被调用获得的数据重新动态分配缓存区的大小
  char *ptr =(char *) realloc(mem->memory, mem->size + realsize + 1); 
  if(ptr == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

static int GetRandName(char *name)
{
	if(NULL == name)
	{
		HK_ERROR("GetRandName param error\n");
		return -1;
	}
	char metachar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	srand(time(NULL));
    for (int i = 0; i < 6; i++)
    {
        name[i] = metachar[rand() % 62]; 
    }
}

static int GetSnapTime(TimeYMD_T *snapTime,DWORD dwAbsTime)
{
	snapTime->year = GET_YEAR(dwAbsTime);
	snapTime->month = GET_MONTH(dwAbsTime);
	snapTime->day = GET_DAY(dwAbsTime);
	snapTime->hour = GET_HOUR(dwAbsTime);
	snapTime->min = GET_MINUTE(dwAbsTime);
	snapTime->sec = GET_SECOND(dwAbsTime);
}

static int GetJpegFromUrl(char *url,unsigned char *jpegdata,int *jpeglen)
{
	if(url == NULL || jpegdata == NULL || jpeglen == NULL)
	{
		HK_ERROR("GetJpegFromUrl param error\n");
		return -1;
	}

	MemoryStruct_T chunk;
	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
  	chunk.size = 0;    /* no data at this point */ 

	CURL *hnd = curl_easy_init();	
	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(hnd, CURLOPT_URL, url);
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	CURLcode ret1 = curl_easy_perform(hnd);
	curl_easy_cleanup(hnd);
	HK_DEBUG("len = %d \n",chunk.size);
	memcpy(jpegdata,chunk.memory,chunk.size);
	*jpeglen = chunk.size;
	free(chunk.memory);
	//HK_DEBUG("result = %s  \n",result);
}

static int GetPhoneNum(char *xml,char *phoneNum)
{
	if(NULL == xml || NULL == phoneNum)
	{
		HK_ERROR("GetPhoneNum param error\n");
		return -1;
	}
	mxml_node_t *tree,*node,*base,*FDIDNode,*nameNode;
	tree = mxmlLoadString(NULL, (char *)xml,MXML_TEXT_CALLBACK);
	mxml_node_t *id,*password;
	char name[64] = {0};

	nameNode = mxmlFindElement(tree,tree,"phoneNumber",NULL,NULL,MXML_DESCEND);
	if(NULL == nameNode)
	{
		HK_ERROR("name not found\n");
		mxmlDelete(tree);
		return -1;
	}
	sprintf(phoneNum,"%s",mxmlGetText(nameNode,NULL));
	mxmlDelete(tree);
	return 0;
}

static int GetFDIDFromNVR(LONG lUserID,char *FaceName,char *FDIDData)
{
	if(0 > lUserID || FaceName == NULL || FDIDData == NULL)
	{
		HK_ERROR("GetFDIDFromNVR param error\n");
		return -1;
	}
	int ret = -1;
	NET_DVR_XML_CONFIG_INPUT   xmllpInputParam;
	xmllpInputParam.dwSize = sizeof(NET_DVR_XML_CONFIG_INPUT);
	char xmlurl[64] = "GET /ISAPI/Intelligent/FDLib";
	xmllpInputParam.lpRequestUrl =(void *) xmlurl;
	xmllpInputParam.dwRequestUrlLen = sizeof("GET /ISAPI/Intelligent/FDLib");
	xmllpInputParam.dwInBufferSize = 0;
	xmllpInputParam.dwRecvTimeOut = 0;
	xmllpInputParam.byForceEncrpt = 0;
	//xmllpInputParam.lpInBuffer = NULL;
	//xmllpInputParam.byForceEncrpt = 0;
	NET_DVR_XML_CONFIG_OUTPUT xmllpOutputParam;
	char outbuf[1024*10];
		
	xmllpOutputParam.dwSize = sizeof(NET_DVR_XML_CONFIG_OUTPUT);
	xmllpOutputParam.lpOutBuffer = (void *)outbuf;
	xmllpOutputParam.dwOutBufferSize = 1024*10;
	xmllpOutputParam.dwReturnedXMLSize = 0;
	xmllpOutputParam.dwStatusSize = 0;
	ret = NET_DVR_STDXMLConfig(lUserID,&xmllpInputParam,&xmllpOutputParam);
	if(!ret)
	{
		HK_ERROR("NET_DVR_STDXMLConfig failed, error code: [%d]", NET_DVR_GetLastError());
	  	return KEY_FALSE;
	}

	//HK_DEBUG("xml=%s",xmllpOutputParam.lpOutBuffer);
	mxml_node_t *tree,*node,*base,*FDIDNode,*nameNode;
	tree = mxmlLoadString(NULL, (char *)xmllpOutputParam.lpOutBuffer,MXML_TEXT_CALLBACK);
	mxml_node_t *id,*password;
	char name[64] = {0};
	
	base = mxmlFindElement(tree, tree, "FDLibBaseCfg",NULL, NULL,MXML_DESCEND);
	while(base)
	{		
		nameNode = mxmlFindElement(base,tree,"name",NULL,NULL,MXML_DESCEND);
		if(NULL == nameNode)
		{
			printf("name not found\n");
			continue;
		}
		else
		{
			sprintf(name,"%s",mxmlGetText(nameNode,NULL));
			if(strcmp(name,FaceName) == 0)
			{
				FDIDNode = mxmlFindElement(base,tree,"FDID",NULL,NULL,MXML_DESCEND);
				if(NULL == FDIDNode)
				{
					printf("FDID not found\n");
					continue;
				}
				sprintf(FDIDData,"%s",mxmlGetText(FDIDNode,NULL));
				break;
			}
			else
			{
				FDIDNode = mxmlFindElement(base,tree,"FDID",NULL,NULL,MXML_DESCEND);
				if(NULL == FDIDNode)
				{
					printf("FDID not found\n");
					continue;
				}
				base = mxmlFindElement(FDIDNode,tree,"FDLibBaseCfg",NULL,NULL,MXML_DESCEND);
			}
			
		}
	}
	mxmlDelete(tree);
}

static int HksdkStorage(LONG lUserID,char *FDID,int FDIDLen,unsigned char *jpeg,int jpeglen,char *name)
{
	if(0 > lUserID || NULL == FDID ||NULL == jpeg || 0 > jpeglen ||NULL == name )
	{
		HK_ERROR("HksdkStorage param error\n");
		return -1;
	}
	int ret = -1;
	NET_DVR_FACELIB_COND struInput;
	struInput.dwSize = sizeof(NET_DVR_FACELIB_COND);
	memcpy(struInput.szFDID,FDID,FDIDLen);
	struInput.byConcurrent = 0;
	struInput.byCover = 1;
	struInput.byCustomFaceLibID = 0;

	LONG lUploadHandle;
	lUploadHandle = NET_DVR_UploadFile_V40(lUserID,IMPORT_DATA_TO_FACELIB,(void *)&struInput,sizeof(struInput),NULL,NULL,0);
	if(-1 == lUploadHandle)
	{
		HK_ERROR("NET_DVR_UploadFile_V40 failed, error code: [%d]", NET_DVR_GetLastError());
	  	return KEY_FALSE;
	}
	char xml[1024];
	int num= 0 ;

	num += sprintf(xml+num,"%s\r\n","<FaceAppendData>");
	num += sprintf(xml+num,"<name>%s</name>\r\n",name);
	num += sprintf(xml+num,"%s\r\n","</FaceAppendData>");
	NET_DVR_SEND_PARAM_IN struSendParamIN = {0};
	struSendParamIN.pSendData =(unsigned char *)jpeg;
	struSendParamIN.dwSendDataLen = jpeglen;
	struSendParamIN.byPicType = 1;
	struSendParamIN.byPicURL = 0;
	struSendParamIN.pSendAppendData  =(unsigned char *)xml;
	struSendParamIN.dwSendAppendDataLen = num;
	ret = NET_DVR_UploadSend(lUploadHandle,&struSendParamIN,NULL);
	unsigned int Progress;
	while (1) 
	{
		ret = NET_DVR_GetUploadState(lUploadHandle,&Progress);
		HK_DEBUG("RET = %d",ret);
		if (ret == 1) 
		{
			HK_DEBUG("上传成功");
			break;
		} 
		else if (ret == 2) 
		{
			//HK_DEBUG("正在上传");
		} 
		else if (ret == 29) 
		{
			HK_DEBUG("图片未识别到目标");
			break;
		} 
		else
		{
			HK_DEBUG("ret  = %d\n",ret);
			HK_DEBUG("其他错误：");
			//NET_DVR_UploadClose(lUploadHandle);
			break;
		}
		sleep(1);
	}

	NET_DVR_UploadClose(lUploadHandle);
	return ret;
}

static int HKsdkRecog(LONG lUserID,char *FDIDData,char *jpegurl,int isworker,DataAtt_T *deviceInfo)
{
	if(0 > lUserID || NULL == FDIDData ||NULL == jpegurl || NULL == deviceInfo)
	{
		HK_ERROR("HKsdkRecog param error\n");
		return -1;
	}
	unsigned char jpegdata[200 *1024] = {0};
	int jpeglen;
	#if 1
	GetJpegFromUrl(jpegurl,jpegdata,&jpeglen);
	#else
	FILE *fp = NULL;
	fp = fopen("1.jpg","r+");
	if(NULL == fp)
	{
		DF_DEBUG("fopen error\n");
		return -1;
	}
	fseek(fp, 0, SEEK_END);
    jpeglen = ftell(fp);
    rewind(fp);
	fread(jpegdata,jpeglen,1,fp);
	fclose(fp);
	#endif
	int ret = -1;
	int Ret = -1;
	NET_DVR_XML_CONFIG_INPUT   xmllpInputParam;
	xmllpInputParam.dwSize = sizeof(NET_DVR_XML_CONFIG_INPUT);
	char xmlurl[64] = "POST /ISAPI/Intelligent/analysisImage/face";
	xmllpInputParam.lpRequestUrl =(void *) xmlurl;
	xmllpInputParam.dwRequestUrlLen = sizeof("POST /ISAPI/Intelligent/analysisImage/face");
	xmllpInputParam.lpInBuffer = (void *)jpegdata;
	xmllpInputParam.dwInBufferSize = jpeglen;
	NET_DVR_XML_CONFIG_OUTPUT xmllpOutputParam;
	char outbuf[1024*20] = {0};
		
	xmllpOutputParam.dwSize = sizeof(NET_DVR_XML_CONFIG_OUTPUT);
	xmllpOutputParam.lpOutBuffer = (void *)outbuf;
	xmllpOutputParam.dwOutBufferSize = 1024*20;
	ret = NET_DVR_STDXMLConfig(lUserID,&xmllpInputParam,&xmllpOutputParam);
	if(!ret)
	{
		HK_ERROR("NET_DVR_STDXMLConfig failed, error code: [%d]", NET_DVR_GetLastError());
	  	return -1;
	}
	HK_ERROR("outbuffxml = %s\n", xmllpOutputParam.lpOutBuffer);

/*
	<FaceContrastTargetsList version="2.0" xmlns="http://www.isapi.org/ver20/XMLSchema">
	<FaceContrastTarget>
	<Rect>
	<height>0.075602</height>
	<width>0.043355</width>
	<x>0.831602</x>
	<y>0.785376</y>
	</Rect>
	<modeData>SElLREZSNDVUWAAAAAAAMzHzQ5BGqZDa0/TpB5T5Dw4kAF4egEMP5/gtM0zz2u/fI5RMYgfw7yG94MzFt0mhK6YXQ+46Q0+dCFczKaYVT/7fV8rsBJ0m+PcCFhr/zyD2JuQzU98z/NUHFEAG2mvV+KEOfwEzFj3MEOPPyOy0wNFieh3fJPDfQ8AWfxHR060dsYAAPTZrB9PA4K2t50BeQKahEgBn8zFizOAjdX//EAcA5DaA/YD3gDPgpiDKFOgmKf+0wH/cgCv++th/yB0H3yALAuNJ+NhAQzF17gAUf39ewD0R+i0vK/te9TGGgBrPxdwRWgAC+l5/1xH5sSsU77otf90x1SvMHcDIAebTyjEx80OQRqmQ2tP06QeU+Q8OJABeHoBDD+f4LTNM89rv3yOUTGIH8O8hveDMxbdJoSumF0PuOkNPnQhXMymmFU/+31fK7ASdJvj3AhYa/88g9ibkM1PfM/zVBxRABtpr1fihDn8BMxY9zBDjz8jstMDRYnod3yTw30PAFn8R0dOtHbGAAD02awfTwOCtredAXkCmoRIAZ/MxYszgI3V//xAHAOQ2gP2A94Az4KYgyhToJin/tMB/3IAr/vrYf8gdB98gCwLjSfjYQEMxde4AFH9/XsA9EfotLyv7XvUxhoAaz8XcEVoAAvpef9cR+bErFO+6LX/dMdUrzB3AyAHm08ox</modeData>
	<recommendFaceRect>
	<height>0.226806</height>
	<width>0.130065</width>
	<x>0.788247</x>
	<y>0.709774</y>
	</recommendFaceRect>
	</FaceContrastTarget>
	</FaceContrastTargetsList>
*/

	mxml_node_t *tree,*node,*base,*FDIDNode,*nameNode;
	tree = mxmlLoadString(NULL, (char *)xmllpOutputParam.lpOutBuffer,MXML_TEXT_CALLBACK);
	mxml_node_t *id,*password;
	char modeData[1024] = {0};

	nameNode = mxmlFindElement(tree,tree,"modeData",NULL,NULL,MXML_DESCEND);
	if(NULL == nameNode)
	{
		HK_ERROR("name not found\n");
		mxmlDelete(tree);
		return -1;
	}
	sprintf(modeData,"%s",mxmlGetText(nameNode,NULL));
	mxmlDelete(tree);
	HK_ERROR("modeData = %s\n",modeData);
	
/*
	<?xml version="1.0" encoding="utf-8"?>
	<FDSearchDescription>
	<FDID>8AFF99DC71DA4B06B5D4F9655988E3A5</FDID>
	<OccurrencesInfo>
	<enabled>true</enabled>
	<occurrences>0</occurrences>
	<occurrencesSearchType>greaterThanOrEqual</occurrencesSearchType>
	</OccurrencesInfo>
	<FaceModeList>
	<FaceMode>
	<ModeInfo>
	<similarity>80</similarity>
	<modeData>SElLREZSNDVUWAAAAAAAM9r/HIBXHd04XvyUBCPvt2I23VpaprcNoSBrDhYt5NzVMScMCMoG+UPI2p3vwwT0/7T4ChDpJuccICsW2CcVHiHdMed/PWsIdYDFi7fwKc/Y+MAg3z0mJ5DslMDaHqnryNF/i5161e0ArbTDEAkdFswb7Q0TBcqAsZn3f9Hh9J0XOuYtsf8b/rF654ZeusDci9UF8PYn5OpJf4CG87GGEsPpytXlM/TMt8UmfycAU+fzz0/29Bbo5DEI6Nz8fya9z8iAw8/2IzbhcE+6Ai8ezPsCT8AdTLdDQzgT2DOtt2L3G/lG0+QErRcvIYYjWrdaAw8x/UPFOpSm0f7zDgZwAFra/xyAVx3dOF78lAQj77diNt1aWqa3DaEgaw4WLeTc1TEnDAjKBvlDyNqd78ME9P+0+AoQ6SbnHCArFtgnFR4h3THnfz1rCHWAxYu38CnP2PjAIN89JieQ7JTA2h6p68jRf4udetXtAK20wxAJHRbMG+0NEwXKgLGZ93/R4fSdFzrmLbH/G/6xeueGXrrA3IvVBfD2J+TqSX+AhvOxhhLD6crV5TP0zLfFJn8nAFPn889P9vQW6OQxCOjc/H8mvc/IgMPP9iM24XBPugIvHsz7Ak/AHUy3Q0M4E9gzrbdi9xv5RtPkBK0XLyGGI1q3WgMPMf1DxTqUptH+8w4GcABa
	</modeData></ModeInfo></FaceMode>
	</FaceModeList>
	<searchID>C96DE809-1410-0001-A0DD-3CBC16401030</searchID><maxResults>50</maxResults><searchResultPosition>0</searchResultPosition>
	</FDSearchDescription>
*/
	char searchID[12] = {0};
	sprintf(searchID,"%d",(int)rand()%1000);
	
	char xml[2*1024] = {0};
	int num= 0 ;
	num += sprintf(xml+num,"%s\r\n","<FDSearchDescription>");
	num += sprintf(xml+num,"<FDID>%s</FDID>\r\n",FDIDData);
	num += sprintf(xml+num,"%s\r\n","<OccurrencesInfo>");
	num += sprintf(xml+num,"<enabled>%s</enabled>\r\n","true");
	num += sprintf(xml+num,"<occurrences>%d</occurrences>\r\n",0);
	num += sprintf(xml+num,"<occurrencesSearchType>%s</occurrencesSearchType>\r\n","greaterThanOrEqual");
	num += sprintf(xml+num,"%s\r\n","</OccurrencesInfo>");
	num += sprintf(xml+num,"%s\r\n","<FaceModeList>");
	num += sprintf(xml+num,"%s\r\n","<FaceMode>");
	num += sprintf(xml+num,"%s\r\n","<ModeInfo>");
	num += sprintf(xml+num,"<similarity>%d</similarity>\r\n",80);
	num += sprintf(xml+num,"<modeData>%s</modeData>\r\n",modeData);	
	num += sprintf(xml+num,"%s\r\n","</ModeInfo>");
	num += sprintf(xml+num,"%s\r\n","</FaceMode>");
	num += sprintf(xml+num,"%s\r\n","</FaceModeList>");
	num += sprintf(xml+num,"<searchID>%s</searchID>\r\n",searchID);
	num += sprintf(xml+num,"<maxResults>%d</maxResults>\r\n",50);
	num += sprintf(xml+num,"<searchResultPosition>%d</searchResultPosition>\r\n",0);
	num += sprintf(xml+num,"%s\r\n","</FDSearchDescription>");

	HK_ERROR("searchID = %s xml = %s\n",searchID,xml);

	memset(&xmllpInputParam,0,sizeof(NET_DVR_XML_CONFIG_INPUT));
	xmllpInputParam.dwSize = sizeof(NET_DVR_XML_CONFIG_INPUT);
	char xmlurl1[64] = "POST /ISAPI/Intelligent/FDLib/FDSearch";
	xmllpInputParam.lpRequestUrl =(void *) xmlurl1;
	xmllpInputParam.dwRequestUrlLen = sizeof("POST /ISAPI/Intelligent/FDLib/FDSearch");
	xmllpInputParam.lpInBuffer = (void *)xml;
	xmllpInputParam.dwInBufferSize = num;
	NET_DVR_XML_CONFIG_OUTPUT xmllpOutputParam1;
	char outbuf1[1024*20] = {0};
	
	xmllpOutputParam1.dwSize = sizeof(NET_DVR_XML_CONFIG_OUTPUT);
	xmllpOutputParam1.lpOutBuffer = (void *)outbuf1;
	xmllpOutputParam1.dwOutBufferSize = 1024*20;
	ret = NET_DVR_STDXMLConfig(lUserID,&xmllpInputParam,&xmllpOutputParam1);
	if(!ret)
	{
		HK_ERROR("NET_DVR_STDXMLConfig failed, error code: [%d]", NET_DVR_GetLastError());
	  	return -1;
	}
	HK_ERROR("outbuffxml = %s\n", xmllpOutputParam1.lpOutBuffer);

	/*

		 <FDSearchResult version="2.0" xmlns="http://www.isapi.org/ver20/XMLSchema">
		<searchID>12121212121212121</searchID>
		<responseStatus>true</responseStatus>
		<responseStatusStrg>NO MATCHES</responseStatusStrg>
		<numOfMatches>0</numOfMatches>
		<totalMatches>0</totalMatches>
		</FDSearchResult>
	*/
	
	tree = mxmlLoadString(NULL, (char *)xmllpOutputParam1.lpOutBuffer,MXML_TEXT_CALLBACK);
	char responseStatusStrg[256] = {0};
	mxml_node_t *matchnode,*didnode,*namenode,*certificatenode,*phonenode;
	char FDID[128] = {0};
	char recogname[64] = {0};
	char certificateNumber[32] = {0};
	char phoneNumber[32] = {0};

	nameNode = mxmlFindElement(tree,tree,"responseStatusStrg",NULL,NULL,MXML_DESCEND);		//没找到
	if(NULL != nameNode)
	{
		sprintf(responseStatusStrg,"%s",mxmlGetText(nameNode,NULL));
		if(strcmp(responseStatusStrg,"OK") == 0)
		{
			matchnode = mxmlFindElement(nameNode,tree,"numOfMatches",NULL,NULL,MXML_DESCEND);
			char matchnumstr[12] = {0};
			int matchnum;
			sprintf(matchnumstr,"%s",mxmlGetText(matchnode,NULL));
			matchnum = atoi(matchnumstr);
			if(matchnum > 0)
			{
				didnode = mxmlFindElement(matchnode,tree,"FDID",NULL,NULL,MXML_DESCEND);
				if(NULL != nameNode)
				{
					sprintf(FDID,"%s",mxmlGetText(didnode,NULL));
					if(strcmp(FDID,FDIDData) == 0)
					{
						Ret = 1;
					}
					else
					{
						Ret = 0;
					}
					HK_ERROR("FDID = %s\n", FDID);
				}
			
				namenode = mxmlFindElement(matchnode,tree,"name",NULL,NULL,MXML_DESCEND);
				if(NULL != namenode)
				{
					sprintf(recogname,"%s",mxmlGetText(namenode,NULL));
					if(isworker == 1)
						memcpy(deviceInfo->DataUnion.attendanceattr.name,recogname,strlen(recogname));
						//sprintf(deviceInfo->DataUnion.attendanceattr.name,"%s",recogname);
					else
						memcpy(deviceInfo->DataUnion.vipcustomerattr.name,recogname,strlen(recogname));
					HK_ERROR("recogname = %s\n", deviceInfo->DataUnion.attendanceattr.name);
				}
				if(isworker == 1)
				{
					certificatenode = mxmlFindElement(matchnode,tree,"certificateNumber",NULL,NULL,MXML_DESCEND);
					if(NULL != certificatenode)
					{
						sprintf(certificateNumber,"%s",mxmlGetText(certificatenode,NULL));
						memcpy(deviceInfo->DataUnion.attendanceattr.usrid,certificateNumber,strlen(certificateNumber));
						HK_ERROR("certificateNumber = %s\n", deviceInfo->DataUnion.attendanceattr.usrid);
					}
					phonenode = mxmlFindElement(matchnode,tree,"phoneNumber",NULL,NULL,MXML_DESCEND);
					if(NULL != phonenode)
					{
						sprintf(phoneNumber,"%s",mxmlGetText(phonenode,NULL));
						memcpy(deviceInfo->DataUnion.attendanceattr.phonenum,phoneNumber,strlen(phoneNumber));
						HK_ERROR("phoneNumber = %s\n", deviceInfo->DataUnion.attendanceattr.phonenum);
					}
				}
				
				mxmlDelete(tree);
				HK_ERROR("recogname = %s\n", deviceInfo->DataUnion.attendanceattr.name);
				return Ret;
			}
			
		}
		else if(strcmp(responseStatusStrg,"NO MATCHES") == 0)
		{
			mxmlDelete(tree);
			Ret = 0;
			return Ret;
		}
	}
	mxmlDelete(tree);
	return 0;
}
static int HKsdkSetIOOutPut(LONG lUserID,int channel,char *status)	//101 102
{
	int ret = -1;
	char xml1[1024];
	int num1= 0 ;

	num1 += sprintf(xml1+num1,"%s\r\n","<IOPortData>");
	num1 += sprintf(xml1+num1,"<outputState>%s</outputState>\r\n",status);
	num1 += sprintf(xml1+num1,"%s\r\n","</IOPortData>");

	
	NET_DVR_XML_CONFIG_INPUT   xmllpInputParam1;
	xmllpInputParam1.dwSize = sizeof(NET_DVR_XML_CONFIG_INPUT);
	char xmlurl1[64] ={0};
	sprintf(xmlurl1,"PUT /ISAPI/System/IO/outputs/%d/trigger",channel);
	xmllpInputParam1.lpRequestUrl =(void *) xmlurl1;
	xmllpInputParam1.dwRequestUrlLen = strlen(xmlurl1);
	xmllpInputParam1.dwInBufferSize = num1;
	xmllpInputParam1.dwRecvTimeOut = 0;
	xmllpInputParam1.byForceEncrpt = 0;
	xmllpInputParam1.lpInBuffer = xml1;
	//xmllpInputParam.byForceEncrpt = 0;
	NET_DVR_XML_CONFIG_OUTPUT xmllpOutputParam1;
	char outbuf1[1024*10];
		
	xmllpOutputParam1.dwSize = sizeof(NET_DVR_XML_CONFIG_OUTPUT);
	xmllpOutputParam1.lpOutBuffer = (void *)outbuf1;
	xmllpOutputParam1.dwOutBufferSize = 1024*10;
	xmllpOutputParam1.dwReturnedXMLSize = 0;
	xmllpOutputParam1.dwStatusSize = 0;
	ret = NET_DVR_STDXMLConfig(lUserID,&xmllpInputParam1,&xmllpOutputParam1);
	if(!ret)
	{
		HK_ERROR("NET_DVR_STDXMLConfig failed, error code: [%d]", NET_DVR_GetLastError());
		return KEY_FALSE;
	}
}


static int FunHkSdkFaceCompare(NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo)
{
	if (NULL == pAlarmInfo)
	{
		return KEY_FALSE;
	}

	int ret = -1;
	DataAtt_T deviceInfo;
	memset(&deviceInfo,0,sizeof(DataAtt_T));
	NET_VCA_FACESNAP_MATCH_ALARM faceAlarmInfo  = {0};//= (NET_VCA_FACESNAP_MATCH_ALARM *)pAlarmInfo;
	memcpy(&faceAlarmInfo, pAlarmInfo, sizeof(NET_VCA_FACESNAP_MATCH_ALARM)); 

	HK_DEBUG("byContrastStatus = %d\n",faceAlarmInfo.byContrastStatus);
	//HK_DEBUG("byFaceScore = %d\n",faceAlarmInfo.struSnapInfo.byFaceScore);
	//HK_DEBUG("byFacePicQuality = %d\n",faceAlarmInfo.struSnapInfo.byFacePicQuality);

	void *resultHanlde = NULL;
	HandleManage_T *head = &GetHkSdkHandle()->deviceListHead;
	HandleManageGetHandleByStr(head, HkDvrInfo.sDvrSerialNumber, (char *)pAlarmer->sSerialNumber,resultHanlde, HKDVRParam_T);
	HKDVRParam_T *dvrparam = (HKDVRParam_T *)resultHanlde;

	//HK_DEBUG("IWorkersFPID = %s\n",dvrparam->HkDvrInfo.IWorkersFPID);
	//HK_DEBUG("ICustomerFPID = %s\n",dvrparam->HkDvrInfo.ICustomerFPID);
	//HK_DEBUG("lUserID = %d\n",dvrparam->HkDvrInfo.lUserID);

	if(1 == faceAlarmInfo.byContrastStatus)			//比对成功
	{
		unsigned char FDID[128];
		memset(FDID,0,sizeof(FDID));
		memcpy(FDID,faceAlarmInfo.struBlockListInfo.pFDID,faceAlarmInfo.struBlockListInfo.dwFDIDLen);
		//HK_DEBUG("FDID = %s\n",FDID);
		HK_DEBUG("name = %s  \n",faceAlarmInfo.struBlockListInfo.struBlockListInfo.struAttribute.byName);

		unsigned char recogjpeg[128]= {0};
		memcpy(recogjpeg,faceAlarmInfo.struBlockListInfo.pBuffer1,faceAlarmInfo.struBlockListInfo.dwBlockListPicLen);
		HK_DEBUG("recogjpeg = %s  \n",recogjpeg);

		if(strlen((char *)(faceAlarmInfo.struBlockListInfo.struBlockListInfo.struAttribute.byName))== 0)
		{
			HK_DEBUG("name is null\n");
			return KEY_TRUE;
		}
		
		if(strcmp(dvrparam->HkDvrInfo.IWorkersFPID,(char *)FDID) == 0)			//员工考勤
		{
			HK_DEBUG("workers");
			char xml[512] = {0};
			char phonenum[12];
			memcpy(xml,faceAlarmInfo.struBlockListInfo.struBlockListInfo.pFDDescriptionBuffer,faceAlarmInfo.struBlockListInfo.struBlockListInfo.dwFDDescriptionLen);
			char xml1[1024] = {0};
			memcpy(xml1,faceAlarmInfo.struBlockListInfo.struBlockListInfo.pFCAdditionInfoBuffer,faceAlarmInfo.struBlockListInfo.struBlockListInfo.dwFCAdditionInfoLen);
			GetPhoneNum(xml,phonenum);
			//HK_DEBUG("xml1 = %s\n",xml1);
			//HK_DEBUG("phonenum = %s\n",phonenum);
			memcpy(deviceInfo.DataUnion.attendanceattr.phonenum,phonenum,sizeof(phonenum));
			memcpy(deviceInfo.DataUnion.attendanceattr.usrid,faceAlarmInfo.struBlockListInfo.struBlockListInfo.struAttribute.byCertificateNumber,sizeof(faceAlarmInfo.struBlockListInfo.struBlockListInfo.struAttribute.byCertificateNumber));
			memcpy(deviceInfo.DataUnion.attendanceattr.name,faceAlarmInfo.struBlockListInfo.struBlockListInfo.struAttribute.byName,sizeof((char *)(faceAlarmInfo.struBlockListInfo.struBlockListInfo.struAttribute.byName)));	
			//HK_DEBUG("name = %s\n",deviceInfo.DataUnion.attendanceattr.name);
			memcpy(deviceInfo.DataUnion.attendanceattr.plateImagePath,faceAlarmInfo.pSnapPicBuffer,faceAlarmInfo.dwSnapPicLen);
			//memcpy(deviceInfo.DataUnion.attendanceattr.plateImagePath,faceAlarmInfo.struBlockListInfo.pBuffer1,faceAlarmInfo.struBlockListInfo.dwBlockListPicLen);
			//HK_DEBUG("plateImagePath = %s\n",deviceInfo.DataUnion.attendanceattr.plateImagePath);
			//memcpy(deviceInfo.DataUnion.attendanceattr.snapImagePath,faceAlarmInfo.struSnapInfo.pBuffer1,faceAlarmInfo.struSnapInfo.dwSnapFacePicLen);
			memcpy(deviceInfo.DataUnion.attendanceattr.snapImagePath,faceAlarmInfo.struBlockListInfo.pBuffer1,faceAlarmInfo.struBlockListInfo.dwBlockListPicLen);
			//HK_DEBUG("snapImagePath = %s\n",deviceInfo.DataUnion.attendanceattr.snapImagePath);
			GetSnapTime(&deviceInfo.DataUnion.attendanceattr.snapTime,faceAlarmInfo.struSnapInfo.dwAbsTime);
			//HK_DEBUG("year = %d - %d - %d - %d - %d - %d\n",deviceInfo.DataUnion.attendanceattr.snapTime.year,deviceInfo.DataUnion.attendanceattr.snapTime.month,deviceInfo.DataUnion.attendanceattr.snapTime.day,deviceInfo.DataUnion.attendanceattr.snapTime.hour,
			//												deviceInfo.DataUnion.attendanceattr.snapTime.min,deviceInfo.DataUnion.attendanceattr.snapTime.sec);
			snprintf(deviceInfo.DataUnion.attendanceattr.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", deviceInfo.DataUnion.attendanceattr.snapTime.year, deviceInfo.DataUnion.attendanceattr.snapTime.month, 
					deviceInfo.DataUnion.attendanceattr.snapTime.day, deviceInfo.DataUnion.attendanceattr.snapTime.hour, deviceInfo.DataUnion.attendanceattr.snapTime.min, deviceInfo.DataUnion.attendanceattr.snapTime.sec, (int)rand()%1000);
			//HK_DEBUG("snapToken = %s\n",deviceInfo.DataUnion.attendanceattr.snapToken);

			GetHKstoreRegistHandle()->attendancehandle.pSetAttendanceRecord(&deviceInfo);
		}
		else if(strcmp(dvrparam->HkDvrInfo.ICustomerFPID,(char *)FDID) == 0)	//路人VIP
		{
			HK_DEBUG("cunstomer");
			memcpy(deviceInfo.DataUnion.vipcustomerattr.name,faceAlarmInfo.struBlockListInfo.struBlockListInfo.struAttribute.byName,strlen((char *)(faceAlarmInfo.struBlockListInfo.struBlockListInfo.struAttribute.byName)));	
			memcpy(deviceInfo.DataUnion.vipcustomerattr.plateImagePath,faceAlarmInfo.pSnapPicBuffer,faceAlarmInfo.dwSnapPicLen);
			memcpy(deviceInfo.DataUnion.vipcustomerattr.snapImagePath,faceAlarmInfo.struSnapInfo.pBuffer1,faceAlarmInfo.struSnapInfo.dwSnapFacePicLen);
			GetSnapTime(&deviceInfo.DataUnion.vipcustomerattr.snapTime,faceAlarmInfo.struSnapInfo.dwAbsTime);
			snprintf(deviceInfo.DataUnion.vipcustomerattr.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", deviceInfo.DataUnion.vipcustomerattr.snapTime.year, deviceInfo.DataUnion.vipcustomerattr.snapTime.month, 
					deviceInfo.DataUnion.vipcustomerattr.snapTime.day, deviceInfo.DataUnion.vipcustomerattr.snapTime.hour, deviceInfo.DataUnion.vipcustomerattr.snapTime.min, deviceInfo.DataUnion.vipcustomerattr.snapTime.sec, (int)rand()%1000);
			GetHKstoreRegistHandle()->attendancehandle.pAddVisitorsInfo(&deviceInfo);
		}
	}
	else if(2 == faceAlarmInfo.byContrastStatus)	//比对失败		
	{
		unsigned char name1[256] = {0};
		unsigned char name2[256] = {0};
		memcpy(name1,faceAlarmInfo.pSnapPicBuffer,faceAlarmInfo.dwSnapPicLen);
		memcpy(name2,faceAlarmInfo.struSnapInfo.pBuffer1,faceAlarmInfo.struSnapInfo.dwSnapFacePicLen);
		
		//HK_DEBUG("name1 = %s\n",name1);
		//HK_DEBUG("name2 = %s\n",name2);
			
		char name[64] = {0};
		char url[128] = {0};
		unsigned char jpegdata[200*1024] = {0};
		int jpeglen = 0;
		memcpy(url,faceAlarmInfo.struSnapInfo.pBuffer1,faceAlarmInfo.struSnapInfo.dwSnapFacePicLen);
		GetRandName(name);
		
		GetJpegFromUrl(url,jpegdata,&jpeglen);
		//HK_DEBUG("lUserID = %d name = %s\n",dvrparam->HkDvrInfo.lUserID,name);
		if(dvrparam->HkDvrInfo.lUserID > -1)
		{
			ret = HksdkStorage(dvrparam->HkDvrInfo.lUserID,dvrparam->HkDvrInfo.ICustomerFPID,sizeof(dvrparam->HkDvrInfo.ICustomerFPID),jpegdata,jpeglen,name);
			if(1 == ret)
			{
				//HK_DEBUG("HksdkStorage success\n");
				memcpy(deviceInfo.DataUnion.vipcustomerattr.name,name,sizeof(name));	
				memcpy(deviceInfo.DataUnion.vipcustomerattr.plateImagePath,faceAlarmInfo.pSnapPicBuffer,faceAlarmInfo.dwSnapPicLen);
				memcpy(deviceInfo.DataUnion.vipcustomerattr.snapImagePath,faceAlarmInfo.struSnapInfo.pBuffer1,faceAlarmInfo.struSnapInfo.dwSnapFacePicLen);
				GetSnapTime(&deviceInfo.DataUnion.vipcustomerattr.snapTime,faceAlarmInfo.struSnapInfo.dwAbsTime);
				snprintf(deviceInfo.DataUnion.vipcustomerattr.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", deviceInfo.DataUnion.vipcustomerattr.snapTime.year, deviceInfo.DataUnion.vipcustomerattr.snapTime.month, 
					deviceInfo.DataUnion.vipcustomerattr.snapTime.day, deviceInfo.DataUnion.vipcustomerattr.snapTime.hour, deviceInfo.DataUnion.vipcustomerattr.snapTime.min, deviceInfo.DataUnion.vipcustomerattr.snapTime.sec, (int)rand()%1000);
				GetHKstoreRegistHandle()->attendancehandle.pAddVisitorsInfo(&deviceInfo);		
			}
			else
			{
				HK_DEBUG("HksdkStorage fail\n");
			}
		}
	}

	return KEY_TRUE;
}

static int FunHKSdkFaceSnap(NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo)
{
	void *resultHanlde = NULL;
	int ret = -1;
	HandleManage_T *head = &GetHkSdkHandle()->deviceListHead;
	HandleManageGetHandleByStr(head, HkDvrInfo.sDvrSerialNumber, (char *)pAlarmer->sSerialNumber,resultHanlde, HKDVRParam_T);
	HKDVRParam_T *dvrparam = (HKDVRParam_T *)resultHanlde;

	//NET_VCA_FACESNAP_RESULT struFaceSnap = {0};
	NET_VCA_FACESNAP_RESULT struFaceSnap;
	memset(&struFaceSnap,0,sizeof(NET_VCA_FACESNAP_RESULT));
	memcpy(&struFaceSnap, pAlarmInfo, sizeof(NET_VCA_FACESNAP_RESULT));
	HK_DEBUG("dwFaceScore = %d\n",struFaceSnap.dwFaceScore);
	HK_DEBUG("byFacePicQuality = %d\n",struFaceSnap.byFacePicQuality);
	HK_DEBUG("byLivenessDetectionStatus = %d\n",struFaceSnap.byLivenessDetectionStatus);
	HK_DEBUG(" %d %d  %d\n",struFaceSnap.byUploadEventDataType,struFaceSnap.dwFacePicLen,struFaceSnap.dwBackgroundPicLen);
	char imagepath[256];
	char Backimagepath[256];
	memset(&imagepath,0,sizeof(imagepath));
	memset(&Backimagepath,0,sizeof(Backimagepath));

	memcpy(imagepath,struFaceSnap.pBuffer1,struFaceSnap.dwFacePicLen);
	memcpy(Backimagepath,struFaceSnap.pBuffer2,struFaceSnap.dwBackgroundPicLen);
	HK_DEBUG("imagepath = %s\n",imagepath);
	HK_DEBUG("Backimagepath = %s\n",Backimagepath);

	HK_DEBUG("************************************************************");
	//AttendanceAtt_T attendanceattr;
	DataAtt_T deviceInfo;
	memset(&deviceInfo,0,sizeof(DataAtt_T));

	ret = HKsdkRecog(dvrparam->HkDvrInfo.lUserID,dvrparam->HkDvrInfo.IWorkersFPID,imagepath,1,&deviceInfo);
	HK_DEBUG("name = %s\n",deviceInfo.DataUnion.attendanceattr.name);
	if( ret== -1)
	{
		HK_DEBUG("no recog\n");
		return 0;
	}
	else if(ret == 1)
	{
		HK_DEBUG("workers\n");
		GetSnapTime(&deviceInfo.DataUnion.attendanceattr.snapTime,struFaceSnap.dwAbsTime);
		memcpy(deviceInfo.DataUnion.attendanceattr.plateImagePath,struFaceSnap.pBuffer2,struFaceSnap.dwBackgroundPicLen);
		memcpy(deviceInfo.DataUnion.attendanceattr.snapImagePath,struFaceSnap.pBuffer1,struFaceSnap.dwFacePicLen);
		snprintf(deviceInfo.DataUnion.attendanceattr.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d",deviceInfo.DataUnion.attendanceattr.snapTime.year, deviceInfo.DataUnion.attendanceattr.snapTime.month, 
					deviceInfo.DataUnion.attendanceattr.snapTime.day, deviceInfo.DataUnion.attendanceattr.snapTime.hour, deviceInfo.DataUnion.attendanceattr.snapTime.min, deviceInfo.DataUnion.attendanceattr.snapTime.sec, (int)rand()%1000);
		HK_DEBUG("name = %s\n",deviceInfo.DataUnion.attendanceattr.name);
		HK_DEBUG("phonenum = %s\n",deviceInfo.DataUnion.attendanceattr.phonenum);
		GetHKstoreRegistHandle()->attendancehandle.pSetAttendanceRecord(&deviceInfo);		
	}
	else if(ret == 0)
	{
		ret = HKsdkRecog(dvrparam->HkDvrInfo.lUserID,dvrparam->HkDvrInfo.ICustomerFPID,imagepath,0,&deviceInfo);
		if(ret == -1)
		{
			HK_DEBUG("no recog\n");
			return 0;
		}
		else if(ret == 1)
		{
			GetSnapTime(&deviceInfo.DataUnion.vipcustomerattr.snapTime,struFaceSnap.dwAbsTime);
			memcpy(deviceInfo.DataUnion.vipcustomerattr.plateImagePath,struFaceSnap.pBuffer2,struFaceSnap.dwBackgroundPicLen);
			memcpy(deviceInfo.DataUnion.vipcustomerattr.snapImagePath,struFaceSnap.pBuffer1,struFaceSnap.dwFacePicLen);
			snprintf(deviceInfo.DataUnion.vipcustomerattr.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d",deviceInfo.DataUnion.vipcustomerattr.snapTime.year, deviceInfo.DataUnion.vipcustomerattr.snapTime.month, 
					deviceInfo.DataUnion.vipcustomerattr.snapTime.day, deviceInfo.DataUnion.vipcustomerattr.snapTime.hour, deviceInfo.DataUnion.vipcustomerattr.snapTime.min, deviceInfo.DataUnion.vipcustomerattr.snapTime.sec, (int)rand()%1000);
			GetHKstoreRegistHandle()->attendancehandle.pAddVisitorsInfo(&deviceInfo);
		}
		else if(ret == 0)
		{	
			char name[64] = {0};
			char url[128] = {0};
			unsigned char jpegdata[200*1024] = {0};
			int jpeglen = 0;
			memcpy(url,struFaceSnap.pBuffer1,struFaceSnap.dwFacePicLen);
			GetRandName(name);
			GetJpegFromUrl(url,jpegdata,&jpeglen);
			if(dvrparam->HkDvrInfo.lUserID > -1)
			{
				ret = HksdkStorage(dvrparam->HkDvrInfo.lUserID,dvrparam->HkDvrInfo.ICustomerFPID,sizeof(dvrparam->HkDvrInfo.ICustomerFPID),jpegdata,jpeglen,name);
				if(1 == ret)
				{
					//HK_DEBUG("HksdkStorage success\n");
					memcpy(deviceInfo.DataUnion.vipcustomerattr.name,name,sizeof(name));	
					memcpy(deviceInfo.DataUnion.vipcustomerattr.plateImagePath,struFaceSnap.pBuffer2,struFaceSnap.dwBackgroundPicLen);
					memcpy(deviceInfo.DataUnion.vipcustomerattr.snapImagePath,struFaceSnap.pBuffer1,struFaceSnap.dwFacePicLen);
					GetSnapTime(&deviceInfo.DataUnion.vipcustomerattr.snapTime,struFaceSnap.dwAbsTime);
					snprintf(deviceInfo.DataUnion.vipcustomerattr.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", deviceInfo.DataUnion.vipcustomerattr.snapTime.year, deviceInfo.DataUnion.vipcustomerattr.snapTime.month, 
						deviceInfo.DataUnion.vipcustomerattr.snapTime.day, deviceInfo.DataUnion.vipcustomerattr.snapTime.hour, deviceInfo.DataUnion.vipcustomerattr.snapTime.min, deviceInfo.DataUnion.vipcustomerattr.snapTime.sec, (int)rand()%1000);
					GetHKstoreRegistHandle()->attendancehandle.pAddVisitorsInfo(&deviceInfo);				
				}
				else
				{
					HK_DEBUG("HksdkStorage fail\n");
				}
			}
			
		}
	}
	HK_DEBUG("************************************************************");
}


static int FunHKSdkGetTraffic(NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo)
{
	//HK_DEBUG("FunHKSdkGetTraffic\n");
	NET_DVR_PDC_ALRAM_INFO struPDCAlarmInfo;
	memset(&struPDCAlarmInfo,0,sizeof(NET_DVR_PDC_ALRAM_INFO));
	memcpy(&struPDCAlarmInfo, pAlarmInfo, sizeof(NET_DVR_PDC_ALRAM_INFO));
	HK_DEBUG("byMode = %d 离开人数 %d 进入人数 %d\n",struPDCAlarmInfo.byMode,struPDCAlarmInfo.dwLeaveNum, struPDCAlarmInfo.dwEnterNum);
	//HK_DEBUG("小孩离开人数 %d 小孩进入人数 %d\n",struPDCAlarmInfo.dwChildLeaveNum, struPDCAlarmInfo.dwChildEnterNum);
	//HK_DEBUG("byMode = %d ip %s channel %d channel %d\n",struPDCAlarmInfo.byMode,struPDCAlarmInfo.struDevInfo.struDevIP.sIpV4,struPDCAlarmInfo.struDevInfo.byChannel,struPDCAlarmInfo.byChannel)

	VolumeOfCommuters_T info = {0};
	TimeYMD_T time;
	if(struPDCAlarmInfo.byMode == 0)
	{
		GetSnapTime(&time,struPDCAlarmInfo.uStatModeParam.struStatFrame.dwAbsTime);
		info.LeaveAndEnterAttr.dwLeaveNum = struPDCAlarmInfo.dwLeaveNum;
		info.LeaveAndEnterAttr.dwEnterNum = struPDCAlarmInfo.dwEnterNum;
		GetHKstoreRegistHandle()->attendancehandle.pPushVolumeOfCommutersNum(time,&info);
	}
	
	if(struPDCAlarmInfo.byMode == 1)
	{
		//HK_DEBUG("starttime = %d-%d-%d-%d-%d-%d\n",struPDCAlarmInfo.uStatModeParam.struStatTime.tmStart.dwYear,struPDCAlarmInfo.uStatModeParam.struStatTime.tmStart.dwMonth,struPDCAlarmInfo.uStatModeParam.struStatTime.tmStart.dwDay,
													//struPDCAlarmInfo.uStatModeParam.struStatTime.tmStart.dwHour,struPDCAlarmInfo.uStatModeParam.struStatTime.tmStart.dwMinute,struPDCAlarmInfo.uStatModeParam.struStatTime.tmStart.dwSecond);
		//HK_DEBUG("endtime = %d-%d-%d-%d-%d-%d\n",struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwYear,struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwMonth,struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwDay,
													//struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwHour,struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwMinute,struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwSecond);
		info.LeaveAndEnterAttr.dwLeaveNum = struPDCAlarmInfo.dwLeaveNum;
		info.LeaveAndEnterAttr.dwEnterNum = struPDCAlarmInfo.dwEnterNum;
		time.year = struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwYear;
		time.month = struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwMonth;
		time.day = struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwDay;
		time.hour = struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwHour;
		time.min = struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwMinute;
		time.sec = struPDCAlarmInfo.uStatModeParam.struStatTime.tmEnd.dwSecond;

		snprintf(info.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d",time.year, time.month,time.day, time.hour, time.min, time.sec, (int)rand()%1000);
		GetHKstoreRegistHandle()->attendancehandle.pAddVolumeOfCommutersNum(time,&info);
	}
}

static void CALLBACK g_MessageCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser)
{
    HK_DEBUG("lCommand = 0x%02x", lCommand);
	int iret = -1, i = 0;
	void *resultHanlde = NULL;
	DataAtt_T deviceInfo;
	HandleManage_T *head = &GetHkSdkHandle()->deviceListHead;
	
	if(NULL == pAlarmer){
		return;
	}
	memset(&deviceInfo, 0, sizeof(DataAtt_T));
	switch(lCommand)
   	{
		case COMM_ITS_PLATE_RESULT:
			{
				iret =ITS_PlatePrase(pAlarmInfo, (char *)pAlarmer->sSerialNumber, &deviceInfo);
				if(iret < 0){
					return;
				}
				iret = GetHKstoreRegistHandle()->DataAnalysishandle.pAddVehicleInfo(&deviceInfo);
				HK_DEBUG("type: %d", deviceInfo.type);
			}
			break;
		case COMM_UPLOAD_FACESNAP_RESULT:
			//FunHKSdkFaceSnap(pAlarmer, pAlarmInfo);
			
			break;
   		case COMM_ALARM_PDC:
			FunHKSdkGetTraffic(pAlarmer, pAlarmInfo);
			break;
   		case COMM_ALARM_RULE:
			NET_VCA_RULE_ALARM struVcaAlarm;
			memset(&struVcaAlarm,0,sizeof(NET_VCA_RULE_ALARM));
			memcpy(&struVcaAlarm, pAlarmInfo, sizeof(NET_VCA_RULE_ALARM));
			WORD wEventType;
			wEventType = struVcaAlarm.struRuleInfo.wEventTypeEx;
			break;
		/*人脸告警*/	
		case COMM_ALARM_FACE_DETECTION:
			break;
		case COMM_SNAP_MATCH_ALARM:
			FunHkSdkFaceCompare(pAlarmer, pAlarmInfo);
			break;
		/*车辆告警*/
		case COMM_UPLOAD_PLATE_RESULT:
			break;		
		/*混合目标告警*/	
		case COMM_VCA_ALARM:
			iret = vca_DataPrase(pAlarmInfo, (char *)pAlarmer->sSerialNumber, &deviceInfo);
			iret = GetHKstoreRegistHandle()->DataAnalysishandle.pAddVehicleInfo(&deviceInfo);
			HK_DEBUG("type: %d", deviceInfo.type);
			//HK_DEBUG("plateNo: %s", deviceInfo.DataUnion.deviceEntrance.plateNo);
			//HK_DEBUG("vehicleImagePath: %s", deviceInfo.DataUnion.deviceEntrance.vehicleImagePath);
			//HK_DEBUG("plateColor: %d", deviceInfo.DataUnion.deviceEntrance.plateColor);
			//printf("智能检测通用报警, 报警数据内容：%s\n", pAlarmInfo);
			break;
		/*温度报警*/
		case COMM_THERMOMETRY_ALARM:
			iret = thermometry_DataPrase(pAlarmInfo, (char *)pAlarmer->sSerialNumber, &deviceInfo);
			iret = GetHKstoreRegistHandle()->DataAnalysishandle.pAddVehicleInfo(&deviceInfo);
			break;
		/*温差报警*/
		case COMM_THERMOMETRY_DIFF_ALARM:
			break;
		/*抽烟报警*/
		case MINOR_SMOKE_DETECT_ALARM_BEGIN:
			break;
		/*火点检测*/	
		case COMM_FIREDETECTION_ALARM:
			iret = firedetection_DataPrase(pAlarmInfo, (char *)pAlarmer->sSerialNumber, &deviceInfo);
			iret = GetHKstoreRegistHandle()->DataAnalysishandle.pAddVehicleInfo(&deviceInfo);
			break;
		default:
			break;
	}
}

static void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize,void* dwUser)
{	
	int mm = 0, channel = -1, queueid = -1;
	HkSdk_DeviceInfo *pLparam = NULL;	
	HandleManage_T   *cur = NULL;
	HandleManage_T   *head = &GetHksdkRegistHandle()->dvrlistHead;
	PFREAE_FD fd = &_frame_fd;

	for(cur = head->next; cur != NULL; cur = cur->next)
	{
		pLparam = (HkSdk_DeviceInfo *)cur->handle;
		if(strlen(pLparam->lDvrSerialNumber) > 0)
		{
			for(mm = 0; mm < NET_IPC_MAX_NUM; mm++)
			{
				if(pLparam->lRealPlayHandle[mm] == lRealHandle)
				{
					channel = pLparam->lChannel[mm];
					queueid = pLparam->lqueueId[mm];
					break;
				}
			}
		}
	}

	if(channel < 0)
	{
		return;
	}
	switch(dwDataType)
	{
		case NET_DVR_SYSHEAD:	   //系统头 
			break;
		case NET_DVR_STREAMDATA:   //码流数据
			{
				int  nI = 0, nCacheSize = 0, i = 0;
				BOOL bVideo = FALSE, bPatialData = FALSE;

				nI = fd->m_mFrameCacheLenth[lRealHandle];
				bPatialData = GetH246FromPS(pBuffer, dwBufSize, &fd->m_pFrameCache[lRealHandle][nI].pCacheBuffer, fd->m_pFrameCache[lRealHandle][nI].nCacheBufLenth, bVideo);
				if(TRUE == bVideo)
				{
					if(TRUE == bPatialData)//部分包数据
					{
						//缓存数据
						fd->m_pFrameCache[lRealHandle][nI].lTimeStamp = clock();
						fd->m_mFrameCacheLenth[lRealHandle]++;
					}
					else//包头
					{
						if(fd->m_mFrameCacheLenth[lRealHandle] > 0)
						{
							long lH264DataLenth = fd->m_mFrameCacheLenth[lRealHandle]*MAX_PACK_SIZE;
							BYTE *pH264Nal      = NULL;
							BYTE *pTempBuffer   = NULL;
							int  nTempBufLenth  = 0;
							pH264Nal = (BYTE*)malloc(lH264DataLenth);
							memset(pH264Nal, 0x00, lH264DataLenth);
							pTempBuffer = pH264Nal;
							
							for(i = 0; i < fd->m_mFrameCacheLenth[lRealHandle]; i++)
							{
								if(fd->m_pFrameCache[lRealHandle][i].pCacheBuffer != NULL && fd->m_pFrameCache[lRealHandle][i].nCacheBufLenth > 0)
								{
									memcpy(pH264Nal+nTempBufLenth, fd->m_pFrameCache[lRealHandle][i].pCacheBuffer, fd->m_pFrameCache[lRealHandle][i].nCacheBufLenth);
									nTempBufLenth += fd->m_pFrameCache[lRealHandle][i].nCacheBufLenth;
								}
								
								if(fd->m_pFrameCache[lRealHandle][i].pCacheBuffer)
								{
									free(fd->m_pFrameCache[lRealHandle][i].pCacheBuffer);
									fd->m_pFrameCache[lRealHandle][i].pCacheBuffer = NULL;
								}	
								fd->m_pFrameCache[lRealHandle][i].nCacheBufLenth = 0; 
							}
							
							if(pH264Nal && nTempBufLenth > 0)
							{
								BOOL bIsKeyFrame = FALSE;
								if(pH264Nal[4]==0x67 || pH264Nal[4] == 0x27)	//查找是否为关键帧
								{
									bIsKeyFrame = TRUE;
								}
								long lTimeStamp = clock();
								WriteH264DataToChace(queueid, pH264Nal, nTempBufLenth, bIsKeyFrame, lTimeStamp);
							}
							
							if(pH264Nal != NULL)
							{
								free(pH264Nal);
								pH264Nal = NULL;
							}
							fd->m_mFrameCacheLenth[lRealHandle] = 0; // 缓存数据个数清0
						}
					}
				}
			}
			break;
		case NET_DVR_AUDIOSTREAMDATA:
			break;
		default:
			break;
	}
}

static void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
	switch(dwType) 
	{
		case EXCEPTION_RECONNECT:  //预览重连
			break;
		case EXCEPTION_LOST_ALARM: //报警丢失
			break;
		default:
			HK_DEBUG("dwType: 0x%x", dwType);
			break;
	}
}

static int HksdkDeviceListGet(HKDVRParam_T *dvrParam)
{
	int dvrNum = 0;
	HKDVRParam_T   *Lparam = NULL;	
	HandleManage_T *cur = NULL;
	HandleManage_T *head = &GetHkSdkHandle()->deviceListHead;

	if(NULL == dvrParam)
	{
		return KEY_FALSE;
	}
	//Lparam = (HKDVRParam_T *)HandleManageGetNextHandle(head);
	//if(NULL != Lparam){
	//	HK_DEBUG("sUserName: %s", Lparam->HkDvrInfo.sUserName);
	//	HK_DEBUG("sDvrSerialNumber: %s", Lparam->HkDvrInfo.sDvrSerialNumber);
	//}
#if 1	
	for(cur = head->next; cur != NULL; cur = cur->next)
	{
		if (NULL == cur->handle){
			continue;
		}
		Lparam = (HKDVRParam_T *)cur->handle;
		HK_DEBUG("sUserName: %s", Lparam->HkDvrInfo.sUserName);
		HK_DEBUG("sDvrSerialNumber: %s", Lparam->HkDvrInfo.sDvrSerialNumber);
		memcpy(&dvrParam[dvrNum], Lparam, sizeof(HKDVRParam_T));
		dvrNum++;
	}
#endif	
	return dvrNum;
}

static int HksdkDeviceListSet(HKDVRParam_T *dvrParam)
{
	int   i = 0;
	void *resultHanlde = NULL;
	HandleManage_T *head = &GetHkSdkHandle()->deviceListHead;

	
	HandleManageGetHandleByStr(head, HkDvrInfo.sDvrSerialNumber, dvrParam->HkDvrInfo.sDvrSerialNumber, resultHanlde, HKDVRParam_T);
	if(NULL == resultHanlde){
		HK_ERROR("dvr %s is not add !", dvrParam->HkDvrInfo.sDvrSerialNumber);
		return KEY_FALSE;
	}
	
	//更改链表数据
	HKDVRParam_T *pHkDvrInfo = (HKDVRParam_T *)resultHanlde;
	pHkDvrInfo->HkDvrInfo.wPort = dvrParam->HkDvrInfo.wPort;
	pHkDvrInfo->HkDvrInfo.wChannel = dvrParam->HkDvrInfo.wChannel;
	memcpy(pHkDvrInfo->HkDvrInfo.sDeviceAddress, dvrParam->HkDvrInfo.sDeviceAddress, NET_DVR_MAX_LEN);
	memcpy(pHkDvrInfo->HkDvrInfo.sUserName, dvrParam->HkDvrInfo.sUserName, NET_DVR_MAX_LEN);
	memcpy(pHkDvrInfo->HkDvrInfo.sPassword, dvrParam->HkDvrInfo.sPassword, NET_DVR_MAX_LEN);
	memcpy(pHkDvrInfo->HkDvrInfo.sDvrSerialNumber, dvrParam->HkDvrInfo.sDvrSerialNumber, NET_DVR_MAX_LEN);
	memcpy(pHkDvrInfo->HkDvrInfo.sDvrName, dvrParam->HkDvrInfo.sDvrName, NET_DVR_MAX_LEN);
	for(i = 0; i< pHkDvrInfo->HkDvrInfo.wChannel; i++)
	{
		if(pHkDvrInfo->HkSdkParam[i].sChannel == dvrParam->HkSdkParam[i].sChannel)
		{
			pHkDvrInfo->HkSdkParam[i].sOnline  = dvrParam->HkSdkParam[i].sOnline;
			pHkDvrInfo->HkSdkParam[i].sChannel = dvrParam->HkSdkParam[i].sChannel;
			pHkDvrInfo->HkSdkParam[i].sQueueID = dvrParam->HkSdkParam[i].sQueueID;
			pHkDvrInfo->HkSdkParam[i].sDeviceType = dvrParam->HkSdkParam[i].sDeviceType;
			memcpy(pHkDvrInfo->HkSdkParam[i].sDeviceName, dvrParam->HkSdkParam[i].sDeviceName, NET_DVR_MAX_LEN);
			memcpy(pHkDvrInfo->HkSdkParam[i].sDeviceAddress, dvrParam->HkSdkParam[i].sDeviceAddress, NET_DVR_MAX_LEN);
		}
	}
	
	//更改配置文件	
	HkDvrListConfigSet(pHkDvrInfo);
	return KEY_TRUE;
}

static int HKsdkDeviceAlarmGet(DeviceAlarmParam_T *alarmParam)
{
	if(NULL == alarmParam)
	{
		return KEY_FALSE;
	}
	HkDeviceAlarmConfigGet(alarmParam);
	return KEY_TRUE;
}

static int HKsdkDeviceAlarmSet(DeviceAlarmParam_T *alarmParam)
{
	if(NULL == alarmParam)
	{
		return KEY_FALSE;
	}

	HkDeviceAlarmConfigSet(alarmParam);
	return KEY_TRUE;
}

static int HksdkAddcameraFunc(void *handle,int argc,void *arg[])
{
	DvrOpt_T *cameraopt = NULL;
	if(handle == NULL){
		return KEY_FALSE;
	}
	cameraopt = (DvrOpt_T *)arg[0];
	HksdkManageHandle_T *curHandle = (HksdkManageHandle_T *)handle;
	if(NULL == curHandle->addCameraCallBack){
		return KEY_FALSE;
	}
	curHandle->addCameraCallBack(cameraopt);
	return KEY_TRUE;
}

static int HksdkDelcameraFunc(void *handle,int argc,void *arg[])
{
	DvrOpt_T *cameraopt = NULL;
	if(handle == NULL){
		return KEY_FALSE;
	}
	
	cameraopt = (DvrOpt_T *)arg[0];
	HksdkManageHandle_T *curHandle = (HksdkManageHandle_T *)handle;
	if(NULL == curHandle->delCameraCallBack){
		return KEY_FALSE;
	}
	curHandle->delCameraCallBack(cameraopt);
	return KEY_TRUE;
}

static int HksdkDeviceAdd(HKDVRParam_T *devInfo)
{
	int   i = 0, queueid = -1;
	LONG  lUserID = -1, lcamUserID = -1, listenHandle = -1, lRealPlayHandle = -1;
	DWORD dwReturned = 0;
	void *resultHanlde = NULL;
	BYTE byIPID,byIPIDHigh;
	int iDevInfoIndex, iIPCh, iDevID;
	HKDVRParam_T HkDvrInfo = {0};
	NET_DVR_IPPARACFG_V40 IpcamCfg = {0};
	HandleManage_T *head = &GetHkSdkHandle()->deviceListHead;
	 int ret = -1;

	if(NULL == devInfo){
		return KEY_FALSE;
	}

	HkSdkInit();
	//登录参数，包括设备地址、登录用户、密码等
	NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
	struLoginInfo.bUseAsynLogin = false; //同步登录方式
	memcpy(struLoginInfo.sDeviceAddress, devInfo->HkDvrInfo.sDeviceAddress, NET_DVR_MAX_LEN); //设备IP地址
	struLoginInfo.wPort = devInfo->HkDvrInfo.wPort; //设备服务端口
	memcpy(struLoginInfo.sUserName, devInfo->HkDvrInfo.sUserName, NET_DVR_MAX_LEN);       //设备登录用户名
	memcpy(struLoginInfo.sPassword, devInfo->HkDvrInfo.sPassword, NET_DVR_MAX_LEN);   	  //设备登录密码
	//HK_DEBUG("%s %s %s %d", struLoginInfo.sDeviceAddress, struLoginInfo.sUserName, struLoginInfo.sPassword, struLoginInfo.wPort);
	
	//设备信息, 输出参数
	NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};
	lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
	if (lUserID < 0)
	{
	  HK_ERROR("Login failed, error code: [%d]", NET_DVR_GetLastError());
	  NET_DVR_Cleanup();
	  return KEY_FALSE;
	}

	//设置告警回调
	NET_DVR_SetDVRMessageCallBack_V51(0, g_MessageCallback, NULL);
	//启用布防
	LONG lHandle; 
	NET_DVR_SETUPALARM_PARAM  struAlarmParam = {0}; 
	struAlarmParam.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM); 
	struAlarmParam.byLevel = 0;  
	struAlarmParam.byAlarmInfoType = 1;  
	struAlarmParam.byFaceAlarmDetection = 1;
	struAlarmParam.bySupport = 1;
	struAlarmParam.byAlarmTypeURL = 0x01;
	
	lHandle = NET_DVR_SetupAlarmChan_V41(lUserID, &struAlarmParam); 
	if (lHandle < 0) 
	{ 
		HK_ERROR("NET_DVR_SetupAlarmChan_V41 error, [%d]", NET_DVR_GetLastError()); 
	} 

	//内部链表
	HkSdk_DeviceInfo dvrInfo = {0};
	NET_DVR_PREVIEWINFO struPlayInfo = {0};
	HkSdk_Regist_T *HkSdkManageHandle = GetHksdkRegistHandle();
	HandleManage_T *dvrHead = &(HkSdkManageHandle->dvrlistHead);

	char workersFDID[128] = {0};
	char customerFDID[128] = {0};
	GetFDIDFromNVR(lUserID,(char*)"customer",customerFDID);
	GetFDIDFromNVR(lUserID,(char*)"workers", workersFDID);

	memcpy(dvrInfo.lDvrSerialNumber, struDeviceInfoV40.struDeviceV30.sSerialNumber, NET_DVR_MAX_LEN);
	NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_IPPARACFG_V40, 0, (void *)&IpcamCfg, sizeof(NET_DVR_IPPARACFG_V40), (LPDWORD)&dwReturned);
	//HK_DEBUG("byIPChanNum:%d  byStartChan:%d", struDeviceInfoV40.struDeviceV30.byIPChanNum, struDeviceInfoV40.struDeviceV30.byStartChan);
	//HK_DEBUG("sSerialNumber %s", struDeviceInfoV40.struDeviceV30.sSerialNumber);
	//HK_DEBUG("dwDChanNum %d  dwStartDChan: %d", IpcamCfg.dwDChanNum, IpcamCfg.dwStartDChan);
	//HK_DEBUG("dwAChanNum %d  dwGroupNum: %d", IpcamCfg.dwAChanNum, IpcamCfg.dwGroupNum);
	HandleManageGetHandleByStr(head, HkDvrInfo.sDvrSerialNumber, devInfo->HkDvrInfo.sDvrSerialNumber, resultHanlde, HKDVRParam_T);
	if(NULL == resultHanlde)
	{
		HkDvrInfo.HkDvrInfo.wPort = devInfo->HkDvrInfo.wPort;
		HkDvrInfo.HkDvrInfo.wChannel = IpcamCfg.dwDChanNum;
		HkDvrInfo.HkDvrInfo.wStartDChan = IpcamCfg.dwStartDChan;
		memcpy(HkDvrInfo.HkDvrInfo.sDeviceAddress, devInfo->HkDvrInfo.sDeviceAddress, NET_DVR_MAX_LEN);
		memcpy(HkDvrInfo.HkDvrInfo.sUserName, devInfo->HkDvrInfo.sUserName, NET_DVR_MAX_LEN);
		memcpy(HkDvrInfo.HkDvrInfo.sPassword, devInfo->HkDvrInfo.sPassword, NET_DVR_MAX_LEN);
		memcpy(HkDvrInfo.HkDvrInfo.sDvrSerialNumber, struDeviceInfoV40.struDeviceV30.sSerialNumber, NET_DVR_MAX_LEN);
		memcpy(HkDvrInfo.HkDvrInfo.sDvrName, devInfo->HkDvrInfo.sDvrName, NET_DVR_MAX_LEN);
		memcpy(HkDvrInfo.HkDvrInfo.ICustomerFPID,customerFDID,NET_FPID_NUM);
		memcpy(HkDvrInfo.HkDvrInfo.IWorkersFPID,workersFDID,NET_FPID_NUM);
		for(i = 0; i< IpcamCfg.dwDChanNum; i++)
		{
			
			//if(1 == IpcamCfg.struIPDevInfo[i].byEnable)
			{
				byIPID = IpcamCfg.struStreamMode[i].uGetStream.struChanInfo.byIPID;
				byIPIDHigh = IpcamCfg.struStreamMode[i].uGetStream.struChanInfo.byIPIDHigh;
				iDevInfoIndex = byIPIDHigh*256 + byIPID - 1; 
				//if(strlen(IpcamCfg.struIPDevInfo[iDevInfoIndex].struIP.sIpV4) > 0)
				//{
				//	HK_DEBUG("IP channel no.%d, IP: %s  sDeviceType:%d", i, IpcamCfg.struIPDevInfo[iDevInfoIndex].struIP.sIpV4, devInfo->HkSdkParam[i].sDeviceType);
				//}
				//HK_DEBUG("IP channel no.%d, IP: %s  sDeviceType:%d", i + 1, IpcamCfg.struIPDevInfo[iDevInfoIndex].struIP.sIpV4, devInfo->HkSdkParam[i].sDeviceType);
				HkDvrInfo.HkSdkParam[i].sOnline  = IpcamCfg.struStreamMode[iDevInfoIndex].uGetStream.struChanInfo.byEnable;
				HkDvrInfo.HkSdkParam[i].sChannel = i + 1; //DVR通道号
				HkDvrInfo.HkSdkParam[i].sDeviceType = DEVICE_TYPE_Entrance; //DEVICE_TYPE_Frontage; //DEVICE_TYPE_NONE;
				if(HkDvrInfo.HkSdkParam[i].sChannel == 4)
				{
					HkDvrInfo.HkSdkParam[i].sDeviceType = DEVICE_TYPE_Entrance;
				}
				memcpy(HkDvrInfo.HkSdkParam[i].sDeviceAddress, IpcamCfg.struIPDevInfo[iDevInfoIndex].struIP.sIpV4, NET_DVR_MAX_LEN);
				snprintf(HkDvrInfo.HkSdkParam[i].sDeviceName, NET_DVR_MAX_LEN, "chn_%d", i+1);

				//请求推流
				memset(&struPlayInfo, 0, sizeof(NET_DVR_PREVIEWINFO));
				struPlayInfo.hPlayWnd	  = (HWND)NULL; //需要SDK解码时句柄设为有效值，仅取流不解码时可设为空
				struPlayInfo.lChannel	  = IpcamCfg.dwStartDChan + i;   //预览通道号
				struPlayInfo.dwStreamType = 0;	  //0-主码流，1-子码流，2-码流3，3-码流4，以此类推
				struPlayInfo.dwLinkMode   = 0;	  //0-TCP方式，1- UDP方式，2- 多播方式，3- RTP方式，4-RTP/RTSP，5-RSTP/HTTP
				struPlayInfo.bBlocked	  = 1;	  //0-非阻塞取流，1- 阻塞取流
				if(HkDvrInfo.HkSdkParam[i].sOnline)
				//if(strlen(IpcamCfg.struIPDevInfo[iDevInfoIndex].struIP.sIpV4) > 0)
				{
					lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, g_RealDataCallBack_V30, NULL);
					if(lRealPlayHandle < 0)
					{
					  HK_ERROR("NET_DVR_RealPlay_V40 error, [%d]", NET_DVR_GetLastError());
					}
					//创建mediaqueue
					queueid = GetMediaqueueRegistHandle()->createQueneFromMediaQuenecb(6*1024*1024);
					if(queueid < 0)
					{
						HK_ERROR("createQueneFromMediaQuenecb error ch[%d]", HkDvrInfo.HkSdkParam[i].sChannel);
						return KEY_FALSE;
					}
					HkDvrInfo.HkSdkParam[i].sQueueID = queueid;
					dvrInfo.lqueueId[i] = HkDvrInfo.HkSdkParam[i].sQueueID;
					dvrInfo.lChannel[i] = HkDvrInfo.HkSdkParam[i].sChannel;
					dvrInfo.lRealPlayHandle[i] = lRealPlayHandle;
					HK_DEBUG("sChannel:%d  lRealPlayHandle:%d  ipadress:%s",  HkDvrInfo.HkSdkParam[i].sChannel, lRealPlayHandle, HkDvrInfo.HkSdkParam[i].sDeviceAddress);

					//回调触发
					void *arg[1] = {0};
					DvrOpt_T dvropt = {0};
					dvropt.wChannel = dvrInfo.lChannel[i];
					dvropt.wQueueID = dvrInfo.lqueueId[i];
					memcpy(dvropt.sDvrSerialNumber, HkDvrInfo.HkDvrInfo.sDvrSerialNumber, NET_DVR_MAX_LEN);
					
					arg[0] = &dvropt;
					HandleManagePostLoop(&(GetHksdkUserRegistHandle()->deviceListHead), HksdkAddcameraFunc, 0, arg);
#if 0
					//登陆IPC设备
					memset(&struLoginInfo, 0, sizeof(NET_DVR_USER_LOGIN_INFO));
					struLoginInfo.bUseAsynLogin = false; //同步登录方式
					memcpy(struLoginInfo.sDeviceAddress, HkDvrInfo.HkSdkParam[i].sDeviceAddress, NET_DVR_MAX_LEN); //设备IP地址
					struLoginInfo.wPort = 8000; 		 //设备服务端口
					memcpy(struLoginInfo.sUserName, "admin", NET_DVR_MAX_LEN);		//设备登录用户名
					memcpy(struLoginInfo.sPassword, "gaozhi2014", NET_DVR_MAX_LEN);	//设备登录密码
					//设备信息, 输出参数
					NET_DVR_DEVICEINFO_V40 struDeviceInfoV40IPC;
					memset(&struDeviceInfoV40IPC, 0, sizeof(NET_DVR_DEVICEINFO_V40));
					lcamUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40IPC);
					if (lcamUserID < 0)
					{
					  HK_ERROR("Login failed, error code: [%d]", NET_DVR_GetLastError());
					  //NET_DVR_Cleanup();
					  return KEY_FALSE;
					}
					HK_DEBUG("lChannel:%d  lcamUserID:%d", dvrInfo.lChannel[i], lcamUserID);
					//HK_DEBUG("byChannel: %d", IpcamCfg.struStreamMode[i].uGetStream.struChanInfo.byChannel)
					//获取音视频参数
					NET_DVR_COMPRESSIONCFG_V30 dvrCompressCfg = {0};
					if(!NET_DVR_GetDVRConfig(lcamUserID, NET_DVR_GET_COMPRESSCFG_V30, IpcamCfg.struStreamMode[i].uGetStream.struChanInfo.byChannel, 
						(void *)&dvrCompressCfg, sizeof(NET_DVR_COMPRESSIONCFG_V30), (LPDWORD)&dwReturned))
					{
						HK_ERROR("NET_DVR_GetDVRConfig failed, error code: [%d]", NET_DVR_GetLastError());
					}
					else
					{
						HK_DEBUG("byBitrateType:%d",    dvrCompressCfg.struNormHighRecordPara.byBitrateType);
						HK_DEBUG("byStreamType:%d",     dvrCompressCfg.struNormHighRecordPara.byStreamType);
						HK_DEBUG("byResolution:%d",     dvrCompressCfg.struNormHighRecordPara.byResolution);
						HK_DEBUG("dwVideoBitrate:%lu",  dvrCompressCfg.struNormHighRecordPara.dwVideoBitrate);
						HK_DEBUG("dwVideoFrameRate:%lu",dvrCompressCfg.struNormHighRecordPara.dwVideoFrameRate);
						HK_DEBUG("byVideoEncType:%d",   dvrCompressCfg.struNormHighRecordPara.byVideoEncType);				
						HK_DEBUG("byAudioEncType:%d",   dvrCompressCfg.struNormHighRecordPara.byAudioEncType);
						HK_DEBUG("byPicQuality:%d",     dvrCompressCfg.struNormHighRecordPara.byPicQuality);				
						//存链表 视频参数发生变化需重启服务器
						//QueueVideoInputInfo_T videoInfo = {0};
						//QueueAudioInputInfo_T audioInfo = {0};
						//byHKChangeToMediaQueue(dvrCompressCfg.struNormHighRecordPara, &videoInfo, &audioInfo);
					}
					NET_DVR_Logout(lcamUserID);
#endif 				
					}
					else
					{
						dvrInfo.lqueueId[i] = -1;
						dvrInfo.lChannel[i] = -1;
						dvrInfo.lRealPlayHandle[i] = -1;
						HkDvrInfo.HkSdkParam[i].sQueueID = -1;
					}
				}
			//HK_DEBUG("_____%s %d %s", IpcamCfg.struIPDevInfo[i].sIpV4, IpcamCfg.struIPDevInfo[i].byCameraType, IpcamCfg.struIPDevInfo[i].szDeviceID);
		}
		//存内部在线链表
		dvrInfo.lUserID = lUserID;
		dvrInfo.lhandle = listenHandle;
		memcpy(dvrInfo.lDvrSerialNumber, struDeviceInfoV40.struDeviceV30.sSerialNumber, NET_DVR_MAX_LEN);
		HandleManageAddHandle(dvrHead, (void *)&dvrInfo);

		//存入链表，包含dvr所有信息  ！！！
		HK_DEBUG("HkDvrInfo:  %s", HkDvrInfo.HkDvrInfo.sDeviceAddress);
		HK_DEBUG("wStartDChan:%d", HkDvrInfo.HkDvrInfo.wStartDChan);
		HK_DEBUG("wChannel:   %d", HkDvrInfo.HkDvrInfo.wChannel);
		HK_DEBUG("sDvrSerialNumber:%s", HkDvrInfo.HkDvrInfo.sDvrSerialNumber);
		HandleManageAddHandle(head, (void *)&HkDvrInfo);

		HKDVRParam_T *pas = NULL;
		pas = (HKDVRParam_T *)HandleManageGetNextHandle(head);;
		HK_DEBUG("====sDvrSerialNumber:%s", pas->HkDvrInfo.sDvrSerialNumber);
		//存配置文件 存配置文件的时候需要判断
		HkDvrListConfigAdd(&HkDvrInfo);
	}
	else
	{
		HK_ERROR("Dvr %s is Added !", devInfo->HkDvrInfo.sDvrSerialNumber);
	}
	
	HK_DEBUG("HkSdkDeviceLogin success %s", devInfo->HkDvrInfo.sDeviceAddress);
	return KEY_TRUE;
}

static int HksdkDeviceDel(HKDVRParam_T *devInfo)
{
	int i = 0;
	
	if(NULL == devInfo){
		return KEY_FALSE;
	}
	
	//删外部链表
	void *resultHanlde = NULL;
	HandleManage_T *head = &GetHkSdkHandle()->deviceListHead;
	HandleManageGetHandleByStr(head, HkDvrInfo.sDvrSerialNumber, devInfo->HkDvrInfo.sDvrSerialNumber, resultHanlde, HKDVRParam_T);
	if(NULL == resultHanlde)
	{
		HK_ERROR("Dvr %s is not Added !", devInfo->HkDvrInfo.sDvrSerialNumber);
		return KEY_FALSE;
	}
	HandleManageDelHandle(head, (void *)resultHanlde);


	//删内部在线链表管理handle
	resultHanlde = NULL;
	head = &GetHksdkRegistHandle()->dvrlistHead;
	HandleManageGetHandleByStr(head, lDvrSerialNumber, devInfo->HkDvrInfo.sDvrSerialNumber, resultHanlde, HkSdk_DeviceInfo);
	if(NULL == resultHanlde)
	{
		HK_ERROR("Dvr %s is not Add !", devInfo->HkDvrInfo.sDvrSerialNumber);
		return KEY_FALSE;
	}
	
	//停止监听
	HkSdk_DeviceInfo *pDeviceInfo = (HkSdk_DeviceInfo *)resultHanlde;
	for(i = 0; i < NET_IPC_MAX_NUM; i++)
	{
		//关闭预览
		if(pDeviceInfo->lRealPlayHandle[i] >= 0){
			NET_DVR_StopRealPlay(pDeviceInfo->lRealPlayHandle[i]);
		}
		
		if(pDeviceInfo->lqueueId[i] >= 0){
			//回调触发
			void *arg[1] = {0};
			DvrOpt_T dvropt = {0};
			dvropt.wChannel = pDeviceInfo->lChannel[i];
			dvropt.wQueueID = pDeviceInfo->lqueueId[i];
			memcpy(dvropt.sDvrSerialNumber, pDeviceInfo->lDvrSerialNumber, NET_DVR_MAX_LEN);
			
			arg[0] = &dvropt;
			HandleManagePostLoop(&(GetHksdkUserRegistHandle()->deviceListHead), HksdkDelcameraFunc, 0, arg);
			//销毁mediaqueue
			GetMediaqueueRegistHandle()->disdroyQueneFromMediaQuenecb(pDeviceInfo->lqueueId[i]);
		}
	}
	//sdk登出 清除缓存资源
	NET_DVR_StopListen_V30(pDeviceInfo->lhandle);
	NET_DVR_Logout(pDeviceInfo->lUserID);
	NET_DVR_Cleanup();
	HandleManageDelHandle(head, (void *)resultHanlde);

	//删配置文件
	HkDvrListConfigDel(devInfo);
	return KEY_TRUE;
}

static int HkSdkInit()
{
	//初始化
	NET_DVR_Init();
	//NET_DVR_SetLogToFile(3, "./log", FALSE);

	//设置HCNetSDKCom组件库
	NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SDK_PATH,    (void *)"/home/ybzhao/lib");
	//设置libcrypto.so组件库
	NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_LIBEAY_PATH, (void *)"/home/ybzhao/lib/libcrypto.so");
	//设置libssl.so组件库
	NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SSLEAY_PATH, (void *)"/home/ybzhao/lib/libssl.so.1.0.0");
	
	//获取sdk版本号
	LONG lVersion = NET_DVR_GetSDKVersion();
	HK_DEBUG("Ecms Build Version ==>%d.%d.%d.%d",(lVersion & 0xff000000) >> 24, (lVersion & 0x00ff0000) >> 16, 
		(lVersion & 0x0000ff00) >> 8, lVersion & 0x000000ff);
	
	//设置连接时间与重连时间
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(10000, true);
	//设置异常消息回调函数
	NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL);
	return KEY_TRUE;
}

static int HkSdkUnInit()
{
	int i = 0;
	HkSdk_DeviceInfo *Lparam = NULL;	
	HandleManage_T   *cur = NULL;
	HandleManage_T   *head = &GetHksdkRegistHandle()->dvrlistHead;

	for(cur = head->next; cur != NULL; cur = cur->next)
	{
		Lparam = (HkSdk_DeviceInfo *)cur->handle;
		for(i = 0; i < NET_IPC_MAX_NUM; i++)
		{
			if(Lparam->lRealPlayHandle[i] >= 0)
			{
				NET_DVR_StopRealPlay(Lparam->lRealPlayHandle[i]);
			}
			
			if(Lparam->lqueueId[i] >= 0){
				GetMediaqueueRegistHandle()->disdroyQueneFromMediaQuenecb(Lparam->lqueueId[i]);
			}
		}
		NET_DVR_StopListen_V30(Lparam->lhandle);
		NET_DVR_Logout(Lparam->lUserID);
		NET_DVR_Cleanup();
	}

	return KEY_TRUE;
}

_OUT_ int HksdkManageRegistHandle(HksdkManageHandle_T *handle)
{
	if(NULL == handle){
		return KEY_FALSE;
	}
	
	handle->pHksdkDeviceAdd			 = HksdkDeviceAdd;
	handle->pHksdkDeviceDel			 = HksdkDeviceDel;
	handle->pHksdkDeviceListGet		 = HksdkDeviceListGet;
	handle->pHksdkDeviceListSet      = HksdkDeviceListSet;
	handle->pHKsdkDeviceAlarmGet     = HKsdkDeviceAlarmGet;
	handle->pHKsdkDeviceAlarmSet     = HKsdkDeviceAlarmSet;


	if(NULL == handle->addCameraCallBack || NULL == handle->delCameraCallBack)
	{
		//HK_DEBUG("handle is NULL .");
		return KEY_TRUE;
	}
	
	HandleManageAddHandle(&(GetHksdkUserRegistHandle()->deviceListHead), (void *)handle);
	return KEY_TRUE;
}

_OUT_ int HksdkManageUnRegistHandle(HksdkManageHandle_T *handle)
{
	if(NULL == handle){
		return KEY_FALSE;
	}
	
	handle->pHksdkDeviceAdd			 = NULL;
	handle->pHksdkDeviceDel			 = NULL;
	handle->pHksdkDeviceListGet      = NULL;
	handle->pHksdkDeviceListSet      = NULL;
	handle->pHKsdkDeviceAlarmGet     = NULL;
	handle->pHKsdkDeviceAlarmSet     = NULL;
	HandleManageDelHandle(&GetHksdkUserRegistHandle()->deviceListHead, (void *)handle);
	return KEY_TRUE;
}

_OUT_ int HksdkApiInit(HksdkManageHandle_T *handle)
{
	int iret = 0;
	//iret = HkSdkInit();
	//if(iret < 0) {
	//	return KEY_FALSE;
	//}
	
	//内部在线链表，存放handle
	HkSdk_Regist_T *HkSdkManageHandle = GetHksdkRegistHandle();
	HandleManageInitHead(&HkSdkManageHandle->dvrlistHead);

	//外部在线链表，存放dvr所有信息
	HkSdk_Inner_T *HkSdkInnerHandle = GetHkSdkHandle();
	HandleManageInitHead(&HkSdkInnerHandle->deviceListHead); 

	HKstore_Regist_T *HkStoreHandle = GetHKstoreRegistHandle();
	VehicleDataAnalysisManageRegister(&HkStoreHandle->DataAnalysishandle);
	AttendanceManageRegister(&HkStoreHandle->attendancehandle);

	Hksdk_Mediaqueue_T *HksdkMediaqueueHandle = GetMediaqueueRegistHandle();
	if(NULL != handle->createQueneFromMediaQuenecb && NULL != handle->disdroyQueneFromMediaQuenecb)
	{
		HksdkMediaqueueHandle->createQueneFromMediaQuenecb = handle->createQueneFromMediaQuenecb;
		HksdkMediaqueueHandle->disdroyQueneFromMediaQuenecb= handle->disdroyQueneFromMediaQuenecb;
	}
	
	if (NULL != handle->writeVideoRawDatacb && NULL != handle->writeAudioRawDatacb)
	{
		HksdkMediaqueueHandle->writeVideoRawDatacb = handle->writeVideoRawDatacb;
		HksdkMediaqueueHandle->writeAudioRawDatacb = handle->writeAudioRawDatacb;
	}
	//读配置文件，添加设备到链表，登陆后需要更新配置列表	
//test
#if 0
	HKDVRParam_T *devInfo = NULL;
	devInfo = (HKDVRParam_T *)malloc(sizeof(HKDVRParam_T)*64);
	memset(devInfo,0, sizeof(HKDVRParam_T)*64);
	HkDvrListConfigGet(devInfo);
	int i = 0, m = 0;
	for(;strlen(devInfo[i].HkDvrInfo.sDvrSerialNumber)>0;i++){
		//HkDeviceInfo_T NvrInfo = {0};
		//memcpy(&NvrInfo, &(devInfo[i].HkDvrInfo), sizeof(HkDeviceInfo_T));
		//for(m = 0; m < NvrInfo.wChannel; i++)
		//{
		//}
		HksdkDeviceAdd(&devInfo[i]);
	}
	//需要创建线程 ！！！？？？
	void *resultHanlde = NULL;
	HandleManageGetHandleByStr(&GetHksdkRegistHandle()->dvrlistHead, lDvrSerialNumber, "iDS-9608NX-I8/FA0820210111CCRRF40231916WCVU", resultHanlde, HkSdk_DeviceInfo);
	if(NULL != resultHanlde)
	{
		HkSdk_DeviceInfo *info = (HkSdk_DeviceInfo *)resultHanlde;
		HK_DEBUG("lDvrSerialNumber: %s", info->lDvrSerialNumber);
	}
#else
	HKDVRParam_T devInfo = {0};
	devInfo.HkDvrInfo.wPort = 8000;
	memcpy(devInfo.HkDvrInfo.sDeviceAddress, "10.67.1.68", NET_DVR_MAX_LEN);
	memcpy(devInfo.HkDvrInfo.sUserName, "admin", NET_DVR_MAX_LEN);
	memcpy(devInfo.HkDvrInfo.sPassword, "gaozhi2014", NET_DVR_MAX_LEN);
	HksdkDeviceAdd(&devInfo);

	void *resultHanlde = NULL;
	HandleManageGetHandleByStr(&GetHksdkRegistHandle()->dvrlistHead, lDvrSerialNumber, "iDS-9608NX-I8/FA0820210111CCRRF40231916WCVU", resultHanlde, HkSdk_DeviceInfo);
	if(NULL != resultHanlde)
	{
		HkSdk_DeviceInfo *info = (HkSdk_DeviceInfo *)resultHanlde;
		HK_DEBUG("lDvrSerialNumber: %s", info->lDvrSerialNumber);
	}	
#endif
	HksdkManageRegistHandle(handle);
	return KEY_TRUE;
}

_OUT_ int HksdkApiUnInit(HksdkManageHandle_T *handle)
{
	HkSdkUnInit();
	
	HKstore_Regist_T *HkStoreHandle = GetHKstoreRegistHandle();
	VehicleDataAnalysisManageUnRegister(&HkStoreHandle->DataAnalysishandle);
	HksdkManageUnRegistHandle(handle);
	return KEY_TRUE;
}


