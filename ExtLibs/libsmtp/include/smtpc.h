/***********************************************************************************
*              Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: smtpc.h
* Description: The main file of SMTPC, supply the user interface: HI_SMTPC_SendMail.
*
* History:
* Version   Date              Author                                 DefectNum              Description
* main\1    2006-09-01   Ou Weiquan 60018927          NULL                       Create this file.
* Version   Date              Author                                 DefectNum              Description
* main\2    2008-02-27   Qu Shen 46326  Add ssl feature to support Gmail and Yahoo!

***********************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#ifndef  _SMTPC_H_
#define  _SMTPC_H_

#include "os_types_interface.h"

/***************************** Macro Definition ******************************/

#define HI_ERR_Processor(ErrStr)  do{printf(ErrStr);printf("\n");}while(0)


/*************************** Structure Definition ****************************/

typedef enum hiSMTPC_CONF_LoginType_E
{
  CramLogin,        
  AuthLogin,         
  AuthLoginPlain, 
  NoLogin             
} SMTPC_CONF_LoginType_E;

typedef enum hiSMTP_SSL_TYPE_E
{
    SSL_TYPE_NOSSL = 0, /*不采取加密算法的非安全传输方式*/
    SSL_TYPE_STARTTLS,  /*采取STARTTLS加密算法的安全传输方式*/    
    SSL_TYPE_BUTT    
}SMTP_SSL_TYPE_E;


typedef struct hiSMTPC_CONF_Configure_s
{
  VOS_CHAR * Server;
  VOS_UINT16 Port;      /*端口为0则使用SMTP默认端口25*/
  VOS_CHAR * User;
  VOS_CHAR * Passwd;
  SMTPC_CONF_LoginType_E LoginType;
  SMTP_SSL_TYPE_E        SslType;
  VOS_UINT32 RespTimeout;   /*单位为秒，设置为0则使用系统默认的超时时间60s*/
  VOS_UINT32 ConnTimeout;   /*单位为秒，设置为0则使用系统默认的超时时间15s*/  
} SMTPC_CONF_Configure_s, *PTR_SMTPC_CONF_Configure_s;

typedef struct hiSMTPC_INF_MailContent_s
{
   VOS_CHAR *From;
   VOS_CHAR *To;
   VOS_CHAR *Cc;
   VOS_CHAR *Bc;
   VOS_CHAR *Text;
   VOS_CHAR *Attachment;
   VOS_CHAR *Subject;
   VOS_CHAR *Charset;
} SMTPC_INF_MailContent_s, *PTR_SMTPC_INF_MailContent_s;

/******************************* API declaration *****************************/

VOS_INT32 HI_SMTPC_SendMail(PTR_SMTPC_INF_MailContent_s  mc, 
	                    PTR_SMTPC_CONF_Configure_s config);
VOS_VOID * SMTPC_Malloc(VOS_UINT32 size);
VOS_VOID * SMTPC_Realloc(VOS_VOID*oldptr, VOS_UINT32 size);
VOS_INT32 SMTPC_CONF_SetConfigure(PTR_SMTPC_CONF_Configure_s conf);
VOS_VOID * SMTPC_Free(VOS_CHAR *ptr);

#endif  /*end of _SMTPC_H_*/

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */
