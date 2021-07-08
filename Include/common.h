#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include "debug_log.h"
#include <unistd.h> 
#include <sys/prctl.h> 
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/types.h>
#include <time.h>
#include <sys/vfs.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef __BDAI_EN__
#define __BDAI_EN__
#endif

#ifndef KEY_PROBE
#define KEY_PROBE				1
#endif

#ifndef KEY_TRUE
#define KEY_TRUE				0
#endif

#ifndef KEY_OUT					
#define KEY_OUT					-2
#endif

//一般错误
#ifndef KEY_FALSE
#define KEY_FALSE				-1
#endif

#ifndef B_FALSE
#define B_FALSE				(0)
#endif

#ifndef B_TRUE
#define B_TRUE				(1)
#endif

typedef enum _ImageFormat_E
{
	Format_JPEG = 0x01,
	Format_PNG,	
	Format_GIF,
	Format_BMP,
	Format_NONE,
}ImageFormat_E;

typedef enum _ENUM_ERRORCODE_E
{
	KEY_NOTFOUNDCONF = -2,				//错误码:未找到配置表
	KEY_UNDEVICEAUTHORIZED = -401, 		//错误码:设备鉴权失败
	KEY_CMDNOTFOUND = -404,				//错误码:找不到cmd对象
	KEY_OK = 200,
	KEY_ERROR = 400,  					//错误码:请求错误
	KEY_UNAUTHORIZED,					//错误码:鉴权失败(用户组)
	KEY_USERREPEATLOGIN,	 			//错误码:用户重复登录
	KEY_FORBIDDEN, 						//错误码:禁止访问
	KEY_NOTFOUND, 						//错误码:找不到对象
	KEY_METHODNOTALLOWED, 				//错误码:没有权限
	KEY_TIMEOUT, 						//错误码:超时
	KEY_USERREPEAT, 					//错误码:用户重复注册
	KEY_DEVICEHADBIND,					//错误码:设备被绑定
	KEY_SERVEROFFLINE,					//错误码:服务器不在线
	KEY_DEVICEOFFLINE,					//错误码:设备不在线
	KEY_MAXUSRNUM,						//错误码:达到最大用户数
	KEY_MAXDEVICENUM,					//错误码:达到设备最大数
	KEY_ERRAUTHCODE,					//错误码:注册码错误
	KEY_NORESPONSE,						//错误码:未响应/不回复
	KEY_DEVICEUNBIND,					//错误码:设备未绑定
	KEY_DEVICEUNACTIVATION,				//错误码:设备未激活
	KEY_PRIMARYKEYERROR,				//错误码:主键错误
	KEY_NOTFACE,						//错误码:找不到人脸
	KEY_FACEUSERREPEAT,					//错误码:人脸重复注册
}ERROR_CODE_E;

#ifndef _OUT_
#define _OUT_
#endif 

#ifndef _IN_
#define _IN_
#endif 


#ifndef WIN32
#define  SleepMs(ms) usleep((ms)*1000)
#define MyFree(p) (free(p), p=NULL) 
#else
#define SleepMs(ms) Sleep(ms)
#endif

#ifndef MUTEX
#define MUTEX
#define MUTEX_LOCK(mutex)	pthread_mutex_lock(mutex)
#define MUTEX_UNLOCK(mutex)	pthread_mutex_unlock(mutex)
#define MUTEX_INIT(mutex)	pthread_mutex_init(mutex,NULL)
#define MUTEX_DESTROY(mutex)	pthread_mutex_destroy(mutex)
#endif

#define DF_MAX_CHN 		 			3
#define DF_MAX_STREAM 		 		1
#define DF_MAX_URLSTRLEN 			128
#define DF_MAX_SUPPORTRESLUTION		32 
#define DF_MAX_ENCODETYPE	 		4
#define DF_MAX_NETETHNUM			10

#ifndef DF_MAX_USRNUM
#define DF_MAX_USRNUM				10
#endif

#ifndef DF_MAXLEN_USRNAME
#define DF_MAXLEN_USRNAME			128
#endif

#ifndef DF_MAXLEN_PASSWD
#define DF_MAXLEN_PASSWD			128
#endif

#ifndef DF_UUID_LEN
#define DF_UUID_LEN					128
#endif

#ifndef DF_MAX_DEVICESHAR
#define DF_MAX_DEVICESHAR			3
#endif

#define DF_USERDEVICENUM_MAX		128

typedef enum _ENUM_TERMINAL_TYPE_E
{
	TERMINAL_TYPE_PC = 0x00,
	TERMINAL_TYPE_APP,
	TERMINAL_TYPE_IPC,
	TERMINAL_TYPE_SUBSERVER,
	TERMINAL_TYPE_OTHERS,
}ENUM_TERMINAL_TYPE_E;

typedef enum _E_ALARMTYPE
{
	ALARMTYPE_NULL = 0x00,
	ALARMTYPE_MOTION = 1 << 0,
	ALARMTYPE_BLIND = 1 << 1,
	ALARMTYPE_IO1 = 1 << 2,
	ALARMTYPE_IO2 = 1 << 3,
	ALARMTYPE_SDCARD = 1 << 4,
	ALARMTYPE_SDCARD1 = 1 << 5,
	ALARMTYPE_LOWBAT = 0x41,
	ALARMTYPE_PIR = 0x42,
	ALARMTYPE_LOWFU = 0x43,
}E_ALARMTYPE;

typedef enum _ENUM_ENABLED_E
{
	ENUM_ENABLED_FALSE = 0x00,
	ENUM_ENABLED_TRUE,
}ENUM_ENABLED_E;

typedef enum _ENUM_STATUS_E
{
	ENUM_STATUS_OFFLINE = 0x00,
	ENUM_STATUS_ONLINE,
}ENUM_STATUS_E;

typedef struct _TimeYMD_T
{
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
}TimeYMD_T;

typedef struct _Rectangle_T
{
	int pointX;
	int pointY;
	int width;
	int high;
}Rectangle_T;

#ifdef __cplusplus
}
#endif

#endif
