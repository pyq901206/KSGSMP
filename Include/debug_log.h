#ifndef __DEBUG_LOG_H__
#define __DEBUG_LOG_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#define __DEBUG_VERSION__

typedef enum 
{
	LOG_ERR = 1,	/* 严重错误 */
	LOG_WAR = 2,	/* 警告 */
	LOG_NOT = 4,	/* 通知 */
	LOG_DBG = 8,	/* 只是调试用的日志 */
	LOG_LAST = 16,
}LOG_LEV_EN;

#define LOG_TTY		0x00 /* 串口 */
#define LOG_FLASH	0x01 /* FLASH */

/* 本地默认的日志输出方向和级别，当读配置文件失败时，取此值 */
#define LOG_LEVEL	LOG_DBG

#define LOGPARAM_INIT     0x00  /* 第一次设置日志参数 */
#define LOGPARAM_UPDATE 0x01  /* 更新日志参数 */

#define LOG_WORK_PATH(file_name)		 "log/"#file_name

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
void LOG(int iLevel, int iLine, char *pcFile, const char *pcFunc, const char *fmt, ...);

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
int LogInit(char *pcFilePath, char *pcFilePrefix, int iMaxFileSize,\
                    int iMaxFileCount, int iLevel, int iDirect, int iParamChangeState);

/****************************************************************************************
函数名称：LogDestroy
函数功能：释放LOG资源
入口参数：无
出口参数：无
返回值：成功返回0，失败返回1
****************************************************************************************/
void LogDestroy(void);
void LogResetTimes(void);
int DisplayMemSize(void);

#ifdef __DEBUG_VERSION__
/* 若为调试版本，日志都需要打印，方便调试 */ 
#define DF_ERROR(fmt,args...)		LOG(LOG_ERR, __LINE__,__FILE__,__FUNCTION__,fmt,## args)
#define DF_WARN(fmt,args...)		LOG(LOG_WAR,__LINE__,__FILE__,__FUNCTION__,fmt,## args)
#define DF_NOT(fmt,args...)   	LOG(LOG_NOT,__LINE__,__FILE__,__FUNCTION__,fmt,## args)
#define DF_DEBUG(fmt,args...)   	LOG(LOG_DBG,__LINE__,__FILE__,__FUNCTION__,fmt,## args)
#define DF_TTT(fmt,args...)   	LOGT(LOG_WAR,fmt,## args)
#else
/* 若为发布版本，只打印错误日志,以免CPU使用率过高 */
#define DF_ERROR(fmt,args...)   LOG(LOG_ERR, __LINE__,__FILE__,__FUNCTION__,fmt,## args)
#define DF_WARN(fmt,args...)  
#define DF_NOT(fmt,args...)   
#define DF_DEBUG(fmt,args...)  
#endif

#ifdef __cplusplus
}
#endif

//#define     LOGFTPDBG(fmt,args...)   LOG(LOG_DBG,"FTP",__LINE__,__FILE__,__FUNCTION__,fmt,## args)
#endif

