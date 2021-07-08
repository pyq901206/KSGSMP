/***********************************************************************************
*              Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: smtpc_code.h
* Description: supply the encode, decode and encrypt method.
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

#ifndef _SMTPC_CODE_H_
#define  _SMTPC_CODE_H_

/***************************** Macro Definition ******************************/

#define SMTPC_CODE_SetBEncodeOutputSize(len)  (len * 4 / 3 + 4 + len / 72 + 1)
#define SMTPC_CODE_SetBDecodeOutputSize(len)  (len * 3 / 4 + 1)

/******************************* API declaration *****************************/

VOS_VOID SMTPC_CODE_Base64Encode(VOS_CHAR *out, VOS_CONST_CHAR*in, VOS_INT32 inlen);
VOS_INT32 SMTPC_CODE_Base64Decode(VOS_CHAR *out, VOS_CONST_CHAR*in, VOS_INT32 inlen);
VOS_VOID SMTPC_CODE_MD5Digest(VOS_UINT8 *text, 
	                      VOS_INT32 text_len, 
	                      VOS_UINT8 *key, 
	                      VOS_INT32 key_len, 
	                      VOS_UINT8 *digest);
VOS_CHAR *SMTPC_CODE_QEncode(VOS_CHAR *sText);
VOS_UINT8 SMTPC_CODEHexDigit(VOS_UINT8 nDigit);

#endif  /*end of _SMTPC_CODE_H_*/

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */
