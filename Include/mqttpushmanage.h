/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0 
|  ����: ����
|  ����: 2019��7��04��
|  ˵��:
|  �޸�:
|	
******************************************************************/
#ifndef __MQTTPUSH_MANAGE_H__
#define __MQTTPUSH_MANAGE_H__

_OUT_ int pushMqttMsg(char * topic, char *resMsg);
_OUT_ int PushManageInit(char *ip);
_OUT_ int PushManageUnInit();

#endif
