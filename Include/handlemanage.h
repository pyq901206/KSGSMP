/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  �汾: 1.0 
|  ����: ̷�� [tanfan0406@163.com]
|  ����: 2018��02��08��
|  ˵��:
******************************************************************/
#ifndef _HANDLEMANNAGE_H__
#define _HANDLEMANNAGE_H__

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HandleManage_T{
	void *handle;
	pthread_mutex_t 	selfMutex;
	struct _HandleManage_T	*next;//��һ��
	struct _HandleManage_T	*pre;//��һ��
}HandleManage_T;

typedef int (*HandleManageLoopDoFunc)(void *handle,int argc,void *arg[]);

int HandleManageInitHead(HandleManage_T *head);
int HandleManageDelHandle(HandleManage_T *head,void *handle);
int HandleManageAddHandle(HandleManage_T *head,void *handle);
int HandleManageAddHandle1(HandleManage_T *head,void *handle);
int HandleManageGetUsrCount(HandleManage_T *head);
int HandleManagePostLoop(HandleManage_T *head,HandleManageLoopDoFunc fun,int argc,void *arg[]);
void* HandleManageGetNextHandle(HandleManage_T *head);

/*
����˵��:�˺�����ģ��C++��ģ����д��
��������: ͨ��ĳ����Ա������ֵ����ȡ����Handle��ͷλ��
����˵��
��һ��������listhead 
�ڶ���������Ҫ�Աȵĳ�Ա��������
������������Ҫ�Աȵĳ�Ա����ֵ
���ĸ������Ƿ���ֵ���
����������Ƿ���ֵ����
ʵ���߼�:
	1. ����List�б�
	2. ǿ��ת���������͵�handleΪstructmem ����
	3. ȡstructmem->���͵�ֵ��value ���жԱ�
	4. ����ɹ��ͽ� resultHanlde = ��ǰ�������͵�handle
*/
#define  HandleManageGetHandleByInt(head,member,value,resultHanlde,structmem) \
{\
	HandleManage_T *tpHead = head;\
	resultHanlde = NULL;\
	if(tpHead != NULL){\
		HandleManage_T *cur = NULL;\
		MUTEX_LOCK(&tpHead->selfMutex);\
		for(cur = tpHead->next;cur != NULL;cur = cur->next){\
			if(((structmem *)cur->handle)->member == value){\
				resultHanlde = cur->handle;\
				break;\
			}\
		}\
		MUTEX_UNLOCK(&tpHead->selfMutex);\
	}\
}

#define  HandleManageGetHandleByStr(head,member,value,resultHanlde,structmem) \
{\
	HandleManage_T *tpHead = head;\
	resultHanlde = NULL;\
	if(tpHead != NULL){\
		HandleManage_T *cur = NULL;\
		MUTEX_LOCK(&tpHead->selfMutex);\
		for(cur = tpHead->next;cur != NULL;cur = cur->next){\
			if(0 == strcmp(((structmem *)cur->handle)->member,value)){\
				resultHanlde = cur->handle;\
				break;\
			}\
		}\
		MUTEX_UNLOCK(&tpHead->selfMutex);\
	}\
}

#ifdef __cplusplus
}
#endif

#endif

