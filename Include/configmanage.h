/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年2月19日
|  说明:
|  修改:
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
