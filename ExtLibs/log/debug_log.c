#ifdef __cplusplus
extern "C"{
#endif

/* Standard Linux headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "debug_log.h"
#include <sys/sysinfo.h>

#define MAX_LOG_SIZE					1048576 /*��СΪ�ֽڣ�Ĭ��Ϊ1M=1024 * 1024Byte*/
#define MAX_FILE_CNT					5 		/* ��־���Ϊ5M */

const char *LOG_LEV[] =
{
	"",
	"ERR",
	"WAN",
	"",
	"NOT",
	"",
	"",
	"",
	"DBG",
};

/* ����LOG_FILE�ṹ�� */
typedef struct LOG_FILE_s
{
	char pcFilePath[256];			/* Log��ǰ�ļ�·�� */
	char pcFilePrefix[12];			/* Logǰ׺ */
	int iMaxFileSize;				/* Log��С */
	int iMaxFileCount;				/* Log�ļ���������Ҫ���ƿռ��С */

	FILE *pfFileHandle;			/* �ļ���� */ 
	int iSeq;						/* Log�ļ���� */
	int iSize;						/* ��ǰ�ļ���С */
	int iLevel;					/* ��ӡ���� */
	int iDirect;					/* �Ƿ�д��־�ļ�������ֱ���������̨ */
	pthread_mutex_t sLockMutex;	/* �߳��� */
       
} LOG_FILE_T;

static LOG_FILE_T sg_LogArgs;

/****************************************************************************************
�������ƣ�logGetSeqID
�������ܣ���ȡLOGĿ¼�µ�log_id.dat�ļ���ȡ��ǰLOG��־��
��ڲ�������
���ڲ�������
����ֵ���ɹ�����0
****************************************************************************************/
static int logGetSeqID(void)
{
	char acPath[512], acTemp[64];
	FILE *pIDFile = NULL;
	
	sprintf(acPath, "%s%sid.dat", sg_LogArgs.pcFilePath, sg_LogArgs.pcFilePrefix);
	pIDFile = fopen(acPath, "r");
	if (pIDFile != NULL)
	{
		if (fgets(acTemp, 64, pIDFile))
		{
			sg_LogArgs.iSeq = atoi(acTemp);
		}
		
		fclose(pIDFile);
		
		return 0;
	}
	
	return 1;
}

/****************************************************************************************
�������ƣ�logGetFSize
�������ܣ��õ���ǰ������־�Ĵ�С
��ڲ�������
���ڲ�������
����ֵ���ɹ�����0
****************************************************************************************/
static void logGetFSize(void)
{
	char acPath[512];
	struct stat stBuf;

	memset(&stBuf, 0, sizeof(stBuf));
	sprintf(acPath, "%s%s%02d.log", sg_LogArgs.pcFilePath, \
		sg_LogArgs.pcFilePrefix, sg_LogArgs.iSeq);
	stat(acPath, &stBuf);//�õ�ָ�����ļ�����Ϣ
	sg_LogArgs.iSize = stBuf.st_size; 
}

/****************************************************************************************
�������ƣ�logSetSeqID
�������ܣ�����LOGĿ¼�µ�log_id.dat�ļ���ȡ��ǰLOG��־��
��ڲ�������
���ڲ�������
����ֵ���ɹ�����0
****************************************************************************************/
static void logSetSeqID(void)
{
	char acPath[512], acTemp[64];
	FILE *pIDFile;
	
	sprintf(acPath, "%s%sid.dat", sg_LogArgs.pcFilePath, sg_LogArgs.pcFilePrefix);
	pIDFile = fopen(acPath, "w");
	if (pIDFile != NULL)
	{
		sprintf(acTemp, "%d", sg_LogArgs.iSeq);
		fputs(acTemp, pIDFile);
		fclose(pIDFile);
	}
}

/****************************************************************************************
�������ƣ�logChangeFile
�������ܣ��л���־�ļ�
��ڲ�������
���ڲ�������
����ֵ���ɹ�����0
****************************************************************************************/
static void logChangeFile(void)
{
	char acPath[512];

	/*��д���ļ�*/
	if (sg_LogArgs.pfFileHandle != NULL)
	{
		fflush(sg_LogArgs.pfFileHandle);
		fclose(sg_LogArgs.pfFileHandle);
	}
	
	/*�������ļ�*/
	sg_LogArgs.iSeq ++;
	if (sg_LogArgs.iSeq > sg_LogArgs.iMaxFileCount)
	{
		sg_LogArgs.iSeq = 1;
	}
	sprintf(acPath, "%s%s%02d.log", sg_LogArgs.pcFilePath, \
		sg_LogArgs.pcFilePrefix, sg_LogArgs.iSeq);
	unlink(acPath);
	sg_LogArgs.pfFileHandle = fopen(acPath, "a+");
	sg_LogArgs.iSize = 0;
	logSetSeqID();
}

/****************************************************************************************
�������ƣ�LogInit
�������ܣ���Ҫ��ʼ��дLog�ļ��е�һЩ����
��ڲ�����pcLogPath--Log�ļ�·������:/irlab/log/
		pcLogPrefix--Log�ļ���ǰ׺
		iMaxFileSize--���Log�ļ���
		iMaxFileCount--��ȡ0ʱ����ʾȡĬ��ֵ 5�����Ϊ1M���ļ�
     	iLevel--��ӡ����
			   0x01 ���ش��� 
			   0x02 ���� 
	             0x04 ֪ͨ
	             0x08 ֻ�ǵ����õ���־	                                      
	     iDirect--�Ƿ�д��־�ļ�������ֱ���������̨
	              0x00 ����
	              0x01 FLASH
	     iParamChangeState--���� or ��һ�γ�ʼ����־����.
	              0x00 ��һ��������־����
	              0x01 ������־����
���ڲ�������
����ֵ���ɹ�����0��ʧ�ܷ���1
****************************************************************************************/
int LogInit(char *pcFilePath, char *pcFilePrefix, int iMaxFileSize, \
                   int iMaxFileCount, int iLevel, int iDirect, int iParamChangeState)
{
	char acPath[512];
	
	if (pcFilePath != NULL)
	{
		strncpy(sg_LogArgs.pcFilePath, pcFilePath, 256);
	}
	else
	{	/* �����ļ���־·�� */
		strcpy(sg_LogArgs.pcFilePath, LOG_WORK_PATH());
	}
	
	/* Ŀ¼���棬�򴴽� */
	if (access(pcFilePath, F_OK) < 0)
	{
		mkdir(pcFilePath, 0777);
	}
	/* ��Ŀ¼������/ */
	if (sg_LogArgs.pcFilePath[strlen(sg_LogArgs.pcFilePath) -1] != '/')
	{
		strcat(sg_LogArgs.pcFilePath, "/");
	}

	if (pcFilePrefix != NULL)
	{
		strncpy(sg_LogArgs.pcFilePrefix, pcFilePrefix, 12);
	}
	else
	{
		strcpy(sg_LogArgs.pcFilePrefix, "ipcam_");
	}
	/* �����ļ���ӡ���� */
	sg_LogArgs.iLevel = iLevel;
	
	/* �����ļ���־������� */
	sg_LogArgs.iDirect = iDirect;

	if (iMaxFileSize > MAX_LOG_SIZE || iMaxFileSize == 0)
	{
		iMaxFileSize = MAX_LOG_SIZE;
	}
	if (iMaxFileCount > MAX_FILE_CNT || iMaxFileCount == 0)
	{
		iMaxFileCount = MAX_FILE_CNT;
	}

	sg_LogArgs.iMaxFileSize = iMaxFileSize;
	sg_LogArgs.iMaxFileCount = iMaxFileCount;
	sg_LogArgs.iSeq = 1;

	if (LOGPARAM_UPDATE == iParamChangeState)
	{
		LogDestroy();
	}

	pthread_mutex_init(&sg_LogArgs.sLockMutex, NULL);
	
	/* �õ�SEQ */
	if (logGetSeqID())
	{
		logSetSeqID(); 
	}
	/* �õ��ļ���С */
	logGetFSize();

	sprintf(acPath,"%s%s%02d.log", sg_LogArgs.pcFilePath, \
		sg_LogArgs.pcFilePrefix, sg_LogArgs.iSeq);
	if ((sg_LogArgs.pfFileHandle = fopen(acPath, "a+")) == NULL)
	{
		printf("fopen %s failed\n", acPath);
		return 1;
	}
	else
	{
		/* ���������е���־��д���ļ�������ʾ��־�ļ��� */
		if(LOG_FLASH == iDirect)
		{
			printf("Log file is %s\n", acPath);
		}
		return 0;
	}
}

/****************************************************************************************
�������ƣ�LogDestroy
�������ܣ��ͷ�LOG��Դ
��ڲ�������
���ڲ�������
����ֵ���ɹ�����0��ʧ�ܷ���1
****************************************************************************************/
void LogDestroy(void)
{
	pthread_mutex_lock(&sg_LogArgs.sLockMutex);
	/*����ļ��еĻ���ͼ�¼д�ļ��е�λ��*/
	if (sg_LogArgs.pfFileHandle != NULL)
	{
		fclose(sg_LogArgs.pfFileHandle);
	}

	pthread_mutex_unlock(&sg_LogArgs.sLockMutex);
	pthread_mutex_destroy(&sg_LogArgs.sLockMutex);
}

/****************************************************************************************
�������ƣ�LOG
�������ܣ���־��ӡ����
��ڲ�����iLevel--LOG_ERR ���ش���LOG_WAN ����LOG_DBG ֻ�ǵ����õ���־
		iLine--Log��Ϣ������
		pcFile--Log��Ϣ�����ļ�
		pcFunc--Log��Ϣ���ں���
		fmt--Log��Ϣ��ʽ�����
���ڲ�������
����ֵ����
****************************************************************************************/
void LOG(int iLevel, int iLine, char *pcFile, const char *pcFunc, const char *fmt, ...)
{
	char acTime[32];
	struct tm stTime;
	struct timeval stDetailTtime;
	FILE *pOut = NULL;
	int iCnt = 0;
	
	#if 1
	if(!(iLevel & sg_LogArgs.iLevel))
	{
		return;
	}
	#endif

	acTime[0] = 0;
	
	pthread_mutex_lock(&sg_LogArgs.sLockMutex);

	/*�ж��ļ��Ƿ���*/
	if (sg_LogArgs.iSize >= sg_LogArgs.iMaxFileSize)
	{
		fflush(sg_LogArgs.pfFileHandle);
		logChangeFile();
	}
	/* �õ�ϵͳʱ�䣬��Ҫ�õ�΢��ʱ�� */
	gettimeofday(&stDetailTtime, NULL);
	localtime_r(&stDetailTtime.tv_sec, &stTime);
	/* ÿ��Log����ǰ�����ʱ�� */
	memset(acTime,0,sizeof(acTime));
	sprintf(acTime, "%04d/%02d/%02d %02d:%02d:%02d:%03ld", stTime.tm_year+1900, \
		stTime.tm_mon + 1, stTime.tm_mday, stTime.tm_hour, stTime.tm_min, stTime.tm_sec, 
		stDetailTtime.tv_usec / 1000);

	va_list vap;
	va_start(vap, fmt);

	pOut = NULL;
	if (sg_LogArgs.iDirect == LOG_TTY)
	{
		pOut = stdout;
	}
	else
	{
		fflush(sg_LogArgs.pfFileHandle);
		pOut = sg_LogArgs.pfFileHandle;
	}

	if (pOut != NULL)
	{
		iCnt = fprintf(pOut, "[%s]-[%s][%s][%s][%d]", LOG_LEV[iLevel%LOG_LAST], \
			acTime, pcFunc, pcFile, iLine);
		iCnt += vfprintf(pOut, fmt, vap);
		iCnt += vfprintf(pOut, "\r\n", vap);
		va_end(vap);
		sg_LogArgs.iSize += iCnt;
	}
	pthread_mutex_unlock(&sg_LogArgs.sLockMutex);
}

/****************************************************************************************
�������ƣ�LOG
�������ܣ���־��ӡ����
��ڲ�����iLevel--LOG_ERR ���ش���LOG_WAN ����LOG_DBG ֻ�ǵ����õ���־
		iLine--Log��Ϣ������
		pcFile--Log��Ϣ�����ļ�
		pcFunc--Log��Ϣ���ں���
		fmt--Log��Ϣ��ʽ�����
���ڲ�������
����ֵ����
****************************************************************************************/
void LOGT(int iLevel,const char *fmt, ...)
{
	char acTime[32];
	struct tm stTime;
	struct timeval stDetailTtime;
	FILE *pOut = NULL;
	int iCnt = 0;

	#if 1
	if(!(iLevel & sg_LogArgs.iLevel))
	{
		return;
	}
	#endif
	
	pthread_mutex_lock(&sg_LogArgs.sLockMutex);

	/*�ж��ļ��Ƿ���*/
	if (sg_LogArgs.iSize >= sg_LogArgs.iMaxFileSize)
	{
		fflush(sg_LogArgs.pfFileHandle);
		logChangeFile();
	}
	/* �õ�ϵͳʱ�䣬��Ҫ�õ�΢��ʱ�� */
	gettimeofday(&stDetailTtime, NULL);
	localtime_r(&stDetailTtime.tv_sec, &stTime);
	/* ÿ��Log����ǰ�����ʱ�� */
	memset(acTime,0,sizeof(acTime));
	#if 0
	sprintf(acTime, "%04d/%02d/%02d %02d:%02d:%02d:%03ld",\
		stTime.tm_year+1900,stTime.tm_mon + 1, stTime.tm_mday,\
		stTime.tm_hour, stTime.tm_min, stTime.tm_sec,stDetailTtime.tv_usec / 1000);
	#else
	sprintf(acTime, "%02d/%02d %02d:%02d:%02d:%03ld",\
		stTime.tm_mon + 1, stTime.tm_mday,\
		stTime.tm_hour, stTime.tm_min, stTime.tm_sec,stDetailTtime.tv_usec / 1000);
	#endif

	va_list vap;
	va_start(vap, fmt);

	pOut = NULL;
	if (sg_LogArgs.iDirect == LOG_TTY)
	{
		pOut = stdout;
	}
	else
	{
		fflush(sg_LogArgs.pfFileHandle);
		pOut = sg_LogArgs.pfFileHandle;
	}

	if (pOut != NULL)
	{
		iCnt = fprintf(pOut, "[%s]",acTime);
		iCnt += vfprintf(pOut, fmt, vap);
		va_end(vap);
		sg_LogArgs.iSize += iCnt;
	}
	pthread_mutex_unlock(&sg_LogArgs.sLockMutex);
	return;
}
static unsigned char resetflag=1;

#if 0
void LogResetTimes(void)
{
	if(resetflag)
	{
		FILE *fp=NULL;
		char acTime[32];
		int filesize=0;
		struct tm stTime;
		struct timeval stDetailTtime;
		/* �õ�ϵͳʱ�䣬��Ҫ�õ�΢��ʱ�� */
		gettimeofday(&stDetailTtime, NULL);
		localtime_r(&stDetailTtime.tv_sec, &stTime);
		if(stTime.tm_year > 70)
		{
			resetflag=0;
		}
		else
		{
			return;
		}
		memset(acTime,0,sizeof(acTime));
		sprintf(acTime, "%04d/%02d/%02d %02d:%02d:%02d:%03ld",\
			stTime.tm_year+1900,stTime.tm_mon + 1, stTime.tm_mday,\
			stTime.tm_hour, stTime.tm_min, stTime.tm_sec,stDetailTtime.tv_usec / 1000);
		fp = fopen(MAIN_WORK_PATH(reset.ini),"r+");
		if(NULL != fp)
		{
			fseek(fp,0,SEEK_END);
			filesize=ftell(fp);
			if(filesize != 0) 
			{
				if(filesize <= 1024)
				{
					fseek(fp,0,SEEK_END);
					fprintf(fp, "[%s]\n",acTime);
				}
				else
				{
					fclose(fp);
					fp = fopen(MAIN_WORK_PATH(reset.ini),"w+");
					fprintf(fp, "[%s]\n",acTime);
				}
			}
			fclose(fp);
		}
		else
		{
			fp = fopen(MAIN_WORK_PATH(reset.ini),"w+");
			fprintf(fp, "[%s]\n",acTime);
			fclose(fp);
		}
	}
	else
	{
		return;
	}
}
#endif

#if 0
struct sysinfo 
{
	long uptime;            /* ���������ھ�����ʱ�� */
	unsigned long loads[3]; /* 1, 5, and 15 minute load averages */
	unsigned long totalram;  /* �ܵĿ��õ��ڴ��С */
	unsigned long freeram;   /* ��δ��ʹ�õ��ڴ��С */
	unsigned long sharedram; /* ����Ĵ洢���Ĵ�С*/
	unsigned long bufferram; /* ����Ĵ洢���Ĵ�С */
	unsigned long totalswap; /* ��������С */
	unsigned long freeswap;  /* �����õĽ�������С */
	unsigned short procs;    /* ��ǰ������Ŀ */
	unsigned long totalhigh; /* �ܵĸ��ڴ��С */
	unsigned long freehigh;  /* ���õĸ��ڴ��С */
	unsigned int mem_unit;   /* ���ֽ�Ϊ��λ���ڴ��С */
	char _f[20-2*sizeof(long)-sizeof(int)]; /* libc5�Ĳ���*/
};
#endif
int DisplayMemSize(void)
{
    struct sysinfo si;
    sysinfo(&si);
    return 0;
}

#ifdef __cplusplus
}
#endif

