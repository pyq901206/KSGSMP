/*******************************************************************
|  Copyright(c) 2015-2016 Graceport Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0
|  作者: FootMan [FootMan@graceport.cn]
|  日期: 2016年6月6日
|  说明:
|
|  版本: 1.1
|  作者:
|  日期:
|  说明:
******************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "usrmem.h"
#include "common.h"


MemHandle_T *CreateMemHandle()
{
	MemHandle_T *handle;
	handle = (MemHandle_T *)malloc(sizeof(MemHandle_T));
	memset(handle,0,sizeof(MemHandle_T));
	list_init(&(handle->_list));
	return handle;
}


char *usr_malloc(MemHandle_T *handle,int len)
{
	MemHandle_T *newHandle = NULL;
	char *p = NULL;
	newHandle = CreateMemHandle();
	p = (char *)malloc(len);
	memset(p,0,len);
	newHandle->mem_addr = p;
	list_add_tail(&(handle->_list),&(newHandle->_list));
	return p;
}

char *usr_free(MemHandle_T *handle)
{
	struct listnode *node = NULL;
	MemHandle_T *tmpHandle = NULL;
	//list_for_each(node,&(handle->_list))
  	for (node = (&(handle->_list))->next; node != (&(handle->_list)); ){
		tmpHandle = (MemHandle_T *)node_to_item(node,MemHandle_T,_list);
		if(NULL != tmpHandle->mem_addr){
			free(tmpHandle->mem_addr);
		}
		list_remove(node);
		node = node->next;
		free(tmpHandle);
	}
	free(handle);
	return NULL;
}

char *usr_strdup(MemHandle_T *handle,char *str)
{
	char *p = NULL;
	p = usr_malloc(handle,strlen(str)+1);
	if(NULL == p){
		return NULL;
	}
	snprintf(p,strlen(str)+1,"%s",str);
	return p;
}

char* usr_addstr_intostr(MemHandle_T *handle,int size,char *str1,char *str2)
{
	char *p = NULL;
	p = usr_malloc(handle,size);
	if(NULL == p){
		return NULL;
	}
	snprintf(p,size,str1,str2);
	return p;
}


char* usr_inttostr(MemHandle_T *handle,int value)
{
	char *p = NULL;
	p = usr_malloc(handle,32);
	if(NULL == p){
		return NULL;
	}
	snprintf(p,32,"%d",value);
	return p;
}

char* usr_floattostr(MemHandle_T *handle,float value)
{
	char *p = NULL;
	p = usr_malloc(handle,32);
	if(NULL == p){
		return NULL;
	}
	snprintf(p,32,"%lf",value);
	return p;
}
