/***********************************************************************************
*              Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: smtpc_code.c
* Description: supply the encode, decode and encrypt method.
*
* History:
* Version   Date              Author                                 DefectNum              Description
* main\1    2006-09-01   Ou Weiquan 60018927          NULL                       Create this file.
***********************************************************************************/

#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 #include <ctype.h>

#include "os_types_interface.h"
#include "smtpc.h"
#include "md5.h"
#include "smtpc_code.h"

#define EMAIL_MD5_PAD_LEN 64
VOS_VOID SMTPC_CODE_MD5Digest(VOS_UINT8* content, VOS_INT32 content_len, 
                     VOS_UINT8* pwd, VOS_INT32 pwd_len, VOS_UINT8* abstract)
{
    VOS_INT i = 0;

    MD5_CTX ctx;
    VOS_UINT8 input_pad[EMAIL_MD5_PAD_LEN+1] = {0};
    VOS_UINT8 output_pad[EMAIL_MD5_PAD_LEN+1] = {0};

    if((!content) || (!pwd) || (!abstract))
    {
        printf("null pointer %s %d\n", __FUNCTION__, __LINE__);
        return ;
    }
    
    if ((pwd_len < 0) || (pwd_len > EMAIL_MD5_PAD_LEN)) 
    {
        MD5_CTX  tmp_ctx; /* 临时定义的ctx */
        VOS_UINT8 tmp_pwd[16]; /* 临时的密码 */
        MD5Init(&tmp_ctx);
        MD5Update(pwd, &tmp_ctx, pwd_len);
        MD5Final(tmp_pwd, &tmp_ctx);
        pwd = tmp_pwd;
        pwd_len = 16;
    }

    memcpy((void*)input_pad, (void*)pwd, pwd_len);
    memcpy((void*)output_pad, (void*)pwd, pwd_len);
    
    for (i=0; i<EMAIL_MD5_PAD_LEN; i++)
    {
        input_pad[i] = input_pad[i] ^ 0x36;
        output_pad[i] = output_pad[i] ^ 0x5c;
    }

    MD5Init(&ctx);
    MD5Update(input_pad, &ctx, EMAIL_MD5_PAD_LEN);
    MD5Update(content, &ctx, content_len);
    MD5Final(abstract, &ctx);
    
    MD5Init(&ctx);
    MD5Update(output_pad, &ctx, EMAIL_MD5_PAD_LEN);
    MD5Update(abstract, &ctx, 16);
    MD5Final(abstract, &ctx);
}



VOS_CHAR *SMTPC_CODE_QEncode(VOS_CHAR *sText)
{
  //Determine if a translation is needed
  VOS_INT32 bTranslationNeeded = 0;
  VOS_UINT32 nSize = strlen(sText);
  VOS_INT32 i;
  VOS_UINT8 c;
  for (i=0; i<nSize && !bTranslationNeeded; i++)
  {
    c = (VOS_UINT8)sText[i];
    bTranslationNeeded = (c > 127);
  }

  if(bTranslationNeeded)
  {
    VOS_UINT8 *sOut = (VOS_UINT8 *)SMTPC_Malloc(nSize - i + (i* 3));
    /*modify l59217 check sOut whether NULL 2008-06-23*/
    if (sOut == VOS_NULL_PTR)
    {
      HI_ERR_Processor("SMTPC_CODE_QEncode: malloc sOut error");
      return VOS_NULL_PTR;
    }
    for (i=0; i<nSize; i++)
    {
      c = sText[i];
      
      if (c == ' ') // A space
        *sOut++ = '_';
      else if ((c > 127) || (c == '=') || (c == '?') || (c == '_'))
      {
        //Must Quote the text
        *sOut++ = '=';
        *sOut++ = SMTPC_CODEHexDigit((c & 0xF0) >> 4);
        *sOut++ = SMTPC_CODEHexDigit(c & 0x0F);
      }
      else
        *sOut++ = c;
    }
    
    return (VOS_CHAR *)sOut;
  }
  else
  {
    return sText;
  }
}

VOS_UINT8 SMTPC_CODEHexDigit(VOS_UINT8 nDigit)
{
  if (nDigit < 10)
    return (VOS_UINT8)(nDigit + '0');
  else
    return (VOS_UINT8)(nDigit - 10 + 'A');
}
