/****************************************************************************************
文件名称：log.h
作 者：罗君华
说 明：写日志文件及查询日志信息功能
修改记录：

当前版本：1.1  作 者：黄信国  日期：2011.08.12
说 明：修改文件格式和代码注释

上一版本：1.0  作 者：罗君华  日期：2011.08.09
说 明：创建文件
****************************************************************************************/

#ifndef __LOG_H__
#define __LOG_H__

#define LOG_TYPE_VLOSS_ALARM		1
#define LOG_TYPE_SENSOR_ALARM	2
#define LOG_TYPE_MOTION_ALARM	3
#define LOG_TYPE_OPERATION		4
#define LOG_TYPE_INIT_ERROR		5
#define LOG_TYPE_START_ERROR		6
#define LOG_TYPE_VLOSS_ALARM_DISABLE   		7
#define LOG_TYPE_MOTION_ALARM_DISABLE  	8
#define LOG_TYPE_SENSOR_ALARM_DISABLE  	9
#define LOG_TYPE_BLIND_ALARM				10        // by tanqw 20150410   遮挡告警
#define LOG_TYPE_BLIND_ALARM_DISABLE  		11


#define ERR_VIDEO_IN_OPEN			1
#define ERR_AUDIO_IN_OPEN			2
#define ERR_H264_CREATE_GROUP	3
#define ERR_H264_ENC_OPEN			4
#define ERR_JPEG_OPEN				5
#define ERR_AIENC_OPEN				6
#define ERR_VMASK_REGION_OPEN	7
#define ERR_MOTION_DETECT_OPEN	8
#define ERR_OSD_REGION_OPEN		9
#define ERR_AUDIOOUT_OPEN			10
#define ERR_AUDIODEC_OPEN			11
#define ERR_NET_CTRL_SVR_OPEN	12
#define ERR_NET_MEDIA_SVR_OPEN	13
#define ERR_RS232_OPEN				14
#define ERR_RS232_SET_PARAM		15
#define ERR_RS485_OPEN				16
#define ERR_RS485_SET_PARAM		17
#define ERR_VIDEO_OUT_OPEN		18
#define ERR_CREATE_REC_THREAD	19
#define ERR_H264ENC_START			20
#define ERR_AIENC_START			21
#define ERR_MOTION_DETECT_START	22
#define ERR_TALK_START				23
#define ERR_REG_START				24
#define ERR_SENSOR_OPEN			25
#define ERR_ALARM_OUT_OPEN		26
#define ERR_AUDIO_START			27
#define ERR_CAPTURE_START			28
#define ERR_INIT_VIDEO_DEVICE		29
#define ERR_HD_INIT_MPP			30
#define ERR_NET_ONVIF_START		31
#define ERR_VIDEO_BLIND_OPEN		32      // by tanqw20150410   
#define ERR_VIDEO_BLIND_START		33


//#include "protocol.h"

/****************************************************************************************
函数名称：WriteToLogFile
函数功能：创建编码通道组
入口参数：byChanID--通道ID
		byLogType--Log类型
		dwOperIpAddr--IP地址
		nCommand--命令
		nStatus--状态
出口参数：无
返回值：成功返回LOG信息
****************************************************************************************/
const char *WriteToLogFile(
	unsigned char byChanID,
	unsigned char byLogType,
	unsigned int dwOperIpAddr,
	unsigned short nCommand,
	unsigned short nStatus);

/****************************************************************************************
函数名称：QueryLogInfo
函数功能：启动查询日志信息线程
入口参数：iSocket--句柄
		nSessionID--会话ID
		pstRequest--PLOGQUERYREQUEST结构
出口参数：无
返回值：成功返回LOG信息
****************************************************************************************/
//int QueryLogInfo(int iSocket,
//	unsigned short nSessionID,
//	PLOGQUERYREQUEST pstRequest);

#endif //__LOG_H__

