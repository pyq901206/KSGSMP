/*******************************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2017年02月08日
|  说明:
|  修改: 
|		1.修改当前节点为尾节点的时候下一个节点不需要指向上一个 2018/02/23  谭帆
********************************************************************************/
#include <string.h>
#include <stdlib.h>
#include "handlemanage.h"


int HandleManageInitHead(HandleManage_T *head)
{
	if(head == NULL){
		return KEY_FALSE;
	}
	MUTEX_INIT(&head->selfMutex);
	MUTEX_LOCK(&head->selfMutex);
	head->handle = NULL;
	head->next = NULL;
	head->pre = NULL;
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;
}

int HandleManageDelHandle(HandleManage_T *head,void *handle)
{
	if(head == NULL || handle == NULL){
		return KEY_FALSE;
	}
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head->next;cur != NULL;cur = cur->next){
		if(handle == cur->handle){
			cur->pre->next = cur->next;
			if(NULL != cur->next){
				cur->next->pre = cur->pre;
			}
			cur->next = NULL;
			cur->pre = NULL;
			free(cur);
			MUTEX_UNLOCK(&head->selfMutex);
			return KEY_TRUE;
		}
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_FALSE;
}

int HandleManageAddHandle(HandleManage_T *head,void *handle)
{
	if(head == NULL || handle == NULL){
		return KEY_FALSE;
	}
	HandleManage_T *cur = NULL;
	HandleManage_T *newNode = (HandleManage_T *)malloc(sizeof(HandleManage_T));
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head;cur->next!= NULL;cur = cur->next);
	cur->next = newNode;
	newNode->pre = cur;
	newNode->next = NULL;
	newNode->handle = handle;
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;
}

int HandleManageAddHandle1(HandleManage_T *head,void *handle)
{
	if(head == NULL || handle == NULL){
		return KEY_FALSE;
	}
	HandleManage_T *next = NULL;
	HandleManage_T *newNode = (HandleManage_T *)malloc(sizeof(HandleManage_T));
	MUTEX_LOCK(&head->selfMutex);
	if(head->next != NULL)
	{
		next = head->next;
		newNode->pre = head;
		newNode->next = next;
		newNode->handle = handle;
		head->next = newNode;
		next->pre = newNode;
	}
	else
	{
		newNode->pre = head;
		newNode->next = NULL;
		newNode->handle = handle;
		head->next = newNode;
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;
}

void* HandleManageGetNextHandle(HandleManage_T *head)
{
	if(NULL == head || NULL == head->next ){
		return NULL;
	}
	void *handle = NULL;
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	if(NULL != head->next){
		cur = head->next;
		handle=cur->handle;
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return handle;
}

int HandleManageGetUsrCount(HandleManage_T *head)
{
	if(head == NULL){
		return KEY_FALSE;
	}
	
	int count = 0;
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head;cur->next!= NULL;cur = cur->next){
		count++;
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return count;
}

int HandleManagePostLoop(HandleManage_T *head,HandleManageLoopDoFunc fun,int argc,void *arg[])
{
	if(head == NULL || fun == NULL){
		return KEY_FALSE;
	}
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head->next;cur != NULL;cur = cur->next){
		fun(cur->handle,argc,arg);
	}
	
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;
}

