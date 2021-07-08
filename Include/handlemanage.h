/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年02月08日
|  说明:
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
	struct _HandleManage_T	*next;//下一个
	struct _HandleManage_T	*pre;//上一个
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
函数说明:此函数是模仿C++的模板类写的
函数作用: 通过某个成员变量的值来获取整个Handle的头位置
参数说明
第一个参数是listhead 
第二个参数是要对比的成员变量名字
第三个参数是要对比的成员变量值
第四个参数是返回值句柄
第五个参数是返回值类型
实现逻辑:
	1. 遍历List列表
	2. 强制转换任意类型的handle为structmem 类型
	3. 取structmem->类型的值与value 进行对比
	4. 如果成功就将 resultHanlde = 当前任意类型的handle
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

