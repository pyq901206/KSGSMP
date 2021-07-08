/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 罗勇
|  日期: 2019年7月04日
|  说明:
|  修改:
|	
******************************************************************/
#ifndef __MQTTPUSH_MANAGE_H__
#define __MQTTPUSH_MANAGE_H__

_OUT_ int pushMqttMsg(char * topic, char *resMsg);
_OUT_ int PushManageInit(char *ip);
_OUT_ int PushManageUnInit();

#endif
