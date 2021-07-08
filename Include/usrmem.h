/*******************************************************************
|  Copyright(c) 2015-2016 Graceport Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0
|  ����: FootMan [FootMan@graceport.cn]
|  ����: 2016��6��6��
|  ˵��:
|
|  �汾: 1.1
|  ����:
|  ����:
|  ˵��:
******************************************************************/
#ifndef __USRMEM_H__
#define __USRMEM_H__
#include "list.h"

typedef struct _MemHandle_T{
	void *mem_addr;
	struct listnode _list;
}MemHandle_T;

MemHandle_T *CreateMemHandle();
char *usr_malloc(MemHandle_T *handle,int len);
char *usr_free(MemHandle_T *handle);
char *usr_strdup(MemHandle_T *handle,char *str);
char* usr_inttostr(MemHandle_T *handle,int value);
char* usr_floattostr(MemHandle_T *handle,float value);
char* usr_addstr_intostr(MemHandle_T *handle,int size,char *str1,char *str2);
#endif
