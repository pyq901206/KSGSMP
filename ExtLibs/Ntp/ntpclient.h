/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年6月21日
|  说明: 基于NTP 开源库修改
|  修改: 2018年6月21日 增加NTP 响应回调
|	
******************************************************************/
#ifndef NTPCLIENT_H
#define NTPCLIENT_H

/* when present, debug is a true global */
#ifdef ENABLE_DEBUG
extern int debug;
#else
#define debug 0
#endif

/* global tuning parameter */
extern double min_delay;

/* prototype for function defined in phaselock.c */
int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq);

#endif
