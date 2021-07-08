/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0 
|  ����: ̷�� [tanfan0406@163.com]
|  ����: 2018��2��19��
|  ˵��: 1.���ñ�ע������Ϊ��
|		 2.��ʹ�ù����� ��ͣ���������ñ� ���п��ܻ����ϵͳ�ļ�������
|		��������������ϵͳɱ������
|  �޸�:
|	
******************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "configmanage.h"
#include "configmanageinner.h"
static Conf_T g_config;

static Conf_T *GetConfHandle()
{
	return &g_config;
}

/********
��������
*******/
static int LoadConfig(const char *path,char *str)
{
	if(NULL == str || NULL == path){
		return -1;
	}
	//DF_DEBUG("path = %s",path);
	FILE *fp = fopen(path, "r"); /* ֻ����ȡ */
    if (NULL == fp) /* ���ʧ���� */
    {
        DF_ERROR("fopen:");
		return -1;
	}
	fseek(fp,0,SEEK_END);
	int outLen = ftell(fp);
	fseek(fp,0,SEEK_SET);
	fread(str,outLen,1,fp);
	fclose(fp);
	return outLen;
}

/****************************************
���ñ�����
*****************************************/
static int WirteConf(const char *path,cJSON* conf)
{
	if(NULL == path ||NULL == conf){
		return KEY_FALSE;
	}
	FILE *fp = fopen(path, "w+"); 
    if (NULL == fp) /* ���ʧ���� */
    {
        DF_ERROR("fopen:");
		return -1;
	}
	
	char *out = cJSON_PrintUnformatted(conf);
	//printf("out = %s\r\n",out);
	fwrite(out,1,strlen(out),fp);
	fclose(fp);
	free(out);
	return 0;
}

_OUT_ int ConfigManageInit()
{
	char confBuf[D_CONFPATH_LEN];
	int  confLen = 0;
	memset(confBuf,0,D_CONFPATH_LEN);
	memset(GetConfHandle(),0,sizeof(Conf_T));
	MUTEX_INIT(&GetConfHandle()->confMutex);
	MUTEX_LOCK(&GetConfHandle()->confMutex);
	confLen = LoadConfig(D_CONFPATH_STR,confBuf);
	if(confLen > 0){
		GetConfHandle()->root = cJSON_Parse(confBuf);
	}
	if(confLen < 0|| NULL == GetConfHandle()->root){
		DF_ERROR("conf broken");
		GetConfHandle()->root = cJSON_CreateObject();
		MUTEX_UNLOCK(&GetConfHandle()->confMutex);
		return KEY_NOTFOUNDCONF;
	}
	MUTEX_UNLOCK(&GetConfHandle()->confMutex);
	return KEY_TRUE;
	
}

int ConfigManageUnInit()
{
	return KEY_TRUE;
}

_OUT_ int ConfigManageSet(cJSON *conf,const char *confName)
{
	if(NULL == conf || NULL == confName){
		cJSON_Delete(conf);
		return KEY_FALSE;
	}
	MUTEX_LOCK(&GetConfHandle()->confMutex);
	cJSON_DeleteItemFromObject(GetConfHandle()->root, confName);
	cJSON_AddItemToObject(GetConfHandle()->root,confName,conf);
	if(KEY_FALSE == WirteConf(D_CONFPATH_STR,GetConfHandle()->root)){
		DF_ERROR("Set Conf");
		assert(0);
	}
	MUTEX_UNLOCK(&GetConfHandle()->confMutex);
	return KEY_TRUE;
}

#if 0//(defined XXX0 )
_OUT_ cJSON *ConfigManageGet2(const char *confName)
{
    cJSON *out=NULL;
	if(NULL == confName){
		return NULL;
	}
	MUTEX_LOCK(&GetConfHandle()->confMutex);
	cJSON *tmp = cJSON_GetObjectItem(GetConfHandle()->root,confName);
    if(NULL==tmp)
    {
        out=cJSON_Duplicate(tmp,1);
    }
	MUTEX_UNLOCK(&GetConfHandle()->confMutex);
	 
	return out;
}
#endif  /* XXX0 */

_OUT_ cJSON *ConfigManageGet(const char *confName)
{
    char *out=NULL;
	if(NULL == confName){
		return NULL;
	}
	MUTEX_LOCK(&GetConfHandle()->confMutex);
	cJSON *tmp = cJSON_GetObjectItem(GetConfHandle()->root,confName);
    if(NULL!=tmp)
    {
        out=cJSON_PrintUnformatted(tmp);
       tmp=NULL;
        if(NULL!=out)
        {
            tmp=cJSON_Parse(out);
            free(out);out=NULL;
        }
    }
	MUTEX_UNLOCK(&GetConfHandle()->confMutex);
	return tmp;
}

