/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  作者: 王思琪
|  日期: 2018年04月25日
|  说明: 设置和获取EMAIL配置表
|
******************************************************************/


#ifndef __EMAILCONFIG_H__
#define __EMAILCONFIG_H__

#include "common.h"
#include "configmanage.h"

//王思琪添加
#ifndef D_CONFEMAILPARAM_STR
#define D_CONFEMAILPARAM_STR "ConfEmail"
#endif

int EmailParamSet(EmailParam_T *info);
int EmailParamGet(EmailParam_T *info);

#endif

