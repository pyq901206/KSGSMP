/***********************************************************************************
*              Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: smtpc_cm.c
* Description: Create the mail.
*
* History:
* Version   Date              Author                                 DefectNum              Description
* main\1    2006-09-01   Ou Weiquan 60018927          NULL                       Create this file.
***********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#include "os_types_interface.h"
#include "smtpc.h"
#include "smtpc_cm.h"
#include "smtpc_code.h"

/***********************************************************************************
* Function:       SMTPC_CM_CreateMail
* Description:   create the mail for sending
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         mc     the mail content which is set by the user
* Output:       none
* Return:       PTR_SMTPC_CM_Mail_s:   success
*                   VOS_NULL_PTR:              fail
* Others:       none
***********************************************************************************/
PTR_SMTPC_CM_Mail_s SMTPC_CM_CreateMail(PTR_SMTPC_INF_MailContent_s mc)
{
    VOS_CHAR *From = mc->From;
    VOS_CHAR *To = mc->To;
    VOS_CHAR *Cc = mc->Cc;
    VOS_CHAR *Bc = mc->Bc;
    VOS_CHAR *Subject = mc->Subject;
    VOS_CHAR *Charset = mc->Charset;
    VOS_CHAR *Text = mc->Text;
    VOS_CHAR *Attachment = mc->Attachment;
    VOS_UINT32 HeaderLen;
    VOS_CHAR *Sender;
    PTR_SMTPC_CM_Mail_s Mail;

    if(From == VOS_NULL_PTR || To == VOS_NULL_PTR)
    {
       HI_ERR_Processor("SMTPC_CM_CreateMail: The Mail must have a sender and a ToRecipient");
       return (PTR_SMTPC_CM_Mail_s)VOS_NULL_PTR;
    }

    if(Charset == VOS_NULL_PTR)
    {
        Charset = SMTPC_CM_DefaultCharset;
    }
    
    if(Subject != VOS_NULL_PTR)
    {
       HeaderLen = strlen(From) + strlen(To) + strlen(Subject) + 4 * strlen(Charset);
    }
    else
    {
       HeaderLen = strlen(From) + strlen(To) + 4 * strlen(Charset);
    }
   
   /*check the sender. One mail has only one sender */
   Sender = From;
  
   if((Sender = strchr(Sender, ';'))  != 0) 
   {     
      while(isspace(*(++Sender)));
      if(*(Sender) != '\0')
      {
          HI_ERR_Processor("SMTPC_CM_CreateMail: the number of sender is greater then 1");
          return (PTR_SMTPC_CM_Mail_s)VOS_NULL_PTR;
      }
   }
   /*end of checking sender*/

  Mail = (PTR_SMTPC_CM_Mail_s)SMTPC_Malloc(sizeof(SMTPC_CM_Mail_s));
  /*modify l59217 check pointer whether NULL 2008-06-23*/
  if (Mail == VOS_NULL_PTR)
  {
     HI_ERR_Processor("SMTPC_CM_CreateMail: malloc Mail error");
     return (PTR_SMTPC_CM_Mail_s)VOS_NULL_PTR;
  }
   /* create sender*/
  if ((Mail->Sender = SMTPC_CMCreateAddresses(From)) == VOS_NULL_PTR) 
  {
      HI_ERR_Processor("SMTPC_CM_CreateMail: set sender error");
      SMTPC_CMFreeMail(Mail);
      return (PTR_SMTPC_CM_Mail_s)VOS_NULL_PTR;
  }
   /*end of creating sender*/
  
   /* create ToRecipient */
  if ((Mail->ToRecipientsHead = SMTPC_CMCreateAddresses (To)) == VOS_NULL_PTR)
  {
      HI_ERR_Processor("SMTPC_CM_CreateMail: set ToRecipient error");
      SMTPC_CMFreeMail(Mail);
      return VOS_NULL_PTR;
  }
  /*end of creating ToRecipient */

  /*create CcRecipient, CcRecipient is not necessary*/
  if(Cc == VOS_NULL_PTR)
  {
      Mail->CcRecipientsHead = VOS_NULL_PTR;
  }
  else
  {
      if((Mail->CcRecipientsHead = SMTPC_CMCreateAddresses (Cc)) == VOS_NULL_PTR)
      {
         HI_ERR_Processor("SMTPC_CM_CreateMail: set CcRecipient error");
         SMTPC_CMFreeMail(Mail);
         return VOS_NULL_PTR;
      }
      HeaderLen += strlen(Cc);
  }
  /*end of creating CcRecipient*/

  /*create CcRecipient, BcRecipient is not necessary*/
  if(Bc == VOS_NULL_PTR)
  {
      Mail->BcRecipientsHead = VOS_NULL_PTR;
  }
  else
  {
      if((Mail->BcRecipientsHead = SMTPC_CMCreateAddresses (Bc)) == VOS_NULL_PTR)
      {
         HI_ERR_Processor("SMTPC_CM_CreateMail: set BcRecipient error");
         SMTPC_CMFreeMail(Mail);
         return VOS_NULL_PTR;
      }
      HeaderLen += strlen(Bc);
  }
  /*end of creating BcRecipient*/
  
  /*create mail header*/
  if ((Mail->Header = SMTPC_CMCreateHeader(Mail->Sender, Mail->ToRecipientsHead, 
                                           Mail->CcRecipientsHead,  Mail->BcRecipientsHead, 
	                                   Subject, Charset, HeaderLen)) == VOS_NULL_PTR) 
  {
     HI_ERR_Processor("SMTPC_CM_CreateMail: create header error");
     SMTPC_CMFreeMail(Mail);
     return VOS_NULL_PTR;
  }
   /*end of creating mail header*/

  /* create mail body*/
  if(Text != VOS_NULL_PTR || Attachment !=VOS_NULL_PTR)
  {
     if((Mail->MailBodyHead = SMTPC_CMCreateMailBody (Text, Attachment, Charset)) == VOS_NULL_PTR)
     {
       HI_ERR_Processor("SMTPC_CM_CreateMail: create MailBody error");
       SMTPC_CMFreeMail(Mail);
       return VOS_NULL_PTR;
     }
  }
  else
  {
     Mail->MailBodyHead = VOS_NULL_PTR;
  }
  /*end of creating mail body*/
  
	return Mail;

}

/***********************************************************************************
* Function:       SMTPC_CMCreateAddresses
* Description:   create the address linklist
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         Address     the address string which contain one or more addresses
* Output:       none
* Return:       PTR_SMTPC_CM_Address_s:  success
*                   VOS_NULL_PTR:                   fail
* Others:       none
***********************************************************************************/
PTR_SMTPC_CM_Address_s SMTPC_CMCreateAddresses(VOS_CHAR *Address)
{
   PTR_SMTPC_CM_Address_s head, ptr;
   VOS_UINT32 i = 0;
   VOS_CHAR *buf;

   if(Address == VOS_NULL_PTR)
   {
      HI_ERR_Processor("SMTPC_CMCreateAddresses: the Address is Null");
      return VOS_NULL_PTR;
   }

   if(strchr(Address, '@') == 0)
   {
      HI_ERR_Processor("SMTPC_CMCreateAddresses: the Address has no '@', wrong email address");
      return VOS_NULL_PTR;
   }

   head =  SMTPC_CMCreateAddressLinkList();
   /*modify l59217 check head whether NULL 2008-06-23*/
   if (head == VOS_NULL_PTR)
   {
      HI_ERR_Processor("SMTPC_CMCreateAddresses: create addresslinklist head error");
      return VOS_NULL_PTR;
   }
   ptr = head;
   while(*Address != '\0')
   {
       for(i = 0; *(Address + i) != ';' && *(Address + i) != '\0'; i++);
       if(i <= 1)
       {
          HI_ERR_Processor("SMTPC_CMCreateAddresses: the Address String is an error");
          /*modify l59217 free head to avoid mem leak 2008-06-23*/
          SMTPC_CMFreeAddress(head);
          return VOS_NULL_PTR;
       }
       buf = (VOS_CHAR *)SMTPC_Malloc(i + 1);
       /*modify l59217 check buf whether NULL 2008-06-23*/
       if (buf == VOS_NULL_PTR)
       {
          HI_ERR_Processor("SMTPC_CMCreateAddresses: malloc buf error");
          SMTPC_CMFreeAddress(head);
          return VOS_NULL_PTR;
       }
       memcpy(buf, Address, i);
       *(buf + i) ='\0';
       if((ptr->Next = SMTPC_CMAddAddressNode(buf, ptr)) == VOS_NULL_PTR)
       {
           HI_ERR_Processor("SMTPC_CMCreateAddresses: add node error");

           Address += i;
           if(*Address == ';')
           {
              //move the pointer to the first non-space char affter the ';'
              while(isspace(*(++Address)));
           }

          /*HI_ERR_Processor(buf);*/
           SMTPC_Free(buf);
          continue;
           /*Modify l59217 to avoid memory leak*/
           //SMTPC_CMFreeAddress(head);
           //三个收件人其中某个收件人的地址错误不应该影响另外两个，创建整个地址里不需要返回  z00183188
           //return VOS_NULL_PTR;   
       }
       Address += i;
       while ((*Address == ';') || isspace(*Address))
       {
          //move the pointer to the first non-space char affter the ';'
          Address++;
       }
       SMTPC_Free(buf);
	   ptr = ptr->Next;
   }

   return head;
}

/***********************************************************************************
* Function:       SMTPC_CMCreateAddressLinkList
* Description:   create the head of address linklist
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         none
* Output:       none
* Return:       PTR_SMTPC_CM_Address_s:  success
* Others:       none
***********************************************************************************/
PTR_SMTPC_CM_Address_s SMTPC_CMCreateAddressLinkList()
{
   PTR_SMTPC_CM_Address_s head = (PTR_SMTPC_CM_Address_s)SMTPC_Malloc(sizeof(SMTPC_CM_Address_s));
   /*modify l59217 check head whether NULL 2008-06-23*/
   if (head == VOS_NULL_PTR)
   {
      HI_ERR_Processor("SMTPC_CMCreateAddressLinkList: malloc head error");
      return (PTR_SMTPC_CM_Address_s)VOS_NULL_PTR;
   }
   head->Name = VOS_NULL_PTR;
   head->Addr = VOS_NULL_PTR;
   head->Next = VOS_NULL_PTR;
   return head;
}

/***********************************************************************************
* Function:       SMTPC_CMAddAddressNode
* Description:   add the node to the address linklist
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         Address     one address for adding
*                   ptr            the pointer of the address node which will be added a  new address node
* Output:       none
* Return:       PTR_SMTPC_CM_Address_s: success
*                   VOS_NULL_PTR:                  fail
* Others:       none
***********************************************************************************/
PTR_SMTPC_CM_Address_s SMTPC_CMAddAddressNode(VOS_CHAR *Address, PTR_SMTPC_CM_Address_s ptr)
{
   VOS_CHAR *TheLastSpace;
   VOS_CHAR *name;
   VOS_CHAR *addr;
   VOS_INT32 NameLen =0;
   PTR_SMTPC_CM_Address_s node;

   if(strchr(Address, '@') == 0)
   {
      HI_ERR_Processor("SMTPC_CMAddAddressNode: the Address String has no '@', wrong email address");
      return (PTR_SMTPC_CM_Address_s)VOS_NULL_PTR;
   }
   
   if((TheLastSpace = rindex(Address, ' ')) == 0)
   {
      name = VOS_NULL_PTR;
      addr = (VOS_CHAR *)SMTPC_Malloc(strlen(Address) + 1);
      /*modify l59217 check addr whether NULL 2008-06-23*/
      if (addr == VOS_NULL_PTR)
      {
         HI_ERR_Processor("SMTPC_CMAddAddressNode: malloc addr error");
         return (PTR_SMTPC_CM_Address_s)VOS_NULL_PTR;
      }
      memcpy(addr, Address,strlen(Address));
	  *(addr + strlen(Address)) = '\0';
   }
   else
   {
      NameLen = TheLastSpace -Address;
      name = (VOS_CHAR *)SMTPC_Malloc(NameLen + 1);
      /*modify l59217 check name whether NULL*/
      if (name == VOS_NULL_PTR)
      {
         HI_ERR_Processor("SMTPC_CMAddAddressNode: malloc name error");
         return (PTR_SMTPC_CM_Address_s)VOS_NULL_PTR;
      }
      memcpy(name, Address, NameLen);
      *(name+NameLen) = '\0';
      addr = (VOS_CHAR *)SMTPC_Malloc(strlen(Address) - NameLen + 1);
      /*modify l59217 check addr whether NULL*/
      if (addr == VOS_NULL_PTR)
      {
         HI_ERR_Processor("SMTPC_CMAddAddressNode: malloc addr error");
         SMTPC_Free(name);
         return (PTR_SMTPC_CM_Address_s)VOS_NULL_PTR;
      }
      ++TheLastSpace;
      memcpy(addr, TheLastSpace,strlen(Address) - NameLen);
      *(addr + strlen(Address) - NameLen) = '\0';
       if(strchr(addr, '@') == 0)
      {
         HI_ERR_Processor("SMTPC_CMAddAddressNode: the Address has no '@', wrong email address");
         SMTPC_Free(name);
         SMTPC_Free(addr);
         return (PTR_SMTPC_CM_Address_s)VOS_NULL_PTR;
      }
   }
   
   if( ptr->Next == VOS_NULL_PTR)
   {
  	   node = (PTR_SMTPC_CM_Address_s)SMTPC_Malloc(sizeof(SMTPC_CM_Address_s));
       /*modify l59217 check node whether NULL 2008-06-23*/
       if (node == VOS_NULL_PTR)
       {
           HI_ERR_Processor("SMTPC_CMAddAddressNode: malloc node error");
           SMTPC_Free(name);
           SMTPC_Free(addr);
           return (PTR_SMTPC_CM_Address_s)VOS_NULL_PTR;
       }
	   node->Name = name;
	   node->Addr = addr;
	   node->Next = VOS_NULL_PTR;
	   //ptr->Next = node;
   }
  else
  {
      HI_ERR_Processor("SMTPC_CMAddAddressNode: Can not add the node");
      SMTPC_Free(name);
      SMTPC_Free(addr);
      return VOS_NULL_PTR;
  }
 
  return node;
}

/*modify l59217 add stand time express + timezone 2008-07-14*/
extern long timezone;
#define TIMESTRLEN 27
#define MAXLEN	     40
/***********************************************************************************
* Function:       SMTPC_CMCreateHeader
* Description:   create the header of the mail
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         sender                 the header of the sender address linklist which only has one node
*                   ToRecipientHead   the header of the To address linklist
*                   CcRecipientHead   the header of the Cc address linklist
*                   BcRecipientHead   the header of the Bc address linklist
*                   Subject                the subject of the mail
*                   Charset                the Charset using in the mail
*                   HeaderLen            the length of header which is estimated for distrubuting the memary
*                                              to contain the header string
* Output:       none
* Return:       the string pointer:  success
*                   VOS_NULL_PTR:     fail
* Others:       none
***********************************************************************************/
VOS_CHAR *SMTPC_CMCreateHeader(PTR_SMTPC_CM_Address_s Sender, 
                                                         PTR_SMTPC_CM_Address_s ToRecipientsHead, 
                                                         PTR_SMTPC_CM_Address_s CcRecipientsHead, 
                                                         PTR_SMTPC_CM_Address_s BcRecipientsHead, 
                                                         VOS_CHAR *Subject, VOS_CHAR *Charset, 
                                                         VOS_UINT32 HeaderLen)
{
   time_t Date;
   /*modify l59217 add stand time express + timezone 2008-07-14*/
   struct tm *p;
   VOS_CHAR* Format = "%a, %d %b %Y %X";
   VOS_CHAR StrTime[MAXLEN];
   VOS_INT32 DateLen;
   /*modify l59217 use user time buffer 2008-06-23*/
   VOS_CHAR DateBuf[TIMESTRLEN];
   
   VOS_CHAR *DateStr = StrTime;
   VOS_CHAR *OtherField1;
   VOS_CHAR *OtherField2;   
   VOS_CHAR *OtherField3;
   VOS_CHAR *OtherField4;
   VOS_UINT32 BufLen;
   VOS_CHAR *Buf;
   PTR_SMTPC_CM_Address_s Ptr;
   VOS_CHAR *FormatStr;
   	
   /*get the time string*/
   time(&Date);
   tzset();
   memset(DateStr, 0, sizeof(StrTime));
   memset(DateBuf, 0, sizeof(DateBuf));
   /*
   DateStr = ctime(&Date);
   DateLen = strlen(DateStr);
   *(DateStr + DateLen - 1) = '\0';
   
   DateStr = ctime_r(&Date, DateBuf);
   DateLen = strlen(DateStr);
   *(DateStr + DateLen - 1) = '\0';
   */
   /*modify l59217 add stand time express + timezone 2008-07-14*/
   p = localtime(&Date);
   strftime(DateBuf, sizeof(DateBuf), Format, p);
   if (timezone >= 0)
   {
      sprintf(DateStr, "%s -0%ld00 ", DateBuf, (timezone/3600));
   }
   else
   {
      sprintf(DateStr, "%s +0%ld00 ", DateBuf, (timezone/(-3600)));
   }
   DateLen = strlen(DateStr);
   /*end of getting the time*/
   
   OtherField1 = "X-Mailer: SMTPC\r\n";
   OtherField2 = "MIME-Version: 1.0\r\n";
   OtherField3 = "Content-Type: Multipart/mixed;\r\n	Boundary=\"NextPart\"\r\n";
   OtherField4 = "This is a multi-part message in MIME format.";

   BufLen = 2 * HeaderLen + strlen(OtherField1)+ strlen(OtherField2) + strlen(OtherField3) + strlen(OtherField4) + DateLen  + 50;
   Buf = (VOS_CHAR *)SMTPC_Malloc(BufLen);
   /*modify l59217 check Buf whether NULL 2008-06-23*/
   if (Buf == VOS_NULL_PTR)
   {
      HI_ERR_Processor("SMTPC_CMCreateHeader: malloc Buf error");
      return VOS_NULL_PTR;
   }
   /*set the From field*/
   Ptr = Sender->Next;
   strcpy(Buf, "From: ");
   if(Ptr->Name == VOS_NULL_PTR)
   {
       strcat(Buf, "<");
       strcat(Buf, Ptr->Addr);
       strcat(Buf, ">");
   }
   else
   {
      FormatStr = SMTPC_CMSetFormat(Ptr->Name, Charset);
      /*modify l59217 check FormatStr whether NULL 2008-06-23*/
      if (FormatStr == VOS_NULL_PTR)
      {
         HI_ERR_Processor("SMTPC_CMCreateHeader: malloc FormatStr error");
         SMTPC_Free(Buf);
         return VOS_NULL_PTR;
      }
      strcat(Buf, FormatStr);
      strcat(Buf, "<");
      strcat(Buf, Ptr->Addr);
      strcat(Buf, ">");
      SMTPC_Free(FormatStr);
   }
   strcat(Buf, "\r\n");
   /*end of setting the From field*/

   /*set the To field*/
   Ptr = ToRecipientsHead;
   strcat(Buf, "To: ");
   while((Ptr = Ptr->Next) != VOS_NULL_PTR)
   {
        if(Ptr->Name == VOS_NULL_PTR)
        {
           strcat(Buf, "<");
           strcat(Buf, Ptr->Addr);
           strcat(Buf, ">");
        }
        else
        {
           FormatStr = SMTPC_CMSetFormat(Ptr->Name, Charset);
           /*modify l59217 check FormatStr whether NULL 2008-06-23*/
           if (FormatStr == VOS_NULL_PTR)
           {
              HI_ERR_Processor("SMTPC_CMCreateHeader: malloc FormatStr error");
              SMTPC_Free(Buf);
              return VOS_NULL_PTR;
           }
           strcat(Buf, FormatStr);
           strcat(Buf, "<");
           strcat(Buf, Ptr->Addr);
           strcat(Buf, ">");
           SMTPC_Free(FormatStr);
        }
        if(Ptr->Next != VOS_NULL_PTR)
        {
          strcat(Buf, ",");
        }
   }
   strcat(Buf, "\r\n");
   /*end of setting the To field*/

  /*set the Cc field if the mail has*/
   Ptr = CcRecipientsHead;
   if(Ptr != VOS_NULL_PTR)
   {
     strcat(Buf, "Cc: ");
     while((Ptr = Ptr->Next) != VOS_NULL_PTR)
     {
          if(Ptr->Name == VOS_NULL_PTR)
          {
             strcat(Buf, "<");
             strcat(Buf, Ptr->Addr);
             strcat(Buf, ">");
          }
          else
          {
             FormatStr = SMTPC_CMSetFormat(Ptr->Name, Charset);
             /*modify l59217 check FormatStr whether NULL 2008-06-23*/
             if (FormatStr == VOS_NULL_PTR)
             {
                HI_ERR_Processor("SMTPC_CMCreateHeader: malloc FormatStr error");
                SMTPC_Free(Buf);
                return VOS_NULL_PTR;
             }
             strcat(Buf, FormatStr);
             strcat(Buf, "<");
             strcat(Buf, Ptr->Addr);
             strcat(Buf, ">");
             SMTPC_Free(FormatStr);
          }
          if(Ptr->Next != VOS_NULL_PTR)
          {
            strcat(Buf, ",");
          }
     }
     strcat(Buf, "\r\n");
   }
   /*end of setting the Cc field*/

   /*set the Bc field if the mail has*/
   Ptr = BcRecipientsHead;
   if(Ptr != VOS_NULL_PTR)
   {
     strcat(Buf, "Bc: ");
     while((Ptr = Ptr->Next) != VOS_NULL_PTR)
     {
          if(Ptr->Name == VOS_NULL_PTR)
          {
             strcat(Buf, "<");
             strcat(Buf, Ptr->Addr);
             strcat(Buf, ">");
          }
          else
          {
             FormatStr = SMTPC_CMSetFormat(Ptr->Name, Charset);
             /*modify l59217 check FormatStr whether NULL 2008-06-23*/
             if (FormatStr == VOS_NULL_PTR)
             {
                HI_ERR_Processor("SMTPC_CMCreateHeader: malloc FormatStr error");
                SMTPC_Free(Buf);
                return VOS_NULL_PTR;
             }
             strcat(Buf, FormatStr);
             strcat(Buf, "<");
             strcat(Buf, Ptr->Addr);
             strcat(Buf, ">");
             SMTPC_Free(FormatStr);
          }
          if(Ptr->Next != VOS_NULL_PTR)
          {
            strcat(Buf, ",");
          }
     }
     strcat(Buf, "\r\n");
   }
   /*end of setting the Cc field*/

   if(Subject != VOS_NULL_PTR)
   {
        strcat(Buf, "Subject: ");
		/*邮件主题使用GB2312编码格式，可支持中文，不需要做转换*/
        //FormatStr = SMTPC_CMSetFormat(Subject, Charset);

        /*modify l59217 check FormatStr whether NULL 2008-06-23*/
        #if 0
        if (FormatStr == VOS_NULL_PTR)
        {
             HI_ERR_Processor("SMTPC_CMCreateHeader: malloc FormatStr error");
             SMTPC_Free(Buf);
             return VOS_NULL_PTR;
        }
        #endif
        strcat(Buf, Subject);
        //SMTPC_Free(FormatStr);
        strcat(Buf, "\r\n");
   }

   strcat(Buf, "Date: ");
   strcat(Buf, DateStr);
   strcat(Buf, "\r\n");
   
   strcat(Buf, OtherField1);
   strcat(Buf, OtherField2);
   strcat(Buf, OtherField3);   
//   strcat(Buf, Charset);
   strcat(Buf, "\r\n");
   strcat(Buf, OtherField4);   
   strcat(Buf, "\r\n");

   return Buf;
}

/***********************************************************************************
* Function:       SMTPC_CMSetFormat
* Description:   encode the input string and set its format
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         Str          the string which will be encoded and formated
*                   Charset   the charset used by the Str
* Output:       none
* Return:       the pointer of the string which has been encoded and formated: success
* Others:       none
***********************************************************************************/
VOS_CHAR *SMTPC_CMSetFormat(VOS_CHAR *Str, const VOS_CHAR *Charset)
{
    VOS_CHAR *QEncodedStr = SMTPC_CODE_QEncode(Str);
    /*modify l59217 check QEncodedStr whether NULL 2008-06-23*/
    if (QEncodedStr == VOS_NULL_PTR)
    {
       HI_ERR_Processor("SMTPC_CMSetFormat: QEncodedStr error");
       return VOS_NULL_PTR;
    }
    VOS_CHAR *Buf = (VOS_CHAR *)SMTPC_Malloc(strlen(QEncodedStr) + strlen(Charset) + 10);
    /*modify l59217 check Buf whether NULL 2008-06-23*/
    if (Buf == VOS_NULL_PTR)
    {
       HI_ERR_Processor("SMTPC_CMSetFormat: malloc Buf error");
       if (QEncodedStr != Str)
       {
         SMTPC_Free(QEncodedStr);
       }
       return VOS_NULL_PTR;
    }
    sprintf(Buf, "=?%s?Q?%s?=", Charset, QEncodedStr);
    /*modify l59217 memory leak 2008-06-23*/
    if (QEncodedStr != Str)
    {
       SMTPC_Free(QEncodedStr);
    }
    return Buf;
}

/***********************************************************************************
* Function:       SMTPC_CMCreateMailBody
* Description:   create the body of mail
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         Text             the text of the mail
*                Attachment   the attachment string which contain one or more file paths of the 
*                                      mail's attachments
*                Charset          the text charset
* Output:       none
* Return:       PTR_SMTPC_CM_MailBody_s: success
*                   VOS_NULL_PTR:                    fail
* Others:       none
***********************************************************************************/
PTR_SMTPC_CM_MailBody_s SMTPC_CMCreateMailBody(VOS_CHAR *Text, VOS_CHAR *Attachment,
VOS_CHAR* Charset)
{
   PTR_SMTPC_CM_MailBody_s head = NULL; 
   PTR_SMTPC_CM_MailBody_s Ptr;
   VOS_UINT32 i =0;
   VOS_CHAR *Buf;
   VOS_CHAR *Headfoot = "\r\n--NextPart--";
   VOS_INT32  lenfoot = strlen(Headfoot);
   head = SMTPC_CMCreateMailBodyLinkList();
   /*modify l59217 check head whether NULL 2008-06-23*/
   if (head == VOS_NULL_PTR)
   {
      HI_ERR_Processor("SMTPC_CMCreateMailBody: malloc head error");
      return VOS_NULL_PTR;
   }
   Ptr = head;

   if(Text != VOS_NULL_PTR)
   {
       Ptr = SMTPC_CMAddMailBodyNode(Charset, Text, Ptr, IsText);
       /*modify l59217 check Ptr whether NULL 2008-06-23*/
       if (Ptr == VOS_NULL_PTR)
       {
         HI_ERR_Processor("SMTPC_CMCreateMailBody: mailbody node error");
         SMTPC_CMFreeMailBody(head);
         return VOS_NULL_PTR;
       }
   }

   if(Attachment == VOS_NULL_PTR)
   {
       goto SMTPC_CMCreateMailBody_out;
   }
   else
   {
      while(*Attachment != '\0')
      {
         for(i = 0; *(Attachment + i) != ';' && *(Attachment + i) != '\0'; i++);
         if(i <= 1)
         {
            HI_ERR_Processor("SMTPC_CMCreateAddresses: the Attachment String is an error");
            /*modify l59217 to avoid mem leak 2008-06-23*/
            SMTPC_CMFreeMailBody(head);
            return VOS_NULL_PTR;
         }
         Buf = (VOS_CHAR *)SMTPC_Malloc(i + 1);
         /*modify l59217 check Buf whether NULL 2008-06-23*/
         if (Buf == VOS_NULL_PTR)
         {
            HI_ERR_Processor("SMTPC_CMCreateAddresses: malloc Buf error");
            SMTPC_CMFreeMailBody(head);
            return VOS_NULL_PTR;
         }
         strncpy(Buf, Attachment, i);
         *(Buf + i) = '\0';
         if((Ptr = SMTPC_CMAddMailBodyNode(Charset, Buf, Ptr, IsAttachment)) == VOS_NULL_PTR)
         {
            HI_ERR_Processor("SMTPC_CMCreateAddresses: add mail body error");
            /*modify l59217 to avoid memleak 2008-06-23*/
            SMTPC_Free(Buf);
            SMTPC_CMFreeMailBody(head);
            return VOS_NULL_PTR;
         }
         SMTPC_Free(Buf);
         Attachment += i;
         if(*Attachment == ';')
         {
            while(isspace(*(++Attachment)));
         }
      }
   }
SMTPC_CMCreateMailBody_out:   
   Ptr->MailBodyFooter = SMTPC_Malloc(lenfoot + 1) ;
   if (Ptr->MailBodyFooter == VOS_NULL_PTR)
   {
      HI_ERR_Processor("SMTPC_CMCreateMailBody: MailBodyFooter error");
      SMTPC_CMFreeMailBody(head);
      return VOS_NULL_PTR;
   }
   memcpy(Ptr->MailBodyFooter,Headfoot,lenfoot);
   *(Ptr->MailBodyFooter + lenfoot) = '\0';
   return head;
}

/***********************************************************************************
* Function:       SMTPC_CMCreateMailBodyLinkList
* Description:   create the header of the MailBody linklist
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         none
* Output:       none
* Return:       PTR_SMTPC_CM_MailBody_s: success
* Others:       none
***********************************************************************************/
PTR_SMTPC_CM_MailBody_s SMTPC_CMCreateMailBodyLinkList()
{
   PTR_SMTPC_CM_MailBody_s head = (PTR_SMTPC_CM_MailBody_s)SMTPC_Malloc(sizeof(SMTPC_CM_MailBody_s));
   /*modify l59217 check head whether NULL 2008-06-23*/
   if (head == VOS_NULL_PTR)
   {
      HI_ERR_Processor("SMTPC_CMCreateMailBodyLinkList: malloe head error");
      return VOS_NULL_PTR;
   }
   head->Attachment = VOS_NULL_PTR;
   head->Text = VOS_NULL_PTR;
   head->MailBodyFooter = VOS_NULL_PTR;
   head->MailBodyHeader = VOS_NULL_PTR;
   head->Next = VOS_NULL_PTR;
   return head;
}


/***********************************************************************************
* Function:       SMTPC_CMAddMailBodyNode
* Description:   add the MailBody node
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:            Charset           the text of mail charset
*                   Content           the text of mail or the file path of the attachment
*                   Ptr                  the pointer of the node which will be added a new node
*                   MailBodyType  the enum of the mail body type, IsText or IsAttachment
* Output:       none
* Return:       PTR_SMTPC_CM_MailBody_s:  success
*                   VOS_NULL_PTR:                     fail
* Others:       none
***********************************************************************************/
PTR_SMTPC_CM_MailBody_s SMTPC_CMAddMailBodyNode(VOS_CHAR *Charset, VOS_CHAR *Content,
                                                PTR_SMTPC_CM_MailBody_s Ptr, 
                                                SMTPC_CM_MailBodyType_E MailBodyType)
{
    VOS_CHAR *tmp;
    VOS_CHAR *TmpContent;
    VOS_CHAR *pFileName = VOS_NULL_PTR;

     if(Ptr->Next != VOS_NULL_PTR)
     {
        return VOS_NULL_PTR;
     }

    TmpContent = (VOS_CHAR *)SMTPC_Malloc(strlen(Content) + 1);
    if ( TmpContent == VOS_NULL_PTR )
    {
        HI_ERR_Processor("no mem");
        return VOS_NULL_PTR;
    }
    strcpy(TmpContent, Content);
    PTR_SMTPC_CM_MailBody_s Node = (PTR_SMTPC_CM_MailBody_s)SMTPC_Malloc(sizeof(SMTPC_CM_MailBody_s));  
    if ( Node == VOS_NULL_PTR )
    {
        SMTPC_Free(TmpContent);
        HI_ERR_Processor("no mem");
        return VOS_NULL_PTR;        
    }
    pFileName = strrchr(TmpContent, '/');
    if ( pFileName == VOS_NULL_PTR )
    {
        pFileName = TmpContent;
    }
    else
    {
        pFileName++;
    }
    switch(MailBodyType)
    {
       case IsText: 
       	      Node->Text = TmpContent;
       	      tmp = (VOS_CHAR *)SMTPC_Malloc(120);
              /*modify l59217 check tmp whether NULL 2008-06-23*/
              if (tmp == VOS_NULL_PTR)
              {
                 HI_ERR_Processor("SMTPC_CMAddMailBodyNode: malloc tmp error");
                 SMTPC_Free(TmpContent);
                 SMTPC_Free((VOS_CHAR*)Node);
                 return VOS_NULL_PTR;
              }
              /*modify l59217 2008-06-20 change strcpy to sprintf, add charset*/
       	      sprintf(tmp, "\r\n\r\n--NextPart\r\nContent-Type: text/plain;\r\n	charset=\"%s\"\r\nContent-Transfer-Encoding: base64\r\n\r\n", Charset);
                    Node->MailBodyHeader = tmp;
                    break;
       case IsAttachment: 
       	      Node->Attachment = TmpContent;
              tmp = (VOS_CHAR *)SMTPC_Malloc(2 * strlen(TmpContent) + 200);
              /*modify l59217 check tmp whether NULL 2008-06-23*/
              if (tmp == VOS_NULL_PTR)
              {
                 HI_ERR_Processor("SMTPC_CMAddMailBodyNode: malloc tmp error");
                 SMTPC_Free(TmpContent);
                 SMTPC_Free((VOS_CHAR*)Node);
                 return VOS_NULL_PTR;
              }
              sprintf(tmp, "\r\n\r\n--NextPart\r\nContent-Type: application/octet-stream;\r\n	name=\"%s\"\r\nContent-Transfer-Encoding: base64\r\nContent-Description: attachment;\r\n	filename=\"%s\"\r\n\r\n", 
                pFileName, pFileName);
              Node->MailBodyHeader = tmp;
              break;
    }
    Node->Next = VOS_NULL_PTR;
    Ptr->Next = Node;
    
    return Node;
}

VOS_VOID SMTPC_CMFreeMail(PTR_SMTPC_CM_Mail_s Mail)
{
    if(Mail == VOS_NULL_PTR)
       return;

    if(Mail->Sender != VOS_NULL_PTR)
       SMTPC_CMFreeAddress(Mail->Sender);

    if(Mail->ToRecipientsHead != VOS_NULL_PTR)
       SMTPC_CMFreeAddress(Mail->ToRecipientsHead);

    if(Mail->CcRecipientsHead != VOS_NULL_PTR)
       SMTPC_CMFreeAddress(Mail->CcRecipientsHead);

    if(Mail->BcRecipientsHead != VOS_NULL_PTR)
       SMTPC_CMFreeAddress(Mail->BcRecipientsHead);

    if(Mail->Header != VOS_NULL_PTR)
       SMTPC_Free(Mail->Header);

    if(Mail->MailBodyHead != VOS_NULL_PTR)
       SMTPC_CMFreeMailBody(Mail->MailBodyHead);
    
    SMTPC_Free((VOS_CHAR *)Mail);
}

VOS_VOID SMTPC_CMFreeAddress(PTR_SMTPC_CM_Address_s Ptr)
{
    if(Ptr->Next != VOS_NULL_PTR)
    {
        SMTPC_CMFreeAddress(Ptr->Next);
    }
    
    SMTPC_Free(Ptr->Addr);

    SMTPC_Free(Ptr->Name);

    SMTPC_Free((VOS_CHAR *)Ptr);
}

VOS_VOID SMTPC_CMFreeMailBody(PTR_SMTPC_CM_MailBody_s Ptr)
{
    if(Ptr->Next != VOS_NULL_PTR)
    {
        SMTPC_CMFreeMailBody(Ptr->Next);
    }

    if(Ptr->Text != VOS_NULL_PTR)
    	SMTPC_Free((VOS_CHAR *)Ptr->Text);

    if(Ptr->Attachment != VOS_NULL_PTR)
    	SMTPC_Free((VOS_CHAR *)Ptr->Attachment);
	
    if(Ptr->MailBodyHeader != VOS_NULL_PTR)
    	SMTPC_Free((VOS_CHAR *)Ptr->MailBodyHeader);

    if(Ptr->MailBodyFooter != VOS_NULL_PTR)
    	SMTPC_Free((VOS_CHAR *)Ptr->MailBodyFooter);

    SMTPC_Free((VOS_CHAR *)Ptr);
}
