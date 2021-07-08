/***********************************************************************************
*              Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: smtpc_send.h
* Description: send the E-mail.
*
* History:
* Version   Date              Author                                 DefectNum              Description
* main\1    2006-08-30   Ou Weiquan 60018927          NULL                       Create this file.
***********************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#ifndef _SMTPC_SEND_H_
#define  _SMTPC_SEND_H_

/***************************** Macro Definition ******************************/

#define SMTP_DEFAULT_PORT 25
#define SMTPC_SEND_RESP_TIMEOUT 60  /*s*/
#define SMTPC_SEND_RESP_OutputBufferSize 100
#define SMTPC_SEND_RESP_BUFSIZE 256
/*modify l59217 2008-06-20 base64 buffer only 76byte*/
//#define SMTPC_SEND_SM_FILEBUFFERSIZE 256
#define SMTPC_SEND_SM_FILEBUFFERSIZE 76
#define SMTPC_SEND_CONN_TIMEOUT 15  /*s*/

/******************************* API declaration *****************************/

VOS_INT32 SMTPC_SEND_Send(PTR_SMTPC_CM_Mail_s Mail, PTR_SMTPC_CONF_Configure_s conf);
VOS_INT32 SMTPC_SENDSocketCreate();
VOS_INT32 SMTPC_SENDSocketConnect(VOS_CHAR *hostName, VOS_UINT16 port);
VOS_INT32 SMTPC_SENDSocketSend(VOS_CHAR *msg);
VOS_INT32 SMTPC_SENDSocketReceive(VOS_CHAR *buf, VOS_INT32 receive_num);
VOS_INT32 SMTPC_SENDIsReadable();
VOS_VOID SMTPC_SENDSocketClose();
VOS_VOID SMTPC_SEND_INF_DisConnect();
VOS_INT32 SMTPC_SEND_Send(PTR_SMTPC_CM_Mail_s Mail, PTR_SMTPC_CONF_Configure_s conf);
VOS_INT32 SMTPC_SEND_CONN_Connect(PTR_SMTPC_CONF_Configure_s config);
VOS_INT32 SMTPC_SEND_CONNLogin(VOS_CHAR *localhostname, VOS_CHAR *username, VOS_CHAR *passwd, SMTPC_CONF_LoginType_E LoginType);
VOS_INT32 SMTPC_SEND_CONNCramLogin(VOS_CHAR *username, VOS_CHAR *passwd);
VOS_INT32 SMTPC_SEND_CONNAuthLogin(VOS_CHAR *username, VOS_CHAR *passwd);
VOS_INT32 SMTPC_SEND_CONNAuthLoginPlain(VOS_CHAR *username, VOS_CHAR *passwd);
VOS_INT32 SMTPC_SEND_RESP_ReadResponse(VOS_UINT32 ExpectCmd, VOS_CHAR *Output, VOS_UINT32 OutLen);
VOS_INT32 SMTPC_SEND_SM_SendMail(PTR_SMTPC_CM_Mail_s Mail);
VOS_INT32 SMTPC_SEND_SMSendMailBody(PTR_SMTPC_CM_MailBody_s mb);
VOS_INT32 SMTPC_SEND_SMSendRecipients(PTR_SMTPC_CM_Address_s Head);

#endif  /*end of _SMTPC_SEND_H_*/

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */
