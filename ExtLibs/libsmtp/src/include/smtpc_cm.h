/***********************************************************************************
*              Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: smtpc_cm.h
* Description: Create the mail.
*
* History:
* Version   Date              Author                                 DefectNum              Description
* main\1    2006-09-01   Ou Weiquan 60018927          NULL                       Create this file.
***********************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#ifndef _SMTPC_CM_H_
#define  _SMTPC_CM_H_

/***************************** Macro Definition ******************************/

#define SMTPC_CM_DefaultCharset  "ISO-8859-1"


/*************************** Structure Definition ****************************/

typedef struct hiSMTPC_CM_Address_s
{
  VOS_CHAR *Name;
  VOS_CHAR *Addr;
  struct hiSMTPC_CM_Address_s *Next;
} SMTPC_CM_Address_s, *PTR_SMTPC_CM_Address_s;

typedef struct hiSMTPC_CM_MailBody_s
{
  VOS_CHAR *Text;
  VOS_CHAR *Attachment;
  VOS_CHAR *MailBodyHeader;
  VOS_CHAR *MailBodyFooter;
  struct hiSMTPC_CM_MailBody_s *Next;
} SMTPC_CM_MailBody_s, *PTR_SMTPC_CM_MailBody_s;


typedef struct hiSMTPC_CM_Mail_s
{
  PTR_SMTPC_CM_Address_s  Sender;
  PTR_SMTPC_CM_Address_s  ToRecipientsHead;
  PTR_SMTPC_CM_Address_s  CcRecipientsHead;
  PTR_SMTPC_CM_Address_s  BcRecipientsHead;
  VOS_CHAR *Header;
  PTR_SMTPC_CM_MailBody_s MailBodyHead;
} SMTPC_CM_Mail_s, *PTR_SMTPC_CM_Mail_s;

typedef enum hiSMTPC_CM_MailBodyType_E
{
   IsText,
   IsAttachment
} SMTPC_CM_MailBodyType_E;

/******************************* API declaration *****************************/

PTR_SMTPC_CM_Mail_s SMTPC_CM_CreateMail(PTR_SMTPC_INF_MailContent_s);
PTR_SMTPC_CM_Address_s SMTPC_CMCreateAddresses(VOS_CHAR *Address);
PTR_SMTPC_CM_Address_s SMTPC_CMCreateAddressLinkList();
PTR_SMTPC_CM_Address_s SMTPC_CMAddAddressNode(VOS_CHAR *Address, PTR_SMTPC_CM_Address_s ptr);
VOS_CHAR *SMTPC_CMCreateHeader(PTR_SMTPC_CM_Address_s Sender, 
                               PTR_SMTPC_CM_Address_s ToRecipientsHead, 
                               PTR_SMTPC_CM_Address_s CcRecipientsHead, 
                               PTR_SMTPC_CM_Address_s BcRecipientsHead, 
                               VOS_CHAR *Subject, VOS_CHAR *Charset, 
                               VOS_UINT32 HeaderLen);
VOS_CHAR *SMTPC_CMSetFormat(VOS_CHAR *Str, const VOS_CHAR *Charset);
PTR_SMTPC_CM_MailBody_s SMTPC_CMCreateMailBody(VOS_CHAR *Text, VOS_CHAR *Attachment, VOS_CHAR *Charset);
PTR_SMTPC_CM_MailBody_s SMTPC_CMCreateMailBodyLinkList();
PTR_SMTPC_CM_MailBody_s SMTPC_CMAddMailBodyNode(VOS_CHAR *Charset, VOS_CHAR *Content,
                                                PTR_SMTPC_CM_MailBody_s Ptr, 
                                                SMTPC_CM_MailBodyType_E MailBodyType);
VOS_VOID SMTPC_CMFreeMail(PTR_SMTPC_CM_Mail_s Mail);
VOS_VOID SMTPC_CMFreeAddress(PTR_SMTPC_CM_Address_s Ptr);
VOS_VOID SMTPC_CMFreeMailBody(PTR_SMTPC_CM_MailBody_s Ptr);

#endif  /*end of _SMTPC_CM_H_*/

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */
