/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  ����: ��˼��
|  ����: 2018��04��25��
|  ˵��: ���úͻ�ȡEMAIL���ñ�
|
******************************************************************/


#ifndef __EMAILCONFIG_H__
#define __EMAILCONFIG_H__

#include "common.h"
#include "configmanage.h"

//��˼�����
#ifndef D_CONFEMAILPARAM_STR
#define D_CONFEMAILPARAM_STR "ConfEmail"
#endif

int EmailParamSet(EmailParam_T *info);
int EmailParamGet(EmailParam_T *info);

#endif

