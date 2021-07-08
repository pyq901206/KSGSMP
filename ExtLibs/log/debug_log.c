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

#define MAX_LOG_SIZE					1048576 /*大小为字节，默认为1M=1024 * 1024Byte*/
#define MAX_FILE_CNT					5 		/* 日志最大为5M */

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

/* 定义LOG_FILE结构体 */
typedef struct LOG_FILE_s
{
	char pcFilePath[256];			/* Log当前文件路径 */
	char pcFilePrefix[12];			/* Log前缀 */
	int iMaxFileSize;				/* Log大小 */
	int iMaxFileCount;				/* Log文件个数，主要控制空间大小 */

	FILE *pfFileHandle;			/* 文件句柄 */ 
	int iSeq;						/* Log文件编号 */
	int iSize;						/* 当前文件大小 */
	int iLevel;					/* 打印级别 */
	int iDirect;					/* 是否写日志文件，还是直接输出控制台 */
	pthread_mutex_t sLockMutex;	/* 线程锁 */
       
} LOG_FILE_T;

static LOG_FILE_T sg_LogArgs;

/****************************************************************************************
函数名称：logGetSeqID
函数功能：读取LOG目录下的log_id.dat文件获取当前LOG日志号
入口参数：无
出口参数：无
返回值：成功返回0
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
函数名称：logGetFSize
函数功能：得到当前索引日志的大小
入口参数：无
出口参数：无
返回值：成功返回0
****************************************************************************************/
static void logGetFSize(void)
{
	char acPath[512];
	struct stat stBuf;

	memset(&stBuf, 0, sizeof(stBuf));
	sprintf(acPath, "%s%s%02d.log", sg_LogArgs.pcFilePath, \
		sg_LogArgs.pcFilePrefix, sg_LogArgs.iSeq);
	stat(acPath, &stBuf);//得到指定的文件的信息
	sg_LogArgs.iSize = stBuf.st_size; 
}

/****************************************************************************************
函数名称：logSetSeqID
函数功能：设置LOG目录下的log_id.dat文件获取当前LOG日志号
入口参数：无
出口参数：无
返回值：成功返回0
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
函数名称：logChangeFile
函数功能：切换日志文件
入口参数：无
出口参数：无
返回值：成功返回0
****************************************************************************************/
static void logChangeFile(void)
{
	char acPath[512];

	/*回写旧文件*/
	if (sg_LogArgs.pfFileHandle != NULL)
	{
		fflush(sg_LogArgs.pfFileHandle);
		fclose(sg_LogArgs.pfFileHandle);
	}
	
	/*创建新文件*/
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
函数名称：LogInit
函数功能：主要初始化写Log文件中的一些参数
入口参数：pcLogPath--Log文件路径，如:/irlab/log/
		pcLogPrefix--Log文件名前缀
		iMaxFileSize--最多Log文件数
		iMaxFileCount--都取0时，表示取默认值 5个最大为1M的文件
     	iLevel--打印级别
			   0x01 严重错误 
			   0x02 警告 
	             0x04 通知
	             0x08 只是调试用的日志	                                      
	     iDirect--是否写日志文件，还是直接输出控制台
	              0x00 串口
	              0x01 FLASH
	     iParamChangeState--更新 or 第一次初始化日志参数.
	              0x00 第一次设置日志参数
	              0x01 更新日志参数
出口参数：无
返回值：成功返回0，失败返回1
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
	{	/* 配置文件日志路径 */
		strcpy(sg_LogArgs.pcFilePath, LOG_WORK_PATH());
	}
	
	/* 目录不存，则创建 */
	if (access(pcFilePath, F_OK) < 0)
	{
		mkdir(pcFilePath, 0777);
	}
	/* 在目录最后加上/ */
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
	/* 配置文件打印级别 */
	sg_LogArgs.iLevel = iLevel;
	
	/* 配置文件日志输出方向 */
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
	
	/* 得到SEQ */
	if (logGetSeqID())
	{
		logSetSeqID(); 
	}
	/* 得到文件大小 */
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
		/* 若程序运行的日志是写入文件，若显示日志文件名 */
		if(LOG_FLASH == iDirect)
		{
			printf("Log file is %s\n", acPath);
		}
		return 0;
	}
}

/****************************************************************************************
函数名称：LogDestroy
函数功能：释放LOG资源
入口参数：无
出口参数：无
返回值：成功返回0，失败返回1
****************************************************************************************/
void LogDestroy(void)
{
	pthread_mutex_lock(&sg_LogArgs.sLockMutex);
	/*清空文件中的缓冲和记录写文件中的位置*/
	if (sg_LogArgs.pfFileHandle != NULL)
	{
		fclose(sg_LogArgs.pfFileHandle);
	}

	pthread_mutex_unlock(&sg_LogArgs.sLockMutex);
	pthread_mutex_destroy(&sg_LogArgs.sLockMutex);
}

/****************************************************************************************
函数名称：LOG
函数功能：日志打印函数
入口参数：iLevel--LOG_ERR 严重错误LOG_WAN 警告LOG_DBG 只是调试用的日志
		iLine--Log信息所在行
		pcFile--Log信息所在文件
		pcFunc--Log信息所在函数
		fmt--Log信息格式化输出
出口参数：无
返回值：无
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

	/*判断文件是否满*/
	if (sg_LogArgs.iSize >= sg_LogArgs.iMaxFileSize)
	{
		fflush(sg_LogArgs.pfFileHandle);
		logChangeFile();
	}
	/* 得到系统时间，主要得到微秒时间 */
	gettimeofday(&stDetailTtime, NULL);
	localtime_r(&stDetailTtime.tv_sec, &stTime);
	/* 每行Log内容前面加上时间 */
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
函数名称：LOG
函数功能：日志打印函数
入口参数：iLevel--LOG_ERR 严重错误LOG_WAN 警告LOG_DBG 只是调试用的日志
		iLine--Log信息所在行
		pcFile--Log信息所在文件
		pcFunc--Log信息所在函数
		fmt--Log信息格式化输出
出口参数：无
返回值：无
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

	/*判断文件是否满*/
	if (sg_LogArgs.iSize >= sg_LogArgs.iMaxFileSize)
	{
		fflush(sg_LogArgs.pfFileHandle);
		logChangeFile();
	}
	/* 得到系统时间，主要得到微秒时间 */
	gettimeofday(&stDetailTtime, NULL);
	localtime_r(&stDetailTtime.tv_sec, &stTime);
	/* 每行Log内容前面加上时间 */
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
		/* 得到系统时间，主要得到微秒时间 */
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
	long uptime;            /* 启动到现在经过的时间 */
	unsigned long loads[3]; /* 1, 5, and 15 minute load averages */
	unsigned long totalram;  /* 总的可用的内存大小 */
	unsigned long freeram;   /* 还未被使用的内存大小 */
	unsigned long sharedram; /* 共享的存储器的大小*/
	unsigned long bufferram; /* 共享的存储器的大小 */
	unsigned long totalswap; /* 交换区大小 */
	unsigned long freeswap;  /* 还可用的交换区大小 */
	unsigned short procs;    /* 当前进程数目 */
	unsigned long totalhigh; /* 总的高内存大小 */
	unsigned long freehigh;  /* 可用的高内存大小 */
	unsigned int mem_unit;   /* 以字节为单位的内存大小 */
	char _f[20-2*sizeof(long)-sizeof(int)]; /* libc5的补丁*/
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

