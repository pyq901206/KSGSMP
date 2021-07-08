#ifndef __MEDIAQUEUEOUT_H__
#define __MEDIAQUEUEOUT_H__

typedef enum _VideoEncoding_E
{
	 VideoEncoding__NULL  = 0,
	 VideoEncoding__JPEG  = 1 << 0,
	 VideoEncoding__MPEG4 = 1 << 1, 
	 VideoEncoding__H264 =  1 << 2,
	 VideoEncoding__H265 =  1 << 3,
	 VideoEncoding__MJPEG = 1 << 4, 
}VideoEncoding_E;

typedef enum _AudioEncoding_E
{
	 AudioEncoding__NULL,
	 AudioEncoding__G711A = 1,//G711A
	 AudioEncoding__G726,
	 AudioEncoding__AAC,
	 AudioEncoding__G711U,
	 AudioEncoding__PCM,
	 AudioEncoding__Max,
}AudioEncoding_E; 

typedef enum _H264Profile_E
{
	 H264Profile__NULL 		=	 0,
	 H264Profile__Baseline 	=	1 << 0,
	 H264Profile__Main 		=	1 << 1, 
	 H264Profile__Extended	=	1 << 2,
	 H264Profile__High 		= 	1 << 3,
}H264Profile_E; 

typedef struct _VideoReslution_T
{
	int width;
	int height;
}VideoReslution_T;

typedef enum _Enum_FrameType
{
	API_FRAME_TYPE_P = 0x61,
	API_FRAME_TYPE_I = 0x65,
	API_FRAME_TYPE_SPS = 0x67,
	API_FRAME_TYPE_PPS = 0x68,
}Enum_FrameType;

typedef struct _VideoEncode_T
{
	int					frameRate;
	int					bitrate;
	int					qulity;
	int					gop;
	int					cvbrMode;//0 CBR
	VideoEncoding_E		encoding;
	H264Profile_E		h264Profile;
	VideoReslution_T	reslution;		
}VideoEncode_T;

typedef enum _MediaClarity_E{
	ENUM_MEDIA_CLEAR = 0x00,  //高清
	ENUM_MEDIA_STANDARD,      //标清
	ENUM_MEDIA_FLUENT,        //流畅
	ENUM_MEDIA_BUTT=0xFFFF,   //非法状态
}MediaClarity_E;

typedef struct _MediaAudioInfo_S{
	int samplerate;     //采样率率
	int prenum;         //采样点
	int bitwidth;       //位宽
	int soundmode;      //声道总数
}MediaAudioInfo_S;

typedef enum _Enum_MediaType
{
	MediaType_H264 = 0x01,
	MediaType_H265,
	MediaType_JPEG = 0x11,
	MediaType_MPEG4,
	MediaType_MJPEG,
	MediaType_G711A = 0x21,
	MediaType_G711U,
	MediaType_G726,
	MediaType_AAC,
	MediaType_PCM,
	MediaType_MSG,
}Enum_MediaType;

typedef struct _AudioFramedataInfo_T
{
	int framesize;
	char* frame;
}AudioFramedataInfo_T;

typedef struct _MediaVideoInfo_S{
	Enum_FrameType   frametype;
	int   chn;
	VideoReslution_T framesize;
	int bitrate;
	int rcmode;//编码器编码选择0 CBR  1 VBR
}MediaVideoInfo_S;

typedef struct _MediaFrame_T
{
	int fps;                    //帧率
	unsigned int subChn;        //子流编号,用途 音频声道标识,视频子流标识  
	Enum_MediaType   mediaType; //编码类型
	int datalen;                //片长度
	unsigned long long pts;
	unsigned char *dataaddr;    //数据
	union
    {
    	MediaVideoInfo_S stvideoinfo;
    	MediaAudioInfo_S staudioinfo;
    };
}MediaFrame_T;

typedef struct  _SnapImageParam_T
{
	VideoReslution_T res;
	int quility;
}SnapImageParam_T;

typedef struct  _SnapImage_T{
	SnapImageParam_T info;
	unsigned long long pts;
	int datalen;        //片长度
	char *dataaddr;     //数据
}SnapImage_T;

//音频参数
typedef struct _AUidoInfo_T
{
	int chn; 
	int fps;                //帧率
	int samplerate;         //采样率率
	int prenum;             //采样点
	int bitwidth;           //位宽
	int soundmode;          //声道总数
	unsigned long long pts; //时间戳
	AudioEncoding_E   audioencode;
	AudioFramedataInfo_T audioframeinfo;
}AudioInfo_T;

typedef struct _VideoSpsPpsInfo_T
{
	short spslen;
	short ppslen;
	char sps[64]; //base64加密
	char pps[64]; //base64加密
}VideoSpsPpsInfo_T;


typedef struct _AudioEncode_T
{
	AudioEncoding_E encoding;
	int bitrate;
	int sampleRate;
	ENUM_ENABLED_E 	enabled;
}AudioEncode_T;

#define D_SYNCDATA_HEAD0 0x77
#define D_SYNCDATA_HEAD1 0x17

typedef struct _MeidaHead_T{
	unsigned char sysncHead0;
	unsigned char sysncHead1;
	unsigned char streamType;   //码流类型
	unsigned char mediaType;    //视音频区分
}MeidaHead_T;

typedef struct _QueueVideoInputInfo_T{
	unsigned char fps;          //帧率
	unsigned char rcmode;       //编码器编码选择0 CBR  1 VBR
	unsigned char frametype;    //帧类型 I帧/P帧
	unsigned char staty0;       //编码类型
	VideoReslution_T reslution;
	int bitrate;
	unsigned int highPts;
	unsigned int lowPts;        //ms 级别
}QueueVideoInputInfo_T;

typedef struct _MeidaVideo_T{
	MeidaHead_T head;
	QueueVideoInputInfo_T info;
	int datalen;  //片长度
}MeidaVideo_T;

typedef struct _QueueAudioInputInfo_T{
	int samplerate;     //采样率率
	int prenum;         //采样点
	int bitwidth;       //位宽
	int soundmode;      //声道总数
	unsigned int highPts;
	unsigned int lowPts;//ms 级别
}QueueAudioInputInfo_T;

typedef struct _MeidaAudio_T{
	MeidaHead_T head;
	QueueAudioInputInfo_T info;
	int datalen; //片长度
}MeidaAudio_T;

#endif
