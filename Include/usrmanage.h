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
#ifndef __USRMANAGE_H__
#define __USRMANAGE_H__
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define  EFFECTIVELENGTH  16 

#ifndef PERMISSION
#define PERMISSION
#define PERMISSION_GETPAYTYPE 0x01 << 0 //��ȡ֧��Ȩ��
#define PERMISSION_SETPAYTYPE 0x01 << 1 //����֧��Ȩ��
#define PERMISSION_GETPAYINFO 0X01 << 2 //��ȡ�豸֧����ϢȨ��(����¼������,��Чʱ�䣬ѭ������ʱ��)
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
	int	(*pGetUserRegistInfo)(UserRegistInfo_T *info);//��ȡע����Ϣ
}UsrManageRegistHandle_T;

typedef int (*AddUsrCbFun)(UsrInfo_T *info);
typedef int (*DelUsrCbFun)(UsrInfo_T *info);
typedef int (*GetUsrInfoCbFun)(UsrInfo_T *info);
typedef int (*SetUsrInfoCbFun)(UsrInfo_T *info);
typedef int (*GetUsrInfoListCbFun)(UsrInfo_T *info);
typedef int (*GetUsrInfoListNumCbFun)(int *num);
typedef int (*ModifyUsrBindInfoCbFun)(UsrInfo_T *info, char *oldusrname);

typedef struct _UsrManageHandle_T{
	int	(*pGetUsrInfoList)(UsrInfo_T *info);//��ȡ�����û� ���2W��
	int	(*pGetUsrInfoNum)(int *num);
	int	(*pModifyUsrInfo)(UsrInfo_T *curUsrInfo,UsrInfo_T *info);//�޸��û���Ϣ
	int (*pAddUsr)(UsrInfo_T *curUsrInfo,UsrInfo_T *info);//����û�
	int (*pcheckUsr)(UsrInfo_T *curUsrInfo,UsrInfo_T *info);//����û�
	int (*pDelUsr)(UsrInfo_T *curUsrInfo,UsrInfo_T *info);//ɾ���û�
	int (*pGetUsrInfo)(UsrInfo_T *curUsrInfo,UsrInfo_T *info);//��ȡ��ǰ�û���Ϣ
	int (*pGetUsrInfoFromUsrName)(char *usrName, UsrInfo_T *info);
	int (*pAuthentication)(const char *usrname,const char *password);//��Ȩ
	int (*pGetPermissionCabByLevel)(ENUM_USRLEVEL_E usrLevel);//����Ȩ������	
	int	(*pModifyPassword)(char *usrname,char *oldPassword,char *newPassword);//�޸��û���Ϣ
	int (*pFindUsr)(char *usrname);
	int (*pGetUsrId)(char *usrname, char *usrid);
	int (*pSetUsrId)(char *usrname, char *usrid);
	int (*pModifyUsrBindInfo)(UsrInfo_T *info, char *oldusrname);
	int (*pResetPassword)(char *usrname, char *countrycode, char *password);
	//���ݿ�����ص�
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
