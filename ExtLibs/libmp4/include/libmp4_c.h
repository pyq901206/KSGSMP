#ifndef _LIBMP4_DEMUX_CEXPORT_H_
#define _LIBMP4_DEMUX_CEXPORT_H_


#ifdef __cplusplus
extern "C"{
#endif

enum eMpeg4FrameType
{
	eMpeg4_errFrame=0,
	eMpeg4_IFrame,
	eMpeg4_PFrame,
	eMpeg4_BFrame,
};

enum eMp4AudioType{
	MP4_AUDIO_NONE = -1,
	MP4_AUDIO_ULAW = 1,
	MP4_AUDIO_ALAW,
	MP4_AUDIO_AMR_NB,
	MP4_AUDIO_AAC,
	MP4_AUDIO_NU
};

enum eVideoType
{
	eVideoType_None = -1,
	eVideoType_Mpeg4=1,
	eVideoType_H264=2,
	eVideoType_Mjpg=3,
};

enum eMp4RtpType{
	RTP_TYPE_ULAW = 0,
	RTP_TYPE_ALAW = 8,
	RTP_TYPE_MJPG = 26,
	//96-127 dynamic
	RTP_TYPE_H264 = 96,
	RTP_TYPE_AMR = 97,
	RTP_TYEP_AAC = 98,
};

enum eMp4H264NaluType{
	NAL_TYPE_SLICE = 1,
	NAL_TYPE_IDR = 5,
	NAL_TYPE_SEI = 6,
	NAL_TYPE_SPS = 7,
	NAL_TYPE_PPS = 8,
	NAL_TYPE_SEQ_END = 9
};

enum ePlaySpeed
{
	ePlaySpeed_Min=0,
	ePlaySpeed_Pause=0,	//pause
	ePlaySpeed_32,		//快放32倍
	ePlaySpeed_16,
	ePlaySpeed_8,
	ePlaySpeed_4,
	ePlaySpeed_2,
	ePlaySpeed_Normal,	//正常速度
	ePlaySpeed_1_2,
	ePlaySpeed_1_4,
	ePlaySpeed_1_8,
	ePlaySpeed_1_16,
	ePlaySpeed_1_32,	//慢放32倍
	//ePlaySpeed_SingleFramePerSecond,//帧进,每秒播放一帧
	ePlaySpeed_Max,
};

enum eDemuxState
{
	eDemuxState_READ_FRAME,		//需要读新帧
	eDemuxState_WAIT_PLAYING,	//已读取一帧,但正在等待playing的时机
};

enum eExtData
{
	eExtData_sps,
	eExtData_pps,
	eExtData_aacdsi,
};

typedef struct tsMP4Info{
	int video_type;
	unsigned short width;
	unsigned short height;
	unsigned long video_timecsclae;
	unsigned long video_duration;
	unsigned long video_frame_count;
	double fps;
	int audio_type;
	unsigned char audio_bit_per_sample;
	unsigned char audio_channel;
	unsigned char profile; //only for aac
	unsigned char reserve;
	int audio_sample;
	unsigned long audio_duration;
	unsigned long audio_frame_count;
}MP4Info;

//mp4 jco extend box,支持修复异常中止的mp4文件.
#define MP4_FLAGS_JCOK			0x0001		//支持修复mp4文件
#define MP4_FLAGS_AUTO_FFLUSH	0x0002		//仅在MP4_FLAGS_JCOK为真时有效,每次fwrite后都马上fflush
//当采用MP4_FLAGS_JCOK标志时,可以设置MP4_FLAGS_AUTO_FFLUSH来叫CMP4Write每次写文件后都自动fflush,
//但这样效率可能比较低(磁盘IO相对缓存操作是很慢的操作)
//为提高效率,程序可以不设置MP4_FLAGS_AUTO_FFLUSH,而是定时(比如每隔5秒或者每写10帧视频)自行调用CMP4Write::fflush
//注意:MP4_FLAGS_JCOK为假时设置MP4_FLAGS_AUTO_FFLUSH无意义,CMP4Write会自动启用MP4_FLAGS_JCOK

typedef void*       MP4FileHandle;
//MP4 mux API
MP4FileHandle MP4Create(const char *pathName, int flag);

int MP4AddVideoTrack(
	MP4FileHandle file,
	int videoType,
	int width, int height,
	char *sps, int sps_len,
	char *pps, int pps_len
	);

int MP4AddAudioTrack(
	MP4FileHandle file,
	int audioType,
	int audio_sample,
	int bit_per_sample,
	int channel
	);

//写aac音频帧时必须去掉adts头 否则quicktime等播放器播放不出
int MP4WriteSample(
	MP4FileHandle file,
	int frameType, //0 audio ,1 video
	unsigned char* framebuf,
	const int framesize,
	double timeStamp
	);

//MP4 demux API
MP4FileHandle MP4Open(const char *pathName);
int MP4GetInfo(MP4FileHandle file, MP4Info *info);
int MP4GetH264SpsPps(MP4FileHandle file, unsigned char *sps, int *sps_len, unsigned char *pps, int *pps_len);

unsigned long MP4GetCurFrameIndex(MP4FileHandle file);
int MP4SeekByVideoIndex(MP4FileHandle file, int index);
int MP4SeekByTimeoffset(MP4FileHandle file, int timeoffset);

//TODO:读取到文件尾部的处理
int MP4GetAudioFrame(MP4FileHandle file, unsigned char *buf, int *bufsize, unsigned int *duration);
//only support H264 ,frame have nalu header 0001
int MP4GetVideoFrame(MP4FileHandle file, unsigned char *buf, int *bufsize, int need_key_frame, unsigned int *duration);

enum{
	MP4_Read = 0,
	MP4_Write = 1
};
void MP4Close(MP4FileHandle file, int flag);
#ifdef __cplusplus
}
#endif
#endif //_LIBMP4_DEMUX_CEXPORT_H_