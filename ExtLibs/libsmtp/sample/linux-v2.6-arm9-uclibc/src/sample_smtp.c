/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_smtp.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/02/27
  Description   : 
  History       :
  1.Date        : 2008/02/27
    Author      : qushen
    Modification: Created file

******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include "smtpc.h"

int main(int argc,char *argv[])
{
    int ret;
    SMTPC_INF_MailContent_s struMc;
    SMTPC_CONF_Configure_s  struConf;
    PTR_SMTPC_INF_MailContent_s mc  = &struMc;
    PTR_SMTPC_CONF_Configure_s conf = &struConf;
    int i;

    memset(mc,0,sizeof(mc));
    memset(conf,0,sizeof(conf));

    mc->From="qushen@hi3510";
    mc->To="qushen@hi3510";
    mc->Text="Like this ...";
    mc->Subject="Test rsp timout!";
    mc->Attachment = "smb.txt";
    //mc->Attachment = "../src/sample_smtp.c; smb.txt";
        
    conf->Server="10.85.77.34";
    conf->Port = 2525;
    conf->LoginType=AuthLogin;
    conf->SslType = SSL_TYPE_NOSSL;
    conf->User="qushen@hi3510";
    conf->Passwd="xxx";
    conf->RespTimeout = 0;
    conf->ConnTimeout = 3;

    for ( i = 0 ; i < 3 ; i++ )
    {
        printf("=====> send email times %d\n", i);
        ret = HI_SMTPC_SendMail(mc, conf);
        usleep(10000);
    }
    return ret;
}


