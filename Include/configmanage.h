/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0 
|  ����: ̷�� [tanfan0406@163.com]
|  ����: 2018��2��19��
|  ˵��:
|  �޸�:
|	
******************************************************************/

#ifndef  __CONFIGMANAGE_H__
#define  __CONFIGMANAGE_H__
#include "cjson.h"


int ConfigManageInit();
int ConfigManageUnInit();
int ConfigManageSet(cJSON *conf,const char *confName);
cJSON *ConfigManageGet(const char *confName);

#endif
