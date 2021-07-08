#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "common.h"
#include "rtmp.h"
#include "rtmpstart.h"
#include "aacenc.h"
#include "adpapi.h"
#include "timemanageout.h"
#include "mediaqueue.h"
#include "rtmpconfig.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef enum _RtmpStatus_E
{
	RTMP_DISCONNECT,   //未连接
	RTMP_CONNECTED,    //已连上
	RTMP_NEEDRECONNECT,//待重连，用于参数发生改变
}RtmpStatus_E;
typedef struct _RtmpMediaStamp_T
{
    unsigned long long  VideoStamp;
	unsigned long long  AudioStamp;
}RtmpMediaStamp_T;

typedef struct _RtmpUserInfo_T
{
	RtmpStatus_E stat;
	int channel;
	int queueid;
	int enable;
	char SerialNumber[64];
	pthread_mutex_t selfmutex;
}RtmpUserInfo_T;

typedef struct _RtmpUSerMng_T
{
	 RtmpUserInfo_T rtmpUser[RTMP_USR_NUM];
	 int rtmpEnable;
}RtmpUSerMng_T;

static RtmpUSerMng_T rtmpMng = {0};
typedef enum {
	nal_unit_type_nr	= 0x01 << 0,
	nal_unit_type_p		= 0x01 << 1,
	nal_unit_type_dataA = 0x01 << 2,
	nal_unit_type_dataB = 0x01 << 3,
	nal_unit_type_dataC = 0x01 << 4,
	nal_unit_type_idr	= 0x01 << 5,
	nal_unit_type_sei	= 0x01 << 6,
	nal_unit_type_sps	= 0x01 << 7,
	nal_unit_type_pps	= 0x01 << 8,
	nal_unit_type_delimiter  = 0x01 << 9,
	nal_unit_type_nalend	= 0x01 << 10,
	nal_unit_type_streamend = 0x01 << 11,
	nal_unit_type_pading	= 0x01 << 12,
}nal_unit_type;


struct SliceInfo{
	unsigned char *Start;
	unsigned int SliceLen;
	nal_unit_type nut;
};
typedef struct {
	unsigned int pic_width;
	unsigned int pic_high;
	unsigned char framerate;
	unsigned char profileidc;
	unsigned char levelidc;
	nal_unit_type nut;
	unsigned char SliceNu;
	struct SliceInfo Sliceinfo[10];
}H264FrameSliceInfo;

#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)
#define FindStartPlace(data) ((data)[0]==0x00 && (data)[1] == 0x00 && (data)[2] == 0x00 && (data)[3] == 0x01)
#define PUSHAAC 

typedef struct _RtmpConfig_T
{
	int  wChannel;
	int  wQueueID;
	int  rtmpEnable;
	RtmpStatus_E stat;
}RtmpConfig_T;

typedef struct _HKSdkHandel_Regist_T
{
	HksdkManageHandle_T HKsdkListhandle;
}HKSdkHandel_Regist_T;
static HKSdkHandel_Regist_T gHKSdkHandelRegist;
static HKSdkHandel_Regist_T *GetHKSdkHandelRegistHandle()
{
	return &gHKSdkHandelRegist;
}

static int GetH264FrameSliceInfo(unsigned char *FrameData, unsigned int Datalen, H264FrameSliceInfo *SliceInfoData)
{

	unsigned int DataPlace = 0;
	int SliceLen = 0;
	memset(SliceInfoData, 0, sizeof(H264FrameSliceInfo));

	for(DataPlace = 0; DataPlace < Datalen - 4; DataPlace++){
		if (!FindStartPlace(&FrameData[DataPlace])){
			continue;
		}

		DataPlace += 4;//让出 00 00 00 01
		switch(0x01 << (FrameData[DataPlace] & 0x1f)){
			case nal_unit_type_nr:
				break;
			case nal_unit_type_p:
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].Start = &FrameData[DataPlace];
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].SliceLen = Datalen - DataPlace;//数组下标与实际少1
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].nut = nal_unit_type_p;
				SliceInfoData->SliceNu++;
				SliceInfoData->nut |= nal_unit_type_p;
				return 0;
				break;
			case nal_unit_type_dataA:
				break;
			case nal_unit_type_dataB:
				break;
			case nal_unit_type_dataC:
				break;
			case nal_unit_type_idr:
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].Start = &FrameData[DataPlace];
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].SliceLen = Datalen - DataPlace;//数组下标与实际少1
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].nut = nal_unit_type_idr;
				SliceInfoData->SliceNu++;
				SliceInfoData->nut |= nal_unit_type_idr;
				return 0;
				break;
			case nal_unit_type_sei:
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].Start = &FrameData[DataPlace];
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].nut = nal_unit_type_sei;
				SliceInfoData->nut |= nal_unit_type_sei;
				for (SliceLen = 0;!FindStartPlace(&FrameData[DataPlace]);DataPlace++, SliceLen++);
				DataPlace--;//回退一个字节，留给for(DataPlace = 0; DataPlace < Datalen - 4; DataPlace++){中的DataPlace++
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].SliceLen = SliceLen;
				SliceInfoData->SliceNu++;
				break;
			case nal_unit_type_sps:
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].Start = &FrameData[DataPlace];
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].nut = nal_unit_type_sps;
				SliceInfoData->nut |= nal_unit_type_sps;
				for (SliceLen = 0;!FindStartPlace(&FrameData[DataPlace]);DataPlace++, SliceLen++);
				DataPlace--;//回退一个字节，留给for(DataPlace = 0; DataPlace < Datalen - 4; DataPlace++){中的DataPlace++
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].SliceLen = SliceLen;

				SliceInfoData->pic_high = 576;
				SliceInfoData->pic_width = 720;

				SliceInfoData->SliceNu++;
				break;
			case nal_unit_type_pps:
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].Start = &FrameData[DataPlace];
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].nut = nal_unit_type_pps;
				SliceInfoData->nut |= nal_unit_type_pps;
				for (SliceLen = 0;!FindStartPlace(&FrameData[DataPlace]);DataPlace++, SliceLen++);
				DataPlace--;//回退一个字节，留给for(DataPlace = 0; DataPlace < Datalen - 4; DataPlace++){中的DataPlace++
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].SliceLen = SliceLen;
				SliceInfoData->SliceNu++;
				break;
			case nal_unit_type_delimiter:
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].Start = &FrameData[DataPlace];
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].nut = nal_unit_type_delimiter;
				SliceInfoData->nut |= nal_unit_type_delimiter;
				for (SliceLen = 0;!FindStartPlace(&FrameData[DataPlace]);DataPlace++, SliceLen++);
				DataPlace--;//回退一个字节，留给for(DataPlace = 0; DataPlace < Datalen - 4; DataPlace++){中的DataPlace++
				SliceInfoData->Sliceinfo[SliceInfoData->SliceNu].SliceLen = SliceLen;
				SliceInfoData->SliceNu++;
				break; 
			default:
				printf ("why default have no nal_unit_type???\n");
				break;
		}
	}

	
	return 0;
}

static char * Putbyte(char *output, uint8_t nVal)
{
	output[0] = nVal;
	return output + 1;
}
static char * Putbe16(char *output, uint16_t nVal)
{
	output[1] = nVal & 0xff;
	output[0] = nVal >> 8;
	return output + 2;
}
static char * Putbe24(char *output, uint32_t nVal)
{
	output[2] = nVal & 0xff;
	output[1] = nVal >> 8;
	output[0] = nVal >> 16;
	return output + 3;
}
static char * Putbe32(char *output, uint32_t nVal)
{
	output[3] = nVal & 0xff;
	output[2] = nVal >> 8;
	output[1] = nVal >> 16;
	output[0] = nVal >> 24;
	return output + 4;
}
static char * Putbe64(char *output, uint64_t nVal)
{
	output = Putbe32(output, nVal >> 32);
	output = Putbe32(output, (uint32_t)nVal);
	return output;
}
static char * PutAmfstring(char *c, const char *str)
{
	uint16_t len = strlen(str);
	c = Putbe16(c, len);
	memcpy(c, str, len);
	return c + len;
}
static char * PutAmfdouble(char *c, double d)
{
	*c++ = AMF_NUMBER; /* type: Number */
	{
		unsigned char *ci, *co;
		ci = (unsigned char *)&d;
		co = (unsigned char *)c;
		co[0] = ci[7];
		co[1] = ci[6];
		co[2] = ci[5];
		co[3] = ci[4];
		co[4] = ci[3];
		co[5] = ci[2];
		co[6] = ci[1];
		co[7] = ci[0];
	}

	return c + 8;
}

int SendVideoSpsPps(RTMP*rtmpcli,unsigned char *pps,int pps_len,unsigned char * sps,int sps_len,unsigned int nTimeStamp)
{
	RTMPPacket * packet=NULL;//rtmp包结构
	unsigned char * body=NULL;
	int nRet = 0;
	int i;
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+1024);
	//RTMPPacket_Reset(packet);//重置packet状态
	memset(packet,0,RTMP_HEAD_SIZE+1024);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;
	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	//printf("sps[1] = [%d] sps[2] = [%d] sps[3] = [%d]\n",sps[1],sps[2],sps[3]);

	/*sps*/
	body[i++]   = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i],sps,sps_len);
	i +=  sps_len;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	memcpy(&body[i],pps,pps_len);
	i +=  pps_len;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = nTimeStamp;//nTimeStamp;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;//streamid相同
	packet->m_nInfoField2 = rtmpcli->m_stream_id;
	
	//packet->m_nInfoField2
	if (RTMP_IsConnected(rtmpcli))
	{
		/*调用发送接口*/
		nRet = RTMP_SendPacket(rtmpcli,packet,FALSE);
		//printf("RTMP_SEND=%d\n",nRet);
	}
	free(packet);    //释放内存
	return nRet;
}

int SendRtmpPacket(RTMP*rtmpcli,unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp) 
{
	RTMPPacket* packet;
	int nRet =0;
	/*分配包内存和初始化,len为包体长度*/
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+size);
	memset(packet,0,RTMP_HEAD_SIZE);
	/*包体内存*/
	RTMPPacket_Reset(packet);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = size;
	memcpy(packet->m_body,data,size);
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = nPacketType; /*此处为类型有两种一种是音频,一种是视频*/
	packet->m_nInfoField2 = rtmpcli->m_stream_id;	//jskk
	packet->m_nChannel = 0x04;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	if(RTMP_PACKET_TYPE_AUDIO ==nPacketType)
	{
		packet->m_nChannel = 0x05;
	}
	if (RTMP_PACKET_TYPE_AUDIO ==nPacketType && size !=4)
	{
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	packet->m_nTimeStamp = nTimestamp;
	/*发送*/
	if (RTMP_IsConnected(rtmpcli))
	{
		nRet = RTMP_SendPacket(rtmpcli,packet,FALSE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}
	/*释放内存*/
	free(packet);
	return nRet;  
}

int RtmpPushVideoData(RTMP*rtmpcli,int frametype,unsigned char *data,unsigned int datasize,unsigned int nTimestamp)
{
	int ret=0;
	unsigned char *body = (unsigned char*)malloc(datasize+9+1);  
	memset(body,0,datasize+9+1);
	body[0]=0x27;
	if(frametype==API_FRAME_TYPE_I)
	{
		body[0]=0x17;  //帧类型区分
	}
	body[1] = 0x01;    //视屏标志
	body[2] = 0x00;  
	body[3] = 0x00;  
	body[4] = 0x00;  
	
	body[5] = (datasize >> 24) & 0xff;  
    body[6] = (datasize >> 16) & 0xff;  
    body[7] = (datasize >>  8) & 0xff;  
    body[8] = (datasize ) & 0xff;  
	memcpy(&body[9],data,datasize); 
	ret=SendRtmpPacket(rtmpcli,RTMP_PACKET_TYPE_VIDEO,body,9+datasize,nTimestamp);
	free(body);
	return ret;
}
int RtmpPushAudioData(RTMP*rtmpcli,int type ,const char *pszData, int iDataLen,unsigned int aTimeStamp)
{
	int ret = -1;

	if(type==0)
	{
		unsigned char preData[1]={0x72};
		unsigned char *body = (unsigned char*)malloc(iDataLen +sizeof(preData)+1);  
		memset(body,0,iDataLen +sizeof(preData));
		memcpy(body,preData,sizeof(preData));
		memcpy(body + sizeof(preData), pszData, iDataLen);
		ret = SendRtmpPacket(rtmpcli,RTMP_PACKET_TYPE_AUDIO, body,iDataLen + sizeof(preData), aTimeStamp);
		free(body);
	}
	else if(type==1)
	{
		unsigned char preData[2]={0xaf,0x01};//修?
		unsigned char *body = (unsigned char*)malloc(iDataLen +sizeof(preData)+1);  
		memset(body,0,iDataLen +sizeof(preData));
		memcpy(body,preData,sizeof(preData));
		memcpy(body + sizeof(preData), pszData, iDataLen);
		ret = SendRtmpPacket(rtmpcli,RTMP_PACKET_TYPE_AUDIO, body,iDataLen + sizeof(preData), aTimeStamp);
		free(body);
	}
	
	return ret;
}

static int PushVideoData(RTMP*rtmpcli,unsigned int type,const char*videodata,unsigned int datalen,unsigned int timestamp)
{
	int ret = 0;
	int i=0;
	H264FrameSliceInfo SliceInfo = {0};
	//unsigned int Vtimestamp=0;
	//Vtimestamp=GetVideoStamp(timestamp);

	if(type==API_FRAME_TYPE_I)
	{
		ret = GetH264FrameSliceInfo((unsigned char *)videodata, datalen, &SliceInfo);
		if(ret!=0)
		{
			return -1;
		}
		for(i = 0; i < SliceInfo.SliceNu; i++)
		{	
			//printf("--------sliceNu=%d-------------------\n",SliceInfo.SliceNu);
			if(SliceInfo.Sliceinfo[i].nut & nal_unit_type_sps)
			{
				//DF_DEBUG("rtmp send sps pps..");
				ret=SendVideoSpsPps(rtmpcli,SliceInfo.Sliceinfo[i + 1].Start,SliceInfo.Sliceinfo[i + 1].SliceLen,SliceInfo.Sliceinfo[i].Start,SliceInfo.Sliceinfo[i].SliceLen,timestamp);
				if(ret<=0)
				{
					printf("SendVideoSpsPps error\n");
					return -1;
				}
			}
			else if(SliceInfo.Sliceinfo[i].nut & nal_unit_type_idr)
			{
				//printf("timestamp=%llu\n",timestamp);
				//DF_DEBUG("rtmp send i..");
				ret = RtmpPushVideoData(rtmpcli,API_FRAME_TYPE_I,SliceInfo.Sliceinfo[i].Start,SliceInfo.Sliceinfo[i].SliceLen,timestamp);
				if(ret < 0)
				{
				    printf("PushMediaData error\n");
				}
			}	
		}
	}
	else if(type==API_FRAME_TYPE_P)
	{
		//DF_DEBUG("rtmp send p..");
		ret=RtmpPushVideoData(rtmpcli,API_FRAME_TYPE_P,(unsigned char *)&videodata[4],datalen-4,timestamp);			
	}
	return ret;
}

//type 0 为g711a ，1为aac
static int PushAudioData(RTMP*rtmpcli, int type, const char*audiodata,unsigned int datalen,unsigned int timestamp)
{
	unsigned int Atimestamp=0;
	int ret;
//	Atimestamp=GetAudioStamp(timestamp);
//  printf("   autimestamp=%d\n",Atimestamp);
	ret=RtmpPushAudioData(rtmpcli,type,audiodata,datalen,timestamp);
	return ret;
}

//type ==1 为aac type==0 wei G711A
int SendAacSpac(RTMP*rtmpcli,int type,unsigned int nTimeStamp)
{
	// af 00 11 90   ;;aac    48000 16bits 
	// af 00 12 10   ;;aac    44100 16bits
	// af 00 13 90	 ;;aac    22050 16bits
	// 72 00 04 88   ;;g711-a 8000 16bits

	int ret=0; 
	unsigned char PreData[4];
	if(type==0)
	{
		//unsigned char PreData[4];
		PreData[0] = 0x72;  //G711A  //8000
		PreData[1] = 0x00;
		PreData[2] = 0x04;
		PreData[3] = 0x88;
	}
	if(type==1)
	{
		//unsigned char PreData[7];
		PreData[0] = 0xaf;
		PreData[1] = 0x00;
		PreData[2] = 0x15;
		PreData[3] = 0x90;
		//PreData[3] = 0x88;
		//PreData[3] = 0x56;PreData[3] = 0xe5;PreData[3] = 0x90;	
	}
	ret=SendRtmpPacket(rtmpcli,RTMP_PACKET_TYPE_AUDIO,PreData,sizeof(PreData),nTimeStamp);
	return ret;
}

int SendMetaPacket(RTMP*rtmpcli,int PicWidth, int PicHight,int FrameRate)
{
	char body[1024] = { 0 };
	char * p = (char *)body;
	int len;
	p = Putbyte(p, AMF_STRING);
	p = PutAmfstring(p, "@setDataFrame");
	p = Putbyte(p, AMF_STRING);
	p = PutAmfstring(p, "onMetaData");
	p = Putbyte(p, AMF_OBJECT);

	p = PutAmfstring(p, "duration");
	p = PutAmfdouble(p, 0);

	p = PutAmfstring(p, "fileSize");
	p = PutAmfdouble(p, 0);

	p = PutAmfstring(p, "copyright");
	p = Putbyte(p, AMF_STRING);
	p = PutAmfstring(p, "firehood");

#define FLV_CODECID_H264 7
#if 0
	p = put_amf_string(p, "videocodecid");
	p = put_amf_double(p, FLV_CODECID_H264);
#else
	p = PutAmfstring(p, "videocodecid");
	p = Putbyte(p, AMF_STRING);
	p = PutAmfstring(p, "avc1");
#endif
	p = PutAmfstring(p, "width");
	p = PutAmfdouble(p, PicWidth);
	p = PutAmfstring(p, "height");
	p = PutAmfdouble(p, PicHight);
	p = PutAmfstring(p, "framerate");
	p = PutAmfdouble(p, FrameRate);
#if 1
	p = PutAmfstring(p, "encoder");
	p = Putbyte(p, AMF_STRING);
	p = PutAmfstring(p, "avWare");

	p = PutAmfstring(p, "audiocodecid");
	p = Putbyte(p, AMF_STRING);
#if (defined PUSHAAC)	
	p = PutAmfstring(p, "mp4a");

	p = PutAmfstring(p, "audiodatarate");
	p = PutAmfdouble(p, 32);

	p = PutAmfstring(p, "audiosamplerate");
	p = PutAmfdouble(p, 44100);

	p = PutAmfstring(p, "audiosamplesize");
	p = PutAmfdouble(p, 16);
	p = PutAmfstring(p, "audiochannels");
	p = PutAmfdouble(p, 2);
#else
    p = PutAmfstring(p, "g711a");

	p = PutAmfstring(p, "audiodatarate");
	p = PutAmfdouble(p,64);

	p = PutAmfstring(p, "audiosamplerate");
	p = PutAmfdouble(p, 8000);

	p = PutAmfstring(p, "audiosamplesize");
	p = PutAmfdouble(p, 16);
	p = PutAmfstring(p, "audiochannels");
	p = PutAmfdouble(p, 1);
#endif
#endif
	p = PutAmfstring(p, "");
	p = Putbyte(p, AMF_OBJECT_END);
	len = p - body;
	return SendRtmpPacket(rtmpcli,RTMP_PACKET_TYPE_INFO, (unsigned char*)body, len, 0);
}
RTMP*RtmpConnect(char*url)
{
	RTMP*m_pRtmp=NULL;
	int i = 0;
	m_pRtmp = RTMP_Alloc();
	RTMP_Init(m_pRtmp);
	m_pRtmp->Link.timeout = 10;
	/*设置URL*/
	if(RTMP_SetupURL(m_pRtmp,(char *)url) == FALSE)
	{
		RTMP_Free(m_pRtmp);
		m_pRtmp = NULL;
		return NULL;
	}
	RTMP_SetBufferMS(m_pRtmp,5*1000);
	snprintf(m_pRtmp->m_szRawURL, sizeof(m_pRtmp->m_szRawURL), "%s", url);
	/*设置可写，这个函数必须在连接钱使用 否则无效*/
	RTMP_EnableWrite(m_pRtmp);
	/*链接服务器*/
	if(RTMP_Connect(m_pRtmp, NULL) == FALSE)
	{
		RTMP_Free(m_pRtmp);
		m_pRtmp = NULL;
		return NULL;
	}
	/*链接流*/
	if(RTMP_ConnectStream(m_pRtmp, 0) == FALSE )
	{
		RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		m_pRtmp= NULL;
		return NULL;
	}
	RTMP_SendChunkSize(m_pRtmp,1400);
	return m_pRtmp;
}
int RtmpDisConnect(RTMP *m_Rtmp)
{
	int i = 0;
	if(NULL != m_Rtmp)
	{
		m_Rtmp->RtmpSendStreamFlag	= -1;
		RTMP_DeleteStream(m_Rtmp);
		RTMP_Close(m_Rtmp);
		RTMP_Free(m_Rtmp);
		return 0;
	}
	return 0;
}

int GetRtmpUserParam(int chn, RtmpUserInfo_T *userinfo)
{
	int i=0;
	RtmpUSerMng_T *fd = &rtmpMng;
	
	if(chn < 0|| NULL == userinfo)
	{
		return -1;
	}
	
	for(i = 0;i < RTMP_USR_NUM; i++)
	{
		if(fd->rtmpUser[i].channel == chn)
		{
			memcpy(userinfo, &fd->rtmpUser[i],sizeof(RtmpUserInfo_T));
			return 0;
		}
	}
	return -1;
}

int SetRtmpUserPram(int chn, RtmpUserInfo_T *userinfo)
{
	int i = 0;
	RtmpUSerMng_T *fd = &rtmpMng;
	
	if(chn < 0|| NULL == userinfo)
	{
		return -1;
	}
	
	for(i = 0; i < RTMP_USR_NUM; i++)
	{
		if(fd->rtmpUser[i].channel == chn)
		{
			DF_DEBUG("SerialNumber: %s %s", fd->rtmpUser[i].SerialNumber, userinfo->SerialNumber);
			if(!strcmp(fd->rtmpUser[i].SerialNumber, userinfo->SerialNumber))
			{
				DF_DEBUG("----------------------------i: %d", i);
				memcpy(&fd->rtmpUser[i], userinfo, sizeof(RtmpUserInfo_T));
				DF_DEBUG("----------------------------%d-%d", fd->rtmpUser[i].enable, fd->rtmpUser[i].channel);
				return 0;
			}
		}
	}
	return -1;
}

int RtmpSetSerconfig(RtmpConfParm_T *rtmp)
{
	int i = 0;
	RtmpUSerMng_T *fd = &rtmpMng;
	
	if(NULL == rtmp){
		return KEY_FALSE;	
	}
	
	RtmpParamSetToConfig(rtmp);
	for(i = 0; i < RTMP_USR_NUM; i++)
	{
		fd->rtmpUser[i].stat = RTMP_NEEDRECONNECT;
	}
	return 0;
}

int RtmpGetSerconfig(RtmpConfParm_T *rtmp)
{
	if(NULL == rtmp){
		return KEY_FALSE;	
	}
	
	RtmpParamGetFromConfig(rtmp);
	return 0;
}

static void* RtmpProc(void *arg)
{
	int   ret = 0, dataLen = 0, i = 0;
	int   channel = (int)arg;
	char  rtmpUrl[512]={0};
	char  data[350*1024] = {0};
	RTMP *rtmpCliInfo=NULL;
	Timeval_T tv = {0};
	MeidaVideo_T *pvideo = NULL;
	AdpAPIHandle_T adphandle={0};
	RtmpMediaStamp_T mediaStamp={0};
    MediaQueueRegistHandle_T mediaHandle = {0};
	RtmpUserInfo_T userinfo = {0};
	RtmpUSerMng_T *fd = &rtmpMng;

	for(i = 0; i < RTMP_USR_NUM; i++){
		if(fd->rtmpUser[i].channel == channel){
			break;
		}
	}
	
	while(1 == fd->rtmpUser[i].enable)
	{
		if(0 == fd->rtmpUser[i].enable)
		{
			usleep(1000*1000);
			continue;
		}
		RtmpConfParm_T confParam = {0};
		RtmpGetSerconfig(&confParam);	

		char serial[64] = {0};
		char number[64] = {0};
		char *delim = "/";
		char *token = NULL;
		memcpy(serial, fd->rtmpUser[i].SerialNumber, 64);
		strtok(serial, delim);
		while((token = strtok(NULL, delim)))
		{
			memcpy(number, token, 64);
		}
		//sprintf(rtmpUrl, "rtmp://%s:%d%stest_%d", confParam.servaddr, confParam.servport, confParam.streamurl, channel);
		sprintf(rtmpUrl, "rtmp://%s:%d%s%s%d", confParam.servaddr, confParam.servport, confParam.streamurl, number, channel);
		DF_DEBUG("url===%s",rtmpUrl);
		rtmpCliInfo = RtmpConnect(rtmpUrl);
		if(rtmpCliInfo == NULL)
		{
			usleep(1000*1000);
			continue;
		}

		//ret = SendMetaPacket(rtmpCliInfo, 704, 576, 25);
		ret = SendMetaPacket(rtmpCliInfo, 1280, 720, 20);
		if(ret <= 0)
		{
			RtmpDisConnect(rtmpCliInfo);
			usleep(1000*1000);
			continue;
		}
        mediaHandle.queueID = fd->rtmpUser[i].queueid;
		DF_DEBUG("queueID: %d", mediaHandle.queueID);
		ret = MediaQueueRegistReadHandle(&mediaHandle);
        if(ret < 0)
        {
            DF_DEBUG("MediaQueueRegistReadHandle error channel: %d !", channel);
            return NULL;
        }
		
        mediaHandle.registerID = ret;
		fd->rtmpUser[i].stat = RTMP_CONNECTED;
		memset(&mediaStamp, 0, sizeof(RtmpMediaStamp_T));
		while(RTMP_CONNECTED == fd->rtmpUser[i].stat && fd->rtmpUser[i].enable == 1)
		{
            if(mediaHandle.pReadData != NULL)
            {
                memset(data, 0, 350*1024);
                ret = mediaHandle.pReadData(mediaHandle.registerID, mediaHandle.queueID, data, 350*1024, &dataLen);
                if(ret != KEY_TRUE)
                {
                    usleep(50*1000);
                    continue;
                }
				GetTimeOfDay(&tv);
                pvideo = (MeidaVideo_T *)data;
                if(pvideo->head.mediaType == MediaType_H264)
                {
					mediaStamp.VideoStamp =(tv.sec*1000+tv.usec/1000);
					//static FILE *fp = NULL;
					//if(fp == NULL)
					//{
					//	fp = fopen("./zz.h264", "wb+");
					//}
					//fwrite((unsigned char*)data + sizeof(MeidaVideo_T), pvideo->datalen, 1, fp);
					ret = PushVideoData(rtmpCliInfo, pvideo->info.frametype,(unsigned char*)data + sizeof(MeidaVideo_T), pvideo->datalen,(unsigned int)mediaStamp.VideoStamp);
					if(ret < 0)
					{
						DF_DEBUG("PushVideoData error.");
						fd->rtmpUser[i].stat = RTMP_DISCONNECT;
						RtmpDisConnect(rtmpCliInfo);
						usleep(1000*1000);
						continue;
					}
                }
                else if(pvideo->head.mediaType ==MediaType_G711A)
                {
					//DF_DEBUG("-------------33");
			    }
			}
		}
	}
    printf("rtmp  will  exit %d\n", channel);
	fd->rtmpUser[i].channel = -1;
	fd->rtmpUser[i].queueid = -1;
	fd->rtmpUser[i].stat = RTMP_DISCONNECT;
	memset(fd->rtmpUser[i].SerialNumber, 0, 64);
    MediaQueueUnRegistReadHandle(&mediaHandle);
	return NULL;
}

static int RtmpParamInit()
{
	pthread_t pthread;
	RtmpUSerMng_T *fd = &rtmpMng;
	int iret = 0, i = 0, j = 0;
	RtmpConfig_T  Rtmpopt = {0};
	HKDVRParam_T *dvrParam = NULL;
	dvrParam = (HKDVRParam_T *)malloc(sizeof(HKDVRParam_T) * 64);

	iret = GetHKSdkHandelRegistHandle()->HKsdkListhandle.pHksdkDeviceListGet(dvrParam);
	if(iret < 0)
	{
		DF_DEBUG("HksdkDeviceListGet error.");
		free(dvrParam);
		return KEY_FALSE;
	}
	
	for(i = 0; i < RTMP_USR_NUM; i++)
	{
		fd->rtmpUser[i].channel = -1;
		fd->rtmpUser[i].queueid = -1;
		fd->rtmpUser[i].enable  = 0;
		fd->rtmpUser[i].stat = RTMP_DISCONNECT;
		memset(fd->rtmpUser[i].SerialNumber, 0, 64);
	}

	fd->rtmpEnable = 1;
	for(i = 0; i < NET_NVR_MAX_NUM; i++)
	{
		if(strlen(dvrParam[i].HkDvrInfo.sDvrSerialNumber) > 0)
		{
			for(j = 0; j < NET_IPC_MAX_NUM; j++)
			{
				if(dvrParam[i].HkSdkParam[j].sQueueID > 0 && fd->rtmpUser[j].channel == -1)
				{
					fd->rtmpUser[j].channel = dvrParam[i].HkSdkParam[j].sChannel;
					fd->rtmpUser[j].queueid = dvrParam[i].HkSdkParam[j].sQueueID;
					fd->rtmpUser[j].stat   = RTMP_DISCONNECT;
					fd->rtmpUser[j].enable = 1;
					memcpy(fd->rtmpUser[j].SerialNumber, dvrParam[i].HkDvrInfo.sDvrSerialNumber, NET_DVR_MAX_LEN);
			

					//创建线程
					iret = pthread_create(&pthread, NULL, (void *)RtmpProc, fd->rtmpUser[j].channel);
					if(iret < 0)
					{
						DF_ERROR("pthread_create error.");
						free(dvrParam);
						return KEY_FALSE;
					}
				}
			}
		}
	}
	free(dvrParam);
	return KEY_TRUE;
}

static int addCameraCallBack(DvrOpt_T *param)
{
	//创建线程
	int i = 0, iret = 0;
	pthread_t pthread;
	RtmpUSerMng_T *fd = &rtmpMng;
	
	if(NULL == param){
		return KEY_FALSE;
	}

	for(i = 0; i < RTMP_USR_NUM; i++)
	{
		if(fd->rtmpUser[i].channel == -1 && strlen(fd->rtmpUser[i].SerialNumber) == 0)
		{
			fd->rtmpUser[i].channel = param->wChannel;
			fd->rtmpUser[i].queueid = param->wQueueID;
			fd->rtmpUser[i].stat   = RTMP_DISCONNECT;
			fd->rtmpUser[i].enable = 1;
			memset(fd->rtmpUser[i].SerialNumber, 0, NET_DVR_MAX_LEN);
			memcpy(fd->rtmpUser[i].SerialNumber, param->sDvrSerialNumber, NET_DVR_MAX_LEN);
			
			//创建线程
			iret = pthread_create(&pthread, NULL, (void *)RtmpProc, fd->rtmpUser[i].channel);
			if(iret < 0)
			{
				DF_ERROR("pthread_create error.");
				return KEY_FALSE;
			}
			return KEY_TRUE;
		}
	}
	return KEY_TRUE;
}

static int delCameraCallBack(DvrOpt_T *param)
{
	if(NULL == param){
		return KEY_FALSE;
	}

	RtmpUserInfo_T userinfo = {0};
	userinfo.channel = param->wChannel;
	userinfo.enable = 0;
	memcpy(userinfo.SerialNumber, param->sDvrSerialNumber, NET_DVR_MAX_LEN);
	SetRtmpUserPram(param->wChannel, &userinfo);
	return KEY_TRUE;
}

int RtmpInit()
{	
	int iret = -1;

	HKSdkHandel_Regist_T *hksdkHandle = GetHKSdkHandelRegistHandle();
	hksdkHandle->HKsdkListhandle.addCameraCallBack = addCameraCallBack;
	hksdkHandle->HKsdkListhandle.delCameraCallBack = delCameraCallBack;
	HksdkManageRegistHandle(&hksdkHandle->HKsdkListhandle);

	iret = RtmpParamInit();
	if(iret < 0){
		return KEY_FALSE;
	}
	return KEY_TRUE;
}

int RtmpDeInit()
{
	int i = 0;
	RtmpUSerMng_T *fd = &rtmpMng;
	
	for(i = 0; i < RTMP_USR_NUM; i++)
	{
		fd->rtmpUser[i].enable = 0;
	}
	
	return KEY_TRUE;
}
