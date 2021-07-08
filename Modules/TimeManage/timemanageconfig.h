/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年02月20日
|  说明: 预留JSON组装和解析接口 为HTTP API 透传做准备
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

