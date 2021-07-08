/***********************************************************************************
*              Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: smtpc.c
* Description: The main file of SMTPC, supply the user interface: HI_SMTPC_SendMail.
*
* History:
* Version   Date              Author                                 DefectNum              Description
* main\1    2006-09-01   Ou Weiquan 60018927          NULL                       Create this file.
***********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "dmalloc.h"
#include "os_types_interface.h"
#include "smtpc.h"
#include "smtpc_cm.h"
#include "smtpc_send.h"
#include "smtpc_log.h"

static int malloctime = 0;
static int realloctime = 0;
static int freetime = 0;

/*modify by q46326, get rid of exit. 2007-04-29*/

/***********************************************************************************
* Function:          HI_SMTPC_SendMail
* Description:      user interface
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:              mc      mail content
*                        config  the SMTPC configure
* Output:            none
* Return:            0:   success
*                        -1: error
***********************************************************************************/
VOS_INT32 HI_SMTPC_SendMail(PTR_SMTPC_INF_MailContent_s  mc, PTR_SMTPC_CONF_Configure_s config)
{
   PTR_SMTPC_CM_Mail_s Mail;
//   VOS_INT32 len = sizeof(struct hiSMTPC_CM_Address_s);
   if(SMTPC_CONF_SetConfigure(config) == -1)
   {
      HI_ERR_Processor("HI_SMTPC_SendMail: set configure error");
      return -1;
   }
   if((Mail = SMTPC_CM_CreateMail(mc)) == VOS_NULL_PTR)
   {
      HI_ERR_Processor("HI_SMTPC_SendMail: create Mail error");
      SMTPC_CMFreeMail(Mail);
      return -1;
   }
   if(SMTPC_SEND_Send(Mail, config) == -1)
   {
      HI_ERR_Processor("HI_SMTPC_SendMail: send Mail error");
      SMTPC_CMFreeMail(Mail);
      return -1;
   }
   SMTPC_CMFreeMail(Mail);   
   return 0;
}

/***********************************************************************************
* Function:          SMTPC_Malloc
* Description:      distrubute the memary
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:              size      mail content
* Output:            none
* Return:            the pointer of memary which has been distrubuted: success
* Others:            if distrubuting the memary is fail, SMTPC will exit.
***********************************************************************************/
VOS_VOID * SMTPC_Malloc(VOS_UINT32 size) 
{
	VOS_CHAR *ptr = NULL;

	malloctime ++;

	ptr = malloc( size + 1);

	if (!ptr) 
	{
		HI_ERR_Processor("system malloc error");
		return NULL;
	}
	
	memset(ptr, 0, size + 1);

	*(ptr + size) = '+';
	
	return ptr;
}

/***********************************************************************************
* Function:          SMTPC_Free
* Description:      free the memary
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:              none
* Output:            pointer of memory
* Return:            none
* Others:            free memory.
***********************************************************************************/
VOS_VOID * SMTPC_Free(VOS_CHAR *ptr) 
{
	if (!ptr) 
	{
//		HI_ERR_Processor("ptr empty");
		return VOS_NULL_PTR;
	}
	free(ptr);
	ptr = NULL;
	freetime++;
	return VOS_NULL_PTR;
}

/***********************************************************************************
* Function:          SMTPC_Realloc
* Description:      redistrubute the memary
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:              oldptr      the pointer of the memary which will be redistrubuted
*                        size         the memary size to redistrubute
* Output:            none
* Return:            the pointer of memary which has been redistrubuted: success
* Others:            if distrubuting the memary is fail, SMTPC will exit.
***********************************************************************************/
VOS_VOID * SMTPC_Realloc(VOS_VOID*oldptr, VOS_UINT32 size)
{
	VOS_VOID *ptr ;
	if (!oldptr) 
	{
		HI_ERR_Processor("oldptr error");
		return NULL;
	}	
	ptr = realloc(oldptr, size);
	realloctime++;
	if (!ptr) 
	{
		HI_ERR_Processor("realloc error");
		return NULL;
	}
	return ptr;
}

/***********************************************************************************
* Function:       SMTPC_CONF_SetConfigure
* Description:   config the SMTPC
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         conf         the pointer of SMTPC_CONF_Configure_s struct which has been set by the user
* Output:       none
* Return:       PTR_SMTPC_CONF_Configure_s: success
*                   VOS_NULL_PTR:                         fail
***********************************************************************************/
VOS_INT32 SMTPC_CONF_SetConfigure(PTR_SMTPC_CONF_Configure_s conf)
{
     if(conf->Server == VOS_NULL_PTR)
     {   
        HI_ERR_Processor("SMTPC_CONF_SetConfigure: The server isn't set");
        return -1;
     }
     if ( conf->Port == 0 )
     {
         conf->Port = SMTP_DEFAULT_PORT;
     }
     if ( conf->LoginType > NoLogin)
     {
         HI_ERR_Processor("SMTPC_CONF_SetConfigure: invalid login type!");
         return -1;         
     }
     else if(conf->LoginType != NoLogin)
     {
         if( conf->User == VOS_NULL_PTR || conf->Passwd == VOS_NULL_PTR)
         {
             HI_ERR_Processor("SMTPC_CONF_SetConfigure: uesrname or password is needed for login");
             return -1;
         }
     }
     if ( conf->SslType >= SSL_TYPE_BUTT )
     {
         HI_ERR_Processor("SMTPC_CONF_SetConfigure: invalid SSL type!");
         return -1;                  
     }
     if ( conf->RespTimeout == 0 )
     {
         conf->RespTimeout = SMTPC_SEND_RESP_TIMEOUT;
     }
     if ( conf->ConnTimeout == 0 )
     {
         conf->ConnTimeout = SMTPC_SEND_CONN_TIMEOUT;
     }     
     return 0;
}


