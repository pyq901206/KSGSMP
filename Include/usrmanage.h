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
#ifndef __USRMANAGE_H__
#define __USRMANAGE_H__
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define  EFFECTIVELENGTH  16 

#ifndef PERMISSION
#define PERMISSION
#define PERMISSION_GETPAYTYPE 0x01 << 0 //获取支付权限
#define PERMISSION_SETPAYTYPE 0x01 << 1 //设置支付权限
#define PERMISSION_GETPAYINFO 0X01 << 2 //获取设备支付信息权限(包括录像类型,有效时间，循环覆盖时间)
#define PERMISSION_GETUSRMANAGE 0X01 << 3 
#endif

typedef enum _ENUM_USRLEVEL_E{
	ENUM_USRLEVEL_Invalid = 0x00,
	ENUM_USRLEVEL_Device,
	ENUM_USRLEVEL_Operator,
	ENUM_USRLEVEL_Administrator,
	ENUM_USRLEVEL_ROOT,
}ENUM_USRLEVEL_E;

typedef struct _UserRegistInfo_T{
	ENUM_USRLEVEL_E  userLevel;
	char 			 usrname[DF_MAXLEN_USRNAME + 1];
	char			 passWord[DF_MAXLEN_PASSWD + 1];	
	char 			 countrycode[DF_MAXLEN_PASSWD + 1];
	char 			 customname[DF_MAXLEN_USRNAME + 1];
	unsigned int	 usrID;
}UserRegistInfo_T;


typedef struct _UsrInfo_T
{
	UserRegistInfo_T	registInfo;
}UsrInfo_T;


typedef struct _UsrManageRegistHandle_T{
	int	(*pGetUserRegistInfo)(UserRegistInfo_T *info);//获取注册信息
}UsrManageRegistHandle_T;

typedef int (*AddUsrCbFun)(UsrInfo_T *info);
typedef int (*DelUsrCbFun)(UsrInfo_T *info);
typedef int (*GetUsrInfoCbFun)(UsrInfo_T *info);
typedef int (*SetUsrInfoCbFun)(UsrInfo_T *info);
typedef int (*GetUsrInfoListCbFun)(UsrInfo_T *info);
typedef int (*GetUsrInfoListNumCbFun)(int *num);
typedef int (*ModifyUsrBindInfoCbFun)(UsrInfo_T *info, char *oldusrname);

typedef struct _UsrManageHandle_T{
	int	(*pGetUsrInfoList)(UsrInfo_T *info);//获取所有用户 最多2W个
	int	(*pGetUsrInfoNum)(int *num);
	int	(*pModifyUsrInfo)(UsrInfo_T *curUsrInfo,UsrInfo_T *info);//修改用户信息
	int (*pAddUsr)(UsrInfo_T *curUsrInfo,UsrInfo_T *info);//添加用户
	int (*pcheckUsr)(UsrInfo_T *curUsrInfo,UsrInfo_T *info);//添加用户
	int (*pDelUsr)(UsrInfo_T *curUsrInfo,UsrInfo_T *info);//删除用户
	int (*pGetUsrInfo)(UsrInfo_T *curUsrInfo,UsrInfo_T *info);//获取当前用户信息
	int (*pGetUsrInfoFromUsrName)(char *usrName, UsrInfo_T *info);
	int (*pAuthentication)(const char *usrname,const char *password);//鉴权
	int (*pGetPermissionCabByLevel)(ENUM_USRLEVEL_E usrLevel);//返回权限能力	
	int	(*pModifyPassword)(char *usrname,char *oldPassword,char *newPassword);//修改用户信息
	int (*pFindUsr)(char *usrname);
	int (*pGetUsrId)(char *usrname, char *usrid);
	int (*pSetUsrId)(char *usrname, char *usrid);
	int (*pModifyUsrBindInfo)(UsrInfo_T *info, char *oldusrname);
	int (*pResetPassword)(char *usrname, char *countrycode, char *password);
	//数据库操作回调
	AddUsrCbFun	addUsrcb;
	DelUsrCbFun delUsrcb;
	GetUsrInfoCbFun getUsrInfocb;
	SetUsrInfoCbFun setUsrInfocb;
	GetUsrInfoListCbFun getUsrInfoListcb;
	GetUsrInfoListNumCbFun getUsrInfoListNumcb;
	ModifyUsrBindInfoCbFun	modifyUsrBindInfocb;
}UsrManageHandle_T;

_OUT_ int UsrManageRegist(UsrManageRegistHandle_T *handle);
_OUT_ int UsrManageUnRegist(UsrManageRegistHandle_T *handle);
_OUT_ int UsrManageInit(UsrManageHandle_T *handle);
_OUT_ int UsrManageUnInit(UsrManageHandle_T *handle);


#ifdef __cplusplus
}
#endif

#endif
