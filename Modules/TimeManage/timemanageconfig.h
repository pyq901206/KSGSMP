/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0 
|  ����: ̷�� [tanfan0406@163.com]
|  ����: 2018��02��20��
|  ˵��: Ԥ��JSON��װ�ͽ����ӿ� ΪHTTP API ͸����׼��
|
******************************************************************/
#ifndef __TIMECONFIG_H__
#define __TIMECONFIG_H__

#include "configmanage.h"
#include "timemanage.h"

#ifndef D_CONFTIMECONF_STR
#define D_CONFTIMECONF_STR "TimeConf"
#endif

int TimeManageConfigSet(TimeConf_T *info);
int TimeManageConfigGet(TimeConf_T *info);

#endif

