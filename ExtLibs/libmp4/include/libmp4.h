#ifndef _LIBMP4_DEMUX_H_
#define _LIBMP4_DEMUX_H_

/*
libmp4����mp4¼�񼰻ط�
֧��video��ʽ:mpeg4/h.264/mjpg
֧��audio��ʽ:amr/ulaw
֧��windows+linuxƽ̨
Ӧ�ó���ֻ��Ҫʹ��CMP4Write��CMP4Read�����mux��demux����

¼��ʹ��CMP4Write
CMP4Write mp4Write;
mp4Write.Open(mp4OutputFileName);
	mp4OutputFileName��.mp4��β
mp4Write.InitVideo(type,szVideo,h264_sprop_paramenter_sets);
	�������InitVideo
	typeΪvideo RTP��payload type,mjpgʱΪ26,mpeg4��h.264ʱΪ96
	szVideo����Ƶ�ߴ�,ͨ��Ϊ320x240,720x576
	h264_sprop_paramenter_sets����h.264ʱ����Ҫ����

mp4Write.InitAudio(audioType);//��ѡ
    InitAudio�ǿ�ѡ��,���¼�񲻴�audio,����Ҫ����.
	audioType������RTP_TYPE_ULAW,RTP_TYPE_AMR֮һ
	audio����������8000hz

mp4Write.WriteFrame(bVideo,pFrame,cbFrame,dwRTPTimeStamp)
	���յ�һ��������audio/video frame�����WriteFrame
	dwRTPTimeStampΪrtp����timestamp

mp4Write.Close();
	¼����ɺ�������Close(),����¼��õ���.mp4�����޷���������

�ط�ʹ��CMP4Read
CMP4Read mp4Read;
mp4Read.Open("xxx.mp4");
���Ե���mp4Read.GetVideoInfo()��mp4Read.GetAudioInfo()�õ�video/audio��Ϣ
����ReadVideoFrame��ReadAudioFrame�õ���һ֡AV����
���Ե���mp4Read.Seek����λ,seek��duration����video trakΪ׼��
�ط���ɺ����Close()

//*/
#include "libmp4_c.h"

#ifndef DBG
#define DBG printf
#endif

#ifndef ERR
#define ERR printf
#endif

#ifdef _MSC_VER
#include<windows.h>
#include<atltypes.h>
#include<cassert>

#define snprintf	_snprintf
#define fstat		_fstat
#define stat		_stat

//ʹ��BOOL32,DWORD32��Ϊ�˼������е�linux����,...
#define BOOL32 BOOL
#define DWORD32 DWORD

#define DT	DebugTrace
#define DW	DebugTrace
#define DE	DebugTrace

#else
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>

//for linux platform
#define ASSERT(x)	do{if(x){}else{DE("###ASSERT Fail:%s (File:[%s:%d])\n",#x,__FILE__,__LINE__);}}while(0)

typedef const char * LPCTSTR;
typedef unsigned int BOOL32;
typedef unsigned int DWORD32,*LPDWORD;
typedef unsigned int UINT;
typedef unsigned short WORD,*LPWORD;
typedef unsigned char BYTE,*LPBYTE;
typedef unsigned long ULONG;
typedef void VOID,*LPVOID;
typedef long long LONGLONG;

#define _strdup strdup

#ifndef FALSE
enum eMp4BoolType{
	FALSE = 0,
	TRUE  = 1,
};
#endif

struct tagSize
{
	int cx;
	int cy;
};
class CSize:public tagSize
{
public:
	CSize()
	{
	}
	CSize(int x,int y)
	{
		cx=x;
		cy=y;
	}
	bool operator==(const CSize& sz)
	{
		return cx==sz.cx && cy==sz.cy;
	}
};

typedef int SOCKET;
#define INVALID_SOCKET	(-1)
#define _snprintf snprintf

#define DT	DebugTrace
#define DW	DebugTrace
#define DE	DebugTrace

void DebugTrace(LPCTSTR lpszFormat, ... );
#endif


#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#include <vector>
#include <string>
using namespace std;

#ifndef Boolean
#define Boolean BOOL32
#endif

#ifndef COUNT_OF
#define COUNT_OF(x)	(sizeof(x)/sizeof((x)[0]))
#endif

#define MP4_FILE_ERROR	(-5)

#ifndef _MSC_VER
	#define MP4_AFX_EXT_CLASS
#elif defined _MP4_REC_TEST_
	#define MP4_AFX_EXT_CLASS
#else
	//#define MP4_AFX_EXT_CLASS	AFX_EXT_CLASS	//����֧��windows dll,exe
        #define MP4_AFX_EXT_CLASS //����֧��windows dll,exe
	#define ASSERT assert
#endif

//�ļ���ȡ�ӿ�
class IReadFile
{
public:
	virtual ~IReadFile(){}
	virtual int		fseek(long offset,int origin)=0;
	virtual long	GetFileSize()=0;
	virtual long	ftell()=0;
	virtual size_t	fread( void *buffer, size_t size, size_t count)=0;
	virtual BOOL32	IsOpen()=0;
};

class CReadFile:public IReadFile
{
public:
	CReadFile() :m_hFile(NULL) { }
	virtual ~CReadFile()
	{
		Close();
	}

	BOOL32 IsOpen()
	{
		return m_hFile != NULL;
	}

	int fseek(long offset,int origin)
	{
		ASSERT(m_hFile);
		int ret = ::fseek(m_hFile,offset,origin);
		return ret;
	}

	long GetFileSize()
	{
		ASSERT(m_hFile);
		
		struct stat fs;
		int ret = fstat(fileno(m_hFile), &fs);
		if(ret!=0 || fs.st_size<=0)
		{
			return 0;
		}

		return fs.st_size;
	}

	long ftell()
	{
		ASSERT(m_hFile);
		long pos = ::ftell(m_hFile);
		return pos;
	}

	size_t fread( void *buffer, size_t size, size_t count)
	{
		ASSERT(m_hFile);
		size_t ret = ::fread(buffer,size,count,m_hFile);
		return ret;
	}

	void Close()
	{
		if(m_hFile)
		{
			fclose(m_hFile);
			m_hFile=NULL;
		}
	}

	static IReadFile * CreateInstance(const char *pszFile,const char *mode)
	{
		CReadFile *pFile = new CReadFile;
		pFile->m_hFile = fopen(pszFile,mode);
		if(!pFile->m_hFile)
		{
			delete pFile;
			pFile = NULL;
		}

		return pFile;
	}

protected:
	FILE *m_hFile;
};

//XiongWanPing 2009.11.05
class MP4_AFX_EXT_CLASS CMP4Base
{
public:
	static unsigned char* base64Decode(char* in, unsigned& resultSize,
			    Boolean trimTrailingZeros = TRUE);
    // returns a newly allocated array - of size "resultSize" - that
    // the caller is responsible for delete[]ing.

	static char* base64Encode(char const* orig, unsigned origLength);
    // returns a 0-terminated string that
    // the caller is responsible for delete[]ing.

	CMP4Base();
	virtual ~CMP4Base();
	static int gettimeofday(struct timeval* tp);
	static const char *GetMpeg4FrameTypeDesc(LPBYTE pData);
	static eMpeg4FrameType GetMpeg4FrameType(LPBYTE pData);
	static BOOL32 IsMpeg4IFrame(LPBYTE pData)
	{
		return GetMpeg4FrameType(pData)==eMpeg4_IFrame;
	}
	
	//pDataָ��һ��������h.264֡
	static BOOL32 IsH264IFrame(LPBYTE pData)
	{
		BYTE nHeadType=(pData[0]&0x1F);
		if(pData[0] == 0x00 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x01)
		{
			nHeadType =(pData[4] & 0x1F);
		}
		
		if(nHeadType == NAL_TYPE_IDR || nHeadType == NAL_TYPE_SPS || nHeadType == NAL_TYPE_PPS)
			return TRUE;
        else if(pData[0] == 0x00 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x01 && (pData[5] == 0x88))
            return TRUE;
		return FALSE;
	}

protected:
	unsigned addByte(unsigned char byte);
	long addAtomHeader(char const* atomName);
	unsigned addWord(unsigned word);
	unsigned addHalfWord(unsigned short halfWord);
	unsigned addZeroWords(unsigned numWords) ;
	unsigned add4ByteString(char const* str);
	unsigned addArbitraryString(char const* str,Boolean oneByteLength);
	void setWord(unsigned filePosn, unsigned size);
	void ModifyByte(unsigned filePosn,BYTE newValue);

	FILE	*m_hFile;
};


struct tagRTSPInterleavedFrameHeader
{
#ifdef _DEBUG
	tagRTSPInterleavedFrameHeader()
	{
		//make sure it's 4 bytes
		ASSERT(sizeof(*this)==4);
	}
#endif

	BYTE	Magic;
	BYTE	Channel;

	//raw length is net order,so protect it by Length()
	WORD	Length()
	{
		return ntohs(length);
	}
protected:
	WORD	length;
};

struct tagRTPFrameHeader
{
#ifdef _DEBUG
	tagRTPFrameHeader()
	{
		ASSERT(sizeof(*this)==12);
	}
#endif

	//ֻ��Ҫ֧��С��
    BYTE		cc				:4;				/* CSRC count */
    BYTE		x				:1;				/* header extension flag */
    BYTE		padding			:1;				/* padding flag */
    BYTE		version			:2;				/* protocol version */
	
    /* byte 1 */		
    BYTE		payload_type    :7;				/* payload type */
    BYTE		marker			:1;				/* marker bit */

	DWORD32 timestamp()
	{
		return ntohl(Timestamp);
	}

	DWORD32 sync_sid()
	{
		return ntohl(Sync_sid);
	}

protected:

	WORD	sequence_number;
	DWORD32	Timestamp;
	DWORD32 	Sync_sid;//sync source id
};

struct tagMP4Node_stts
{
	DWORD32 SampleCount;
	DWORD32 SampleDuration;
	tagMP4Node_stts()
	{
		memset(this,0,sizeof(*this));
	}
};

struct tagMP4Node_stsc
{
	DWORD32 FirstChunk;
	DWORD32 SamplesPerChunk;
	DWORD32 SampleDescriptionID;
	tagMP4Node_stsc()
	{
		memset(this,0,sizeof(*this));
	}
};


class CMP4Write;
//XiongWanPing 2009.11.04
class CMP4Trak:public CMP4Base
{
	friend class CMP4Write;
	friend class CMP4Repair;
public:
	CMP4Trak();
	virtual ~CMP4Trak();

	BOOL32 IsAudioTrak()
	{
		return strncmp(m_handlerType,"soun",4)==0;
	}

	BOOL32 IsVideoTrak()
	{
		return strncmp(m_handlerType,"vide",4)==0;
	}

	BOOL32 IsIFrame(LPBYTE pData);
	DWORD32 GetFrameCount()
	{
		//20160609 add aac
		if (IsAudioTrak() && (m_type != MP4_AUDIO_AAC)) {
			return m_stsz_count;
		}
		return m_lst_stsz.size();
	}

	BOOL32 IsH264()
	{
		return m_fmtp_spropparametersets!=NULL;
	}
	BOOL32 IsMjpg()
	{
		return m_trakid == 26;//26�Ǳ�׼MJPG rtp payload type
	}
	DWORD32 GetTimeScale()
	{
		return m_timescale;
	}

protected:
	int  PreClose();
	void OnNewDuration(DWORD32 dwDurationLastFrame);

	long addAtom_trak();
	long addAtom_tkhd();
	long addAtom_mdia();
	long addAtom_mdhd();
	long addAtom_hdlr();
	long addAtom_minf();
	long addAtom_vmhd();
	long addAtom_mp4v();
	long addAtom_esds_mp4v();
	long addAtom_smhd();
	long addAtom_dinf();
	long addAtom_dref();
	long addAtom_stts();
	long addAtom_stss();
	long addAtom_stsc();
	long addAtom_stsz();
	long addAtom_stco();
	long addAtom_stsd();
	long addAtom_samr();
	long addAtom_ulaw();
	long addAtom_alaw();
	long addAtom_stbl();
	long addAtom_avc1();
	long addAtom_avcC();
	long addAtom_mjpa();


	int WriteVideoFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteAudioFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteAudioFrameEx(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteAudioFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp,BOOL32 bRTPHeader);


	int OnAmrFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int OnAmrFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int OnUlawFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int OnUlawFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);

	//add 20160706
	long addAtom_mp4a();
	long put_esds_descr(int tag, unsigned int length);
	long addAtom_esds_aac();
	int OnAacFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int OnAacFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int GetAacDecSpecificInfo(unsigned char *dsi);
	
	tagMP4Node_stsc* GetLastStscNode()
	{
		if(m_lst_stsc.size()>0)
		{
			return m_lst_stsc[m_lst_stsc.size()-1];
		}
		return NULL;
	}

	tagMP4Node_stts* GetLastSttsNode()
	{
		if(m_lst_stts.size()>0)
		{
			return m_lst_stts[m_lst_stts.size()-1];
		}
		return NULL;
	}

	int InitAudioTrakInfo(DWORD32 type,const char *handlerType,DWORD32 sample=8000, int bit_per_sample = 16, int channel = 1, int bitrate = 128 *1024);
	int InitVideoTrakInfo(DWORD32 type,const char *handlerType,CSize szVideo,const char *sprop_parameter_sets=NULL,DWORD32 timescale=90000);

	void Empty();
	vector<tagMP4Node_stts*>	m_lst_stts;
	vector<tagMP4Node_stsc*>	m_lst_stsc;
	vector<DWORD32>		m_lst_stco;
	vector<DWORD32>		m_lst_stss;//ֻ��videoʱ����,audio����Ҫstss
	vector<DWORD32>		m_lst_stsz;//audio֡�ߴ�һ���ǹ̶���,����m_lst_stszAudioֻ��һ��item
	DWORD32				m_stsz_count;//����audioʱ�õ�
	
	DWORD32				m_trakid;
	unsigned			m_timescale;//for audio sample == timesacle
	unsigned			m_duration;

	CSize				m_szVideo;//ֻ��video track��Ч

	char				m_handlerType[4];
	CMP4Write			*m_pMP4Write;

	DWORD32				m_dwFrameIndex;
	DWORD32				m_nFirstChunk;

	DWORD32				m_dwTickRTPLast;//������һ֡��RTP timestamp,�Ѿ���host order
	DWORD32				m_type;//trak type

	char*				m_fmtp_spropparametersets;//only for h.264 trak
	BOOL32				m_bH264IFrame;
	static WORD			m_amr_packed_size[8];
	BYTE				m_amr_ft;//only for audio amr
	
	int					m_audio_channel;//only for audio
	int					m_audio_bit_per_sample;//only for audio
	int					m_audio_bitrate; //only for audio aac
};



//mp4 jco extend box,֧���޸��쳣��ֹ��mp4�ļ�.
#define MP4_FLAGS_JCOK			0x0001		//֧���޸�mp4�ļ�
#define MP4_FLAGS_AUTO_FFLUSH	0x0002		//����MP4_FLAGS_JCOKΪ��ʱ��Ч,ÿ��fwrite������fflush
//������MP4_FLAGS_JCOK��־ʱ,��������MP4_FLAGS_AUTO_FFLUSH����CMP4Writeÿ��д�ļ����Զ�fflush,
//������Ч�ʿ��ܱȽϵ�(����IO��Ի�������Ǻ����Ĳ���)
//Ϊ���Ч��,������Բ�����MP4_FLAGS_AUTO_FFLUSH,���Ƕ�ʱ(����ÿ��5�����ÿд10֡��Ƶ)���е���CMP4Write::fflush
//ע��:MP4_FLAGS_JCOKΪ��ʱ����MP4_FLAGS_AUTO_FFLUSH������,CMP4Write���Զ�����MP4_FLAGS_JCOK
#ifdef _MSC_VER
#pragma pack(push,1)
#endif

struct tagMP4SampleHeader
{
#ifdef _DEBUG
	tagMP4SampleHeader()
	{
		int cbSize = sizeof(tagMP4SampleHeader);
		ASSERT(cbSize == 8);
	}
#endif
	
	DWORD32	trakid:8;
	DWORD32	size:24;
	DWORD32 flags:8;	//bit0:keyframe
	DWORD32 duration:24;
};

#ifdef _MSC_VER
#pragma pack(pop)
#endif
//XiongWanPing 2010.01.04 end

class MP4_AFX_EXT_CLASS CMP4Write:public CMP4Base
{
	friend class CMP4Trak;
	friend class CMP4Repair;
public:
	int InitVideo(DWORD32 dwType,CSize szVideo,const char* sprop_parameter_sets=NULL);
	int InitAudio(DWORD32 dwType, int sample = 8000, int bit_per_sample = 16, int channel = 1);

	CMP4Write();
	virtual ~CMP4Write();

	int Attach(FILE *hFile);//Attach��Detach�����޸�mp4ʹ��
	void Detach();

	int Open(const char * pszFile,DWORD32 dwFlags=MP4_FLAGS_JCOK);//dwFlagsΪMP4_FLAGS_xxx
	int Close();
	//write audio amrʱ,���������:дrtp��װ�Ķ��amr,���߶�����amr��
	//rtp��װ����WriteFrame
	//������amr����WriteFrameEx
	//videoû�д�����.
	int WriteFrame(BOOL32 bVideo,LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteFrameEx(BOOL32 bVideo,LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);

	//					��ȡ��ǰ�ļ��ߴ�
	BOOL32 GetFileSize(LONGLONG* lpFileSize);
	DWORD32 GetRecordTick();

	int fflush();
	DWORD32 GetMP4Flags() const
	{
		return m_dwMP4Flags;
	}

protected:
	int WriteAudioFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp,BOOL32 bRTPHeader);
	int WriteVideoFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteAudioFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteAudioFrameEx(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);

	int WriteHeader();
	BOOL32 m_bHasWriteFtyp;

	CMP4Trak * FindVideoTrak();
	CMP4Trak * FindAudioTrak();

	void Empty();
	long addAtom_ftyp();
	long addAtom_JCOK();
	long addAtom_moov();
	long addAtom_mvhd();

	long				m_pos_mdat;
	long				m_pos_jcok;
	long				m_pos_moov;

	vector<CMP4Trak*>	m_arrTrak;
	CMP4Trak			*m_pTrakLastWrite;//��һ��writefrmae��trak
	DWORD32				m_dwTickStartRecord;

#define MP4F_G711	0x0001	//ͨ��ʹ�õ�g.711��ulaw��ʽ,�ֳ�pcmu,��audioΪulawʱ,Ҫ��һЩ���⴦��
#define MP4F_ATTACH	0x0002	//�޸�mp4
	DWORD32				m_dwFlags;
	DWORD32				m_dwMP4Flags;

	int Repair();
	BOOL32 IsRepairMode()
	{
		return m_dwFlags & MP4F_ATTACH;
	}
};

struct tagMP4AudioInfo
{
	tagMP4AudioInfo()
	{
		memset(this,0,sizeof(*this));
	}
	
	BOOL32 bValid;
	int type;//g711 ,amr, aac
	DWORD32 duration;
	DWORD32 frame_count;
	int channel;
	int bit_per_sample;
	int sample;
	int profile; //for aac is profile ��for amr-nb is encoder mode eg. MR122 is 7.
};

struct tagMP4VideoInfo
{
	tagMP4VideoInfo()
	{
		memset(this,0,sizeof(*this));
	}

	BOOL32 bValid;
	int type;//mpeg4,h.264
	int width;
	int height;
	double fps;
	DWORD32 timescale;
	DWORD32 duration;
	DWORD32 frame_count;
	eVideoType video_type;
};


class MP4_AFX_EXT_CLASS CMP4Read_atom
{
	friend class CMP4Read;
public:
	CMP4Read_atom();
	virtual ~CMP4Read_atom();

	static CMP4Read_atom* CreateAtom(const char* type);
	void SetFile(IReadFile *hFile)
	{
		m_hFile=hFile;
	}
	void SetStart(DWORD32 start)
	{
		m_start=start;
	}
	void SetSize(DWORD32 size)
	{
		m_size=size;
	}
	void SetEnd(DWORD32 end)
	{
		m_end=end;
	}
	virtual int Read();//alter by lcs , add int  return.for avoid av_stream fault.
	void Dump();
	CMP4Read_atom * FindChildAtom(const char *type);
protected:
	CMP4Read_atom * FindChildAtomHelper(const char *type,int nIndex=0);

	DWORD32	m_start;
	DWORD32	m_end;
	DWORD32	m_size;
	char	m_type[5];
	IReadFile	*m_hFile;

	BOOL32	m_bHasChildAtom;
	vector<CMP4Read_atom*>	m_arrChildAtom;			//ʵ���϶�ȡ����child atom
	UINT	m_depth;
};

class CMP4Read_atom_tkhd:public CMP4Read_atom
{
public:
	int Read();
	int m_width;
	int m_height;
	DWORD32 m_duration;
};

class CMP4Read_atom_mdhd:public CMP4Read_atom
{
public:
	int Read();
	DWORD32 m_timescale;
	DWORD32 m_duration;
};

class CMP4Read_atom_hdlr:public CMP4Read_atom
{
public:
	int Read();

	BOOL32 IsVideo()
	{
		return strncmp(m_handlerType,"vide",4)==0;
	}
	char m_handlerType[5];
};

class CMP4Read_atom_stsd:public CMP4Read_atom
{
public:
	int Read();
};

class CMP4Read_atom_avc1:public CMP4Read_atom
{
public:
	int Read();
};

class CMP4Read_atom_avcC:public CMP4Read_atom
{
public:
	CMP4Read_atom_avcC()
	{
		m_pps = NULL;
		m_pps_count = NULL;
		m_sps = NULL;
		m_sps_count=NULL;
	}
	~CMP4Read_atom_avcC()
	{
		Empty();
	}
	void Empty();

	int Read();

	LPBYTE m_pps;
	int m_pps_count;
	LPBYTE m_sps;
	int m_sps_count;
};
class CMP4Read_atom_mp4v:public CMP4Read_atom
{
public:
	int Read();
	int m_width;
	int m_height;
};

//TODO:add read atom mp4a esds 20160712
class CMP4Read_atom_mp4a:public CMP4Read_atom
{
public:
	int Read();
};

class CMP4Read_atom_mp4a_aac_esds:public CMP4Read_atom
{
public:
	int Read();
	char channel;
	char profile;
	char bit_per_samle;
	int sample;
};

class CMP4Read_atom_stts:public CMP4Read_atom
{
public:
	CMP4Read_atom_stts()
	{
		m_stts=NULL;
		m_stts_count=0;
	}

	~CMP4Read_atom_stts()
	{
		delete []m_stts;
		m_stts=NULL;
		m_stts_count=0;
	}

	int Read();
	
	tagMP4Node_stts *m_stts;
	UINT m_stts_count;
};

class CMP4Read_atom_stsc:public CMP4Read_atom
{
public:
	CMP4Read_atom_stsc()
	{
		m_stsc=NULL;
		m_stsc_count=0;
	}

	~CMP4Read_atom_stsc()
	{
		delete []m_stsc;
		m_stsc=NULL;
		m_stsc_count=0;
	}

	int Read();
	
	tagMP4Node_stsc *m_stsc;
	UINT m_stsc_count;
};

class CMP4Read_atom_stsz:public CMP4Read_atom
{
public:
	CMP4Read_atom_stsz()
	{
		m_stsz=NULL;
		m_stsz_count=0;
		m_size=0;
	}

	~CMP4Read_atom_stsz()
	{
		delete []m_stsz;
		m_stsz=NULL;
		m_stsz_count=0;
	}

	int Read();
	
	DWORD32 *m_stsz;
	DWORD32 m_size;
	UINT m_stsz_count;
};

class CMP4Read_atom_stco:public CMP4Read_atom
{
public:
	CMP4Read_atom_stco()
	{
		m_stco=NULL;
		m_stco_count=0;
		m_size=0;
	}

	~CMP4Read_atom_stco()
	{
		delete []m_stco;
		m_stco=NULL;
		m_stco_count=0;
	}

	int Read();
	
	DWORD32 *m_stco;
	UINT m_stco_count;
};

class CMP4Read_atom_stss:public CMP4Read_atom
{
public:
	CMP4Read_atom_stss()
	{
		m_stss=NULL;
		m_stss_count=0;
	}

	~CMP4Read_atom_stss()
	{
		delete []m_stss;
		m_stss=NULL;
		m_stss_count=0;
	}

	int Read();
	
	DWORD32 *m_stss;
	UINT m_stss_count;
};

class MP4_AFX_EXT_CLASS CMP4Read_atom_trak:public CMP4Read_atom
{
public:
	CMP4Read_atom_trak()
	{
		m_bInit=FALSE;
		m_bVideoTrak=FALSE;
		m_bAmr=FALSE;
		m_bMjpg=FALSE;

		m_stts=NULL;
		m_stsc=NULL;
		m_stsz=NULL;
		m_stco=NULL;
		m_stss=NULL;

		m_dwFrameIndex = 0;
		m_curDuration=0;
	}

	BOOL32 IsIFrame(int nVideoFrameIndex);
	
	DWORD32 GetDurationSum(int nVideoFrame);
	DWORD32 GetCurDuration();
	int GetCurFrame();

	int Read();
	BOOL32 IsVideoTrak();
	BOOL32 IsUlaw()
	{
		return MP4_AUDIO_ULAW == m_audio_type ? TRUE : FALSE;
	}

	BOOL32 IsMjpgTrak()
	{
		return eVideoType_Mjpg == m_video_type ? TRUE : FALSE;
	}

	int ReadVideoFrame(LPBYTE pFrame,int cbFrame,int&cbRead,DWORD32& duration);
	int ReadAudioFrame(LPBYTE pFrame,int cbFrame,int&cbRead,DWORD32& duration);
	int Test();

	int GetFrameCount();

	int Seek(DWORD32 durationSeek);

protected:
	int GetVideoFrameCount()
	{
		ASSERT(IsVideoTrak());
		return m_stsz->m_stsz_count;
	}

	int GetAudioFrameCount();

	int		GetChunkIndex(int nSampleIndex,int& nChunkIndex,int& nChunkSampleIndex);
	DWORD32	GetSampleDuration(int nSampleIndex);

	BOOL32	m_bInit;
	BOOL32	m_bVideoTrak;//otherwise is audio trak(ֻ��m_bInitΪTRUEʱ��Ч)
	BOOL32	m_bAmr;//otherwise is ulaw(ֻ��m_bInitΪTRUE����m_bVideoTrakΪFALSEʱ��Ч)
	BOOL32	m_bMjpg;
	int m_audio_type;
	int m_video_type;

	CMP4Read_atom_stts *m_stts;
	CMP4Read_atom_stsc *m_stsc;
	CMP4Read_atom_stsz *m_stsz;
	CMP4Read_atom_stco *m_stco;
	CMP4Read_atom_stss *m_stss;

	DWORD32	m_dwFrameIndex;
	DWORD32	m_curDuration;//��ǰλ�õ��ܹ�duration,����demux
	int		m_nCurFrame;
private:
	enum{
		ADTS_HEADER_SIZE = 7
};
	void Mp4AacAddAdts(unsigned char* buf, int length);
	char m_aac_profile;
	char m_aac_sample_freq_index;
	char m_aac_channel;
};

class MP4_AFX_EXT_CLASS CMP4Read
{
public:
	int GetExtData(eExtData type,LPBYTE pBuf,int cbBuf,int& cbDataBytes);
	BOOL32 IsOpen()
	{
		return m_iFile!=NULL;
	}

	BOOL32 IsIFrame(int nVideoFrameIndex);

	IReadFile	*m_iFile;

	CMP4Read();
	virtual ~CMP4Read();
	int Open(const char * pszFile);
	int Attach(IReadFile *iReadFile);
	int Close();
	int GetAudioInfo(tagMP4AudioInfo& ai);
	int GetVideoInfo(tagMP4VideoInfo& vi);
	int ReadVideoFrame(LPBYTE pFrame,int cbFrame,int& cbRead,DWORD32& duration);
	int ReadAudioFrame(LPBYTE pFrame,int cbFrame,int& cbRead,DWORD32& duration);
	
	int GetCurVideoFrame();
	int Seek(DWORD32 durationVideoSeek);
	int SeekToFrame(int nVideoFrame);
	int SeekToIFrame();
	int SeekToPrevIFrame();
	DWORD32 GetCurVideoDuration();
	DWORD32 GetCurAudioDuration();
	int GetIndexSplNum(char trakType, int& currMp4SampleIndex, int& currMp4SampleSum);
	
protected:
	int OpenHelper();
	int GetTrakCount();
	CMP4Read_atom_trak *FindAudioTrak();
	CMP4Read_atom_trak *FindVideoTrak();

	int  CheckValid();
	int  ParseAtom();
	void DumpAtom();

//.int get_audio_info(int&type,DWORD32& dwMaxTimeStamp);
//.int get_video_info(int& width,int &height,int& type,int& fps,DWORD32& dwMaxTimeStamp);
//.int read_audio_frame(LPBYTE pFrame,int cbFrame,DWORD32& dwTimeStamp);
//.int read_video_frame(LPBYTE pFrame,int cbFrame,DWORD32& dwTimeStamp);
//.int seek(DWORD32 dwTimeStamp);//ͬʱseek audio&video����ӽ�dwTimeStamp��λ��,��video dwTimeStampΪ׼(audio�����video���л���)
protected:
	DWORD32	m_cbFile;//�ļ�����

	//CMP4Read_moov	m_moov;
	CMP4Read_atom	*m_pRootAtom;

	tagMP4VideoInfo m_vi;
	tagMP4AudioInfo m_ai;

	CMP4Read_atom_trak *m_pVideoTrak;
	CMP4Read_atom_trak *m_pAudioTrak;
};

//for parse starvalley rtsp  stream config
class MP4_AFX_EXT_CLASS CBitStream  
{
public:
	CBitStream();
	virtual ~CBitStream();

	BOOL32	Init(LPBYTE pBitStream,int cbBit,int nBitPos);
	BOOL32	Init(const char *pszHex,int cbLen=-1);
	void	UnInit();

	UINT	Read(int nBit);
	UINT	Peek(int nBit);
	BOOL32	SkipBit(int nBit);

	BOOL32	SetBit(UINT nBitPos,BOOL32 bSet=TRUE);
	BOOL32	WriteBit(UINT nBitPos,UINT nValue,int cbBit);

	UINT	GetBitPos()
	{
		return m_nBitPos;
	}

protected:
	LPBYTE	m_pBitStream;
	int		m_cbBitStream;//in bits
	int		m_nBitPos;	  //bit pos

	LPBYTE	m_byBuf;

};

//for parse mp4 decode config
class CMpeg4BitStream:public CBitStream
{
public:
	int Parse();
protected:
	int Parse_VisualObject();
	int Parse_VisualObjectSequence();
	int Parse_video_signal_type();
	int VideoObjectLayer();

	
	int  next_start_code();
	bool bytealigned();
	UINT next_bits(int nBit);
};

//#ifdef _MSC_VER

//˵��:ֻ��Ҫ��Windowsƽ̨֧���޸�mp4�ļ�

enum eMP4FileStatus
{
	eMP4File_Fail=-1,	//����ʧ��,�������ļ�������,�޷���
	eMP4File_Unknown=0,	//�޷�ʶ����ļ�
	eMP4File_JCOK,		//ok,����¼��������ļ�
	eMP4File_JCOX,		//x=wrong,�쳣¼��������ļ�,��Ҫ�޸�
	eMP4File_JCOR,		//r=repaired,�쳣¼��������ļ�,�Ѿ��޸�
};

class MP4_AFX_EXT_CLASS CMP4Repair
{
public:
	CMP4Repair();
	virtual ~CMP4Repair();
	int Open(const char *pszMP4File);
	void Close();
	eMP4FileStatus GetMP4FileStatus();
	int Repair();
protected:
	FILE *m_hFile;
	string m_szFile;
	eMP4FileStatus m_fs;
	long	m_pos_jcox;
	long	m_pos_mdat;
	long	m_pos_moov;
	long	m_cbFile;
	long	m_jcox_size;
};

//#endif

#endif

