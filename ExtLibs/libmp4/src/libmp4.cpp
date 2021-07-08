#include "libmp4.h"

#ifdef __cplusplus
extern "C"{
#endif

#include "libmp4_c.h"

#ifdef __cplusplus
}
#endif

#ifdef _MSC_VER
#include <io.h>
#endif

static int GetAacSampleFromSamplingFreqIndex(int index)
{
	switch (index) {
	case 0: return 96000; 
	case 1: return 88200; 
	case 3: return 48000; 
	case 4: return 44100; 
	case 5: return 32000; 
	case 6: return 24000; 
	case 7: return 22050; 
	case 8: return 16000; 
	case 9: return 12000; 
	case 10: return 11025;
	case 11: return 8000; 
	case 12: return 7350; 
	default:
		return 0;
	}
}


static int GetAacSampleFrequencyIndex(int sampling_frequency)
{
	switch (sampling_frequency) {
	case 96000: return 0;
	case 88200: return 1;
	case 64000: return 2;
	case 48000: return 3;
	case 44100: return 4;
	case 32000: return 5;
	case 24000: return 6;
	case 22050: return 7;
	case 16000: return 8;
	case 12000: return 9;
	case 11025: return 10;
	case 8000:  return 11;
	case 7350:  return 12;
	default:
		return 3;
	}
}


CBitStream::CBitStream()
{
	m_pBitStream = NULL;
	m_cbBitStream=0;
	m_nBitPos=0;

	m_byBuf=NULL;
}

CBitStream::~CBitStream()
{
	UnInit();
}

BOOL32 CBitStream::Init(LPBYTE pBitStream,int cbBitStream,int nBitPos)
{
	UnInit();

	m_pBitStream = pBitStream;
	m_cbBitStream=cbBitStream;
	m_nBitPos=nBitPos;

	return nBitPos<cbBitStream;
}

static int GetHex(char ch)
{
	ch=tolower(ch);
	ASSERT((ch>='0' && ch<='9') || (ch>='a' && ch<='f'));
	if((ch>='0' && ch<='9'))
		return ch-'0';
	return ch-'a'+10;
}


BOOL32 CBitStream::Init(const char *pszHex,int cbLen)
{
	UnInit();

	char szBS[4096];
	ASSERT(cbLen < (int)sizeof(szBS));
	szBS[sizeof(szBS)-1]=0;
	strncpy(szBS,pszHex,sizeof(szBS)-1);
	ASSERT((strlen(szBS)%2)==0);
	
	if(cbLen==-1)
	{
		cbLen=strlen(szBS);
	}

	int bsLen=cbLen/2;
	m_byBuf = new BYTE[bsLen];
	memset(m_byBuf,0,bsLen);

	LPBYTE p = m_byBuf;
	for(int i=0;i<cbLen;i+=2)
	{
		BYTE value = (GetHex(szBS[i])<<4)|GetHex(szBS[i+1]);
		*p++=value;
	}

	m_pBitStream=m_byBuf;
	m_cbBitStream=bsLen*8;

	return TRUE;
}

void CBitStream::UnInit()
{
	if(m_byBuf)
	{
		delete []m_byBuf;
		m_byBuf=NULL;
	}

	m_pBitStream = NULL;
	m_cbBitStream=0;
	m_nBitPos=0;
}


//�ӵ�ǰλ�ö�ȡnBitλ����
UINT CBitStream::Read(int nBit)
{
	ASSERT(nBit<=32);

	if(m_nBitPos + nBit>=m_cbBitStream)
	{
		ASSERT(FALSE);
		return 0;
	}

	UINT value=0;
	for(int i=0;i<nBit;i++)
	{
		int bytepos = (m_nBitPos +i)/8;
		int bitpos = 7 - ((m_nBitPos +i)%8);
		//DT("bitpos=%d",bitpos);
		value<<=1;
		BYTE by=m_pBitStream[bytepos];
		if( by & (1<<bitpos))
		{
			value|=1;
		}
	}

	m_nBitPos += nBit;
	return value;
}

UINT CBitStream::Peek(int nBit)
{
	UINT ret = Read(nBit);
	m_nBitPos -= nBit;//����
	return ret;
}

BOOL32 CBitStream::SkipBit(int nBit)
{
	if(m_nBitPos + nBit>=m_cbBitStream)
	{
		ASSERT(FALSE);
		return 0;
	}
	m_nBitPos += nBit;
	return TRUE;
}

BOOL32 CBitStream::SetBit(UINT nBitPos,BOOL32 bSet)
{
	if(nBitPos>=(UINT)m_cbBitStream)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	
	UINT nBytePos=nBitPos/8;
	UINT nbpos=7-(nBitPos%8);

	if(bSet)
	{
		m_pBitStream[nBytePos]|=(1<<nbpos);
	}
	else
	{
		m_pBitStream[nBytePos]&=~(1<<nbpos);
	}

	return TRUE;
}

//��С��ģʽ��nBitPos��ʼ,дcbBitλ,ԭ���ݻᱻ����
BOOL32 CBitStream::WriteBit(UINT nBitPos,UINT nValue,int cbBit)
{
	ASSERT(cbBit<=32);

	int nbpos = nBitPos;
	for(int i=0;i<cbBit;i++)
	{
		int bit = (cbBit-i-1);
		
		BOOL32 bset= (BOOL32)((nValue & (1<<bit)) == (UINT)(1<<bit));
		SetBit(nbpos,bset);
		nbpos++;
	}
	
	return TRUE;
}


#define visual_object_sequence_start_code	0x000001B0
#define visual_object_sequence_end_code		0x000001B1
#define user_data_start_code				0x000001B2
#define visual_object_start_code			0x000001B5

int CMpeg4BitStream::Parse()
{
	int ret = Parse_VisualObjectSequence();
	return ret;
}

int CMpeg4BitStream::Parse_VisualObject()
{
	DWORD32 dwCode = Read(32);
	ASSERT(dwCode == visual_object_start_code);
	BOOL32 is_visual_object_identifier=Read(1);
	if(is_visual_object_identifier)
	{
		ASSERT(FALSE);
		//visual_object_verid
		//visual_object_priority
	}

	BYTE visual_object_type = (BYTE)Read(4);
	switch(visual_object_type)
	{
	case 0x01://video ID
	{
		Parse_video_signal_type();
		break;
	}
	default:
	{
		ASSERT(FALSE);
		break;
	}
	}

	next_start_code();

	dwCode = Peek(32);
	if(dwCode == user_data_start_code)
	{
		ASSERT(FALSE);
	}

	if(visual_object_type == 0x01)//video ID
	{
		BYTE video_object_start_code = Read(32);
		ASSERT(video_object_start_code<=0x1F);
		VideoObjectLayer();
		return -1;
	}
	
	return 0;
}

UINT CMpeg4BitStream::next_bits(int nBit)
{
	return Peek(nBit);
}

int CMpeg4BitStream::VideoObjectLayer()
{
	//video_object_layer_start_code:0x20~0x2F
	BYTE video_object_layer_start_code = next_bits(32);
	if(video_object_layer_start_code>=0x20 && video_object_layer_start_code<=0x2F)
	{
		BYTE short_video_header = 0;
		video_object_layer_start_code=Read(32);
		BOOL32 random_accessible_vol=Read(1);
		BYTE video_object_type_indication=(BYTE)Read(8);
		BOOL32 is_object_layer_identifier=Read(1);

		BYTE video_object_layer_verid =0;
		BYTE video_object_layer_priority=0;
		if(is_object_layer_identifier)
		{
			video_object_layer_verid = (BYTE)Read(4);
			video_object_layer_priority=(BYTE)Read(3);
		}

		BYTE aspect_ratio_info=(BYTE)Read(4);
#define EXTENDED_PAR	15
		if(aspect_ratio_info == EXTENDED_PAR)
		{
			BYTE par_width = Read(8);
			BYTE par_height = Read(8);
		}

		BOOL32 vol_control_parameters=Read(1);
		if(vol_control_parameters)
		{
		ASSERT(FALSE);
		}

		BYTE video_object_layer_shape=(BYTE)Read(2);
#define rectangular	0x00 
#define binary		0x01
#define binary_only	0x02 
#define grayscale	0x03	//ISO 14496-2_2001 pdf page 130
		ASSERT(video_object_layer_shape != grayscale);

		BYTE marker_bit=(BYTE)Read(1);
		ASSERT(marker_bit==1);
		WORD vop_time_increment_resolution=Read(16);
		marker_bit=(BYTE)Read(1);
		ASSERT(marker_bit==1);

		BYTE fixed_vop_rate=(BYTE)Read(1);
		ASSERT(!fixed_vop_rate);

		if(video_object_layer_shape != binary_only)
		{
			if(video_object_layer_shape == rectangular)
			{
				marker_bit=(BYTE)Read(1);
				ASSERT(marker_bit==1);
				{
					//int nbpos = GetBitPos();
					//DT("bit pos = %d[%d:%d]",nbpos,nbpos/8,nbpos%8);
				}
				WORD video_object_layer_width=Read(13);
				marker_bit=(BYTE)Read(1);
				ASSERT(marker_bit==1);
				WORD video_object_layer_height=Read(13);
				marker_bit=(BYTE)Read(1);
				ASSERT(marker_bit==1);

				//DT("size =%d,%d",video_object_layer_width,video_object_layer_height);
			}

			BYTE interlaced=(BYTE)Read(1);
			BYTE obmc_disable=(BYTE)Read(1);
			BYTE sprite_enable=0;
			if(video_object_layer_verid==0x01)
			{
				sprite_enable=Read(1);
			}
			else
			{
				sprite_enable=Read(2);
			}
#define SpriteCodingMode_static		0x01
#define SpriteCodingMode_GMC		0x02
#define SpriteCodingMode_Reserved	0x03

			ASSERT(sprite_enable==0);

			ASSERT(video_object_layer_verid==0x01);
			//if (video_object_layer_verid != ��0001�� && video_object_layer_shape != ��rectangular��)

			BYTE not_8_bit=Read(1);
			if(not_8_bit)
			{
				BYTE quant_precision=(BYTE)Read(4);
				BYTE bits_per_pixel=(BYTE)Read(4);
			}

			ASSERT(video_object_layer_shape != grayscale);

			BYTE quant_type=(BYTE)Read(1);
			ASSERT(!quant_type);

			if(video_object_layer_verid != 0x01)
			{
				BYTE quarter_sample=(BYTE)Read(1);
			}

			BYTE complexity_estimation_disable=(BYTE)Read(1);
			if(!complexity_estimation_disable)
			{
				ASSERT(FALSE);
			}

			BYTE resync_marker_disable=(BYTE)Read(1);
			BYTE data_partitioned=(BYTE)Read(1);
			if(data_partitioned)
			{
				BYTE reversible_vlc=(BYTE)Read(1);
			}

			if(video_object_layer_verid != 0x01) 
			{
				ASSERT(FALSE);
			}
		}
		
		//����Ľ��������д�,14496-2 pdf��Ƕ��̫��,block��β����׿����
		BYTE scalability=(BYTE)Read(1);
		if(scalability)
		{
		ASSERT(FALSE);
		}
		
		//todo
		int here=0;
	}

	return 0;
}

int CMpeg4BitStream::Parse_VisualObjectSequence()
{
	do
	{
		DWORD32 dwCode = Read(32);
		ASSERT(dwCode == visual_object_sequence_start_code);

		BYTE profile = Read(8);

		while(Peek(32) == user_data_start_code)
		{
			ASSERT(FALSE);
			//user_data();
		}

		if(-1==Parse_VisualObject())
			return -1;

	}while(Peek(32) != visual_object_sequence_end_code);

	DWORD32 dwCode = Read(32);
	ASSERT(dwCode == visual_object_sequence_end_code);

	return 0;
}

int CMpeg4BitStream::Parse_video_signal_type()
{
	BOOL32 video_signal_type = Read(1);
	if(video_signal_type)
	{
		BYTE video_format=Read(3);
//video_format Meaning
//000 Component
//001 PAL
//010 NTSC
//011 SECAM
//100 MAC
//101 Unspecified video format
//110 Reserved
//111 Reserved

		BYTE video_range=Read(1);
		BYTE colour_description=Read(1);
		if(colour_description)
		{
			BYTE colour_primaries=Read(8);
			BYTE transfer_characteristics=Read(8);
			BYTE matrix_coefficients=Read(8);
		}
	}
	
	return 0;
}

int CMpeg4BitStream::next_start_code()
{
	BOOL32 zero_bit=(BOOL32)Read(1);
	ASSERT(!zero_bit);
	while(!bytealigned())
	{
		register BOOL32 one_bit = Read(1);
		ASSERT(one_bit);
	}
	return 0;
}

bool CMpeg4BitStream::bytealigned()
{
	if( (GetBitPos()%8)==0)
		return true;
	return false;
}

CMP4Base::CMP4Base()
{
	m_hFile=NULL;
}

CMP4Base::~CMP4Base()
{

}

unsigned CMP4Base::addByte(unsigned char byte) 
{
	ASSERT(m_hFile);
	putc(byte, m_hFile);
	return 1;
}

long CMP4Base::addAtomHeader(char const* atomName) 
{
  // Output a placeholder for the 4-byte size:
  addWord(0);
  // Output the 4-byte atom name:
  add4ByteString(atomName);
  return 8;
}

//ע��:
//����ο���live555�е�openRTSP,����word��ʾ4���ֽ�,��HalfWord��ʾ�����ֽ�
//��Ҫ��Windows�µ�DWORD32,WORD���
unsigned CMP4Base::addWord(unsigned word) 
{
	addByte(word>>24); addByte(word>>16);
	addByte(word>>8); addByte(word);
	return 4;
}

unsigned CMP4Base::addHalfWord(unsigned short halfWord) 
{
	addByte((unsigned char)(halfWord>>8)); addByte((unsigned char)halfWord);
	return 2;
}

unsigned CMP4Base::addZeroWords(unsigned numWords) 
{
	for (unsigned i = 0; i < numWords; ++i) 
	{
		addWord(0);
	}

	return numWords*4;
}

unsigned CMP4Base::add4ByteString(char const* str) 
{
	addByte(str[0]); addByte(str[1]); addByte(str[2]); addByte(str[3]);
	return 4;
}

unsigned CMP4Base::addArbitraryString(char const* str,Boolean oneByteLength) 
{
  unsigned size = 0;
  if (oneByteLength) 
  {
    // Begin with a byte containing the string length:
    unsigned strLength = strlen(str);
    if (strLength >= 256) 
	{
		ASSERT(FALSE);
    }
    size += addByte((unsigned char)strLength);
  }

  while (*str != '\0') 
  {
    size += addByte(*str++);
  }

  return size;
}

void CMP4Base::setWord(unsigned filePosn, unsigned size) 
{
	do
	{
		if (fseek(m_hFile, filePosn, SEEK_SET) < 0) break;
		addWord(size);
		if (fseek(m_hFile, 0, SEEK_END) < 0) break; // go back to where we were
		
		return;
	} while (0);
	
	// One of the fseek()s failed, probable because we're not a seekable file
	ASSERT(FALSE);
}

void CMP4Base::ModifyByte(unsigned filePosn,BYTE newValue)
{
	long pos = ftell(m_hFile);
	fseek(m_hFile,filePosn,SEEK_SET);
	putc(newValue, m_hFile);
	fseek(m_hFile,pos,SEEK_SET);
}

int CMP4Base::gettimeofday(struct timeval* tp) 
{
#ifdef _MSC_VER
	
	static LARGE_INTEGER tickFrequency, epochOffset;
	
	// For our first call, use "ftime()", so that we get a time with a proper epoch.
	// For subsequent calls, use "QueryPerformanceCount()", because it's more fine-grain.
	static Boolean isFirstCall = TRUE;
	
	LARGE_INTEGER tickNow;
	QueryPerformanceCounter(&tickNow);
	
	if (isFirstCall) 
	{
		struct timeb tb;
		ftime(&tb);
		tp->tv_sec = tb.time;
		tp->tv_usec = 1000*tb.millitm;
		
		// Also get our counter frequency:
		QueryPerformanceFrequency(&tickFrequency);
		// And compute an offset to add to subsequent counter times, so we get a proper epoch:
		epochOffset.QuadPart = tb.time*tickFrequency.QuadPart + (tb.millitm*tickFrequency.QuadPart)/1000 - tickNow.QuadPart;
		
		isFirstCall = FALSE; // for next time
	} 
	else 
	{
		// Adjust our counter time so that we get a proper epoch:
		tickNow.QuadPart += epochOffset.QuadPart;
		
		tp->tv_sec = (long) (tickNow.QuadPart / tickFrequency.QuadPart);
		tp->tv_usec = (long) (((tickNow.QuadPart % tickFrequency.QuadPart) * 1000000L) / tickFrequency.QuadPart);
	}
#else
	//linux

	::gettimeofday(tp, NULL);
#endif

	return 0;
}

const char *CMP4Base::GetMpeg4FrameTypeDesc(LPBYTE pData)
{
	static char *arr[]=
	{
		"invalid mpeg4 frame",
		"I Frame",
		"P Frame",
		"B Frame",
	};

	UINT idx = GetMpeg4FrameType(pData);
	if(idx>eMpeg4_BFrame)
		idx=eMpeg4_errFrame;
	return arr[idx];
}

// �жϵ�ǰMPEG4��Ƶ֡�Ƿ���I֡
eMpeg4FrameType CMP4Base::GetMpeg4FrameType(LPBYTE pData)
{
/*
vop_coding_type   is   encoded   in   the   first   2   bits   after   vop_start_code.   
vop_start_code   is   00   00   01   b6   (hex)   

		vop_coding_type   means:   
		0   0     I		 VOP   
		0   1     P		 VOP   
		1   0     B		 VOP   
		1   1     S		 VOP   
		
	*/
	UINT nHead = *((UINT *)pData);   
	if(nHead == 0xb3010000 || nHead == 0xb0010000)
	{
		return eMpeg4_IFrame;   
	}
	else if(nHead == 0xb6010000)
	{   
		unsigned char vt_byte = pData[4];  
		
		vt_byte &= 0xC0;   
		vt_byte  = vt_byte >> 6;
		
		switch(vt_byte)   
		{   
		case 0:					// I Frame
			return eMpeg4_IFrame;   
			break;   
		case 1:   
			return eMpeg4_PFrame;		// P Frame   
			break;   
		case 2:   
			return eMpeg4_BFrame;		// B Frame
			break;   
		default:
			break;
		}   
	}
	
	return eMpeg4_errFrame;				// Unkown Frame		
}

static char base64DecodeTable[256];

static void initBase64DecodeTable() {
  int i;
  for (i = 0; i < 256; ++i) base64DecodeTable[i] = (char)0x80;
      // default value: invalid

  for (i = 'A'; i <= 'Z'; ++i) base64DecodeTable[i] = 0 + (i - 'A');
  for (i = 'a'; i <= 'z'; ++i) base64DecodeTable[i] = 26 + (i - 'a');
  for (i = '0'; i <= '9'; ++i) base64DecodeTable[i] = 52 + (i - '0');
  base64DecodeTable[(unsigned char)'+'] = 62;
  base64DecodeTable[(unsigned char)'/'] = 63;
  base64DecodeTable[(unsigned char)'='] = 0;
}

unsigned char* CMP4Base::base64Decode(char* in, unsigned& resultSize,
			    Boolean trimTrailingZeros) 
{
  static Boolean haveInitedBase64DecodeTable = FALSE;
  if (!haveInitedBase64DecodeTable) {
    initBase64DecodeTable();
    haveInitedBase64DecodeTable = TRUE;
  }

#ifdef _MSC_VER
  unsigned char* out = (unsigned char*)_strdup(in); // ensures we have enough space
#else
  unsigned char* out = (unsigned char*)strdup(in); // ensures we have enough space
#endif

  int k = 0;
  int const jMax = strlen(in) - 3;
     // in case "in" is not a multiple of 4 bytes (although it should be)
  for (int j = 0; j < jMax; j += 4) {
    char inTmp[4], outTmp[4];
    for (int i = 0; i < 4; ++i) {
      inTmp[i] = in[i+j];
      outTmp[i] = base64DecodeTable[(unsigned char)inTmp[i]];
      if ((outTmp[i]&0x80) != 0) outTmp[i] = 0; // pretend the input was 'A'
    }

    out[k++] = (outTmp[0]<<2) | (outTmp[1]>>4);
    out[k++] = (outTmp[1]<<4) | (outTmp[2]>>2);
    out[k++] = (outTmp[2]<<6) | outTmp[3];
  }

  if (trimTrailingZeros) {
    while (k > 0 && out[k-1] == '\0') --k;
  }
  resultSize = k;
  unsigned char* result = new unsigned char[resultSize];
  memmove(result, out, resultSize);
  delete[] out;

  return result;
}

static const char base64Char[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* CMP4Base::base64Encode(char const* origSigned, unsigned origLength) 
{
  unsigned char const* orig = (unsigned char const*)origSigned; // in case any input bytes have the MSB set
  if (orig == NULL) return NULL;

  unsigned const numOrig24BitValues = origLength/3;
  Boolean havePadding = origLength > numOrig24BitValues*3;
  Boolean havePadding2 = origLength == numOrig24BitValues*3 + 2;
  unsigned const numResultBytes = 4*(numOrig24BitValues + havePadding);
  char* result = new char[numResultBytes+1]; // allow for trailing '\0'

  // Map each full group of 3 input bytes into 4 output base-64 characters:
  unsigned i;
  for (i = 0; i < numOrig24BitValues; ++i) {
    result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];
    result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
    result[4*i+2] = base64Char[((orig[3*i+1]<<2) | (orig[3*i+2]>>6))&0x3F];
    result[4*i+3] = base64Char[orig[3*i+2]&0x3F];
  }

  // Now, take padding into account.  (Note: i == numOrig24BitValues)
  if (havePadding) {
    result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];
    if (havePadding2) {
      result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
      result[4*i+2] = base64Char[(orig[3*i+1]<<2)&0x3F];
    } else {
      result[4*i+1] = base64Char[((orig[3*i]&0x3)<<4)&0x3F];
      result[4*i+2] = '=';
    }
    result[4*i+3] = '=';
  }

  result[numResultBytes] = '\0';
  return result;
}

//#ifndef _MSC_VER
void DebugTrace(LPCTSTR lpszFormat, ... )
{
	char szBuf[1024];//should be enough
	szBuf[0]=0;

	va_list argList;
	va_start(argList, lpszFormat);
	vsprintf(szBuf,lpszFormat, argList);
	va_end(argList);

	printf(szBuf);
	printf("\n");
}
//#endif

CMP4Trak::CMP4Trak()
{
	m_pMP4Write = NULL;
	m_trakid=0;
	m_timescale=1;
	m_duration=0;
	memset(m_handlerType,0,sizeof(m_handlerType));
	m_szVideo=CSize(0,0);
	m_dwFrameIndex=0;
	m_nFirstChunk=0;

	m_dwTickRTPLast=0;
	m_type=0;
	m_stsz_count=0;
	m_fmtp_spropparametersets=NULL;
	m_bH264IFrame=FALSE;
	m_amr_ft=0;
	m_audio_channel = 1;
}

CMP4Trak::~CMP4Trak()
{
	Empty();
}

void CMP4Trak::Empty()
{
	{
		for(vector<tagMP4Node_stts*>::iterator it=m_lst_stts.begin();it!=m_lst_stts.end();it++)
		{
			tagMP4Node_stts* ps = *it;
			delete ps;
		}
		m_lst_stts.clear();
	}

	{
		for(vector<tagMP4Node_stsc*>::iterator it=m_lst_stsc.begin();it!=m_lst_stsc.end();it++)
		{
			tagMP4Node_stsc* ps = *it;
			delete ps;
		}
		
		m_lst_stsc.clear();
	}

	if(m_fmtp_spropparametersets)
	{
		free(m_fmtp_spropparametersets);
		m_fmtp_spropparametersets=NULL;
	}

	m_amr_ft=0;
}

int CMP4Trak::InitAudioTrakInfo(DWORD32 type,const char *handlerType, DWORD32 sample, int bit_per_sample ,int channel, int bitrate)
{
	m_type=type;
	ASSERT(m_pMP4Write);
	ASSERT(strlen(handlerType)==sizeof(m_handlerType));

	//����amr��rtp payload type������audio track id,��ʹ������audio��ulaw,��Ҳ������������
	//mp4��׼Ҫ��ÿ��trak��id��ͬ����,����֮��,track idû����������
	//ulaw��payload typeΪ0,������Ч��trak id
	m_trakid = RTP_TYPE_AMR;//97
	
	ASSERT( (m_trakid & 0xFF) == m_trakid);//����֧��MP4_FLAGS_JCOK,����Ҫ����trakidΪһ���ֽ�
	memcpy(m_handlerType,handlerType,sizeof(m_handlerType));;
	m_timescale = sample;
	m_hFile=m_pMP4Write->m_hFile;
	m_audio_channel = channel;
	m_audio_bit_per_sample = bit_per_sample;
	m_audio_bitrate = bitrate;
	return 0;
}

int CMP4Trak::InitVideoTrakInfo(DWORD32 type,const char *handlerType,CSize szVideo,const char *sprop_parameter_sets,DWORD32 timescale)
{
	if (NULL == m_pMP4Write) {
		printf("[%s][%d] VideoTrak member *MP4Write is null", __FILE__, __LINE__);
		return -1;
	}
	
	ASSERT(strlen(handlerType)==sizeof(m_handlerType));
	m_type=type;
	//use rtp payload for Mp4 Atom TKHD trak id .trak id ����Ϊ0 һ��mp4�ļ�������tkhd��track id �����ظ�
	m_trakid = RTP_TYPE_H264;

	ASSERT( (m_trakid & 0xFF) == m_trakid);//����֧��MP4_FLAGS_JCOK,����Ҫ����trakidΪһ���ֽ�
	memcpy(m_handlerType,handlerType,sizeof(m_handlerType));
	m_szVideo = szVideo;
	m_timescale=timescale;
	m_hFile=m_pMP4Write->m_hFile;

	ASSERT(!m_fmtp_spropparametersets);
	if(sprop_parameter_sets && strlen(sprop_parameter_sets)>0)
	{
		m_fmtp_spropparametersets=_strdup(sprop_parameter_sets);
//		m_timescale=600;//h.264
		DT("m_fmtp_spropparametersets=[%s]",m_fmtp_spropparametersets);
	}

	return 0;
}

long CMP4Trak::addAtom_trak()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("trak");

	size += addAtom_tkhd();
	size += addAtom_mdia();

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_tkhd()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("tkhd");

	struct timeval		m_fStartTime;
	gettimeofday(&m_fStartTime);
	unsigned   m_fAppleCreationTime = m_fStartTime.tv_sec - 0x83dac000;
	
	//audioΪulawʱ,video trak��audio trak��Flags��ҪΪ0x0F.���򲻼���quicktimeplayer
	//xwpcom 2009.11.06
	DWORD32 dwVerFlags=(m_pMP4Write->m_dwFlags & MP4F_G711)?0x0000000F:0x00000001;
	size += addWord(dwVerFlags); // Version +  Flags(EnableTrack)

	size += addWord(m_fAppleCreationTime); // Creation time
	size += addWord(m_fAppleCreationTime); // Modification time
	size += addWord(m_trakid); // Track ID
	size += addWord(0x00000000); // Reserved
	
	//ע��Ҫ�ȳ�m_timescale,��*600,����¼��ʱ���Գ�,m_duration * 600�����0xFFFFFFFF����Խ�����.
	//DWORD32��ʾ��duration���֧��13Сʱ�ĵ���¼��ʱ��(0xFFFFFFFF/90000).
	size += addWord((m_duration/m_timescale)*600); // Duration.
	size += addZeroWords(3); // Reserved+Layer+Alternate grp
	size += addWord(IsVideoTrak()?0x0:0x01000000); // Volume + Reserved
	size += addWord(0x00010000); // matrix top left corner
	size += addZeroWords(3); // matrix
	size += addWord(0x00010000); // matrix center
	size += addZeroWords(3); // matrix
	size += addWord(0x40000000); // matrix bottom right corner
	
	if (IsVideoTrak())
	{
		size += addWord(m_szVideo.cx<<16); // Track width
		size += addWord(m_szVideo.cy<<16); // Track height
	}
	else 
	{
		size += addZeroWords(2); // not video: leave width and height fields zero
	}
	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_mdia()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("mdia");
	
	size += addAtom_mdhd();
	size += addAtom_hdlr();
	size += addAtom_minf();

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_mdhd()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("mdhd");
	size += addWord(0x00000000); // Version+flags

	struct timeval fStartTime;
	gettimeofday(&fStartTime);
	unsigned fCreationTime = fStartTime.tv_sec - 0x83dac000;

	size += addWord(fCreationTime); // create time
	size += addWord(fCreationTime); // modify time
	size += addWord(m_timescale);		 // timescale
	size += addWord(m_duration);		 // duration
	size += addHalfWord(0x0000); //language
	size += addHalfWord(0x0000); //reserved

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_hdlr()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("hdlr");
	size += addWord(0x00000000); // Version+flags
	if ((m_type == MP4_AUDIO_ULAW) || (m_type == MP4_AUDIO_ALAW))
	{
		//ulaw
		size += add4ByteString("mhlr");
	}
	else
	{
		size += addWord(0x00000000); // reserved
	}
	size += add4ByteString((const char*)m_handlerType);//video,soun,...

	{
		if ((m_type == MP4_AUDIO_ULAW) || (m_type == MP4_AUDIO_ALAW))
		{
			//ulaw
			size += add4ByteString("appl");
			for(int i=0;i<8;i++)
			{
				size += addByte(0x00);
			}
		}
		else
		{
			//12 bytes 0x00,reserved
			for(int i=0;i<12;i++)
			{
				size += addByte(0x00);
			}
		}
	}

	if ((m_type == MP4_AUDIO_ULAW) || (m_type == MP4_AUDIO_ALAW))
	{
		const char *name="Apple Sound Media Handler";
		size += addArbitraryString(name,TRUE);
	}
	else
	{
		size += addByte(0x00);//null name
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_minf()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("minf");

	if(IsVideoTrak())
	{
		size +=addAtom_vmhd();
	}
	else
	{
		size +=addAtom_smhd();
	}

	size +=addAtom_dinf();
	size +=addAtom_stbl();

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_vmhd()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("vmhd");
	size += addWord(0x00000001); // Version+flags

	if(m_fmtp_spropparametersets)
	{
	  size += addWord(0x00408000); // Graphics mode + Opcolor[red]
	  size += addWord(0x80008000); // Opcolor[green} + Opcolor[blue]
	}
	else
	{
		//8 bytes 0x00,reserved
		for(int i=0;i<8;i++)
		{
			size += addByte(0x00);
		}
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_smhd()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("smhd");
	size += addWord(0x00000000); // Version+flags
	size += addWord(0x00000000); 
	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_dinf()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("dinf");
	size += addAtom_dref();

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_dref()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("dref");
	size += addWord(0x00000000); // Version+flags
	size += addWord(0x00000001); // entries
	{
		//url null
		size += addWord(0x0000000C); 
		
		size+=add4ByteString("url ");
		size+=addWord(0x00000001);
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_stts()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("stts");
	size += addWord(0x00000000); // Version+flags
	size += addWord(m_lst_stts.size()); 

	{
		for(vector<tagMP4Node_stts*>::iterator it=m_lst_stts.begin();it!=m_lst_stts.end();it++)
		{
			tagMP4Node_stts* ps = *it;
			size += addWord(ps->SampleCount);
			size += addWord(ps->SampleDuration>0?ps->SampleDuration:1);
		}
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_stss()
{
	if(!IsVideoTrak() || IsMjpg())
	{
		return 0;
	}

	long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("stss");
	size += addWord(0x00000000); // Version+flags
	size += addWord(m_lst_stss.size()); 

	{
		for(vector<DWORD32>::iterator it=m_lst_stss.begin();it!=m_lst_stss.end();it++)
		{
			DWORD32 idx = *it;
			size += addWord(idx);
		}
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_stsc()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("stsc");
	size += addWord(0x00000000); // Version+flags
	size += addWord(m_lst_stsc.size()); 

	{
		for(vector<tagMP4Node_stsc*>::iterator it=m_lst_stsc.begin();it!=m_lst_stsc.end();it++)
		{
			tagMP4Node_stsc* ps = *it;
			size += addWord(ps->FirstChunk);
			size += addWord(ps->SamplesPerChunk);
			size += addWord(ps->SampleDescriptionID);
		}
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_stsz()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("stsz");
	size += addWord(0x00000000); // Version+flags

	//�ж�����sample�ǲ��Ǿ�����ͬ��С
	BOOL32 bHasDifferentSize = FALSE;
	DWORD32 dwSize = 0;
		for(vector<DWORD32>::iterator it=m_lst_stsz.begin();it!=m_lst_stsz.end();it++)
		{
			DWORD32 sz = *it;
		if (dwSize && sz != dwSize) {
				bHasDifferentSize=TRUE;
				break;
			}

			dwSize = sz;
		}
	
	//write 0 if the samples have different sizes
	//write single size if all the samples are the same size
	size += addWord(bHasDifferentSize?0x00000000:dwSize);

	//20160709 add aac
	if (IsAudioTrak() && (m_type != MP4_AUDIO_AAC)) {
		size += addWord(m_stsz_count);
	} else {
		size += addWord(m_lst_stsz.size());
	}
	//size += addWord(IsAudioTrak()?m_stsz_count:m_lst_stsz.size());
	
	if(bHasDifferentSize)
	{
		for(vector<DWORD32>::iterator it=m_lst_stsz.begin();it!=m_lst_stsz.end();it++)
		{
			DWORD32 sz = *it;
			size += addWord(sz);
		}
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_stco()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("stco");
	size += addWord(0x00000000); // Version+flags
	size += addWord(m_lst_stco.size()); 
	{
		for(vector<DWORD32>::iterator it=m_lst_stco.begin();it!=m_lst_stco.end();it++)
		{
			DWORD32 sz = *it;
			size += addWord(sz);
		}
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_stsd()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("stsd");
	size += addWord(0x00000000); // Version+flags
	size += addWord(0x00000001); // entries
	if(IsVideoTrak())
	{
		if(IsH264())
		{
			size += addAtom_avc1();//h.264
		}
		else if(IsMjpg())
		{
			size += addAtom_mjpa();//mjpg-a
		}
		else
		{
			size += addAtom_mp4v();//mpeg4
		}
	}
	else
	{
		if (m_type == MP4_AUDIO_ULAW) {
			size += addAtom_ulaw();
		} else if (MP4_AUDIO_ALAW == m_type) {
			size += addAtom_alaw();
		} else if(m_type == MP4_AUDIO_AMR_NB) {
			size += addAtom_samr();
		} else if (MP4_AUDIO_AAC == m_type) {
			size += addAtom_mp4a();
		}
	}

	setWord(initFilePosn, size);
	return size;
}

//AudioSampleEntry(ISO_IEC_14496_12.pdf)
long CMP4Trak::addAtom_samr()
{
	unsigned initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("samr");
	for (int i=0;i<6;i++)
	{
		size += addByte(0);
	}

	size += addHalfWord(0x0001);//data-reference-index

	size += addWord(0x00000000);
	size += addWord(0x00000000);

	//size += addHalfWord(0x0002);//reserved
	size += addHalfWord(m_audio_channel);//channel
	
	size += addHalfWord(0x0010);//reserved
	size += addWord(0x00000000);
	
	size += addHalfWord(8000);//timescale,����Ϊ16bit
	size += addHalfWord(0000);

	//AMRSpecifiBox
	size += addWord(0x00000011);
	size += add4ByteString("damr");
	size += add4ByteString("JCO ");//vendor,can be other chars
	size += addByte(0x01);//decoder version
	WORD mode_set=0x0080;//0x0080;//0x81FF
	size += addHalfWord(mode_set);//mode_set
	size += addByte(0x00);//mode_change_period
	size += addByte(0x01);//frames_per_sample

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_ulaw()
{
	unsigned initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("ulaw");
	
	// General sample description fields:
	size += addWord(0x00000000); // Reserved
	size += addWord(0x00000001); // Reserved+Data reference index

	// Sound sample description fields:
	unsigned short const version = 0;
	size += addWord(version<<16); // Version+Revision level
	size += addWord(0x00000000); // Vendor
	unsigned short numChannels=1;
	size += addHalfWord(numChannels); // Number of channels
	size += addHalfWord(0x0010); // Sample size
	size += addWord(0xfffe0000); // Compression ID+Packet size #####

	ASSERT(m_timescale==8000);

	DWORD32 timescale=m_timescale;
	DWORD32 sampleRateFixedPoint = timescale<<16;
	size += addWord(sampleRateFixedPoint); // Sample rate
	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_alaw()
{
	unsigned initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("alaw");
	
	// General sample description fields:
	size += addWord(0x00000000); // Reserved
	size += addWord(0x00000001); // Reserved+Data reference index

	// Sound sample description fields:
	unsigned short const version = 0;
	size += addWord(version<<16); // Version+Revision level
	size += addWord(0x00000000); // Vendor
	unsigned short numChannels=1;
	size += addHalfWord(numChannels); // Number of channels
	size += addHalfWord(0x0010); // Sample size
	size += addWord(0xfffe0000); // Compression ID+Packet size #####

	ASSERT(m_timescale==8000);

	DWORD32 timescale=m_timescale;
	DWORD32 sampleRateFixedPoint = timescale<<16;
	size += addWord(sampleRateFixedPoint); // Sample rate
	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_avc1()
{
	unsigned initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("avc1");

	// General sample description fields:
	size += addWord(0x00000000); // Reserved
	size += addWord(0x00000001); // Reserved+Data       reference index
	// Video sample       description     fields:
	size += addWord(0x00000000); // Version+Revision level
	size += add4ByteString("appl"); // Vendor
	size += addWord(0x00000000); // Temporal quality
	size += addWord(0x00000000); // Spatial quality
	unsigned const widthAndHeight       = (m_szVideo.cx<<16)|m_szVideo.cy;
	size += addWord(widthAndHeight); // Width+height
	size += addWord(0x00480000); // Horizontal resolution
	size += addWord(0x00480000); // Vertical resolution
	size += addWord(0x00000000); // Data size
	size += addWord(0x00010548); // Frame       count+Compressor name (start)
	// "H.264"
	size += addWord(0x2e323634); // Compressor name (continued)
	size += addZeroWords(6); // Compressor name (continued - zero)
	size += addWord(0x00000018); // Compressor name (final)+Depth
	size += addHalfWord(0xffff); // Color       table id
	size += addAtom_avcC();
	
	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_avcC()
{
	unsigned initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("avcC");

	// Begin by Base-64 decoding the "sprop" parameter sets strings:
	ASSERT(m_fmtp_spropparametersets);
	char* psets = _strdup(m_fmtp_spropparametersets);
	if (psets == NULL) return 0;
	
	size_t comma_pos = strcspn(psets, ",");
	psets[comma_pos] = '\0';
	char* sps_b64 = psets;
	char* pps_b64 = &psets[comma_pos+1];
	unsigned sps_count;
	unsigned char* sps_data = base64Decode(sps_b64, sps_count, false);
	unsigned pps_count;
	unsigned char* pps_data = base64Decode(pps_b64, pps_count, false);
	
	// Then add the decoded data:
	size += addByte(0x01); // configuration version
	size += addByte(sps_data[1]); // profile
	size += addByte(sps_data[2]); // profile compat
	size += addByte(sps_data[3]); // level
	size += addByte(0xff); /* 0b11111100 | lengthsize = 0x11 */
	size += addByte(0xe0 | (sps_count > 0 ? 1 : 0) );
	if (sps_count > 0) 
	{
		size += addHalfWord(sps_count);
		for (unsigned i = 0; i < sps_count; i++) 
		{
			size += addByte(sps_data[i]);
		}
	}
	size += addByte(pps_count > 0 ? 1 : 0);
	if (pps_count > 0) 
	{
		size += addHalfWord(pps_count);
		for (unsigned i = 0; i < pps_count; i++) 
		{
			size += addByte(pps_data[i]);
		}
	}
	
	// Finally, delete the data that we allocated:
	delete[] pps_data; 
	pps_data=NULL;
	delete[] sps_data;
	sps_data=NULL;
	delete[] psets;
	psets=NULL;

	setWord(initFilePosn, size);	
	return size;
}

//VisualSampleEntry(ISO_IEC_14496_12.pdf)
long CMP4Trak::addAtom_mp4v()
{
	unsigned initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("mp4v");
	for (int i=0;i<6;i++)
	{
		size += addByte(0);
	}

	size += addHalfWord(0x0001);//data-reference-index.todo

	size += addHalfWord(0x0000);//pre_defined
	size += addHalfWord(0x0000);//reserved

	size += addWord(0x00000000);//reserved
	size += addWord(0x00000000);//reserved
	size += addWord(0x00000000);//reserved

	//width,height
	size += addHalfWord((WORD)m_szVideo.cx);
	size += addHalfWord((WORD)m_szVideo.cy);

	size += addWord(0x00480000);//horizresolution = 0x00480000; // 72 dpi
	size += addWord(0x00480000);//vertresolution = 0x00480000; // 72 dpi

	size += addWord(0x00000000);//reserved
	size += addHalfWord(0x0001);//frame_count=1

	//string[32] compressorname
	{
		for(int i=0;i<32;i++)
		{
			size += addByte(0x00);
		}
	}

	size += addHalfWord(0x0018);//unsigned int(16) depth = 0x0018;
	size += addHalfWord(0xFFFF);//int(16) pre_defined = -1;

	size += addAtom_esds_mp4v();
	
	setWord(initFilePosn, size);	
	return size;
}

long CMP4Trak::addAtom_mjpa()
{
	unsigned initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("mjpa");
	for (int i=0;i<6;i++)
	{
		size += addByte(0);
	}

	size += addHalfWord(0x0001);//data-reference-index.todo

	size += addHalfWord(0x0000);//pre_defined
	size += addHalfWord(0x0000);//reserved

	size += addWord(0x00000000);//reserved
	size += addWord(0x00000000);//reserved
	size += addWord(0x00000000);//reserved

	//width,height
	size += addHalfWord((WORD)m_szVideo.cx);
	size += addHalfWord((WORD)m_szVideo.cy);

	size += addWord(0x00480000);//horizresolution = 0x00480000; // 72 dpi
	size += addWord(0x00480000);//vertresolution = 0x00480000; // 72 dpi

	size += addWord(0x00000000);//reserved
	size += addHalfWord(0x0001);//frame_count=1

	//string[32] compressorname
	{
		for(int i=0;i<32;i++)
		{
			size += addByte(0x00);
		}
	}

	size += addHalfWord(0x0018);//unsigned int(16) depth = 0x0018;
	size += addHalfWord(0xFFFF);//int(16) pre_defined = -1;

	{
		BYTE arr[]=
		{
			0x00,0x00,0x00,0x0A,'f','i','e','l',0x02,0x01,
			0x00,0x00,0x00,0x0C,'g','a','m','a',0x00,0x01,0xCC,0xCC,
			0x00,0x00,0x00,0x00,
		};
		
		for(UINT i=0;i<COUNT_OF(arr);i++)
		{
			size += addByte(arr[i]);
		}
	}

	setWord(initFilePosn, size);
	return size;
}

//esds:�ο�ffmpeg��movenc.c���static int mov_write_esds_tag(ByteIOContext *pb, MOVTrack *track) // Basic
//32bit size
//32bit type('esds')
//32bit 8bit version+24bit flags)
	//8bit	0x03��ʾES descriptor
	//8bit	ES descriptor����)
		//16bit track id(�������Ϊ0,��trak���trackid����Ϊ0
		//8bit  flags=0
		//8bit  0x04��ʾDecoderConfig descriptor
		//8bit  DecoderConfig descriptor����
			//8bit codec tag,��ffmpeg����ff_mp4_obj_type����,0x20��ʾCODEC_ID_MPEG4,0x21��ʾCODEC_ID_H264
			//8bit made of 6 bits to identify the streamtype (4 for video, 5 for audio) 
				// plus 1 bit to indicate upstream and 1 bit set to 1 (reserved)
				//0x15 ��ʾAudiostream
				//0x11 ��ʾVisualstream
			//24bit Buffersize DB
			//32bit maxBitrate
			//32bit avgBitrate
			//8bit  0x05��ʾDecoderSpecific info descriptor
			//8bit  DecoderSpecific info descriptor����decLen
				//decLen���ֽ�,mpeg4�Ļ�Ϊ0x000001b0xxx decoder config����
			//8bit 0x06��ʾSL descriptor
			//8bit 0x01
			//8bit 0x02
long CMP4Trak::addAtom_esds_mp4v()
{
	int width=m_szVideo.cx;
	int height=m_szVideo.cy;
	BYTE mpeg4Config[]=
	{
		0x00,0x00,0x01,0xb0,0xf3,0x00,0x00,0x01,0xb5,0x0e,0xe0,0x40,0xc0,0xcf,0x00,0x00,
		0x01,0x00,0x00,0x00,0x01,0x20,0x00,0x84,0x40,0xfa,0x28,0x2f,0xa0,0xf0,0xa2,0x1f,
	};

	CMpeg4BitStream bs;
	bs.Init(mpeg4Config,sizeof(mpeg4Config)*8,0);
	bs.WriteBit(213,width,13);
	bs.WriteBit(213+13+1,height,13);
	
	long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("esds");
	size += addWord(0x00000000); // Version+flags

	{
		size += addByte(0x03);//es descriptor
		size+=addByte(0x80);
		size+=addByte(0x80);
		size+=addByte(0x80);
		long pos_es_desc=ftell(m_hFile);//remember for update
		size += addByte(0x00);//placeholder for es descriptor len
		size += addHalfWord(0x0000);//track id
		size += addByte(0x00);//flags

		{
			size += addByte(0x04);//decoder config desc
			//���������3��0x80�Ǽ���quicktime,���ӵĻ�,����replayer,vlc�в���,��������quicktime�в���
			size+=addByte(0x80);
			size+=addByte(0x80);
			size+=addByte(0x80);
			long pos_dec_config=ftell(m_hFile);//remember for update
			size += addByte(0x00);//placeholder for decoder config desc len
			size += addByte(0x20);//mpeg4
			size += addByte(0x11);//visualstream
			size += addByte(0x00);//24bit buf size
			size += addHalfWord(0xFFFF);
			size += addWord(0x000FFFFF);//32bit maxbitrate
			size += addWord(0x000FFFFF);//32bit avgbitrate
			
			size += addByte(0x05);//DecoderSpecific info descriptor
			size+=addByte(0x80);
			size+=addByte(0x80);
			size+=addByte(0x80);
			size += addByte(sizeof(mpeg4Config));//8bit  DecoderSpecific info descriptor����decLen
			for(UINT i=0;i<sizeof(mpeg4Config);i++)
			{
				size += addByte(mpeg4Config[i]);
			}

			ModifyByte(pos_dec_config,(BYTE)(ftell(m_hFile)-pos_dec_config-1));
		}

		size += addByte(0x06);//SL descriptor
		size+=addByte(0x80);
		size+=addByte(0x80);
		size+=addByte(0x80);
		size += addByte(0x01);
		size += addByte(0x02);
		ModifyByte(pos_es_desc,(BYTE)(ftell(m_hFile)-pos_es_desc-1));
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Trak::addAtom_stbl()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("stbl");

	size += addAtom_stsd();
	size += addAtom_stts();
	size += addAtom_stsc();
	size += addAtom_stsz();
	size += addAtom_stco();
	size += addAtom_stss();

	setWord(initFilePosn, size);
	return size;
}

int CMP4Trak::WriteAudioFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	return WriteAudioFrameHelper(pFrame,cbFrame,dwRTPTimeStamp,TRUE);
}

int CMP4Trak::WriteAudioFrameEx(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	return WriteAudioFrameHelper(pFrame,cbFrame,dwRTPTimeStamp,FALSE);
}

int CMP4Trak::WriteAudioFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp,BOOL32 bRTPHeader)
{
	int ret=-1;
	if ((m_type == MP4_AUDIO_ULAW) || (MP4_AUDIO_ALAW == m_type))
	{
		ret = OnUlawFrame(pFrame,cbFrame,dwRTPTimeStamp);
	}
	else if (m_type == MP4_AUDIO_AMR_NB)
	{
		if(bRTPHeader)
		{
			ret = OnAmrFrame(pFrame,cbFrame,dwRTPTimeStamp);
		}
		else
		{
			ret = OnAmrFrameHelper(pFrame,cbFrame,dwRTPTimeStamp);
		}
	} else if (MP4_AUDIO_AAC == m_type) {
		ret = OnAacFrameHelper(pFrame, cbFrame, dwRTPTimeStamp);
	}

	return ret;
}

//OnUlawFrameHelper������OnUlawFrame���������������
int CMP4Trak::OnUlawFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	LPBYTE pUlawData = pFrame;
	const UINT cbUlawData = cbFrame;

	m_dwTickRTPLast = dwRTPTimeStamp;

	//bJCOK��ʾ֧��mp4�޸�
	const BOOL32 bJCOK = (m_pMP4Write->GetMP4Flags() & MP4_FLAGS_JCOK);

	BOOL32 bAppendLastChunk = (m_pMP4Write->m_pTrakLastWrite==this);
	m_pMP4Write->m_pTrakLastWrite=this;
	if(bJCOK)
	{
		//֧��mp4�޸�ʱ,chunk������
		bAppendLastChunk = FALSE;
	}

	//stco
	{
		if(!bAppendLastChunk)
		{
			long pos = ftell(m_hFile);
			if(bJCOK)
			{
				//������audio/video sampleλ��tagMP4SampleHeader����
				m_lst_stco.push_back(pos+sizeof(tagMP4SampleHeader));
			}
			else
			{
				m_lst_stco.push_back(pos);
			}
		}
	}

	if(bJCOK)
	{
		tagMP4SampleHeader sh;
		ASSERT(sizeof(sh) == 8);
		sh.trakid=(BYTE)m_trakid;
		sh.duration = cbUlawData;//ע��:���ﱣ�������һ֡��duration,�޸�ʱҪ������һ֡
		sh.flags = 1;//Ŀǰֻ��һ����־λ,���ڱ�ʶ�ؼ�֡,audio��Ϊ�ؼ�֡
		//size������sh����Ĵ�С
		sh.size = cbFrame;
		
		//��shת��net˳��,����hostΪС��
		DWORD32 sz = sh.size;
		DWORD32 dur = sh.duration;
		sh.size = ((sz&0xFF0000)>>16) | (sz&0x00FF00) | ((sz&0xFF)<<16);
		sh.duration= ((dur&0xFF0000)>>16) | (dur&0x00FF00) | ((dur&0xFF)<<16);
		fwrite(&sh,1,sizeof(sh),m_hFile);
	}

	fwrite(pUlawData,1,cbUlawData,m_hFile);

	//stsz
	{
		if(m_lst_stsz.size()==0)
		{
			m_lst_stsz.push_back(cbUlawData);
		}
		else
		{
			ASSERT(cbUlawData==m_lst_stsz[0]);//audio packet should be the sample size
		}

		m_stsz_count++;
	}
	
	//stsc
	{
		tagMP4Node_stsc* ps = NULL;
		if(bAppendLastChunk)
		{
			ps = GetLastStscNode();
		}
		else
		{
			ps = new tagMP4Node_stsc;
			ps->FirstChunk=++m_nFirstChunk;
			ps->SampleDescriptionID=1;
			m_lst_stsc.push_back(ps);
		}

		ps->SamplesPerChunk++;

		//todo:stsc�����������Ż�,��samplesPerChunk��ͬ������chuck�ϲ���һ��chunk entry
	}

	//stts
	{
		if(m_dwFrameIndex>0)
		{
			OnNewDuration(cbUlawData);
		}
	}

	//audioû��stss

	m_dwFrameIndex++;
	return 0;
}

//OnAmrFrameHelper������OnAmrFrame���������������
int CMP4Trak::OnAmrFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	BYTE amrHeader = pFrame[0];
	BYTE ft = (amrHeader >> 3) & 0x0F;
	ASSERT(m_amr_ft == 0 || m_amr_ft == ft);
	m_amr_ft = ft;
	ASSERT(cbFrame == m_amr_packed_size[m_amr_ft] + 1);

	int time_scale_per_sample=160;

	m_dwTickRTPLast = dwRTPTimeStamp;

	//bJCOK��ʾ֧��mp4�޸�
	const BOOL32 bJCOK = (m_pMP4Write->GetMP4Flags() & MP4_FLAGS_JCOK);

	BOOL32 bAppendLastChunk = (m_pMP4Write->m_pTrakLastWrite==this);
	m_pMP4Write->m_pTrakLastWrite=this;
	if(bJCOK)
	{
		//֧��mp4�޸�ʱ,chunk������
		bAppendLastChunk = FALSE;
	}

	//stco
	{
		if(!bAppendLastChunk)
		{
			long pos = ftell(m_hFile);
			if(bJCOK)
			{
				//������audio/video sampleλ��tagMP4SampleHeader����
				m_lst_stco.push_back(pos+sizeof(tagMP4SampleHeader));
			}
			else
			{
				m_lst_stco.push_back(pos);
			}
		}
	}

	if(bJCOK)
	{
		tagMP4SampleHeader sh;
		ASSERT(sizeof(sh) == 8);

		sh.trakid=(BYTE)m_trakid;
		sh.duration = time_scale_per_sample;//ע��:���ﱣ�������һ֡��duration,�޸�ʱҪ������һ֡
		sh.flags = 1;//Ŀǰֻ��һ����־λ,���ڱ�ʶ�ؼ�֡,audio��Ϊ�ؼ�֡
		//size������sh����Ĵ�С
		sh.size = cbFrame;
		
		//��shת��net˳��,����hostΪС��
		DWORD32 sz = sh.size;
		DWORD32 dur = sh.duration;
		sh.size = ((sz&0xFF0000)>>16) | (sz&0x00FF00) | ((sz&0xFF)<<16);
		sh.duration= ((dur&0xFF0000)>>16) | (dur&0x00FF00) | ((dur&0xFF)<<16);
		fwrite(&sh,1,sizeof(sh),m_hFile);
                if(ferror(m_hFile))
                {
                    ERR("fwrite error!\n");
                    return -1;
                }
	}

	fwrite(pFrame,1,cbFrame,m_hFile);
        if(ferror(m_hFile))
        {
            ERR("fwrite error!\n");
            return -1;
        }
	
	//stsz
	{
		if(m_lst_stsz.size()==0)
		{
			m_lst_stsz.push_back(m_amr_packed_size[m_amr_ft]+1);
		}
		m_stsz_count++;
	}
	
	//stsc
	{
		tagMP4Node_stsc* ps = NULL;
		if(bAppendLastChunk)
		{
			ps = GetLastStscNode();
		}
		else
		{
			ps = new tagMP4Node_stsc;
			ps->FirstChunk=++m_nFirstChunk;
			ps->SampleDescriptionID=1;
			m_lst_stsc.push_back(ps);

			bAppendLastChunk=TRUE;//prepare for the second amr data frame
		}

		ps->SamplesPerChunk++;
	}

	//stts
	{
		if(m_dwFrameIndex>0)
		{
			OnNewDuration(time_scale_per_sample);//todo
		}
	}

	//audioû��stss
	m_dwFrameIndex++;
	return 0;
}

WORD CMP4Trak::m_amr_packed_size[8] = {12, 13, 15, 17, 19, 20, 26, 31,};
//todo:��ʱ���þ�����������֡,�����ٸĽ�
int CMP4Trak::OnAmrFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    int ret = 0;
	/*
	{
		static DWORD32 dwTimeStampLast = 0;
		if(dwTimeStampLast)
		{
			DWORD32 delta = dwRTPTimeStamp - dwTimeStampLast;
			DT("amr delta = %d",delta);
		}

		dwTimeStampLast = dwRTPTimeStamp;
	}
	//*/

	ASSERT(pFrame[0]==0xF0);

	//����amr������
	int nPack=1;
	{
		for(int i=1;i<cbFrame;i++)
		{
			if((pFrame[i]&0x80) == 0x00)
			{
				break;
			}
			nPack++;
		}
	}

	BYTE amrHeader = pFrame[nPack];
	BYTE ft = (amrHeader >> 3) & 0x0F;
	if(ft>=COUNT_OF(m_amr_packed_size))
	{
		//DW("invalid amr");
		return -1;
	}

	m_amr_ft = ft;

	if(cbFrame!= (1+m_amr_packed_size[m_amr_ft])*nPack + 1)
	{
		printf("[%s][%d]amr rtp pack length err", __FILE__, __LINE__);//ASSERT(FALSE);
		return -1;
	}

	DWORD32 dwDurationLastFrame = 0;
	if(m_dwFrameIndex>0)
	{
		//������һ֡��duration
		if(dwRTPTimeStamp > m_dwTickRTPLast)
		{
			dwDurationLastFrame  = dwRTPTimeStamp - m_dwTickRTPLast;
		}
		else
		{
#ifdef _MSC_VER
			if(abs((long)(0xFFFFFFFF - dwRTPTimeStamp))<8000*10)//��ֵ:10��
			{
				dwDurationLastFrame  = (0xFFFFFFFF - m_dwTickRTPLast) + dwRTPTimeStamp;
/* 20160602 Ӱ����� ��ʱ�رմ�ӡ
				DW("RTP timestamp����,m_dwTickRTPLast=0x%x,dwRTPTimeStamp=0x%x,dwDurationLastFrame=0x%x",
					m_dwTickRTPLast,dwRTPTimeStamp,dwDurationLastFrame);
*/
			}
			else
#endif
			{
				DW("��⵽�쳣amr rtp timestamp����:m_dwTickRTPLast=0x%x,dwRTPTimeStamp=0x%x",m_dwTickRTPLast,dwRTPTimeStamp);
				ASSERT(FALSE);
				//����Ĭ��ֵ
				dwDurationLastFrame=160;//amr
			}
		}
	}

	/*
	dwDurationLastFrame = ((dwDurationLastFrame+10)/(160*nPack))*(160*nPack);//����΢С���,amr

	int nBlock = dwDurationLastFrame/(160*nPack);//amr
	if(nBlock>1)
	{
		//˵������Ƶ��֡,��Ҫ��䶪ʧ�ľ�������
		BYTE mute[65];
		memset(mute,0x11,sizeof(mute));
		memcpy(mute,pFrame,3);//copy amr header
		for(int i=0;i<nBlock-1;i++)
		{
			static int nTimes=0;
			DT("insert mute audio frame,nTimes=%d",nTimes++);
			OnAmrFrameHelper(pFrame,cbFrame,m_dwTickRTPLast+(i+1)*320);//amr
		}
	}
	//*/

	{
		LPBYTE pamr = pFrame + 1 + nPack;//skip header
		for(int i=0;i<nPack;i++)
		{
			BYTE amr[32];//���Ϊ32
			amr[0]=amrHeader;
			memcpy(amr+1,pamr,m_amr_packed_size[m_amr_ft]);

			ret = OnAmrFrameHelper(amr,m_amr_packed_size[m_amr_ft]+1,m_dwTickRTPLast+160);
			pamr+=m_amr_packed_size[m_amr_ft];
		}
	}
	
	return ret;
}

//XiongWanPign 2009.11.11
//�Ѳ���,������video��������duration�ķ�����������Ƶ��ֻ֡��VLC��Ч,����QuickTimePlayer��Ч
//������ò��뾲��֡��������Ƶ��֡
int CMP4Trak::OnUlawFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	//LPBYTE pUlawData = pFrame;
	//UINT cbUlawData = cbFrame;

	DWORD32 dwDurationLastFrame = 0;
	if(m_dwFrameIndex>0)
	{
		//������һ֡��duration
		if(dwRTPTimeStamp > m_dwTickRTPLast)
		{
			dwDurationLastFrame  = dwRTPTimeStamp - m_dwTickRTPLast;
		}
		else
		{
#ifdef _MSC_VER
			if(abs((long)(0xFFFFFFFF - dwRTPTimeStamp))<8000*10)//��ֵ:10��
			{
				dwDurationLastFrame  = (0xFFFFFFFF - m_dwTickRTPLast) + dwRTPTimeStamp;
/* 20160602 Ӱ����� ��ʱ�رմ�ӡ
				//DW("RTP timestamp����,m_dwTickRTPLast=0x%x,dwRTPTimeStamp=0x%x,dwDurationLastFrame=0x%x",
					m_dwTickRTPLast,dwRTPTimeStamp,dwDurationLastFrame);
*/
			}
			else
#endif
			{
				DW("��⵽�쳣ulaw rtp timestamp����:m_dwTickRTPLast=0x%x,dwRTPTimeStamp=0x%x",m_dwTickRTPLast,dwRTPTimeStamp);
				ASSERT(FALSE);

				//����Ĭ��ֵ
				dwDurationLastFrame=cbFrame;
			}
		}
	}

	//dwDurationLastFrame��ʱ������cbFrame��������,�������΢С���
	dwDurationLastFrame = ((dwDurationLastFrame+10)/cbFrame)*cbFrame;

	int nBlock = dwDurationLastFrame/cbFrame;
	if(nBlock>1)
	{
		//˵������Ƶ��֡,�þ�����䶪ʧ������
		BYTE mute[4096];
		memset(mute,0xEF,sizeof(mute));
		LPBYTE pMute = mute;
		if(cbFrame>(int)sizeof(mute))
		{
			pMute = new BYTE[cbFrame];//remember to delete
			memset(pMute,0xEF,cbFrame);
		}

		for(int i=0;i<nBlock-1;i++)
		{
			static int nTimes=0;
			DT("insert mute audio frame,nTimes=%d",nTimes++);
			OnUlawFrameHelper(pFrame,cbFrame,m_dwTickRTPLast+(i+1)*cbFrame);
		}

		if(pMute != mute)
		{
			delete []pMute;
			pMute=NULL;
		}
	}

	OnUlawFrameHelper(pFrame,cbFrame,dwRTPTimeStamp);
	return 0;
}


//add 20160706
//AudioSampleEntry
//�ο�ffmpeg  Movenc.c  static int mov_write_audio_tag(AVFormatContext *s, AVIOContext *pb, MOVMuxContext *mov, MOVTrack *track)
long CMP4Trak::addAtom_mp4a()
{
	unsigned initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("mp4a");

	for (int i = 0; i < 6; i++) {
		size += addByte(0);
	}
	size += addHalfWord(0x0001);//data-reference-index.todo

	size += addHalfWord(0x0000);//Version
	size += addHalfWord(0x0000);//reserved level
	size += addWord(0x00000000);//Reserved

	//reserved for mp4/3gp
	size += addHalfWord(0x0002);
	size += addHalfWord(0x0010);
	size += addHalfWord(0x0000);

	size += addHalfWord(0x0000); //packet size(=0)
	size += addHalfWord(m_timescale);//sample_rate
	size += addHalfWord(0x0000);//Reserved

	//add esds
	size += addAtom_esds_aac();

	setWord(initFilePosn, size);
	return size;
}

//�ο�ffmpeg movenc.c static int mov_write_esds_tag(AVIOContext *pb, MOVTrack *track) // Basic 
long CMP4Trak::put_esds_descr(int tag, unsigned int length)
{
	int size = 0;
    int i = 3;
    size += addByte(tag);
	for (; i > 0; i--) {
		size += addByte((length >> (7 * i)) | 0x80);
	}
    size += addByte(length & 0x7F);
	return size;
}


int CMP4Trak::GetAacDecSpecificInfo(unsigned char *dsi)
{
	if (NULL == dsi) {
		return 0;
	}

	//AAC LC AudioObjectType is 2 (14496-3 table 1.1 Page 21)
	unsigned char object_type = 2;//AAC LC
	unsigned char sampling_frequency_index = GetAacSampleFrequencyIndex(m_timescale);
	dsi[0] = (object_type << 3) | (sampling_frequency_index >> 1);
	dsi[1] = ((sampling_frequency_index & 1) << 7) | (m_audio_channel << 3);
	return 2;
}

static const int AAC_DESCR_HENDER_LENGTH = 5;
long CMP4Trak::addAtom_esds_aac()
{
	unsigned initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("esds");

	size += addWord(0x00000000); //Version

	unsigned char decoder_specific_info[2] = { 0 };
	int dsi_len = GetAacDecSpecificInfo(decoder_specific_info);

	int decoder_specific_info_len = dsi_len ? AAC_DESCR_HENDER_LENGTH + dsi_len : 0;
	//ES descriptor 0x03
	//size += addByte(0x03);//ES desc
	//size += addHalfWord(0x8080);
	//size += addByte(0x80);
	//size += addByte(3+5+13+5+1+decoder_specific_info_len);//length �ο���ffmpeg��03��������ͷ�� 04 05�������ֵ��ܺ�
	size += put_esds_descr(0x03, 3 + AAC_DESCR_HENDER_LENGTH + 13 + decoder_specific_info_len + AAC_DESCR_HENDER_LENGTH + 1);

	size += addHalfWord(0x0002);//track id
	size += addByte(0x00);//flags = (no flags)


	//DecoderConfig descriptor 0x04
	//size += addByte(0x04); //DecoderConf desc
	//size += addHalfWord(0x8080);
	//size += addByte(0x80);
	//size += addByte(13 + decoder_specific_info_len);//length //
	size += put_esds_descr(0x04, 13 + decoder_specific_info_len);

	//Identify the syntax and semantics of the bitstream.
	/* http://www.mp4ra.org */
	/* ordered by muxing preference */
	size += addByte(0x40);//0x40 AAC
	size += addByte(0x15);//flags 0x15 audiostream 0x11 visualstream
	
	//24bit Buffersize DB set 0
	size += addByte(0x00);
	size += addHalfWord(0x0000);
	//maxbitrate
	size += addWord(m_audio_bitrate);
	//avg_bitrate
	size += addWord(m_audio_bitrate);


	//DecoderSpecific info descriptor 0x05
	//size += addByte(0x05);
	//size += addHalfWord(0x8080);
	//size += addByte(0x80);
	//size += addByte(decoder_specific_info_len - 5);
	size += put_esds_descr(0x05, dsi_len);
	for (int i = 0; i < dsi_len; i++) {
		size += addByte(decoder_specific_info[i]);
	}

	//SL descriptor 0x06
	size += put_esds_descr(0x06, 1);
	size += addByte(0x02);

	setWord(initFilePosn, size);
	return size;
}

int CMP4Trak::OnAacFrame(LPBYTE pFrame, int cbFrame, DWORD32 dwRTPTimeStamp)
{
	//��RTP��ȡaac��Ƶ֡��ʱ��ʵ��
	return 0;
}

int CMP4Trak::OnAacFrameHelper(LPBYTE pFrame, int cbFrame, DWORD32 dwRTPTimeStamp)
{
	LPBYTE pAacData = pFrame;
	const UINT cbAacData = cbFrame;
	m_dwTickRTPLast = dwRTPTimeStamp;

	const BOOL32 bRep = (m_pMP4Write->GetMP4Flags()&MP4_FLAGS_JCOK);
	BOOL32 bAppendLastChunk = (m_pMP4Write->m_pTrakLastWrite == this);
	m_pMP4Write->m_pTrakLastWrite = this;
	if (bRep) {
		bAppendLastChunk = FALSE;
	}

	//stco
	{
		if (!bAppendLastChunk) {
			long pos = ftell(m_hFile);
			if (bRep) {
				m_lst_stco.push_back(pos + sizeof(tagMP4SampleHeader));
			} else {
				m_lst_stco.push_back(pos);
			}
		}
	}

	if (bRep) {
		tagMP4SampleHeader sh;
		ASSERT(8 == sizeof(sh));
		sh.trakid=(BYTE)m_trakid;
		sh.duration = cbAacData;//ע��:���ﱣ�������һ֡��duration,�޸�ʱҪ������һ֡
		sh.flags = 1;//Ŀǰֻ��һ����־λ,���ڱ�ʶ�ؼ�֡,audio��Ϊ�ؼ�֡
		//size������sh����Ĵ�С
		sh.size = cbFrame;
		
		//��shת��net˳��,����hostΪС��
		DWORD32 sz = sh.size;
		DWORD32 dur = sh.duration;
		sh.size = ((sz&0xFF0000)>>16) | (sz&0x00FF00) | ((sz&0xFF)<<16);
		sh.duration= ((dur&0xFF0000)>>16) | (dur&0x00FF00) | ((dur&0xFF)<<16);
		fwrite(&sh,1,sizeof(sh),m_hFile);
	}
	fwrite(pAacData, 1, cbAacData, m_hFile);
#if 1
	//stsz
	{
		//aac֡���Ƕ����ģ�����Ҫ��¼ÿ��sample size
		m_lst_stsz.push_back(cbFrame);//h.264ʱ����ǰ���д��dwFrameLen,����Ҫ����4�ֽ�
		/*
		if (0 == m_lst_stsz.size()) {
			m_lst_stsz.push_back(cbAacData);
		} else {
			ASSERT(cbAacData == m_lst_stsz[0]);
		}
		++m_stsz_count;
		*/
	}

	//stsc
	{
		tagMP4Node_stsc *ps = NULL;
		if (bAppendLastChunk) {
			ps = GetLastStscNode();
		} else {
			ps = new tagMP4Node_stsc;
			ps->FirstChunk = ++m_nFirstChunk;
			ps->SampleDescriptionID = 1;
			m_lst_stsc.push_back(ps);
		}
		ps->SamplesPerChunk++;
	}

	//stts
	{
		if (m_dwFrameIndex > 0) {
			OnNewDuration(1024);//����Ƶ��timescaleΪ��Ƶ��ÿ���ӵ�sampleʱ��һ֡��duration��ÿ֡�Ĳ�������
		}
	}
	m_dwFrameIndex++;
#else
	//stsz
	{
		//��Ϊÿ��sample size������ͬ,����Ҫ��¼ÿ��sample size
		m_lst_stsz.push_back(cbFrame+(bH264?sizeof(cbFrame):0));//h.264ʱ����ǰ���д��dwFrameLen,����Ҫ����4�ֽ�
	}

	//stsc
	{
		tagMP4Node_stsc* ps = NULL;
		if(bAppendLastChunk)
		{
			ps = GetLastStscNode();
		}
		else
		{
			ps = new tagMP4Node_stsc;
			ps->FirstChunk=++m_nFirstChunk;
			ps->SampleDescriptionID=1;
			m_lst_stsc.push_back(ps);
		}

		ps->SamplesPerChunk++;
	}

	//stts
	if(m_dwFrameIndex>0)
	{
		//��Ƶ֡��durationÿ������ͬ,���ҵ�ǰ֡��durationֻ�ܵȵ��Ѿ��յ���һ֡ʱ�������
		OnNewDuration(dwDurationLastFrame);
	}

	//realplayerҪ����������֡,�������ʾ������
	m_dwFrameIndex++;
#endif

	return 0;
}

void CMP4Trak::OnNewDuration(DWORD32 dwDurationLastFrame)
{
//#ifdef _DEBUG
	if(IsVideoTrak())
	{
		if(dwDurationLastFrame>90000*10)
		{
			//DW("######### dwDurationLastFrame=%d",dwDurationLastFrame);
			//set as default
			dwDurationLastFrame = 90000/25;
		}
	}
//#endif
	
	m_duration += dwDurationLastFrame;

	tagMP4Node_stts *ps=GetLastSttsNode();
	if(ps && ps->SampleDuration == dwDurationLastFrame)
	{
		ps->SampleCount++;
	}
	else
	{
		ps = new tagMP4Node_stts;
		ps->SampleDuration=dwDurationLastFrame;
		ps->SampleCount++;
		m_lst_stts.push_back(ps);
	}
}

int CMP4Trak::WriteVideoFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	if(!m_hFile)
	{
		return -1;
	}

	const BOOL32 bH264=IsH264();
	
	if(bH264)
	{
		const BOOL32 bHasH264Prefix= (bH264 && (pFrame[0] == 0x00 && pFrame[1] == 0x00 && pFrame[1] == 0x00 && pFrame[3] == 0x01));
        //��������DecDraw�ṩ��h.264������0x00000001ͷ
		if(bHasH264Prefix)
		{
			//ȥ��0x00000001ͷ
			pFrame+=4;
			cbFrame-=4;
		}
		
		/*
		BYTE naluType = (pFrame[0]&0x1F);
		if(
			naluType==7 
			|| naluType==8
			)
		{
			if(bHasWrite_sps_pps)
				return 0;

			//¼����Ҫ����sps,pps
			//���������sps,pps,quicktimeplayer����ʱ��������,vlc�򲥷�����.
			m_bH264IFrame=TRUE;
			return 0;
		}
		//*/
	}
	else if(IsMjpg())
	{

	}
	else
	{
		//����mpeg4��1֡������I֡
		if(m_dwFrameIndex==0 && !IsIFrame(pFrame))
		{
			return 0;
		}
	}

	DWORD32 dwDurationLastFrame = 0;
	if(m_dwFrameIndex>0)
	{
		//������һ֡��duration
		if(dwRTPTimeStamp >= m_dwTickRTPLast)
		{
			dwDurationLastFrame  = dwRTPTimeStamp - m_dwTickRTPLast;
		}
		else
		{
			dwDurationLastFrame  = (0xFFFFFFFF - m_dwTickRTPLast) + dwRTPTimeStamp;

			DW("RTP timestamp loop back,m_dwTickRTPLast=%u,dwRTPTimeStamp=%u,dwDurationLastFrame=%u",
				m_dwTickRTPLast,dwRTPTimeStamp,dwDurationLastFrame);

		}
	}

	//DT("rtp ts=%d,last ts=%d",dwRTPTimeStamp,dwDurationLastFrame);

	m_dwTickRTPLast = dwRTPTimeStamp;

	//bJCOK��ʾ֧��mp4�޸�
	const BOOL32 bJCOK = (m_pMP4Write->GetMP4Flags() & MP4_FLAGS_JCOK);

	//�ж��Ƿ���Ҫ�½�chunk
	//bAppendLastChunkΪTRUEʱ,��ʾ��һ��write sampleҲ�Ǵ�trak������,��˲���Ҫ���·�chunk
	BOOL32 bAppendLastChunk = (m_pMP4Write->m_pTrakLastWrite==this);
	m_pMP4Write->m_pTrakLastWrite=this;
	if(bJCOK)
	{
		//֧��mp4�޸�ʱ,chunk������
		bAppendLastChunk = FALSE;
	}

	//stco
	{
		if(!bAppendLastChunk)
		{
			long pos = ftell(m_hFile);
			if(bJCOK)
			{
				//������audio/video sampleλ��tagMP4SampleHeader����
				m_lst_stco.push_back(pos+sizeof(tagMP4SampleHeader));
			}
			else
			{
				m_lst_stco.push_back(pos);
			}
		}
	}

	BOOL32 bIFrame = IsIFrame(pFrame);
	if(m_bH264IFrame)
	{
		bIFrame =TRUE;
		m_bH264IFrame=FALSE;
	}
	else if(IsMjpg())
	{
		bIFrame =TRUE;
	}

	if(bJCOK)
	{
		tagMP4SampleHeader sh;
		ASSERT(sizeof(sh) == 8);

		sh.trakid=(BYTE)m_trakid;
		sh.duration = dwDurationLastFrame;//ע��:���ﱣ�������һ֡��duration,�޸�ʱҪ������һ֡
		sh.flags = bIFrame?1:0;//Ŀǰֻ��һ����־λ,���ڱ�ʶ�ؼ�֡
		//size������sh����Ĵ�С
		sh.size = cbFrame + (bH264?sizeof(cbFrame):0);//h.264ʱ����ǰ���д��dwFrameLen,����Ҫ����4�ֽ�
		
		//��shת��net˳��,����hostΪС��
		DWORD32 sz = sh.size;
		DWORD32 dur = sh.duration;
		sh.size = ((sz&0xFF0000)>>16) | (sz&0x00FF00) | ((sz&0xFF)<<16);
		sh.duration= ((dur&0xFF0000)>>16) | (dur&0x00FF00) | ((dur&0xFF)<<16);
		fwrite(&sh,1,sizeof(sh),m_hFile);
    	if(ferror(m_hFile))
        {
        	ERR("A write error has occured!\n");
            return -1;
    	}
	}

	if(bH264)
	{
		DWORD32 dwLen=htonl(cbFrame);
		fwrite(&dwLen,1,sizeof(dwLen),m_hFile);//h264����һ֡��С�������ֽ��򱣴�������֡ͷ
        	if(ferror(m_hFile))
        	{
            	ERR("A write error has occured!\n");
                return -1;
            }
	}

	int ret = fwrite(pFrame,1,cbFrame,m_hFile);//д��Ƶ���ݵ��ļ�
	//ASSERT(ret == cbFrame);
	if(ferror(m_hFile))
	{
    	ERR("A write error has occured!\n");
        return -1;
    }

	//stsz
	{
		//��Ϊÿ��sample size������ͬ,����Ҫ��¼ÿ��sample size
		m_lst_stsz.push_back(cbFrame+(bH264?sizeof(cbFrame):0));//h.264ʱ����ǰ���д��dwFrameLen,����Ҫ����4�ֽ�
	}

	//stsc
	{
		tagMP4Node_stsc* ps = NULL;
		if(bAppendLastChunk)
		{
			ps = GetLastStscNode();
		}
		else
		{
			ps = new tagMP4Node_stsc;
			ps->FirstChunk=++m_nFirstChunk;
			ps->SampleDescriptionID=1;
			m_lst_stsc.push_back(ps);
		}

		ps->SamplesPerChunk++;
	}

	//stts
	if(m_dwFrameIndex>0)
	{
		//��Ƶ֡��durationÿ������ͬ,���ҵ�ǰ֡��durationֻ�ܵȵ��Ѿ��յ���һ֡ʱ�������
		OnNewDuration(dwDurationLastFrame);
	}

	//realplayerҪ����������֡,�������ʾ������
	m_dwFrameIndex++;

	//stss
	{
		if(bIFrame)
		{
			m_lst_stss.push_back(m_dwFrameIndex);
		}
	}

	return 0;
}

//CMP4Write��ֹͣ¼��ʱ���������trak��PreClose
//trak������PreClose()������β����
//���ڵ���β��������:�����һ֡��durationд��ȥ
int CMP4Trak::PreClose()
{
	if(m_dwFrameIndex<=0)
	{
		return 0;
	}
	
	ASSERT(m_lst_stsz.size()>0);

	//�ٶ����һ֡Ϊ����״̬
	if(IsVideoTrak())
	{
		OnNewDuration(m_timescale/25);
	}
	else
	{
		if ((m_type == MP4_AUDIO_ULAW) || (MP4_AUDIO_ALAW == m_type))
		{
			//ulaw
			OnNewDuration(m_lst_stsz[0]);
		}
		else if (m_type == MP4_AUDIO_AMR_NB)
		{
			OnNewDuration(160);//amr
		} else if (m_type == MP4_AUDIO_AAC) {
			//add aac 20160709
			OnNewDuration(1024);
		}
	}
	
	return 0;
}

BOOL32 CMP4Trak::IsIFrame(LPBYTE pData)
{
	ASSERT(IsVideoTrak());

	if(IsH264())
	{
		return IsH264IFrame(pData);
	}

	return IsMpeg4IFrame(pData);
}

CMP4Write::CMP4Write()
{
	m_hFile =  NULL;
	m_dwFlags=0;
	m_bHasWriteFtyp=FALSE;

	m_pTrakLastWrite=0;
	m_dwTickStartRecord=0;
	m_pos_mdat=0;
	m_pos_jcok=0;
	m_pos_moov=0;

	m_dwMP4Flags = 0;
}

CMP4Write::~CMP4Write()
{
	if(m_hFile)
	{
		fclose(m_hFile);
		m_hFile=0;
	}

	Empty();
}

int CMP4Write::WriteHeader()
{
	if(!m_bHasWriteFtyp)
	{
		// Use the current time as the file's creation and modification
		// time.  Use Apple's time format: seconds since January 1, 1904
		struct timeval		m_fStartTime;
		unsigned			m_fAppleCreationTime;;
		gettimeofday(&m_fStartTime);
		m_fAppleCreationTime = m_fStartTime.tv_sec - 0x83dac000;//0x83dac000ʲô��˼������ ƻ����1904�꿪ʼ��ʱ�� ����1904-1900 ��= 0x784CE00 

		{
			//ע��ftypҪ��moovǰ��,����quicktime���ܲ���
			addAtom_ftyp();

			//bJCOK��ʾ֧��mp4�޸�
			const BOOL32 bJCOK = (GetMP4Flags() & MP4_FLAGS_JCOK);
			if(bJCOK)
			{
				//���jabsco��չJCOK box
				m_pos_jcok=ftell(m_hFile);
				addAtom_JCOK();
			}

			m_pos_mdat=ftell(m_hFile);
			addAtomHeader("mdat");
		}

		m_bHasWriteFtyp = TRUE;
		
		fflush();
	}

	return 0;
}

//ֱ�ӵ��ײ�api fflush��ȷ���ļ�������д��洢�豸
int CMP4Write::fflush()
{
	int ret = EOF;

	if(m_hFile)
	{
		ret = ::fflush(m_hFile);
#ifndef _MSC_VER
		sync();
#endif
	}

	return ret;
}

//Attach��Detach�����޸�mp4ʹ��
int CMP4Write::Attach(FILE *hFile)
{
	if(m_hFile)
	{
		return -1;
	}

	m_hFile = hFile;
	m_dwFlags |= MP4F_ATTACH;

	return 0;
}

void CMP4Write::Detach()
{
	m_hFile = NULL;
	m_dwFlags = 0;
}

//Ĭ�ϲ�֧���޸�¼��
//Ҫ�����޸�¼��,dwFlags������ΪMP4_FLAGS_JCOK,�ɲ���ϵͳ����fflush
//�������dwFlags����MP4_FLAGS_AUTO_FFLUSH,��libmp4�Զ�fflushÿһ֡����,Ч�ʿ����Ե�
int CMP4Write::Open(const char * pszFile,DWORD32 dwFlags)
{
	ASSERT(!m_hFile);
	if(m_hFile)
	{
		return -1;
	}
	else
	{
		m_hFile = fopen(pszFile,"wb");
		if (NULL == m_hFile) {
			perror("CMP4Write create file:");
			return -1;
		}
	}

	//ע��:MP4_FLAGS_JCOKΪ��ʱ����MP4_FLAGS_AUTO_FFLUSH������,CMP4Write���Զ�����MP4_FLAGS_JCOK
	if(dwFlags & MP4_FLAGS_AUTO_FFLUSH)
	{
		dwFlags |= MP4_FLAGS_JCOK;
	}

	m_dwMP4Flags = dwFlags;

	return m_hFile?0:-1;
}

//Repair()�Ĳ�����CMP4Write::Close()���
int CMP4Write::Repair()
{
	if(!m_hFile || !IsRepairMode())
	{
		ASSERT(FALSE);
		return -1;
	}

	if(m_pos_mdat)
	{
		long pos = ftell(m_hFile) - m_pos_mdat;
		setWord(m_pos_mdat,pos);
		
		if(m_pos_moov)
		{
			fseek(m_hFile,m_pos_moov,SEEK_SET);
		}

		pos = ftell(m_hFile);
#ifdef _MSC_VER
		int ret = _chsize(m_hFile->_file,pos);//�ض��ļ�
#else
		int ret = ftruncate(fileno(m_hFile), pos);
#endif
		addAtom_moov();

		if(m_pos_jcok)
		{
			long pos = ftell(m_hFile);
			fseek(m_hFile,m_pos_jcok+4,SEEK_SET);
			fwrite("JCOR",1,4,m_hFile);//���Ϊjabsco Repair
			fflush();
		}
	}
	return 0;
}

int CMP4Write::Close()
{
	ASSERT(!IsRepairMode());//�޸�ģʽʱ�����Repair()
	
	if(m_hFile)
	{
		fflush();
		if(m_pos_mdat)
		{
			long pos = ftell(m_hFile) - m_pos_mdat;
			setWord(m_pos_mdat,pos);
			
			{
				//notify all trak to preclose
				for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin();it!=m_arrTrak.end();it++)
				{
					CMP4Trak* pTrak = *it;
					pTrak->PreClose();
				}
			}
			
			addAtom_moov();

			const BOOL32 bJCOK = (GetMP4Flags() & MP4_FLAGS_JCOK);
			if(bJCOK)
			{
				if(m_pos_jcok)
				{
					long pos = ftell(m_hFile);
					fseek(m_hFile,m_pos_jcok+4,SEEK_SET);
					fwrite("JCOK",1,4,m_hFile);
					fseek(m_hFile,pos,SEEK_SET);
					fflush();
				}
			}
		}

		fclose(m_hFile);
		m_hFile = NULL;

		m_dwFlags=0;
		m_bHasWriteFtyp=FALSE;

		m_pTrakLastWrite=0;
		m_dwTickStartRecord=0;
		m_pos_mdat=0;
		m_pos_jcok=0;
	}

	Empty();

	return 0;
}

void CMP4Write::Empty()
{
	{
		for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin();it!=m_arrTrak.end();it++)
		{
			CMP4Trak* pTrak = *it;
			delete pTrak;
		}
		m_arrTrak.clear();
	}
}

long CMP4Write::addAtom_ftyp()
{
	BOOL32 bEnableQuickTime = TRUE;//ΪTRUEʱ,¼������amr.mp4����quicktime�в���,
	
	if(m_dwFlags & MP4F_G711)
	{
		bEnableQuickTime = FALSE;
	}

	//bEnableQuickTime=FALSE;//xwpcom 2009.11.30 test mjpg

	if(bEnableQuickTime)
	{
		//quicktime��realplayer������������amr
		
		long initFilePosn = ftell(m_hFile);
		unsigned size = addAtomHeader("ftyp");
		size += add4ByteString("3gp5");//mp42,3gp5
		size += addWord(0x00000000);
		size += add4ByteString("3gp5");//mp42
		size += add4ByteString("isom");
		//size += add4ByteString("qt  ");//xwpcom 2009.10.28 for quicktime
		//��trackͷ�����track�ĳ�����Ϣ
		setWord(initFilePosn, size);
		return size;
	}
	else
	{
		//realplayer����������amr,quicktimeΪ����
		
		long initFilePosn = ftell(m_hFile);
		unsigned size = addAtomHeader("ftyp");
		size += add4ByteString("mp42");//mp42,3gp5
		size += addWord(0x00000000);
		size += add4ByteString("mp42");//mp42
		size += add4ByteString("isom");
		size += add4ByteString("qt  ");//xwpcom 2009.10.28 for quicktime
		setWord(initFilePosn, size);
		return size;
	}
}

CMP4Trak * CMP4Write::FindVideoTrak()
{
	for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin();it!=m_arrTrak.end();it++)
	{
		CMP4Trak* pTrak = *it;
		if(pTrak->IsVideoTrak())
		{
			return pTrak;
		}
	}

	return NULL;
}

CMP4Trak * CMP4Write::FindAudioTrak()
{
	for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin();it!=m_arrTrak.end();it++)
	{
		CMP4Trak* pTrak = *it;
		if(pTrak->IsAudioTrak())
		{
			return pTrak;
		}
	}

	return NULL;
}

//mpeg4ʱtrackid��96
int CMP4Write::InitVideo(DWORD32 type,CSize szVideo,const char* sprop_parameter_sets)
{
	//only support a single video trak
	CMP4Trak *pt = FindVideoTrak();

	if(pt)
	{
		printf("[%s][%d]only support one video trak\n", __FILE__, __LINE__);
		return -1;
	}

	ASSERT(m_hFile);

	pt = new CMP4Trak;
	pt->m_pMP4Write=this;
	pt->m_hFile=m_hFile;
	pt->InitVideoTrakInfo(type,"vide",szVideo,sprop_parameter_sets);
	m_arrTrak.push_back(pt);
	return 0;
}

//audioΪulawʱdwType����0
//audioΪamrdwType����97
int CMP4Write::InitAudio(DWORD32 dwType, int sample, int bit_per_sample, int channel)
{
	//only support a single audio trak
	CMP4Trak *pt = FindAudioTrak();

	if(pt)
	{
		ASSERT(FALSE);
		return -1;
	}

	ASSERT(m_hFile);

	pt = new CMP4Trak;
	pt->m_pMP4Write=this;
	pt->m_hFile=m_hFile;
	pt->InitAudioTrakInfo(dwType, "soun", sample, bit_per_sample, channel);
	m_arrTrak.push_back(pt);

	//��ƵΪG711ʱ��Ҫ���⴦��
	if((MP4_AUDIO_ULAW == dwType)||(MP4_AUDIO_ALAW == dwType)) {
		m_dwFlags |= MP4F_G711;
	}

	return 0;
}

struct tag_atom_jcox_trak
{
	//trakidֵ����� ԭ����RTP���� ��Ϊ�ڲ������ Enum eMp4AudioType �� eVideoType 20160730
	DWORD32 trakid;
	BYTE	handlerType[4];
	DWORD32 width;
	DWORD32 height;
	DWORD32 timescale;
	DWORD32 fmtp_spropparametersets_len;
	//...�����fmtp_spropparametersets_len���ֽڵ�h.264����
};

//addAtom_JCOK()����ͬaddAtom_moov()
long CMP4Write::addAtom_JCOK()
{
	/*
	jcox��Ҫ������Ϣ�����ؽ�moov:
	trakcount:4bytes
		//������trakcount��trakinfo
		trakid:4bytes
		handleType:4 bytes char("vide" or "soun")
		width
		height
		timescale
		sprop_parameter_sets_len;//��ʾ������ŵ�sprop_parameter_sets���ֽ���,һ��Ϊ0,ֻ��h.264ʱ��Ч
		BYTE sprop_parameter_sets[sprop_parameter_sets_len];
	//*/

    long initFilePosn = ftell(m_hFile);
	//JCOKʵ������һ���յ�moov box,���Ե���ʱ���Խ�JCOX��CJOK��Ϊmoov,��������������.
	unsigned size = addAtomHeader("JCOX");//�����ر�mp4¼���ļ�ʱ���дΪJCOK
	DWORD32 dwVerFlags=0;//�汾��,Ŀǰ��Ϊ0
	size += addWord(dwVerFlags);
	
	DWORD32 dwTrackCount = m_arrTrak.size();
	size += addWord(dwTrackCount);
	for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin();it!=m_arrTrak.end();it++)
	{
		CMP4Trak *pt = *it;
		//trakidֵ����� ԭ����RTP���� ��Ϊ�ڲ������ Enum eMp4AudioType �� eVideoType 20160730
		size += addWord(pt->m_type);

		size += add4ByteString(pt->m_handlerType);
		size += addWord(pt->m_szVideo.cx);
		size += addWord(pt->m_szVideo.cy);
		size += addWord(pt->m_timescale);

		if(pt->m_fmtp_spropparametersets)
		{
			size += addWord(strlen(pt->m_fmtp_spropparametersets));
			size += addArbitraryString(pt->m_fmtp_spropparametersets,FALSE);
		}
		else
		{
			size += addWord(0);
		}
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Write::addAtom_moov()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("moov");

	size += addAtom_mvhd();

	{
		for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin();it!=m_arrTrak.end();it++)
		{
			CMP4Trak *pt = *it;
			if(pt->GetFrameCount()>0)//����û���κ����ݵ�trak
			{
				size += pt->addAtom_trak();
			}
		}
	}

	setWord(initFilePosn, size);
	return size;
}

long CMP4Write::addAtom_mvhd()
{
    long initFilePosn = ftell(m_hFile);
	unsigned size = addAtomHeader("mvhd");

	struct timeval		m_fStartTime;
	unsigned			m_fAppleCreationTime;;
	gettimeofday(&m_fStartTime);
	m_fAppleCreationTime = m_fStartTime.tv_sec - 0x83dac000;

	size += addWord(0x00000000); // Version + Flags
	size += addWord(m_fAppleCreationTime); // Creation time
	size += addWord(m_fAppleCreationTime); // Modification time

	// For the "Time scale" field, use the largest RTP timestamp frequency
	// that we saw in any of the subsessions.
	DWORD32 timescale = 1;
	DWORD32 duration =  1;
	CMP4Trak *pTrakA = FindAudioTrak();
	CMP4Trak *pTrakV = FindVideoTrak();

	if(pTrakV && pTrakV->GetFrameCount()>0)
	{
		timescale = pTrakV->m_timescale;
		duration = pTrakV->m_duration;
	}
	else if(pTrakA && pTrakA->GetFrameCount()>0)
	{
		timescale = pTrakA->m_timescale;
		duration = pTrakA->m_duration;
	}

	{
		double rate = 1.0*timescale/600;
		timescale=600;
		duration /= rate;
	}

	size += addWord(timescale);

	size += addWord(duration); // Duration

	size += addWord(0x00010000); // Preferred rate
	size += addWord(0x01000000); // Preferred volume + Reserved[0]
	size += addZeroWords(2); // Reserved[1-2]
	size += addWord(0x00010000); // matrix top left corner
	size += addZeroWords(3); // matrix
	size += addWord(0x00010000); // matrix center
	size += addZeroWords(3); // matrix
	size += addWord(0x40000000); // matrix bottom right corner
	size += addZeroWords(6); // various time fields
	size += addWord(2009);// Next track ID,����Ϊ����ֵ,���͵�ǰ��video/audio trackid��ͬ�Ϳ�����

	setWord(initFilePosn, size);
	return size;
}

int CMP4Write::WriteAudioFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp,BOOL32 bRTPHeader)
{
	if(!m_hFile)
	{
		//ASSERT(FALSE);
		return -1;
	}

	if(!m_bHasWriteFtyp)
	{
		WriteHeader();
	}

	if(m_dwTickStartRecord==0)
	{
		if(FindVideoTrak())//���ͬʱ¼��Ƶ,��ȴ���һ��I֡��ſ�ʼ¼audio
			return 0;
#ifdef _MSC_VER
		m_dwTickStartRecord = GetTickCount();
#else
		time_t tm;
		m_dwTickStartRecord = time(&tm);
#endif
	}

	int ret =0;
	CMP4Trak *pt = FindAudioTrak();
	if(pt)
	{
		if(bRTPHeader)
		{
			ret = pt->WriteAudioFrame(pFrame,cbFrame,dwRTPTimeStamp);
		}
		else
		{
			ret = pt->WriteAudioFrameEx(pFrame,cbFrame,dwRTPTimeStamp);
		}
	}
	
	if(ret)
	{
		//Close();
	}

	return ret;
}


//pFrameָ��һ��������audio frame
//cbFrame��audio frame�ֽ���
//dwRTPTimeStampΪrtp timestamp
int CMP4Write::WriteAudioFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	return WriteAudioFrameHelper(pFrame,cbFrame,dwRTPTimeStamp,TRUE);
}

int CMP4Write::WriteAudioFrameEx(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	return WriteAudioFrameHelper(pFrame,cbFrame,dwRTPTimeStamp,FALSE);
}

//pFrameָ��һ��������video frame
//cbFrame��video frame�ֽ���
//dwRTPTimeStampΪrtp timestamp
int CMP4Write::WriteVideoFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	if(!m_hFile)
	{
		//ASSERT(FALSE);
		return -1;
	}

	if(!m_bHasWriteFtyp)
	{
		WriteHeader();
	}

	CMP4Trak *pt = FindVideoTrak();
	ASSERT(pt);

	if(m_dwTickStartRecord==0)
	{
		if(!pt->IsH264() && !pt->IsMjpg() && !pt->IsIFrame(pFrame))
		{
			return 0;
		}
		
#ifdef _MSC_VER
		m_dwTickStartRecord = GetTickCount();
#else
		time_t tm;
		m_dwTickStartRecord = time(&tm);
#endif
	}
	
	int ret =0;
	if(pt)
	{
		ret = pt->WriteVideoFrame(pFrame,cbFrame,dwRTPTimeStamp);
	}

	if(ret)
	{
		//Close();
	}

	return ret;
}

int CMP4Write::WriteFrame(BOOL32 bVideo,LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
#ifdef _DEBUG
	/*
	{
		DT("WriteFrame(bVideo=%d,pFrame=0x%02x%02x%02x%02x,0x%02x%02x%02x%02x,cbFrame=%d",
			bVideo,
			pFrame[0],pFrame[1],pFrame[2],pFrame[3],
			pFrame[4],pFrame[5],pFrame[6],pFrame[7],
			cbFrame
			);
	}
	//*/
#endif
	
	int ret = bVideo?WriteVideoFrame(pFrame,cbFrame,dwRTPTimeStamp):WriteAudioFrame(pFrame,cbFrame,dwRTPTimeStamp);

	if(m_dwMP4Flags & MP4_FLAGS_AUTO_FFLUSH)
	{
		fflush();
	}

	return ret;
}

int CMP4Write::WriteFrameEx(BOOL32 bVideo,LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	int ret = bVideo?WriteVideoFrame(pFrame,cbFrame,dwRTPTimeStamp):WriteAudioFrameEx(pFrame,cbFrame,dwRTPTimeStamp);
	
	if(m_dwMP4Flags & MP4_FLAGS_AUTO_FFLUSH)
	{
		fflush();
	}
	
	return ret;
}

BOOL32 CMP4Write::GetFileSize(LONGLONG* lpFileSize)
{
	if (NULL == m_hFile) {
		return FALSE;
	}
#ifdef _MSC_VER	
	struct _stati64 buffer;
	int ret = _fstati64(m_hFile->_file,&buffer);
	if(ret==0 && lpFileSize)
	{
		*lpFileSize=buffer.st_size;
	}
	return ret==0;
#else
	long pos = ftell(m_hFile);
	fseek(m_hFile,0,SEEK_END);
	long size = ftell(m_hFile);
	fseek(m_hFile,pos,SEEK_SET);
	*lpFileSize=size;
	return TRUE;
#endif
}

DWORD32 CMP4Write::GetRecordTick()
{
	if(!m_dwTickStartRecord)
		return 0;
#ifdef _MSC_VER
	return GetTickCount() - m_dwTickStartRecord;
#else
		time_t tm;
		time(&tm);
		return tm - m_dwTickStartRecord;
#endif

	
}

#ifdef _LINUX_APP
#include "test_record.h"
int main()
{
	{
		CMP4Record rec;
		rec.Test();
		return 0;
	}

	
	/*
	//parse frame
	{
		CMP4Read mp4Read;
		int ret = mp4Read.Open("./t.mp4");
		DT("ret = %d",ret);
		mp4Read.Seek(0);

		int cbVideoBuf=512*1024;
		LPBYTE pVideoBuf = new BYTE[cbVideoBuf];
		int nFrame=0;
		while(1)
		{
			int cbRead=0;
			DWORD32 duration =0;
			int ret = mp4Read.ReadVideoFrame(pVideoBuf,cbVideoBuf,cbRead,duration);

			if(cbRead==0)
			{
				break;
			}
		
			DT("frame[%06d]=%d,hex=0x%02x%02x%02x%02x",nFrame,cbRead,
				pVideoBuf[0],pVideoBuf[1],pVideoBuf[2],pVideoBuf[3]
				);

			nFrame++;
		}
	}
	//*/
	
	return 0;
}
#endif

CMP4Read::CMP4Read()
{
	m_iFile=NULL;

	m_cbFile=0;
	m_pRootAtom=NULL;

	m_pVideoTrak = NULL;
	m_pAudioTrak = NULL;
}

CMP4Read::~CMP4Read()
{
	Close();
}

//˵��:CMP4Read���Զ�delete iReadFile
int CMP4Read::Attach(IReadFile *iReadFile)
{
	if(m_iFile || !iReadFile || !iReadFile->IsOpen())
	{
		ASSERT(FALSE);

		return -1;
	}

	m_iFile = iReadFile;
	return OpenHelper();
}

int CMP4Read::Open(const char * pszFile)
{
	ASSERT(!m_iFile);
	
	IReadFile * iReadFile = CReadFile::CreateInstance(pszFile,"rb");
	if(!iReadFile)
	{
		return -1;
	}

	m_iFile = iReadFile;
	return OpenHelper();
}

int CMP4Read::OpenHelper()
{
	int ret = CheckValid();
	if(ret)
	{
		DW("invalid mp4 file");
		Close();
		return ret;
	}

	ret = ParseAtom();//�˴��д�����Ҫ�޸ġ�
	if (ret == MP4_FILE_ERROR){
		DW("invalid mp4 file");
		Close();
		return ret;
	}

	//������ȡ��ͬ��trak atom
	{
		int ncTrak = GetTrakCount();
		if(ncTrak<=0 || ncTrak>2)
		{
			DW("ncTrak=%d",ncTrak);
			Close();
			return -1;
		}
	}

	//�ռ�audio/video info
	{
		m_pVideoTrak = FindVideoTrak();
		m_pAudioTrak = FindAudioTrak();

		m_vi.video_type=eVideoType_None;

		if(m_pVideoTrak)
		{
			CMP4Read_atom_tkhd *pAtom_tkhd = (CMP4Read_atom_tkhd *)m_pVideoTrak->FindChildAtom("tkhd");
			if(pAtom_tkhd)
			{
				m_vi.bValid=TRUE;
				m_vi.type=96;//todo
				m_vi.width=pAtom_tkhd->m_width;
				m_vi.height=pAtom_tkhd->m_height;
				m_vi.duration=pAtom_tkhd->m_duration;
				m_vi.frame_count=m_pVideoTrak->GetFrameCount();

				//����������ֵ
				m_vi.fps=25;
				m_vi.timescale=90000;

				//�ټ���ʵ��֡��
				CMP4Read_atom_mdhd *pAtom_mdhd = (CMP4Read_atom_mdhd *)m_pVideoTrak->FindChildAtom("mdia.mdhd");
				if(pAtom_mdhd && pAtom_mdhd->m_timescale!=0)
				{
					m_vi.timescale=pAtom_mdhd->m_timescale;
					m_vi.duration=pAtom_mdhd->m_duration;
					double second = 1.0 * pAtom_mdhd->m_duration/pAtom_mdhd->m_timescale;
					m_vi.fps = m_vi.frame_count/second;
				}

				//DT("m_vi.fps =%.1f fps",m_vi.fps );

				if(m_pVideoTrak->FindChildAtom("mdia.minf.stbl.stsd.mp4v"))
				{
					m_vi.video_type=eVideoType_Mpeg4;
				}
				else if(m_pVideoTrak->FindChildAtom("mdia.minf.stbl.stsd.avc1"))
				{
					m_vi.video_type=eVideoType_H264;
				}
				else if(m_pVideoTrak->FindChildAtom("mdia.minf.stbl.stsd.mjpa"))
				{
					m_vi.video_type=eVideoType_Mjpg;
				}
			}
		}
		
		m_ai.bValid = FALSE;
		if(m_pAudioTrak)
		{
			CMP4Read_atom *pAtom_ulaw = m_pAudioTrak->FindChildAtom("mdia.minf.stbl.stsd.ulaw");
			CMP4Read_atom *pAtom_alaw = m_pAudioTrak->FindChildAtom("mdia.minf.stbl.stsd.alaw");
			CMP4Read_atom *pAtom_samr = m_pAudioTrak->FindChildAtom("mdia.minf.stbl.stsd.samr");
			CMP4Read_atom *pAtom_aac = m_pAudioTrak->FindChildAtom("mdia.minf.stbl.stsd.mp4a.esds");

			CMP4Read_atom_tkhd *pAtom_tkhd = (CMP4Read_atom_tkhd *)m_pAudioTrak->FindChildAtom("tkhd");
			if(pAtom_tkhd && (pAtom_ulaw || pAtom_alaw || pAtom_samr || pAtom_aac))
			{
				//m_ai.type = pAtom_ulaw?RTP_TYPE_ULAW:RTP_TYPE_AMR;
				if (NULL != pAtom_aac) {
					m_ai.type = MP4_AUDIO_AAC;
				} else if (NULL != pAtom_samr) {
					m_ai.type = MP4_AUDIO_AMR_NB;
				} else if (NULL != pAtom_ulaw) {
					m_ai.type = MP4_AUDIO_ULAW;
				} else if (NULL != pAtom_alaw) {
					m_ai.type = MP4_AUDIO_ALAW;
				}

				m_ai.bValid = TRUE;
				m_ai.duration=pAtom_tkhd->m_duration;
				m_ai.frame_count=m_pAudioTrak->GetFrameCount();
				if (MP4_AUDIO_AAC != m_ai.type) {
					m_ai.channel = 1;
					m_ai.bit_per_sample = m_ai.type == MP4_AUDIO_AMR_NB ? 16 : 8;
					m_ai.profile = 0;
					m_ai.sample = 8000;
				} else {
					m_ai.channel = ((CMP4Read_atom_mp4a_aac_esds*)pAtom_aac)->channel;
					m_ai.bit_per_sample = ((CMP4Read_atom_mp4a_aac_esds*)pAtom_aac)->bit_per_samle;
					m_ai.sample = ((CMP4Read_atom_mp4a_aac_esds*)pAtom_aac)->sample;
					m_ai.profile = ((CMP4Read_atom_mp4a_aac_esds*)pAtom_aac)->profile;
				}
			}

			CMP4Read_atom_mdhd *pAtom_mdhd = (CMP4Read_atom_mdhd *)m_pAudioTrak->FindChildAtom("mdia.mdhd");
			if(pAtom_mdhd && pAtom_mdhd->m_timescale!=0)
			{
				//m_ai.timescale=pAtom_mdhd->m_timescale;
				m_ai.duration=pAtom_mdhd->m_duration;
			}
		}

		if(!m_ai.bValid)
		{
			m_pAudioTrak=NULL;
		}
	}

	m_iFile->fseek(0,SEEK_SET);

	if(m_vi.video_type == eVideoType_None)
	{
		Close();
		return -1;
	}

	return 0;
}

int CMP4Read::Close()
{
	if(m_pRootAtom)
	{
		delete m_pRootAtom;
		m_pRootAtom=NULL;
	}
	
	if(m_iFile)
	{
		//DT("delete m_iFile,m_iFile=0x%x",m_iFile);
		delete m_iFile;
		m_iFile=NULL;
	}

	memset(&m_vi,0,sizeof(m_vi));
	memset(&m_ai,0,sizeof(m_ai));
	m_cbFile=0;
	m_pVideoTrak=NULL;
	m_pAudioTrak=NULL;

	return 0;
}

//return 0 on success,cbDataBytes is actual bytes return
//return -1 on fail
int CMP4Read::GetExtData(eExtData type,LPBYTE pBuf,int cbBuf,int& cbDataBytes)
{
	cbDataBytes=0;

	switch(type)
	{
	case eExtData_sps:
	case eExtData_pps:
	{
		CMP4Read_atom_trak* pv= FindVideoTrak();
		if(!pv)
		{
			return -1;
		}

		CMP4Read_atom_avcC * pAtom = (CMP4Read_atom_avcC*)pv->FindChildAtom("mdia.minf.stbl.stsd.avc1.avcC");
		if(!pAtom)
		{
			return -1;
		}

		LPBYTE psrc = NULL;
		int cbsrc=0;
		if(type == eExtData_sps)
		{
			psrc=pAtom->m_sps;
			cbsrc=pAtom->m_sps_count;
		}
		else if(type == eExtData_pps)
		{
			psrc=pAtom->m_pps;
			cbsrc=pAtom->m_pps_count;
		}

		cbDataBytes=cbsrc;
		if(psrc && cbsrc)
		{
			if(cbBuf >= cbsrc)
			{
				memcpy(pBuf,psrc,cbsrc);
				return 0;
			}
			else
			{
				cbBuf=cbsrc;
				return -1;
			}
		}

		break;
	}
	case eExtData_aacdsi:{
		CMP4Read_atom_trak* pa = FindAudioTrak();
		if(!pa) {
			return -1;
		}

		CMP4Read_atom_avcC * pAtom = (CMP4Read_atom_avcC*)pa->FindChildAtom("mdia.minf.stbl.stsd.mp4a.esds");
		if(!pAtom) {
			return -1;
		}

		break;
	}
	}
	return -1;
}

//����Ƿ�Ϊ��Ч��mp4�ļ�
int CMP4Read::CheckValid()
{
	IReadFile *hFile = m_iFile;
	if(!hFile)
	{
		return -1;
	}

	m_cbFile = hFile->GetFileSize();
	if(m_cbFile<=0)
	{
		return -1;
	}

	//����ͨ��atom size�����ж�mp4�Ƿ���Ч
	long pos_moov=-1;
	int  cb_moov_next=0;
	int ret = -1;
	hFile->fseek(0,SEEK_SET);
	do
	{
		long start = hFile->ftell();
		BYTE buf[8];
		ret = hFile->fread(buf,1,sizeof(buf));
		if(ret!=sizeof(buf))
		{
			break;
		}

		long *pAtom = (long*)buf;
		DWORD32 dwAtomSize = ntohl(*pAtom);
		long next = start + dwAtomSize;
		const char *pszAtomType = (char*)pAtom+4;
		if(dwAtomSize<8)
		{
#ifdef _MSC_VER
		DT("[%.4s] start=%d(0x%x),dwAtomSize=%d(0x%x),next=%d(0x%x)",
			pszAtomType,
			start,start,
			dwAtomSize,dwAtomSize,
			next,next
			);
			DW("invalid atom size");
#endif
			return -1;
		}
		
		if(strncmp(pszAtomType,"moov",4)==0)
		{
			pos_moov=start;
			cb_moov_next=next;
		}

		if(next>(long)m_cbFile)
		{
			break;
		}
		else if(next==(long)m_cbFile)
		{
			ret=0;
			break;
		}

		//DT("next=%d",next);
		hFile->fseek(next,SEEK_SET);
	}while(1);

	if(ret || pos_moov==-1)
	{
		return -1;
	}

	long pos = hFile->ftell();
	hFile->fseek(pos,SEEK_SET);
	return ret;
}

int CMP4Read::ParseAtom()
{
	//DT("CMP4Read::ParseAtom()");
	int ret = 0;
	
	m_iFile->fseek(0,SEEK_SET);
	ASSERT(!m_pRootAtom);

	m_pRootAtom = CMP4Read_atom::CreateAtom(NULL);
	m_pRootAtom->SetFile(m_iFile);
	m_pRootAtom->SetStart(0);
	m_pRootAtom->SetSize(m_cbFile);
	m_pRootAtom->SetEnd(m_cbFile);

	ret = m_pRootAtom->Read();
	return ret;
}

void CMP4Read::DumpAtom()
{
	if(m_pRootAtom)
	{
		m_pRootAtom->Dump();	
	}
}

int CMP4Read::GetTrakCount()
{
	if(!m_pRootAtom)
	{
		ASSERT(FALSE);
		return 0;
	}

	int nc=0;
	for(int i=0;i<100;i++)//should be enough
	{
		char szItem[64];
		snprintf(szItem,sizeof(szItem)-1,"moov.trak[%d]",i);
		CMP4Read_atom_trak * pAtom = (CMP4Read_atom_trak*)m_pRootAtom->FindChildAtom(szItem);
		if(!pAtom)
		{
			break;
		}

		nc++;
	}

	return nc;												   
}

CMP4Read_atom_trak *CMP4Read::FindVideoTrak()
{
	if(!m_pRootAtom)
	{
		ASSERT(FALSE);
		return 0;
	}

	for(int i=0;i<2;i++)
	{
		char szItem[64];
		snprintf(szItem,sizeof(szItem)-1,"moov.trak[%d]",i);
		CMP4Read_atom_trak * pAtom = (CMP4Read_atom_trak*)m_pRootAtom->FindChildAtom(szItem);
		if(pAtom && pAtom->IsVideoTrak())
		{
			return pAtom;
		}
	}

	return NULL;												   
}

CMP4Read_atom_trak *CMP4Read::FindAudioTrak()
{
	if(!m_pRootAtom)
	{
		ASSERT(FALSE);
		return 0;
	}

	for(int i=0;i<2;i++)
	{
		char szItem[64];
		snprintf(szItem,sizeof(szItem)-1,"moov.trak[%d]",i);

		CMP4Read_atom_trak * pAtom = (CMP4Read_atom_trak*)m_pRootAtom->FindChildAtom(szItem);
		if(pAtom && !pAtom->IsVideoTrak())
		{
			return pAtom;
		}
	}

	return NULL;												   
}

int CMP4Read::GetAudioInfo(tagMP4AudioInfo& ai)
{
	memset(&ai,0,sizeof(ai));
	if(m_ai.bValid)
	{
		memcpy(&ai,&m_ai,sizeof(ai));
	}

	return m_ai.bValid?0:-1;
}

int CMP4Read::GetVideoInfo(tagMP4VideoInfo& vi)
{
	memset(&vi,0,sizeof(vi));
	if(m_vi.bValid)
	{
		memcpy(&vi,&m_vi,sizeof(vi));
	}

	return m_vi.bValid?0:-1;
}

int CMP4Read::ReadAudioFrame(LPBYTE pFrame,int cbFrame,int& cbRead,DWORD32& duration)
{
	if(m_pAudioTrak)
	{
		int ret = m_pAudioTrak->ReadAudioFrame(pFrame,cbFrame,cbRead,duration);
		return ret;
	}

	return -1;
}

int CMP4Read::ReadVideoFrame(LPBYTE pFrame, int cbFrame, int& cbRead, DWORD32& duration)
{
	if(m_pVideoTrak)
	{
		int ret = m_pVideoTrak->ReadVideoFrame(pFrame,cbFrame,cbRead,duration);
		if(ret==0)
		{
#ifdef _DEBUG
			/*
			{
				static idx=0;
				DT("video frame [%d],size=%d",idx++,cbRead);
			}

			DT("ReadVideoFrame cbRead=%d(0x%x),hex=0x%02x%02x%02x%02x,0x%02x%02x%02x%02x",
				cbRead,
				cbRead,
				pFrame[0],pFrame[1],pFrame[2],pFrame[3],
				pFrame[4],pFrame[5],pFrame[6],pFrame[7]
				);
			if(0 && cbRead>=10*1024)
			{
				static int idx=0;
				CString szFile;
				szFile.Format("c:\\save1\\xwp\\%03d.bin",idx++);
				CFile file;
				file.Open(szFile,CFile::modeCreate | CFile::modeWrite);
				file.Write(pFrame,cbRead);
			}
			//*/
#endif
		}
		return ret;
	}

	ASSERT(FALSE);
	return -1;
}

//������һ��ReadVideoFrame��ȡ��֡�±�,��֡��Ϊ��ǰ֡
int CMP4Read::GetCurVideoFrame()
{
	if(m_pVideoTrak)
	{
		return m_pVideoTrak->GetCurFrame();
	}

	ASSERT(FALSE);
	return -1;
}

//seek��ָ����video֡�±�
int CMP4Read::SeekToFrame(int nVideoFrame)
{
	if(m_pVideoTrak)
	{
		if(nVideoFrame<0 || nVideoFrame >= m_pVideoTrak->GetFrameCount())
		{
			return -1;
		}

		//�õ���nVideoFrame֡��duration,Ȼ��seek
		DWORD32 duration = m_pVideoTrak->GetDurationSum(nVideoFrame);
		return Seek(duration);
	}

	return -1;
}

//��λ���ӵ�ǰ֡��ʼ(������ǰ֡)�������ĵ�һ��I֡
int CMP4Read::SeekToIFrame()
{
	if(!m_pVideoTrak)
	{
		return -1;
	}
	
	int i = GetCurVideoFrame();
	int nc = m_vi.frame_count;
	while(i<nc)
	{
		if(IsIFrame(i))
		{
			return SeekToFrame(i);
		}

		i++;
	}

	return -1;
}

//�ӵ�ǰ֡��ʼ(��������ǰ֡)��ǰ����,��λ�������ĵ�һ��I֡
//������֡�˲���(����ʱֻ����I֡)
int CMP4Read::SeekToPrevIFrame()
{
	int nCurFrame = GetCurVideoFrame();
	for(int i=nCurFrame-1;i>=0;i--)
	{
		if(IsIFrame(i))
		{
			SeekToFrame(i);
			return 0;
		}
	}

	return -1;
}

BOOL32 CMP4Read::IsIFrame(int nVideoFrameIndex)
{
	if(!m_pVideoTrak)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	
	if(!m_vi.bValid || nVideoFrameIndex < 0 || nVideoFrameIndex >= (int)m_vi.frame_count)
	{
		return FALSE;
	}

	return m_pVideoTrak->IsIFrame(nVideoFrameIndex);
}

//��������:
//		trakType			Ҫ�����trak������	
//		currMp4SampleNo		��ǰ��mp4�еĴ����sample��Ӧ��������
//		currMp4SampleSum	��ǰ��mp4�е��ܵ�֡��
//
int CMP4Read::GetIndexSplNum(char trakType, int& currMp4SampleIndex, int& currMp4SampleSum)
{	
	//DBG_INFO("\tCall %s.\n", __FUNCTION__);

	//index		= m_pVideoTrak->m_index;
	//printf("\t(%d):	  index = %d.\n", __LINE__, index);		
	//sampleNum	= m_pVideoTrak->m_sampleCnt;
	//printf("\t(%d):	  sampleNum = %d.\n", __LINE__, sampleNum);

	if (0 == trakType)
	{
		//������Ƶtrak
		currMp4SampleIndex			= m_pVideoTrak->GetCurFrame();		
		currMp4SampleSum			= m_pVideoTrak->GetFrameCount();		
	}
	else if (1 == trakType)
	{
		//������Ƶtrak	
		currMp4SampleIndex			= m_pAudioTrak->GetCurFrame();				
		currMp4SampleSum			= m_pAudioTrak->GetFrameCount();		
	}
	else
	{
		printf("\t(%s-%d):	Find a exception, trakType = %d!!!\n", trakType);
		return -1;
	}		
	
	//DBG_INFO("\tOut %s.\n", __FUNCTION__);	
	return 0;
}  

//seekʱ��video durationΪ׼
int CMP4Read::Seek(DWORD32 durationVideo)
{
	int ret=0;

	if(m_pVideoTrak)
	{
		ret |= m_pVideoTrak->Seek(durationVideo);
		if(m_pAudioTrak)
		{
			//ת����audio trak�е�duration
			//TODO: 20160711 ��Ƶ��timescale����д��
			DWORD32 durationAudio = (DWORD32)(8000.0*durationVideo/90000);
			ret |= m_pAudioTrak->Seek(durationAudio);
		}
	}

	return ret;
}

DWORD32 CMP4Read::GetCurVideoDuration()
{
	ASSERT(m_pVideoTrak);
	return m_pVideoTrak->GetCurDuration();
}

DWORD32 CMP4Read::GetCurAudioDuration()
{
	ASSERT(m_pAudioTrak);
	return m_pAudioTrak->GetCurDuration();
}

CMP4Read_atom::CMP4Read_atom()
{
	m_start=0;
	m_size=0;
	m_end=0;
	m_hFile=0;
	memset(m_type,0,sizeof(m_type));
	m_bHasChildAtom=FALSE;
	m_depth=0;
}

CMP4Read_atom::~CMP4Read_atom()
{
	{
		for(UINT i=0;i<m_arrChildAtom.size();i++)
		{
			CMP4Read_atom *pAtom = m_arrChildAtom[i];
			delete pAtom;
		}
		m_arrChildAtom.clear();
	}
}

CMP4Read_atom* CMP4Read_atom::CreateAtom(const char* type)
{
	CMP4Read_atom* pAtom = NULL;

	if(type)
	{
		if(0)
		{
		}
		else if(!strcmp(type,"trak"))
			pAtom = new CMP4Read_atom_trak;
		else if(!strcmp(type,"tkhd"))
			pAtom = new CMP4Read_atom_tkhd;
		else if(!strcmp(type,"hdlr"))
			pAtom = new CMP4Read_atom_hdlr;
		else if(!strcmp(type,"mdhd"))
			pAtom = new CMP4Read_atom_mdhd;
		else if(!strcmp(type,"stsd"))
			pAtom = new CMP4Read_atom_stsd;
		else if(!strcmp(type,"avc1"))
			pAtom = new CMP4Read_atom_avc1;
		else if(!strcmp(type,"avcC"))
			pAtom = new CMP4Read_atom_avcC;
		else if(!strcmp(type,"mp4v"))
			pAtom = new CMP4Read_atom_mp4v;
		else if(!strcmp(type,"stsc"))
			pAtom = new CMP4Read_atom_stsc;
		else if(!strcmp(type,"stsz"))
			pAtom = new CMP4Read_atom_stsz;
		else if(!strcmp(type,"stco"))
			pAtom = new CMP4Read_atom_stco;
		else if(!strcmp(type,"stss"))
			pAtom = new CMP4Read_atom_stss;
		else if(!strcmp(type,"stts"))
			pAtom = new CMP4Read_atom_stts;
		else if (!strcmp(type, "mp4a"))
			pAtom = new CMP4Read_atom_mp4a;
		else if (!strcmp(type, "esds"))
			pAtom = new CMP4Read_atom_mp4a_aac_esds;
	}
	
	if(!pAtom)
	{
		pAtom = new CMP4Read_atom;
	}
	
	if(type)
	{
		strncpy(pAtom->m_type,type,sizeof(pAtom->m_type)-1);
	}

	if( !type
		|| !strcmp(type,"moov")
		|| !strcmp(type,"trak")
		|| !strcmp(type,"mdia")
		|| !strcmp(type,"minf")
		|| !strcmp(type,"stbl")
		|| !strcmp(type,"stsd")
		|| !strcmp(type,"mp4v")
		|| !strcmp(type,"avc1")
		|| !strcmp(type,"mp4a")
		)
	{
		pAtom->m_bHasChildAtom=TRUE;
	}

	return pAtom;
}

int CMP4Read_atom::Read()
{
	BYTE buf[8];
	char type[8];
	int retValue = 0;
	
	if(!m_bHasChildAtom)
	{
		return retValue;
	}

	while(1)
	{
		DWORD32 start = m_hFile->ftell();
		if(start>=m_end)
		{
			DW("reach end,start=%d,end=%d, retValue = %d\n",start,m_end, retValue);
			break;
		}

		int ret = m_hFile->fread(buf,1,sizeof(buf));
		if(ret!=sizeof(buf))
		{
			DW("fread fail,ret=%d",ret);
			break;
		}

		LPDWORD pSize = (LPDWORD)buf;
		DWORD32 size = ntohl(*pSize);
		DWORD32 end  = start + size;

		type[4]=0;
		strncpy(type,(char*)buf+4,4);

		CMP4Read_atom *pAtom = CMP4Read_atom::CreateAtom(type);

		ASSERT(pAtom);
		//DT("[%s]",type);

		pAtom->m_depth=m_depth+1;
		pAtom->SetFile(m_hFile);
		pAtom->SetStart(start);
		pAtom->SetSize(size);
		pAtom->SetEnd(end);
		retValue += pAtom->Read();

		m_hFile->fseek(end,SEEK_SET);
		m_arrChildAtom.push_back(pAtom);

		if(size == 0)
		{
			break;
		}
	}
	return retValue;
}

void CMP4Read_atom::Dump()
{
#ifdef _MSC_VER
	if(m_type[0]==0)
	{
		DT("root");
	}
	else
	{
		char szText[1024];
		szText[0]=0;

		for(UINT i=0;i<m_depth;i++)
		{
			strcat(szText,"|   ");
		}
	
		char szType[1024];
		strcpy(szType,szText);
		strcat(szType,(char*)m_type);
		//DT("%s",szType);
	}

	for(int i=0;i<m_arrChildAtom.size();i++)
	{
		m_arrChildAtom[i]->Dump();
	}
#endif
}

int CMP4Read_atom_stsd::Read()
{
	BYTE buf[8];
	m_hFile->fread(buf,1,sizeof(buf));//skip version and flags
	CMP4Read_atom::Read();
	return 0;
}

int CMP4Read_atom_avc1::Read()
{
	BYTE buf[8];
	m_hFile->fread(buf,1,sizeof(buf));//skip version and flags
	{
		//skip un-need data 70Byte
		int size=0;
		size += 4;//addWord(0x00000000); // Version+Revision level
		size += 4;//add4ByteString("appl"); // Vendor
		size += 4;//addWord(0x00000000); // Temporal quality
		size += 4;//addWord(0x00000000); // Spatial quality
		//unsigned const widthAndHeight       = (m_szVideo.cx<<16)|m_szVideo.cy;
		size += 4;//addWord(widthAndHeight); // Width+height
		size += 4;//addWord(0x00480000); // Horizontal resolution
		size += 4;//addWord(0x00480000); // Vertical resolution
		size += 4;//addWord(0x00000000); // Data size
		size += 4;//addWord(0x00010548); // Frame       count+Compressor name (start)
		// "H.264"
		size += 4;//addWord(0x2e323634); // Compressor name (continued)
		size += 4*6;//addZeroWords(6); // Compressor name (continued - zero)
		size += 4;//addWord(0x00000018); // Compressor name (final)+Depth
		size += 2;//addHalfWord(0xffff); // Color       table id
		for(int i=0;i<size;i++)
		{
			m_hFile->fread(buf,1,1);//skip
		}
	}
	CMP4Read_atom::Read();
	return 0;
}

void CMP4Read_atom_avcC::Empty()
{
	if(m_pps)
	{
		delete []m_pps;
		m_pps=NULL;
		m_pps_count=0;
	}
	if(m_sps)
	{
		delete []m_sps;
		m_sps=NULL;
		m_sps_count=0;
	}
}

int CMP4Read_atom_avcC::Read()
{
	Empty();

	BYTE buf[8];
	{
		//skip un-need data
		int size=0;
		size += 1;//addByte(0x01); // configuration version
		size += 1;//addByte(sps_data[1]); // profile
		size += 1;//addByte(sps_data[2]); // profile compat
		size += 1;//addByte(sps_data[3]); // level
		size += 1;//addByte(0xff); /* 0b11111100 | lengthsize = 0x11 */
		m_hFile->fseek(size,SEEK_CUR);
		m_hFile->fread(buf,1,1);
		ASSERT(buf[0]==0xe0 || buf[0]==0xe1);
		if(buf[0]&0xe0)//has sps
		{
			m_hFile->fread(buf,1,2);
			int sps_count=((buf[0]<<8)| buf[1]);
			if(sps_count)
			{
				m_sps_count=sps_count;
				m_sps = new BYTE[m_sps_count];
				m_hFile->fread(m_sps,1,m_sps_count);
			}
		}
		m_hFile->fread(buf,1,1);
		if(buf[0]==0x01)
		{
			m_hFile->fread(buf,1,2);
			int pps_count=((buf[0]<<8)| buf[1]);
			if(pps_count>0)
			{
				m_pps_count=pps_count;
				m_pps = new BYTE[m_pps_count];
				m_hFile->fread(m_pps,1,m_pps_count);
			}
		}
	}

	CMP4Read_atom::Read();
	return 0;
}

//�ο�long CMP4Trak::addAtom_mp4v()
int CMP4Read_atom_mp4v::Read()
{
	long pos = m_hFile->ftell();
	m_hFile->fseek(pos
		+6	//reserved 
		+2	//data-reference-index,must be 0x0001
		+16	//reserved
		,
		SEEK_SET
		);

	WORD width,height;
	m_hFile->fread(&width,1,sizeof(width));
	m_hFile->fread(&height,1,sizeof(width));
	m_width=ntohs(width);
	m_height=ntohs(height);
	
	pos = m_hFile->ftell();
	m_hFile->fseek(pos
		+4	//horizresolution
		+4	//vertresolution
		+4	//reserved
		+2	//frame_count
		+32	//string[32] compressorname
		+2	//unsigned int(16) depth = 0x0018;
		+2	//int(16) pre_defined = -1;
		,
		SEEK_SET
		);
	
	CMP4Read_atom::Read();
	return 0;
}


//TODO:��mp4a atom������sample bit_per_sample channel 
int CMP4Read_atom_mp4a::Read()
{
	BYTE buf[8];
	m_hFile->fread(buf, 1, sizeof(buf));//skip version and flags
	{
		//skip unuse data
		int size = 0;
		size += 5 * 4;
		for (int i = 0; i < size; i++) {
			m_hFile->fread(buf, 1, 1);
		}
	}
	CMP4Read_atom::Read();
	return 0;
}

int CMP4Read_atom_mp4a_aac_esds::Read()
{
	BYTE buf[8];
	m_hFile->fread(buf, 1, 4);//skip flags
	unsigned char tmp[64];
	int date_size = m_size - 4 * 3;
	m_hFile->fread(tmp, 1, date_size);
	int pos = 0;
	while (pos < date_size - 4) {	
		//��DecoderConfigDescriptor�н�����Ƶ�����Ϣ���ں�������aac��adtsͷ
		if ((0x05 == tmp[pos]) 
			&& (0x80 == tmp[pos + 1])
			&& (0x80 == tmp[pos + 2])
			&& (0x80 == tmp[pos + 3])
			) {
			const unsigned char dsi1 = tmp[pos + 5];
			const unsigned char dsi2 = tmp[pos + 6];
			profile = dsi1 >> 3;
			sample = GetAacSampleFromSamplingFreqIndex( ((dsi1 & (0x07)) << 1) | (dsi2 >> 7) );
			channel = (dsi2 >> 3) & (0x0F);
			bit_per_samle = 16;//��ʱд�� ���Ӧ�ô�atom mp4a�����ȡ
			CMP4Read_atom::Read();
			return 0;
		}
		++pos;
	}

	//can`t find Decoder spec info set default param
	printf("%s %d Can`t find Decoder spec info set default param", __FILE__, __LINE__);
	channel = 2;
	bit_per_samle = 16;
	profile = 2;//LC
	sample = 48000;
	CMP4Read_atom::Read();
	return 0;
}


int CMP4Read_atom_stts::Read()
{
	ASSERT(!m_stts);
	
	DWORD32 pos = m_hFile->ftell();
	m_hFile->fseek(pos+4,SEEK_SET);//skip version + flags

	DWORD32 nc;
	m_hFile->fread(&nc,1,sizeof(nc));
	m_stts_count=ntohl(nc);
	if(m_stts_count>0)
	{
		m_stts = new tagMP4Node_stts[m_stts_count];
		ASSERT(m_stts);
	}

	m_hFile->fread(m_stts,1,sizeof(tagMP4Node_stts)*m_stts_count);

	//ntohl
	{
		for(UINT i=0;i<m_stts_count;i++)
		{
			m_stts[i].SampleCount=ntohl(m_stts[i].SampleCount);
			m_stts[i].SampleDuration=ntohl(m_stts[i].SampleDuration);
		}
	}

	/*
	//dump,for test
	{
		for(UINT i=0;i<m_stts_count;i++)
		{
			DT("SampleCount,SampleDuration=[%d,%d]",m_stts[i].SampleCount,m_stts[i].SampleDuration);
		}
	}
	//*/
        return 0;
}

//�ο�long CMP4Trak::addAtom_stsc()
int CMP4Read_atom_stsc::Read()
{
	ASSERT(!m_stsc);
	
	DWORD32 pos = m_hFile->ftell();
	m_hFile->fseek(pos+4,SEEK_SET);//skip version + flags

	DWORD32 nc;
	m_hFile->fread(&nc,1,sizeof(nc));
	m_stsc_count=ntohl(nc);
	if(m_stsc_count>0)
	{
		m_stsc = new tagMP4Node_stsc[m_stsc_count];
		ASSERT(m_stsc);
	}

	m_hFile->fread(m_stsc,1,sizeof(tagMP4Node_stsc)*m_stsc_count);

	//ntohl
	{
		for(UINT i=0;i<m_stsc_count;i++)
		{
			m_stsc[i].FirstChunk=ntohl(m_stsc[i].FirstChunk);
			m_stsc[i].SamplesPerChunk=ntohl(m_stsc[i].SamplesPerChunk);
			m_stsc[i].SampleDescriptionID=ntohl(m_stsc[i].SampleDescriptionID);
		}
	}
	
	/*
	//dump,for test
	{
		for(UINT i=0;i<m_stsc_count;i++)
		{
			DT("FirstChunk,SamplesPerChunk=[%d,%d]",m_stsc[i].FirstChunk,m_stsc[i].SamplesPerChunk);
		}
	}
	//*/
        return 0;
}

//�ο�long CMP4Trak::addAtom_stsz()
int CMP4Read_atom_stsz::Read()
{
	DWORD32 pos = m_hFile->ftell();
	m_hFile->fseek(pos+4,SEEK_SET);//skip version + flags

	DWORD32 dwSize,dwCount;
	m_hFile->fread(&dwSize,1,sizeof(dwSize));
	m_hFile->fread(&dwCount,1,sizeof(dwCount));
	dwSize = ntohl(dwSize);
	dwCount= ntohl(dwCount);

	m_size=dwSize;
	m_stsz_count = dwCount;

	if(m_size == 0 && m_stsz_count>0)//every sample has different size
	{
		m_stsz = new DWORD32[m_stsz_count];
		ASSERT(m_stsz);
		m_hFile->fread(m_stsz,1,sizeof(DWORD32)*m_stsz_count);

		//ntohl
		{
			for(UINT i=0;i<m_stsz_count;i++)
			{
				m_stsz[i]=ntohl(m_stsz[i]);
				//DT("stsz[%d]=%d",i,m_stsz[i]);
			}
		}
	}
	return 0;
}

int CMP4Read_atom_stco::Read()
{
	DWORD32 pos = m_hFile->ftell();
	m_hFile->fseek(pos+4,SEEK_SET);//skip version + flags

	DWORD32 dwCount;
	m_hFile->fread(&dwCount,1,sizeof(dwCount));
	dwCount= ntohl(dwCount);

	m_stco_count = dwCount;
	if(m_stco_count>0)
	{
		m_stco = new DWORD32[m_stco_count];
		ASSERT(m_stco);
		m_hFile->fread(m_stco,1,sizeof(DWORD32)*m_stco_count);

		//ntohl
		{
			for(UINT i=0;i<m_stco_count;i++)
			{
				m_stco[i]=ntohl(m_stco[i]);
				//DT("stco[%d]=%d",i,m_stco[i]);
			}
		}
	}
	return 0;
}

int CMP4Read_atom_stss::Read()
{
	DWORD32 pos = m_hFile->ftell();
	m_hFile->fseek(pos+4,SEEK_SET);//skip version + flags

	DWORD32 dwCount;
	m_hFile->fread(&dwCount,1,sizeof(dwCount));
	dwCount= ntohl(dwCount);

	m_stss_count = dwCount;
	if(m_stss_count>0)
	{
		m_stss = new DWORD32[m_stss_count];
		ASSERT(m_stss);
		m_hFile->fread(m_stss,1,sizeof(DWORD32)*m_stss_count);

		//ntohl
		{
			for(UINT i=0;i<m_stss_count;i++)
			{
				m_stss[i]=ntohl(m_stss[i]);
				//DT("stss[%d]=%d",i,m_stss[i]);
			}
		}
	}
	return 0;
}

int CMP4Read_atom_mdhd::Read()
{
	DWORD32 pos = m_hFile->ftell();
	m_hFile->fseek(pos
		+4		//version + flags
		+4		//create time
		+4		//modify time
		,SEEK_SET);
	DWORD32 timescale,duration;
	m_hFile->fread(&timescale,1,sizeof(timescale));
	m_hFile->fread(&duration,1,sizeof(duration));
	m_timescale=ntohl(timescale);
	m_duration=ntohl(duration);
	return 0;
}

int CMP4Read_atom_hdlr::Read()
{
	DWORD32 pos = m_hFile->ftell();
	m_hFile->fseek(pos
		+4		//version + flags
		+4		//reserved
		,SEEK_SET);
	
	m_hFile->fread(m_handlerType,1,sizeof(m_handlerType));
	m_handlerType[4]=0;
	return 0;
}

int CMP4Read_atom_tkhd::Read()
{
	DWORD32 pos = m_hFile->ftell();
	m_hFile->fseek(pos
		+4		//version + flags
		+4		//Creation time
		+4		//Modification time
		+4		//trak id
		+4		//reserved
		,SEEK_SET);

	DWORD32 duration;//+4		//duration
	m_hFile->fread(&duration,1,sizeof(duration));
	m_duration=ntohl(duration);

	pos = m_hFile->ftell();
	m_hFile->fseek(pos
		+3*4	//Reserved+Layer+Alternate grp
		+4		// Volume + Reserved
		+4		// matrix top left corner
		+3*4	// matrix
		+4		// matrix center
		+3*4	// matrix
		+4		// matrix bottom right corner
		,SEEK_SET);

	DWORD32 w,h;
	m_hFile->fread(&w,1,sizeof(w));
	m_hFile->fread(&h,1,sizeof(h));

	m_width=(ntohl(w)>>16);
	m_height=(ntohl(h)>>16);
	return 0;
}

//������atom,֧�ֶ��,���ʱ��.����,����typeΪmoov.trak.mdia.minf.stbl
//����atom���ж����ͬ��typeʱ,������[]��ʾ�±�
//moov.trak[0].mdia.hdlr
//moov.trak[1].mdia.hdlr
CMP4Read_atom * CMP4Read_atom::FindChildAtom(const char *type)
{
	CMP4Read_atom *pAtom = NULL;
	
	char szBuf[1024];//should be enough
	memset(szBuf,0,sizeof(szBuf));
	strncpy(szBuf,type,sizeof(szBuf)-1);
	char *pszSave=szBuf;
	char *psz = pszSave;
	while(psz && *psz)
	{
		int nIndex=0;
		char* period = strstr(psz,".");
		if(period)
		{
			*period=0;
		}

		char *pszIndex=strstr(psz,"[");
		if(pszIndex)
		{
			nIndex=atoi(pszIndex+1);
			*pszIndex=0;
		}
		
		if(pAtom)
			pAtom = pAtom->FindChildAtomHelper(psz,nIndex);
		else
			pAtom = FindChildAtomHelper(psz,nIndex);

		if(period)
		{
			psz=period+1;
		}
		else
		{
			break;
		}
	}

	return pAtom;
}

//����ֱ����atom,��֧�ֶ��
//nIndex��ʾ��nIndex������Ϊtype��atom,nIndex��0��ʼ
CMP4Read_atom * CMP4Read_atom::FindChildAtomHelper(const char *type,int nIndex)
{
	ASSERT(strlen(type)==4);

	int nMatchIndex=0;
	for(UINT i=0;i<m_arrChildAtom.size();i++)
	{
		CMP4Read_atom *pAtom = m_arrChildAtom[i];
		if(pAtom->m_type[0] && strcmp(pAtom->m_type,type)==0)
		{
			if(nIndex == nMatchIndex)
			{
				return pAtom;
			}

			nMatchIndex++;
		}
	}

	return NULL;
}

BOOL32 CMP4Read_atom_trak::IsVideoTrak()
{
	ASSERT(m_bInit);
	return m_bVideoTrak;
}

BOOL32 CMP4Read_atom_trak::IsIFrame(int nVideoFrameIndex)
{
	if(!IsVideoTrak())
	{
		ASSERT(FALSE);
		return TRUE;
	}

	if(nVideoFrameIndex < 0 || nVideoFrameIndex >= GetFrameCount())
	{
		return FALSE;
	}

	if(!m_stss || !m_stss->m_stss || IsMjpgTrak())
	{
		return TRUE;
	}

	int nc = m_stss->m_stss_count;
	DWORD32 *pidx = m_stss->m_stss;
	for(int i=0;i<nc;i++)
	{
		DWORD32 idx = pidx[i];
		if((int)idx == (nVideoFrameIndex+1))	//ע��:stss���index��1��ʼ,nVideoFrameIndex���Ǵ�0��ʼ��
		{
			return TRUE;
		}
		else  if((int)idx > nVideoFrameIndex)
		{
			return FALSE;
		}
	}

	return FALSE;
}

int CMP4Read_atom_trak::Read()
{
	CMP4Read_atom::Read();

	m_bInit = TRUE;
	CMP4Read_atom_hdlr * pAtom = (CMP4Read_atom_hdlr*)FindChildAtom("mdia.hdlr");
	m_bVideoTrak  = (pAtom && pAtom->IsVideo());
	if(m_bVideoTrak)
	{
		if (FindChildAtom("mdia.minf.stbl.stsd.mjpa")) {
			m_video_type = eVideoType_Mjpg;
		} else if (FindChildAtom("mdia.minf.stbl.stsd.avc1.avcC")) {
			m_video_type = eVideoType_H264;
		}
		CMP4Read_atom *pAtom_mjpa = FindChildAtom("mdia.minf.stbl.stsd.mjpa");
		m_bMjpg = (pAtom_mjpa != NULL);
	} else {
		if (FindChildAtom("mdia.minf.stbl.stsd.samr")) {
			m_audio_type = MP4_AUDIO_AMR_NB;
		} else if (FindChildAtom("mdia.minf.stbl.stsd.mp4a")) {
			m_audio_type = MP4_AUDIO_AAC;
			CMP4Read_atom *pAtom_aac = FindChildAtom("mdia.minf.stbl.stsd.mp4a.esds");
			m_aac_channel = ((CMP4Read_atom_mp4a_aac_esds*)pAtom_aac)->channel;
			m_aac_profile = ((CMP4Read_atom_mp4a_aac_esds*)pAtom_aac)->profile;
			m_aac_sample_freq_index = GetAacSampleFrequencyIndex(((CMP4Read_atom_mp4a_aac_esds*)pAtom_aac)->sample);
		} else if (FindChildAtom("mdia.minf.stbl.stsd.ulaw")) {
			m_audio_type = MP4_AUDIO_ULAW;
		} else if (FindChildAtom("mdia.minf.stbl.stsd.alaw")) {
			m_audio_type = MP4_AUDIO_ALAW;
	}
		CMP4Read_atom *pAtom_samr = FindChildAtom("mdia.minf.stbl.stsd.samr");
		m_bAmr = (pAtom_samr!=NULL);
	}

	//cache items
	m_stts=(CMP4Read_atom_stts *)FindChildAtom("mdia.minf.stbl.stts");
	m_stsc=(CMP4Read_atom_stsc *)FindChildAtom("mdia.minf.stbl.stsc");
	m_stsz=(CMP4Read_atom_stsz *)FindChildAtom("mdia.minf.stbl.stsz");
	m_stco=(CMP4Read_atom_stco *)FindChildAtom("mdia.minf.stbl.stco");
	m_stss=(CMP4Read_atom_stss *)FindChildAtom("mdia.minf.stbl.stss");

	ASSERT(m_stts && m_stsc && m_stsz && m_stco);
	if(IsVideoTrak())
	{
		if(!IsMjpgTrak())
		{
			ASSERT(m_stss);
		}
	}

	m_dwFrameIndex=0;
	
	if (m_stts && m_stsc && m_stsz && m_stco)
		return 0;
	else
		return MP4_FILE_ERROR;
}

//�ɹ�ʱ����0
//ʧ��ʱ����-1���ߵ�ǰ֡�ֽ���
//����ֵret����0ʱ,��ʾ��ǰcbFrame buffer������,��ǰ֡Ϊret�ֽ�
//durationΪ��ǰ֡�ĳ���ʱ��
int CMP4Read_atom_trak::ReadVideoFrame(LPBYTE pFrame,int cbFrame,int &cbRead,DWORD32& duration)
{
	cbRead=0;
	duration=0;
	
	if(!IsVideoTrak())
	{
		ASSERT(FALSE);
		return -1;
	}

	if(m_dwFrameIndex<0 || m_dwFrameIndex>=m_stsz->m_stsz_count)
	{
		return 0;
	}

	const int nSampleSize = m_stsz->m_stsz[m_dwFrameIndex];
	if(nSampleSize > cbFrame)
	{
		DW("ReadVideoFrame buf too small,nSampleSize = %d > cbFrame=%d",
			nSampleSize,
			cbFrame
			);

		return nSampleSize;
	}

	duration = GetSampleDuration(m_dwFrameIndex);
	
	int nChunkIndex=-1;
	int nChunkSampleIndex=-1;
	int i=m_dwFrameIndex;
	{
		int ret = GetChunkIndex(i,nChunkIndex,nChunkSampleIndex);
		ASSERT(ret==0);

		ASSERT(nChunkIndex>=0 && nChunkIndex<(int)m_stco->m_stco_count);

		long posChuck = m_stco->m_stco[nChunkIndex];
		long posSample = posChuck;
		if(nChunkSampleIndex > 0)
		{
			//Ҫ�ҵ�sample��������chuck�ĵ�һ��sample
			//����ǰ���nChunkSampleIndex��sample
			int nChuckFirstSample = i - nChunkSampleIndex;//��chuck�ĵ�һ��sample index
			for(int s=0;s<nChunkSampleIndex;s++)
			{
				posSample += m_stsz->m_stsz[nChuckFirstSample++];
			}
		}

		m_hFile->fseek(posSample,SEEK_SET);
		ret = m_hFile->fread(pFrame,1,nSampleSize);
		if(ret != nSampleSize)
		{
			return -1;
		}
		cbRead=nSampleSize;
	}

	m_curDuration += duration;
	m_dwFrameIndex++;

	return 0;
}

void CMP4Read_atom_trak::Mp4AacAddAdts(unsigned char* buf, int length)
{
	length += ADTS_HEADER_SIZE;
	buf[0] = (unsigned char)0xFF;
    buf[1] = (unsigned char)0xF9;
	buf[2] = (unsigned char)(((m_aac_profile - 1) << 6) + (m_aac_sample_freq_index << 2) + (m_aac_channel >> 2));
	buf[3] = (unsigned char)(((m_aac_channel & 3) << 6) + (length >> 11));
	buf[4] = (unsigned char)((length & 0x7FF) >> 3);
	buf[5] = (unsigned char)(((length & 7) << 5) + 0x1F);
    buf[6] = (unsigned char)0xFC;
}

////durationΪ��ǰ֡�ĳ���ʱ��
//aac ���adtsͷ
int CMP4Read_atom_trak::ReadAudioFrame(LPBYTE pFrame,int cbFrame,int &cbRead,DWORD32& duration)
{
	cbRead=0;
	duration=0;
	
	if (IsVideoTrak()) {
		ASSERT(FALSE);
		return -1;
	}

	if (m_dwFrameIndex<0 || m_dwFrameIndex>=(UINT)GetAudioFrameCount()) {
		return 0;
	}
	
	int nSampleSize = 0;
	if (MP4_AUDIO_AAC == m_audio_type) {
		nSampleSize = m_stsz->m_stsz[m_dwFrameIndex];
	} else {
		//֡����
		nSampleSize = m_stsz->m_size;
	}

	if (MP4_AUDIO_AAC != m_audio_type) {
		if (nSampleSize > cbFrame) {
		DW("ReadAudioFrame buf too small,nSampleSize = %d > cbFrame=%d",nSampleSize,cbFrame);
		return nSampleSize;
	}
	} else if (nSampleSize + ADTS_HEADER_SIZE > cbFrame) {
			DW("ReadAudioFrame buf too small,nSampleSize = %d > cbFrame=%d",nSampleSize + ADTS_HEADER_SIZE,cbFrame);
			return nSampleSize + ADTS_HEADER_SIZE;
	}

	if (MP4_AUDIO_AMR_NB == m_audio_type) {
		duration = 160;
	} else {
		duration = GetSampleDuration(m_dwFrameIndex);
	}
	
	int nChunkIndex=-1;
	int nChunkSampleIndex=-1;
	int i=m_dwFrameIndex;
	{
		int ret = GetChunkIndex(i,nChunkIndex,nChunkSampleIndex);
		ASSERT(ret==0);

		ASSERT(nChunkIndex>=0 && nChunkIndex<(int)m_stco->m_stco_count);

		long posChuck = m_stco->m_stco[nChunkIndex];
		long posSample = posChuck;
		if(nChunkSampleIndex > 0)
		{
			//Ҫ�ҵ�sample��������chuck�ĵ�һ��sample
			//����ǰ���nChunkSampleIndex��sample
			int nChuckFirstSample = i - nChunkSampleIndex;//��chuck�ĵ�һ��sample index
			for(int s=0;s<nChunkSampleIndex;s++)
			{
				if (MP4_AUDIO_AAC == m_audio_type) {
					posSample += m_stsz->m_stsz[nChuckFirstSample++];
				} else {
					//֡����
					if (m_stsz->m_size) posSample += m_stsz->m_size;
				}
			}
		}

		m_hFile->fseek(posSample,SEEK_SET);
		if (MP4_AUDIO_AAC == m_audio_type) {
			//add adts head
			ret = m_hFile->fread(&pFrame[ADTS_HEADER_SIZE], 1, nSampleSize);
			if(ret != nSampleSize)
			{
				return -1;
			}
			Mp4AacAddAdts(pFrame, nSampleSize);
			cbRead = nSampleSize + ADTS_HEADER_SIZE;
		} else {
		ret = m_hFile->fread(pFrame,1,nSampleSize);
		if(ret != nSampleSize)
		{
			return -1;
		}
		cbRead=nSampleSize;
	}
	}
	
	m_curDuration += duration;
	m_dwFrameIndex++;

	return 0;
}

DWORD32 CMP4Read_atom_trak::GetSampleDuration(int nSampleIndex)
{
	ASSERT(m_stts);

	int nFristSampleIndex = 0;
	for(UINT i=0;i<m_stts->m_stts_count;i++)
	{
		int sampleCount = m_stts->m_stts[i].SampleCount;
		if(nFristSampleIndex <= nSampleIndex && nSampleIndex < nFristSampleIndex + sampleCount)
		{
			return m_stts->m_stts[i].SampleDuration;
		}
		
		nFristSampleIndex += sampleCount;
	}

	ASSERT(FALSE);
	return 0;
}

int CMP4Read_atom_trak::Test()
{
	ASSERT(IsVideoTrak());
	return 0;
}

//��sample index�õ�chunk index
//nSampleIndex��0��ʼ,��������Ч��
//����ֵ:
//sample���ڵ�chunk�±�:nChunkIndex,
//sample���ڴ�chunk�еĵ�nChunkSampleIndex��sample,��Ҫ�ڴ�chunk������nChunkSampleIndex��sample��������ҵ�sample����
int CMP4Read_atom_trak::GetChunkIndex(int nSampleIndex,int& nChunkIndex,int& nChunkSampleIndex)
{
	nChunkIndex=-1;
	nChunkSampleIndex=-1;

	int nSampleSum = 0;
	for(UINT i=0;i<m_stsc->m_stsc_count;i++)
	{
		int nSample = m_stsc->m_stsc[i].SamplesPerChunk;
		int nSampleEntry = nSample;//��entry������sample����

		if(i < m_stsc->m_stsc_count-1)
		{
			int nChunk = m_stsc->m_stsc[i+1].FirstChunk - m_stsc->m_stsc[i].FirstChunk;
			if(nChunk > 1)
			{
				//��entry�������chunk,��sample������ͬ
				nSampleEntry = nSample * nChunk;
			}
		}
		
		if(nSampleIndex >= nSampleSum && nSampleIndex < nSampleSum + nSampleEntry)
		{
			//ok,�ҵ���
			nChunkIndex = i + (nSampleIndex - nSampleSum)/nSample;
			nChunkSampleIndex = ((nSampleIndex - nSampleSum) % nSample);
			return 0;
		}

		nSampleSum += nSampleEntry;
	}

	DW("invalid sample index for mp4 file!");
	ASSERT(FALSE);
	return -1;
}

int CMP4Read_atom_trak::GetFrameCount()
{
	return IsVideoTrak()?GetVideoFrameCount():GetAudioFrameCount();
}

//amr:ÿ֡32byte,durationΪ160
//ulaw:ÿ֡320byte,ÿ��sample��durationΪ1
int CMP4Read_atom_trak::GetAudioFrameCount()
{
	ASSERT(!IsVideoTrak());
	//��Ƶ֡������ʱ stsz��sample size == 0��
	if ((MP4_AUDIO_ALAW == m_audio_type) || (MP4_AUDIO_ULAW == m_audio_type) || (MP4_AUDIO_AMR_NB == m_audio_type)) {
	ASSERT(m_stsz->m_size);
	}

	return m_stsz->m_stsz_count;
}

//durationSeek��Ҫ��λ��sample duration֮��
//��seet video trakʱ,CMP4Read_atom_trak����֤seek֮��ĵ�һ��ReadVideoFrame�õ�����I֡
int CMP4Read_atom_trak::Seek(DWORD32 durationSeek)
{
	BOOL32 bOK = FALSE;
	
	DWORD32 durationPrev=0;
	int nSampleIndex=0;
	for(UINT i=0;i<m_stts->m_stts_count;i++)
	{
		DWORD32 dwCurrent = durationPrev + m_stts->m_stts[i].SampleDuration * m_stts->m_stts[i].SampleCount;

		if(durationPrev <= durationSeek && durationSeek < dwCurrent)
		{
			//ok,�Ѷ�λ�ڵ�ǰentry��,�����ǰentry�������sample,Ҫ�ҵ���ӽ���sample
			int nIndex=0;
			if(m_stts->m_stts[i].SampleCount>1)
			{
				DWORD32 delta = durationSeek - durationPrev;
				nIndex = (delta+m_stts->m_stts[i].SampleDuration/2)/m_stts->m_stts[i].SampleDuration;//��������
				nSampleIndex += nIndex;
			}

			m_curDuration = durationPrev + nIndex * m_stts->m_stts[i].SampleDuration;

			m_dwFrameIndex=(DWORD32)nSampleIndex;
			if(m_dwFrameIndex>=(UINT)GetFrameCount())
			{
				m_dwFrameIndex = GetFrameCount()-1;
			}

			bOK = TRUE;
			break;
		}

		durationPrev = dwCurrent;
		nSampleIndex += m_stts->m_stts[i].SampleCount;
	}

	return bOK?0:-1;
}

DWORD32 CMP4Read_atom_trak::GetCurDuration()
{
	return m_curDuration;
}

//������һ��ReadVideoFrame��ȡ��֡�±�
int CMP4Read_atom_trak::GetCurFrame()
{
	return m_dwFrameIndex-1;
}

//���ص���nVideoFrame֮֡ǰ���ܹ�duration
DWORD32 CMP4Read_atom_trak::GetDurationSum(int nVideoFrame)
{
	if(!IsVideoTrak())
	{
		return 0;
	}

	int nSampleIndex=nVideoFrame;
	DWORD32 durationSum=0;

	ASSERT(m_stts);

	int nFristSampleIndex = 0;
	for(UINT i=0;i<m_stts->m_stts_count;i++)
	{
		int sampleCount = m_stts->m_stts[i].SampleCount;
		if(nFristSampleIndex <= nSampleIndex && nSampleIndex < nFristSampleIndex + sampleCount)
		{
			int idx = nVideoFrame - nFristSampleIndex;
			durationSum += idx * m_stts->m_stts[i].SampleDuration;
			return durationSum;
		}
		
		nFristSampleIndex += sampleCount;
		durationSum += sampleCount * m_stts->m_stts[i].SampleDuration;
	}

	return durationSum;
}



#ifndef MIN
#define MIN(x,y)	((x)<(y)?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y)	((x)>(y)?(x):(y))
#endif

int CMP4Repair::Open(const char *pszMP4File)
{
	if(m_hFile)
	{
		ASSERT(FALSE);
		return -1;
	}

	m_szFile = pszMP4File;
	m_hFile = fopen(pszMP4File,"r+b");
	if(!m_hFile)
	{
		DW("fail to open mp4 file:[%s]",pszMP4File);
		return -1;
	}

	//����Ƿ�Ϊ�쳣��ֹ��mp4¼���ļ�
	//ͨ������JCOx���ж�
	m_fs = eMP4File_Unknown;

	long cbFileSize = 0;
	{
#ifdef _MSC_VER
		struct _stati64 buffer;
		int ret = _fstati64(m_hFile->_file,&buffer);
#else
		struct stat buffer;
		int ret = stat(pszMP4File, &buffer);
#endif
		if(ret==0)
		{
			cbFileSize = (long)buffer.st_size;
			m_cbFile = cbFileSize;
			
			//BYTE arr[1024];
			//size_t ret = fread(arr,1,sizeof(arr),m_hFile);
			//ret=0;
		}
	}
	
	BOOL32 bFind_mdat_atom = FALSE;
	while(1)
	{
		long pos = ftell(m_hFile);
		//printf("zhy[%s][%d] ftell pos[%ld] cbFileSize[%ld]\n", __FILE__, __LINE__, pos, cbFileSize);
		if(pos+sizeof(DWORD32)>cbFileSize)
		{
			break;
		}

		DWORD32 dwAtomSize = 0;
		int ret = fread(&dwAtomSize,1,sizeof(DWORD32),m_hFile);
		//printf("zhy[%s][%d] fread ret[%d] dwAtomSize[%uld]\n", __FILE__, __LINE__, ret, dwAtomSize);
		ASSERT(ret == sizeof(DWORD32));

		dwAtomSize = ntohl(dwAtomSize);
		
		if(pos + MAX(dwAtomSize,8) > cbFileSize)
		{
			DW("atom out of mp4 file end!");
			m_fs = eMP4File_Unknown;
			break;
		}

		char szAtom[5];
		szAtom[4]=0;
		fread(szAtom,1,4,m_hFile);
		//DT("atom=[%s]",szAtom);

		if(strcmp(szAtom,"mdat")==0)
		{
			bFind_mdat_atom=TRUE;
			m_pos_mdat = pos;
		}
		if(strcmp(szAtom,"moov")==0)
		{
			m_pos_moov = pos;
		}
		if(strcmp(szAtom,"JCOK")==0)
		{
			m_fs = eMP4File_JCOK;
			m_pos_jcox = pos;
			m_jcox_size = dwAtomSize;
		}
		else if(strcmp(szAtom,"JCOX")==0)
		{
			m_fs = eMP4File_JCOX;
			m_pos_jcox = pos;
			m_jcox_size = dwAtomSize;
			//printf("zhy eMP4File_JCOX XXX abnormal write m_pos_jcox[%ld] m_jcox_size[%ld]\n", m_pos_jcox, m_jcox_size);
		}
		else if(strcmp(szAtom,"JCOR")==0)
		{
			m_fs = eMP4File_JCOR;
			m_pos_jcox = pos;
			m_jcox_size = dwAtomSize;
		}

		if(dwAtomSize<=8)
		{
			break;//invalid atom,�쳣��ֹ¼��ʱ,����һ����mdat atom
		}

		DWORD32 posNextAtom = pos+dwAtomSize;
		fseek(m_hFile,posNextAtom,SEEK_SET);
	}

	if(!bFind_mdat_atom || m_pos_mdat == 0)
	{
		m_fs = eMP4File_Unknown;//û���ҵ�mdatʱ,�޷��޸����ļ�
	}

	return 0;
}


//CMP4Repair������ļ��������ǷǷ�mp4�ļ�,����Ҫ�ر�ע����ִ���Ĵ���
//��ʼ¼��ʱ����ΪJCOX,��ʾx,����¼����...,JCO��ʾJabsco logo
//��������¼����Ǹ�ΪJCOK,��ʾOK
//�޸����Ǹ�ΪJCOR,��ʾRepaired
CMP4Repair::CMP4Repair()
{
	m_hFile = NULL;
	Close();
}

CMP4Repair::~CMP4Repair()
{
	Close();
}

void CMP4Repair::Close()
{
	if(m_hFile)
	{
		fclose(m_hFile);
		m_hFile=0;
	}

	m_fs = eMP4File_Fail;
	m_pos_jcox=0;
	m_pos_mdat = 0;
	m_pos_moov=0;
	m_jcox_size=0;
	m_cbFile = 0;
	m_szFile="";
}

//�ж�mp4�ļ�״̬
eMP4FileStatus CMP4Repair::GetMP4FileStatus()
{
	printf("zhy[%s][%d] GetMP4FileStatus status[%d]\n", __FILE__, __LINE__, m_fs);
	return m_fs;
}

int CMP4Repair::Repair()
{
	//ȷ���ļ�����Ч��JCOx
	if(m_hFile == NULL || m_fs != eMP4File_JCOX
//#ifdef _DEBUG
		//�����ظ��޸�
		//&& m_fs != eMP4File_JCOK
		//&& m_fs != eMP4File_JCOR
//#endif
		)
	{
		return -1;
	}

	if(!m_pos_mdat)
	{
		ASSERT(m_pos_mdat);
		return -1;
	}

	fseek(m_hFile,m_pos_jcox+8+4,SEEK_SET);//seek to the first child atom
	//��ȡjcox��Ϣ
	DWORD32 dwTrakCount = 0;
	fread(&dwTrakCount,1,sizeof(dwTrakCount),m_hFile);
	dwTrakCount=ntohl(dwTrakCount);

	CMP4Write mp4Write;
	mp4Write.Attach(m_hFile);
	
	DWORD32 videoTrakId=0;
	DWORD32 audioTrakId=0;
	for(DWORD32 i=0;i<dwTrakCount;i++)
	{
		tag_atom_jcox_trak trak;
		int ret = fread(&trak,1,sizeof(trak),m_hFile);
		trak.trakid=ntohl(trak.trakid);
		trak.width=ntohl(trak.width);
		trak.height=ntohl(trak.height);
		trak.timescale=ntohl(trak.timescale);
		trak.fmtp_spropparametersets_len=ntohl(trak.fmtp_spropparametersets_len);
		char *sprop_parameter_sets=NULL;
		if(trak.fmtp_spropparametersets_len>0)
		{
			sprop_parameter_sets = new char [trak.fmtp_spropparametersets_len+1];
			fread(sprop_parameter_sets,1,trak.fmtp_spropparametersets_len,m_hFile);
			sprop_parameter_sets[trak.fmtp_spropparametersets_len]=0;
		}

		if(strncmp((char*)trak.handlerType,"vide",4)==0)
		{
			videoTrakId = trak.trakid;
			ret = mp4Write.InitVideo(trak.trakid,CSize(trak.width,trak.height),sprop_parameter_sets);
			ASSERT(ret==0);

		}
		else
		{
			audioTrakId = trak.trakid;
			ret = mp4Write.InitAudio(trak.trakid);
			ASSERT(ret==0);
		}

		if(sprop_parameter_sets)
		{
			delete []sprop_parameter_sets;
			sprop_parameter_sets = NULL;
		}
	}
	CMP4Trak * pVideoTrak = mp4Write.FindVideoTrak();
	CMP4Trak * pAudioTrak = mp4Write.FindAudioTrak();

	{
		//����mdat��tagMP4SampleHeader��ȡmp4������Ϣ
		//һֱ�������ļ�β����moov��
		//moov���ܴ���,������ʱ,��moov����д��moov
		//��moov������ʱ,���ļ�β��ʼ�½�moov
		long pos_mdat_end = m_cbFile;
		if(m_pos_moov)
		{
			pos_mdat_end = m_pos_moov;
		}

		fseek(m_hFile,m_pos_mdat+8,SEEK_SET);
		tagMP4SampleHeader sh;
		while(1)
		{
			long pos = ftell(m_hFile);
			if(pos+sizeof(tagMP4SampleHeader)>=pos_mdat_end)
			{
				break;
			}

			int ret = fread(&sh,1,sizeof(sh),m_hFile);
			ASSERT(ret == sizeof(sh));

			DWORD32 sz=sh.size;
			DWORD32 dur=sh.duration;
			sh.size = ((sz&0xFF0000)>>16) | (sz&0x00FF00) | ((sz&0xFF)<<16);
			sh.duration= ((dur&0xFF0000)>>16) | (dur&0x00FF00) | ((dur&0xFF)<<16);

			long pos_sample_end = pos+sizeof(tagMP4SampleHeader) + sh.size;
			if(pos_sample_end > pos_mdat_end)
			{
				//��ǰsampleֻ�ɹ�д��һ����!��Ϊ��Чsample
				break;
			}

			if(sh.trakid == videoTrakId && pVideoTrak)
			{
				//stts:ÿ��sample����Ҫ������duration
				{
					pVideoTrak->OnNewDuration(sh.duration);
				}

				//stsc:none,ֻ��Ҫһ��entry,�������ݶ�Ϊ1
				//stsc:����Э��ֻ��Ҫһ��entry,��VLC�����ǵĲ�������֧�ֵĲ���,����...
				{
					tagMP4Node_stsc* ps = NULL;//pVideoTrak->GetLastStscNode();
					if(!ps)
					{
						ps = new tagMP4Node_stsc;
						ps->FirstChunk=1;
						ps->SampleDescriptionID=1;
						pVideoTrak->m_lst_stsc.push_back(ps);
					}
					ps->SamplesPerChunk++;
				}

				//stsz:ÿ��sample����Ҫ������size
				{
					pVideoTrak->m_lst_stsz.push_back(sh.size);
				}

				//stco:ÿ��sample����Ҫ�����ľ���ƫ��offset
				{
					pVideoTrak->m_lst_stco.push_back(pos+sizeof(tagMP4SampleHeader));
				}

				//stss:�ؼ�֡�±�
				{
					if(sh.flags&0x01)
					{
						DWORD32 idx=pVideoTrak->m_lst_stsz.size();
						pVideoTrak->m_lst_stss.push_back(idx);
					}
				}
			}
			else if(sh.trakid == audioTrakId && pAudioTrak)
			{
				//stts:��Ҫsample������һ���̶���duration
				{
					pAudioTrak->OnNewDuration(sh.duration);
				}

				//stsc:ֻ��Ҫһ��entry
				//stsc:����Э��ֻ��Ҫһ��entry,��VLC�����ǵĲ�������֧�ֵĲ���,����...
				{
					tagMP4Node_stsc* ps = NULL;//pAudioTrak->GetLastStscNode();
					if(!ps)
					{
						ps = new tagMP4Node_stsc;
						ps->FirstChunk=1;
						ps->SampleDescriptionID=1;
						pAudioTrak->m_lst_stsc.push_back(ps);
					}
					ps->SamplesPerChunk++;
				}

				//stsz:none,��Ҫsample������Ҫһ���̶��Ĵ�С
				{
					if(pAudioTrak->m_lst_stsz.size()==0)
					{
						pAudioTrak->m_lst_stsz.push_back(sh.size);
					}
					else
					{
						ASSERT(sh.size==pAudioTrak->m_lst_stsz[0]);//audio packet should be the sample size
					}

					pAudioTrak->m_stsz_count++;
				}

				//stco:ÿ��sample����Ҫ�����ľ���ƫ��offset
				{
					pAudioTrak->m_lst_stco.push_back(pos+sizeof(tagMP4SampleHeader));
				}

			}
			else
			{
				//��Ч����!
				break;
			}
			
			long posNextSample = pos + sizeof(sh)+sh.size;
			ASSERT(posNextSample<=pos_mdat_end);
			fseek(m_hFile,posNextSample,SEEK_SET);
		}
	}
	long pos_new_moov = m_cbFile;
	if(m_pos_moov)
	{
		pos_new_moov = m_pos_moov;
	}
	
	fseek(m_hFile,pos_new_moov,SEEK_SET);
	mp4Write.m_pos_jcok=m_pos_jcox;
	mp4Write.m_pos_mdat=m_pos_mdat;
	mp4Write.m_pos_moov=m_pos_moov;

	mp4Write.Repair();
	mp4Write.Detach();
	return 0;
}



//export C Interface
MP4FileHandle MP4Create(const char *pathName, int flag)
{
	CMP4Write *p_mp4_write = new CMP4Write();
	if (NULL == p_mp4_write) return NULL;
	int ret = p_mp4_write->Open(pathName, flag);
	if (0 != ret) {
		printf("MP4 Write Open faile\n");
		return NULL;
	}
	return (MP4FileHandle)p_mp4_write;
}

int MP4AddVideoTrack(
	MP4FileHandle file,
	int videoType,
	int width, int height,
	char *sps, int sps_len,
	char *pps, int pps_len
	)
{
	if ((NULL == file) || (eVideoType_None == videoType)) return -1;
	if ((eVideoType_H264 == videoType) && ((NULL == sps) || (NULL == pps))) return -1;

	int ret = 0;
	CMP4Write *p_mp4_write = (CMP4Write*)file;
	switch (videoType) {
	case eVideoType_H264:{
		char sprop_param[512] = { 0 };
		char *sps_base64 = p_mp4_write->base64Encode(sps + 4, sps_len - 4);
		char *pps_base64 = p_mp4_write->base64Encode(pps + 4, pps_len - 4);
		strcpy(sprop_param, sps_base64);
		sprop_param[strlen(sps_base64)] = ',';
		strcpy(sprop_param + strlen(sps_base64) + 1, pps_base64);
		delete[] sps_base64;
		delete[] pps_base64;

		int type = RTP_TYPE_H264;
		ret = p_mp4_write->InitVideo(type, CSize(width, height), sprop_param);
		break;
	}
	case eVideoType_Mjpg:{
		return -1;
		break;
	}
	default:
		break;
	}
	return ret;
}

int MP4AddAudioTrack(
	MP4FileHandle file,
	int audioType,
	int audio_sample,
	int bit_per_sample,
	int channel
	)
{
	if (NULL == file) return -1;
	CMP4Write *p_mp4_write = (CMP4Write*)file;
	return p_mp4_write->InitAudio(audioType, audio_sample, bit_per_sample, channel);
}

int MP4WriteSample(
	MP4FileHandle file,
	int frameType, //0 audio ,1 video
	unsigned char* framebuf,
	const int framesize,
	double timeStamp
	)
{
	if (NULL == file) return -1;
	CMP4Write *p_mp4_write = (CMP4Write*)file;
	return p_mp4_write->WriteFrameEx(frameType, framebuf, framesize, timeStamp);
}

void MP4Close(MP4FileHandle file, int flag)
{
	if (NULL == file) return;
	if (MP4_Write == flag) {
		CMP4Write *p_mp4 = (CMP4Write*)file;
		p_mp4->Close();
	} else if (MP4_Read == flag) {
		CMP4Read *p_mp4 = (CMP4Read*)file;
		p_mp4->Close();
	}
}


MP4FileHandle MP4Open(const char *pathName)
{
	CMP4Read *p_mp4_read = new CMP4Read();
	if (0 > p_mp4_read->Open(pathName)) { return NULL; }
	return (MP4FileHandle)p_mp4_read;
}

int MP4GetInfo(MP4FileHandle file, MP4Info *info)
{
	if ((NULL == file) || (NULL == info)) {
		return -1;
	}
	CMP4Read *p_mp4_read = (CMP4Read*)file;

	tagMP4VideoInfo vi;
	p_mp4_read->GetVideoInfo(vi);
	if (false == vi.bValid) {
		return -1;
	}
	info->video_type = vi.video_type;
	info->width = vi.width;
	info->height = vi.height;
	info->fps = vi.fps;
	info->video_timecsclae = vi.timescale;
	info->video_duration = vi.duration;
	info->video_frame_count = vi.frame_count;

	tagMP4AudioInfo ai;
	p_mp4_read->GetAudioInfo(ai);
	if (ai.type != MP4_AUDIO_NU) {
		info->audio_type = ai.type;
		info->audio_sample = ai.sample;
		info->audio_channel = ai.channel;
		info->audio_duration = ai.duration;
		info->audio_frame_count = ai.frame_count;
		info->audio_bit_per_sample = ai.bit_per_sample;
		info->profile = ai.profile;
	}
	return 0;
}

int MP4GetH264SpsPps(MP4FileHandle file, unsigned char *sps, int *sps_len, unsigned char *pps, int *pps_len)
{
	if ((NULL == file) || (NULL == sps) || (NULL == pps)) {
		return -1;
	}

	CMP4Read *p_mp4_read = (CMP4Read*)file;
	tagMP4VideoInfo vi;
	p_mp4_read->GetVideoInfo(vi);
	if (false == vi.bValid) {
		return -1;
	}
	if (eVideoType_H264 != vi.video_type) return -1;

	unsigned char   sps_tmp[256] = { 0 };
	sps_tmp[3] = 1;
	unsigned char   pps_tmp[48] = { 0 };
	pps_tmp[3] = 1;
	int             sps_size = 0; 
	int             pps_size = 0;

	p_mp4_read->GetExtData(eExtData_sps, (LPBYTE)(&sps_tmp[4]), sizeof(sps_tmp) - 4, sps_size); 
	p_mp4_read->GetExtData(eExtData_pps, (LPBYTE)(&pps_tmp[4]), sizeof(pps_tmp) - 4, pps_size); 
	if ((sps_size == 0) || (pps_size == 0) || (sps_size > *sps_len) || (pps_size > *pps_len))
	{
		return -1;
	}
	
	*sps_len = sps_size +4 - 2;
	memcpy(sps, sps_tmp, *sps_len);
	*pps_len = pps_size +4 - 2;
	memcpy(pps, pps_tmp, *pps_len);
	return 0;
}

unsigned long MP4GetCurFrameIndex(MP4FileHandle file)
{
	if (NULL == file) {
		return -1;
	}
	CMP4Read *p_mp4_read = (CMP4Read*)file;
	return p_mp4_read->GetCurVideoFrame();
}

int MP4SeekByVideoIndex(MP4FileHandle file, int index)
{
	if ((NULL == file) || (0 > index)) {
		return -1;
	}
	CMP4Read *p_mp4_read = (CMP4Read*)file;
	return p_mp4_read->SeekToFrame(index);
}

int MP4SeekByTimeoffset(MP4FileHandle file, int timeoffset)
{
	if ((NULL == file) || (0 > timeoffset)) {
		return -1;
	}
	CMP4Read *p_mp4_read = (CMP4Read*)file;
	tagMP4VideoInfo vi;
	p_mp4_read->GetVideoInfo(vi);
	if (false == vi.bValid) {
		return -1;
	}
	int index = vi.fps * timeoffset;
	return MP4SeekByVideoIndex(file, index);
}

int MP4GetAudioFrame(MP4FileHandle file, unsigned char *buf, int *bufsize, unsigned int *duration)
{
	if ((NULL == file) || (NULL == buf) || (NULL == bufsize)) {
		return -1;
	}
	CMP4Read *p_mp4_read = (CMP4Read*)file;
	int ret = 0;
	int read_size = 0;
    DWORD32 dt;
	ret = p_mp4_read->ReadAudioFrame(buf, *bufsize, read_size, dt);
	if (ret < 0 || read_size <= 0) {
        *duration = 0;
        return -1;
    }
	*bufsize = read_size;
    *duration = dt;
	return 0;
}

int MP4GetVideoFrame(MP4FileHandle file, unsigned char *buf, int *bufsize, int need_key_frame, unsigned int *duration)
{
	if ((NULL == file) || (NULL == buf) || (NULL == bufsize)) {
		return -1;
	}
	CMP4Read *p_mp4_read = (CMP4Read*)file;
	int ret = 0;
	int read_size = 0;
    DWORD32 dt;
	while (1) {
		ret = p_mp4_read->ReadVideoFrame(buf, *bufsize, read_size, dt);
		if (ret < 0 || read_size <= 0) {
            *duration = 0;
            return -1;
        }
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 1;
		if ((1 == need_key_frame) && (buf[4] & 0x1f) == 5) break;
		if (0 == need_key_frame) break;
	}
    *duration = dt;
	*bufsize = read_size;
    
	return 0;
}

