/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年2月22日
|  说明:
|  修改:
|	
******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usrmanage.h"
#include "userdefine.h"
#include "handlemanage.h"

typedef struct _UsrManageInner_T{
	HandleManage_T listHead;
	AddUsrCbFun	addUsrcb;
	DelUsrCbFun delUsrcb;
	GetUsrInfoCbFun getUsrInfocb;
	SetUsrInfoCbFun setUsrInfocb;
	GetUsrInfoListCbFun getUsrInfoListcb;
	GetUsrInfoListNumCbFun getUsrInfoListNumcb;
	ModifyUsrBindInfoCbFun modifyUsrBindInfocb;
}UsrManageInner_T;

typedef struct _UsrManageInfo_T{
	pthread_mutex_t 	selfMutex;
	UsrInfo_T			usrInfo;
}UsrManageInfo_T;

static UsrManageInner_T g_usrList;

static UsrManageInner_T *GetUsrListHandle()
{
	return &g_usrList;
} 

static int IsValidEmail(const char *s)
{
	char* ms;
	if ((ms=strchr(s,'@')) == NULL)
	{
		return KEY_FALSE;
	}
	if (strchr(ms + 1, '@') != NULL)
	{
		return KEY_FALSE;
	}
	if (strchr(ms + 1, '.') == NULL)
	{
		return KEY_FALSE;
	}
	if (strchr(s,'.') < ms)
	{
		if (strchr(strchr(s, '.')+1,'.') < ms)
		{
			return KEY_FALSE;
		}
	}
	if (strlen(strrchr(s,'.')+1) > 4 || strlen(strrchr(s,'.')+1) < 2)
	{
		return KEY_FALSE;
	}
	
	if (64 < strlen(s))
	{	
		return KEY_FALSE;
	}
	return KEY_TRUE;
}

static int CheckUsrInfoValidity(UsrInfo_T *info)
{
	if (strlen(info->registInfo.usrname) >  DF_MAXLEN_USRNAME + 1
		 || strlen(info->registInfo.passWord) >  16
		 || ENUM_USRLEVEL_Invalid > info->registInfo.userLevel)
	{
		DF_ERROR("Invalid parameter.!!!");
		return KEY_FALSE;
	}
	return KEY_TRUE;
}

static int	GetUserRegistInfo(UserRegistInfo_T *info)
{
	int iret = KEY_FALSE;
	void *result = NULL;
	HandleManageGetHandleByStr(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrname,info->usrname, result, UsrManageInfo_T);
	UsrManageInfo_T *usrinfo = (UsrManageInfo_T *)result;
	MUTEX_LOCK(&usrinfo->selfMutex);
	memcpy(info,&(usrinfo->usrInfo.registInfo),sizeof(UserRegistInfo_T));
	MUTEX_UNLOCK(&usrinfo->selfMutex);
	return iret;
}

static int	ModifyPassword(char *username,char *oldPassword,char *newPassword)
{
	int iret = KEY_FALSE;
	void *result = NULL;
	UsrInfo_T userInfo = {0};
	if (EFFECTIVELENGTH < strlen(oldPassword) || EFFECTIVELENGTH < strlen(newPassword)){
		DF_ERROR("Invalid parameter.!!!");
		return KEY_FALSE;
	}
	
	HandleManageGetHandleByStr(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrname,username, result, UsrManageInfo_T);
	UsrManageInfo_T *info = (UsrManageInfo_T *)result;
	MUTEX_LOCK(&info->selfMutex);
	//写入链表
	if(0 == strcmp(info->usrInfo.registInfo.passWord,oldPassword)){
		sprintf(info->usrInfo.registInfo.passWord,"%s",newPassword);
		iret = KEY_TRUE;
	}
	else
	{
		DF_ERROR("Invalid parameter.!!!");
		MUTEX_UNLOCK(&info->selfMutex);
		return iret;
	}
	memcpy(&userInfo, &info->usrInfo, sizeof(UsrInfo_T));
	MUTEX_UNLOCK(&info->selfMutex);
	//存数据库
	UsrManageInner_T *gHandle =  GetUsrListHandle();
	if (NULL != gHandle){
		if (NULL != gHandle->setUsrInfocb){
		//	DF_DEBUG("passWord = [%s]",info->usrInfo.registInfo.passWord);
		//	DF_DEBUG("email = [%s]",info->usrInfo.registInfo.email);
			iret = gHandle->setUsrInfocb(&userInfo);
		}
	}
	return iret;
}

static int ResetPassword(char *username, char *countrycode, char *password)
{
	int iret = -1;
	void *result = NULL;
	UsrInfo_T userInfo = {0};
	if (NULL == username || NULL == countrycode || NULL == password)
	{
		return KEY_FALSE;
	}
	HandleManageGetHandleByStr(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrname, username, result, UsrManageInfo_T);
	if (NULL == result)
	{
		return KEY_NOTFOUND;
	}
	UsrManageInfo_T *info = (UsrManageInfo_T *)result;
	MUTEX_LOCK(&info->selfMutex);
	sprintf(info->usrInfo.registInfo.passWord, "%s", password);
	memcpy(&userInfo, &info->usrInfo, sizeof(UsrInfo_T));
	MUTEX_UNLOCK(&info->selfMutex);
	
	//存数据库
	UsrManageInner_T *gHandle =  GetUsrListHandle();
	if (NULL != gHandle){
		if (NULL != gHandle->setUsrInfocb){
			iret = gHandle->setUsrInfocb(&userInfo);
		}
	}
	return iret;
}

static int Authentication(const char *usrname,const char *password)//鉴权
{
	int usrLevel = 0;
	int iret = ENUM_USRLEVEL_Invalid;
	void *result = NULL;
	HandleManageGetHandleByStr(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrname,usrname, result, UsrManageInfo_T);
	if (NULL == result){
		return ENUM_USRLEVEL_Invalid;
	}
	UsrManageInfo_T *info = (UsrManageInfo_T *)result;
	MUTEX_LOCK(&info->selfMutex);
	if(0 == strcmp(info->usrInfo.registInfo.passWord,password)){
		iret = info->usrInfo.registInfo.userLevel;
	}
	MUTEX_UNLOCK(&info->selfMutex);
	return iret;
}

static int GetPermissionCabByLevel(ENUM_USRLEVEL_E usrLevel)//返回权限能力  接口保留
{
	int permission = 0;
	return permission;
}

//以下可以先不实现
static int GetUsrInfoList(UsrInfo_T *info)//最多2W个 接口保留
{
	int i = 0;
	UsrManageInfo_T *usrManageinfo = NULL;
	HandleManage_T*	head = &GetUsrListHandle()->listHead;
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head->next;cur != NULL;cur = cur->next)
	{
		usrManageinfo = cur->handle;		
		if (usrManageinfo != NULL){
			if (strlen(usrManageinfo->usrInfo.registInfo.usrname)){
				memcpy(&info[i], &usrManageinfo->usrInfo, sizeof(UsrInfo_T));
				i ++;
			}
		}
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;
}

static int GetUsrInfoListNum(int *num)//内部接口
{
	(*num) = 0;
	UsrManageInfo_T *usrManageinfo = NULL;
	HandleManage_T*	head = &GetUsrListHandle()->listHead;
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head->next;cur != NULL;cur = cur->next)
	{
		usrManageinfo = cur->handle;		
		if (usrManageinfo != NULL){
			if (strlen(usrManageinfo->usrInfo.registInfo.usrname)){
				(*num) ++;
			}
		}
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;
}

static int ModifyUsrInfo(UsrInfo_T *curUsrInfo, UsrInfo_T *usrInfo)//修改用户信息  接口保留
{	
	int lenvel = -1;
	char szPassWord[DF_MAXLEN_PASSWD+1] = {0};
	void *result = NULL;
	int iret = KEY_FALSE;
	//DF_DEBUG("[%s][%s]", curUsrInfo->registInfo.usrname, usrInfo->registInfo.usrname);
	if (KEY_FALSE == CheckUsrInfoValidity(usrInfo)){
		return KEY_FALSE;
	}
	
	if (strcmp(curUsrInfo->registInfo.usrname, usrInfo->registInfo.usrname) || curUsrInfo->registInfo.userLevel < usrInfo->registInfo.userLevel)
	{
		DF_ERROR("User information mismatch!");
		return iret;
	}

	HandleManageGetHandleByStr(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrname, usrInfo->registInfo.usrname, result, UsrManageInfo_T);
	if (NULL == result){
		DF_ERROR("ModifyUsrInfo error.");
		return iret;
	}
	UsrManageInfo_T *info = (UsrManageInfo_T *)result;
	lenvel = info->usrInfo.registInfo.userLevel;
	snprintf(szPassWord, DF_MAXLEN_PASSWD+1, "%s", info->usrInfo.registInfo.passWord);
	MUTEX_LOCK(&info->selfMutex);
	//写入链表
	memcpy(&info->usrInfo, usrInfo, sizeof(UsrInfo_T));
	//权限能力不能被修改，需要重新添加用户
	info->usrInfo.registInfo.userLevel = lenvel;
	//此处不能对密码进行修改，修改密码调用修改密码接口
	snprintf(info->usrInfo.registInfo.passWord, DF_MAXLEN_PASSWD+1, "%s", szPassWord);
	
	MUTEX_UNLOCK(&info->selfMutex);
	//存数据库
	UsrManageInner_T *gHandle = GetUsrListHandle();
	if (NULL != gHandle){
		if (NULL != gHandle->setUsrInfocb){
			iret = gHandle->setUsrInfocb(&info->usrInfo);
		}
	}
	return iret;
}

static int FindUsr(char *usrName) //查找用户是否存在
{
	UsrManageInfo_T *usrManageinfo = NULL;
	HandleManage_T*	head = &GetUsrListHandle()->listHead;
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head->next;cur != NULL;cur = cur->next)
	{
		usrManageinfo = cur->handle;		
		if (0 == strcmp(usrName, usrManageinfo->usrInfo.registInfo.usrname))
		{
			MUTEX_UNLOCK(&head->selfMutex);
			return KEY_TRUE;
		}
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_FALSE;		
}

static int GetUsrId(char *usrName, char *usrid)
{

}

static int SetUsrId(char *usrName, char *usrid)
{

}

static int ModifyUsrBindInfo(UsrInfo_T *info, char *oldusrname)
{
	void *result = NULL;
	int iret = -1;
	if (NULL == info || NULL == oldusrname)
	{
		return KEY_FALSE;
	}
	HandleManageGetHandleByStr(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrname, oldusrname, result, UsrManageInfo_T);
	if (NULL == result){
		DF_ERROR("ModifyUsrInfo error.");
		return iret;
	}
	UsrManageInfo_T *usrinfo = (UsrManageInfo_T *)result;
	MUTEX_LOCK(&usrinfo->selfMutex);
	memset(usrinfo->usrInfo.registInfo.usrname, 0, sizeof(usrinfo->usrInfo.registInfo.usrname));
	snprintf(usrinfo->usrInfo.registInfo.usrname, 129, "%s", info->registInfo.usrname);
	snprintf(usrinfo->usrInfo.registInfo.countrycode, 129, "%s", info->registInfo.countrycode);
	memcpy(info, &usrinfo->usrInfo, sizeof(UsrInfo_T));
	MUTEX_UNLOCK(&usrinfo->selfMutex);
	//存数据库
	UsrManageInner_T *gHandle = GetUsrListHandle();
	if (NULL != gHandle){
		if (NULL != gHandle->modifyUsrBindInfocb){
			iret = gHandle->modifyUsrBindInfocb(info, oldusrname);
		}
	}
	return iret;
}

static int CreateUsrid()
{
	static int i = 0;
	int usrid = 0;
	void *result = NULL;
	for (;;)
	{
		usrid = 1000000 + i;
		result = NULL;
		HandleManageGetHandleByInt(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrID, usrid, result, UsrManageInfo_T);
		if (NULL == result)
		{
			break;
		}
		i++;
	}
	return usrid;
}

static int CheckUser(UsrInfo_T *curUsrInfo,UsrInfo_T *info)
{
	void *result = NULL;
	UsrManageInfo_T *usrManageinfo = NULL;
	if (KEY_FALSE == CheckUsrInfoValidity(info))
	{
		return KEY_FALSE;
	}
	
	if (curUsrInfo->registInfo.userLevel < info->registInfo.userLevel){
		return KEY_FALSE;
	}
	HandleManageGetHandleByStr(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrname, info->registInfo.usrname, result, UsrManageInfo_T);
	if (NULL != result){
		usrManageinfo = (UsrManageInfo_T *)result;
		DF_DEBUG("%s  %s",info->registInfo.countrycode, usrManageinfo->usrInfo.registInfo.countrycode);
		//if (!strcmp(info->registInfo.countrycode, usrManageinfo->usrInfo.registInfo.countrycode))
		//{
		//	DF_ERROR("The user already exists.");
		//	return KEY_USERREPEAT;
		//}
		return KEY_USERREPEAT;
	}
	return KEY_TRUE;
}

static int AddUsr(UsrInfo_T *curUsrInfo,UsrInfo_T *info)//添加用户 接口保留
{
	int iret = KEY_FALSE;
	UsrManageInfo_T *usrManageinfo = NULL;
	void *result = NULL;
	if (KEY_FALSE == CheckUsrInfoValidity(info))
	{
		return KEY_FALSE;
	}
	
	if (KEY_TRUE != CheckUser(curUsrInfo, info))
	{
		return KEY_FALSE;
	}
	
	//写入链表
	usrManageinfo = (UsrManageInfo_T *)malloc(sizeof(UsrManageInfo_T));
	MUTEX_INIT(&usrManageinfo->selfMutex);
	memcpy(&usrManageinfo->usrInfo, info, sizeof(UsrInfo_T));
	//生成唯一的标识usrid
	usrManageinfo->usrInfo.registInfo.usrID = CreateUsrid();
	//添加到链表
	iret = HandleManageAddHandle(&GetUsrListHandle()->listHead, (void *)usrManageinfo);
	//存数据库
	if (iret == KEY_TRUE)
	{
		UsrManageInner_T *gHandle = GetUsrListHandle();
		if (NULL != gHandle){
			if (NULL != gHandle->addUsrcb){
				iret = gHandle->addUsrcb(&usrManageinfo->usrInfo);
			}
		}
	}
	return iret;
}
 
static int DelUsr(UsrInfo_T *curUsrInfo,UsrInfo_T *UsrInfo)//删除用户 接口保留
{
	void *result = NULL;
	int iret = KEY_FALSE;
	UsrManageInfo_T *usrManageinfo = NULL;
	//写入链表
	HandleManageGetHandleByStr(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrname, UsrInfo->registInfo.usrname, result, UsrManageInfo_T);
	if (NULL == result){
		DF_ERROR("DelUsr error.");
		return iret;
	}
	UsrManageInfo_T *info = (UsrManageInfo_T *)result;
	if (curUsrInfo->registInfo.userLevel < info->usrInfo.registInfo.userLevel){
		DF_ERROR("User is Invalid!");
		return KEY_FALSE;
	}
	iret = HandleManageDelHandle(&GetUsrListHandle()->listHead, (void *)info);
	MyFree(info);

	//存数据库
	if (iret == KEY_TRUE)
	{
		UsrManageInner_T *gHandle = GetUsrListHandle();
		if (NULL != gHandle){
			if (NULL != gHandle->delUsrcb){
				iret = gHandle->delUsrcb(&info->usrInfo);
			}
		}
	}

	return iret;
}

static int GetUsrInfo(UsrInfo_T *curUsrInfo,UsrInfo_T *UsrInfo)
{
	void *result = NULL;
	int iret = KEY_FALSE;
	UsrManageInfo_T *usrManageinfo = NULL;
	if (strcmp(curUsrInfo->registInfo.usrname, UsrInfo->registInfo.usrname) || curUsrInfo->registInfo.userLevel < UsrInfo->registInfo.userLevel)
	{
		DF_ERROR("User information mismatch!");
		return iret;
	}
	//写入链表
	HandleManageGetHandleByStr(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrname, UsrInfo->registInfo.usrname, result, UsrManageInfo_T);
	if (NULL == result){
		DF_ERROR("Get UsrInfo error.");
		return iret;
	}
	UsrManageInfo_T *info = (UsrManageInfo_T *)result;
	MUTEX_LOCK(&info->selfMutex);
	memcpy(UsrInfo, &info->usrInfo, sizeof(UsrInfo_T));
	MUTEX_UNLOCK(&info->selfMutex);
	return KEY_TRUE;
}

static int GetUsrInfoFromUsrName(char *usrName, UsrInfo_T *UsrInfo)
{
	void *result = NULL;
	int iret = KEY_FALSE;
	HandleManageGetHandleByStr(&GetUsrListHandle()->listHead, usrInfo.registInfo.usrname, usrName, result, UsrManageInfo_T);
	if (NULL == result){
		DF_ERROR("Get UsrInfo error.");
		return iret;
	}
	UsrManageInfo_T *info = (UsrManageInfo_T *)result;
	MUTEX_LOCK(&info->selfMutex);
	memcpy(UsrInfo, &info->usrInfo, sizeof(UsrInfo_T));
	MUTEX_UNLOCK(&info->selfMutex);
	return iret;
}

static int InitUsrList()
{
	//初始化用户列表，从数据库取，存入链表
	int iret = KEY_FALSE;
	int i = 0, num = 0;
	UsrManageInfo_T *usrManageinfo = NULL;
	UsrInfo_T *usrInfo;
	UsrManageInner_T *gHandle =  GetUsrListHandle();
	if (NULL == gHandle->getUsrInfoListcb || NULL == gHandle->getUsrInfoListNumcb){
		DF_ERROR("Init Usr List Fail.");
		return KEY_FALSE;
	}
	
	gHandle->getUsrInfoListNumcb(&num);
	if (num <= 0)
	{
		DF_ERROR("Init Usr List Fail.");
		return KEY_FALSE;
	}
	usrInfo = (UsrInfo_T *)malloc(sizeof(UsrInfo_T) * num);
	if (NULL == usrInfo){
		return KEY_FALSE;
	}
	
	iret = gHandle->getUsrInfoListcb(usrInfo);
	if (iret == KEY_FALSE)
	{
		free(usrInfo);
		usrInfo = NULL;
		return KEY_FALSE;
	}

	for (i = 0; i < num; i++)
	{
		usrManageinfo = (UsrManageInfo_T *)malloc(sizeof(UsrManageInfo_T));
		if (usrInfo[i].registInfo.userLevel <= ENUM_USRLEVEL_Invalid){
			continue;
		}
		
		MUTEX_INIT(&usrManageinfo->selfMutex);
		usrManageinfo->usrInfo.registInfo.userLevel = usrInfo[i].registInfo.userLevel;
		snprintf(usrManageinfo->usrInfo.registInfo.usrname, DF_MAXLEN_USRNAME + 1, "%s", usrInfo[i].registInfo.usrname);
		snprintf(usrManageinfo->usrInfo.registInfo.passWord, DF_MAXLEN_USRNAME + 1, "%s", usrInfo[i].registInfo.passWord);
		snprintf(usrManageinfo->usrInfo.registInfo.countrycode, DF_MAXLEN_USRNAME + 1, "%s", usrInfo[i].registInfo.countrycode);
		usrManageinfo->usrInfo.registInfo.usrID = usrInfo[i].registInfo.usrID;
		snprintf(usrManageinfo->usrInfo.registInfo.customname, DF_MAXLEN_USRNAME + 1, "%s", usrInfo[i].registInfo.customname);
		HandleManageAddHandle(&GetUsrListHandle()->listHead, (void *)usrManageinfo);
	}
	
	free(usrInfo);
	usrInfo = NULL;
	return KEY_TRUE;
}

static int UnInitUsrList()
{
	HandleManage_T*	head = &GetUsrListHandle()->listHead;
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head->next;cur != NULL;cur = cur->next){
		free(cur->handle);
		if(cur->pre != head && cur->pre != NULL){
			free(cur->pre);
		}
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;
}

_OUT_ int UsrManageRegist(UsrManageRegistHandle_T *handle)
{
	if(NULL == handle){
		return KEY_FALSE;
	}
	handle->pGetUserRegistInfo = GetUserRegistInfo;
	return KEY_TRUE;
}

_OUT_ int UsrManageUnRegist(UsrManageRegistHandle_T *handle)
{
	if(NULL == handle){
		return KEY_FALSE;
	}
	memset(handle,0,sizeof(UsrManageRegistHandle_T));
	return KEY_TRUE;
}

_OUT_ int UsrManageInit(UsrManageHandle_T *handle)
{
	if (NULL == handle)
	{
		return KEY_FALSE;
	}

	UsrManageInner_T *gHandle =  GetUsrListHandle();
	HandleManageInitHead(&gHandle->listHead);
	gHandle->addUsrcb = handle->addUsrcb;
	gHandle->delUsrcb = handle->delUsrcb;
	gHandle->getUsrInfocb = handle->getUsrInfocb;
	gHandle->setUsrInfocb = handle->setUsrInfocb;
	gHandle->getUsrInfoListcb = handle->getUsrInfoListcb;
	gHandle->getUsrInfoListNumcb = handle->getUsrInfoListNumcb;
	gHandle->modifyUsrBindInfocb = handle->modifyUsrBindInfocb;
	if (KEY_FALSE == InitUsrList()){
		return KEY_FALSE;
	}
	
	handle->pGetUsrInfoList = GetUsrInfoList;
	handle->pGetUsrInfoNum = GetUsrInfoListNum;
	handle->pModifyUsrInfo = ModifyUsrInfo;
	handle->pAddUsr = AddUsr;
	handle->pcheckUsr = CheckUser;
	handle->pDelUsr = DelUsr;
	handle->pGetUsrInfo = GetUsrInfo;
	handle->pGetUsrInfoFromUsrName = GetUsrInfoFromUsrName;
	handle->pAuthentication = Authentication;
	handle->pGetPermissionCabByLevel = GetPermissionCabByLevel;
	handle->pModifyPassword = ModifyPassword;
	handle->pResetPassword = ResetPassword;
	handle->pFindUsr = FindUsr;
	handle->pGetUsrId = GetUsrId;
	handle->pSetUsrId = SetUsrId;
	handle->pModifyUsrBindInfo = ModifyUsrBindInfo;
	return KEY_TRUE;
}

_OUT_ int UsrManageUnInit(UsrManageHandle_T *handle)
{
	if(NULL == handle){
		return KEY_FALSE;
	}
	handle->pGetUsrInfoList = NULL;
	handle->pGetUsrInfoNum = NULL;
	handle->pModifyUsrInfo = NULL;
	handle->pAddUsr = NULL;
	handle->pcheckUsr= NULL;
	handle->pDelUsr = NULL;
	handle->pAuthentication = NULL;
	handle->pGetPermissionCabByLevel = NULL;
	handle->pModifyPassword = NULL;
	handle->pResetPassword = NULL;
	handle->pGetUsrInfoFromUsrName = NULL;
	UnInitUsrList();
	
	return KEY_TRUE;
}
