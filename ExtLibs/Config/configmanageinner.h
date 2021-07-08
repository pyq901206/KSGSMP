/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0 
|  ����: ̷�� [tanfan0406@163.com]
|  ����: 2018��2��22��
|  ˵��:
|  �޸�:
|	
******************************************************************/

#ifndef __CONFIGMANAGEINNER_H__
#define __CONFIGMANAGEINNER_H__

#include <pthread.h>
#include "cjson.h"
#include "common.h"

#ifndef D_CONFPATH_STR
#define D_CONFPATH_STR "./conf.config"
#endif

#ifndef D_CONFPATH_LEN	
#define D_CONFPATH_LEN 1024*1*1024
#endif

typedef struct _Conf_T{
	 cJSON *root;
	pthread_mutex_t 	confMutex;
}Conf_T;


#endif

