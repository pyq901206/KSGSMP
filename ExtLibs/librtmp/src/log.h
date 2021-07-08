/****************************************************************************************
�ļ����ƣ�log.h
�� �ߣ��޾���
˵ ����д��־�ļ�����ѯ��־��Ϣ����
�޸ļ�¼��

��ǰ�汾��1.1  �� �ߣ����Ź�  ���ڣ�2011.08.12
˵ �����޸��ļ���ʽ�ʹ���ע��

��һ�汾��1.0  �� �ߣ��޾���  ���ڣ�2011.08.09
˵ ���������ļ�
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
#define LOG_TYPE_BLIND_ALARM				10        // by tanqw 20150410   �ڵ��澯
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
�������ƣ�WriteToLogFile
�������ܣ���������ͨ����
��ڲ�����byChanID--ͨ��ID
		byLogType--Log����
		dwOperIpAddr--IP��ַ
		nCommand--����
		nStatus--״̬
���ڲ�������
����ֵ���ɹ�����LOG��Ϣ
****************************************************************************************/
const char *WriteToLogFile(
	unsigned char byChanID,
	unsigned char byLogType,
	unsigned int dwOperIpAddr,
	unsigned short nCommand,
	unsigned short nStatus);

/****************************************************************************************
�������ƣ�QueryLogInfo
�������ܣ�������ѯ��־��Ϣ�߳�
��ڲ�����iSocket--���
		nSessionID--�ỰID
		pstRequest--PLOGQUERYREQUEST�ṹ
���ڲ�������
����ֵ���ɹ�����LOG��Ϣ
****************************************************************************************/
//int QueryLogInfo(int iSocket,
//	unsigned short nSessionID,
//	PLOGQUERYREQUEST pstRequest);

#endif //__LOG_H__

