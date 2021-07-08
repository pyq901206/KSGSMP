/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年03月07日
|  说明:Emial加入MSG队列 队列对应回调和MSGID
******************************************************************/
#include <stdlib.h>
#include "emailmanage.h"
#include "jsonmap.h"
#include "handlemanage.h"
#include "configmanage.h"
#include "smtpc.h"
#include "emailconfig.h"

typedef struct _EmailMsgManage_T
{
	HandleManage_T headlist;	
	int msgnum;
	
	pthread_mutex_t emailmutex;
}EmailMsgManage_T;
static int EmailModEnable=0;
static SendEmailResultCbFun MsgResult=NULL;

static EmailMsgManage_T EmailMsgMng={0};

static int SetEmailParam(EmailParam_T *param)//设置邮件参数
{
	return EmailParamSet(param);
}

static int GetEmailParam(EmailParam_T *param)//获取邮件参数
{
	return EmailParamGet(param);
}

static int SendEmailSync(EmailMsgInfo_T*EmailMsg)
{
	SMTPC_INF_MailContent_s mc;
	int ret=0;
	SMTPC_CONF_Configure_s config;
	EmailParam_T param;
	GetEmailParam(&param);
	memset(&mc,0,sizeof(SMTPC_INF_MailContent_s));
	memset(&config,0,sizeof(SMTPC_CONF_Configure_s));
	mc.From=param.sender;
	//memcpy(mc.From,"18890202503@163.com",64);
	mc.To=param.receiver;
	//memcpy(mc.To,"18890202503@163.com",64);
	printf("[%s,%d]from=%s, to=%s\n",__FUNCTION__,__LINE__,mc.From,mc.To);
	mc.Subject=EmailMsg->title;
	mc.Attachment=EmailMsg->attchment;
	mc.Text=EmailMsg->text;
	   printf("[%s,%d]Subject=%s, text=%s,attachmen=%s\n",__FUNCTION__,__LINE__,mc.Subject,mc.Text,mc.Attachment);
	config.ConnTimeout=0;
	config.LoginType=1;
	config.Passwd=param.passWord;
	//memcpy(config.Passwd,"tiaotiao2017",64);
	config.Port=param.servicePort;
	config.Port=465;
	config.RespTimeout=30;
	config.Server=param.serverAddress;
	//memcpy(config.Server,"smtp.163.com",64);
	printf("[%s,%d]Server=%s, Passwd=%s\n",__FUNCTION__,__LINE__,config.Server,config.Passwd);
	switch(param.EncodeType)
	{
		case EmailEncryption_SSL:
			//break;
		case EmailEncryption_TLS:
			config.SslType=SSL_TYPE_STARTTLS;
			break;
		case EmailEncryption_Last:
			break;
		case EmailEncryption_NULL:
			config.SslType=SSL_TYPE_NOSSL;
			break;
	}
	config.SslType=SSL_TYPE_STARTTLS;
	if(config.SslType==SSL_TYPE_NOSSL)
	{
		config.Port=25;
	}

	config.User=param.userName;
     	printf("username=%s\n",config.User);
	//发送数据
	ret=HI_SMTPC_SendMail(&mc,&config);
	if(ret)
	{
		//printf();
		return KEY_FALSE;
	}
	free(EmailMsg->attchment);
	free(EmailMsg->text);
	EmailMsg->attchment=NULL;
	EmailMsg->text=NULL;
	return KEY_TRUE;
}
#if 0
static int SendEmailSync(char *data,int len)//同步发送邮件
{
	PTR_SMTPC_INF_MailContent_s mc;
	
	PTR_SMTPC_CONF_Configure_s config;
	EmailParam_T param;
	GetEmailParam(&param);
	
	mc.From=param.sender;
	mc.To=param.receiver;
	mc.Subject=param.title
	mc.Charset=param.
	mc.Attachment=param.
	mc.Text=param.
	mc.Bc=
	mc.Cc=

	config.ConnTimeout=0;
	config.LoginType=1;
	config.Passwd=param.passWord;
	config.Port=param.servicePort;
	config.RespTimeout=30;
	config.Server=param.serverAddress;
	switch(param.EncodeType)
	{
		case EmailEncryption_SSL:
			//break;
		case EmailEncryption_TLS:
			config.SslType=SSL_TYPE_STARTTLS;
			break;
		case EmailEncryption_Last:
			break;
		case EmailEncryption_NULL:
			config.SslType=SSL_TYPE_NOSSL;
			break;
	}
	//=param.EncodeType;
	config.User=param.userName;
	//发送数据
	HI_SMTPC_SendMail(mc,config);
	return KEY_TRUE;
}
#endif
static int SendEmail(EmailMsgInfo_T*EmailMsg)
{
	EmailMsgInfo_T*pushmsg=NULL;
	int ret=0;
	MUTEX_LOCK(&(EmailMsgMng.emailmutex));
	pushmsg=(EmailMsgInfo_T*)malloc(sizeof(EmailMsgInfo_T));
	if(pushmsg==NULL)
	{
		MUTEX_UNLOCK(&(EmailMsgMng.emailmutex));
		return -1;
	}
	memcpy(pushmsg,EmailMsg,sizeof(EmailMsgInfo_T));
	ret=HandleManageAddHandle(&(EmailMsgMng.headlist),pushmsg);
	if(ret)
	{
		printf("HandlManageAddHandle error\n");
		MUTEX_UNLOCK(&(EmailMsgMng.emailmutex));
		return -1;
	}
	EmailMsgMng.msgnum++;
	MUTEX_UNLOCK(&(EmailMsgMng.emailmutex));
	return 0;
}

static int EmailMsgOut(EmailMsgInfo_T*EmailMsg)
{
	EmailMsgInfo_T*pushmsg=NULL;
	MUTEX_LOCK(&(EmailMsgMng.emailmutex));
	if(EmailMsgMng.msgnum==0)
	{
		MUTEX_UNLOCK(&(EmailMsgMng.emailmutex));
		return -1;
	}
	pushmsg=HandleManageGetNextHandle(&(EmailMsgMng.headlist));
	if(pushmsg==NULL)
	{
		MUTEX_UNLOCK(&(EmailMsgMng.emailmutex));
		return -1;
	}
	memcpy(EmailMsg,pushmsg,sizeof(EmailMsgInfo_T));
	MUTEX_UNLOCK(&(EmailMsgMng.emailmutex));
	return 0;
}
static int EmailMsgDel(EmailMsgInfo_T*EmailMsg)
{
	int ret=0;
	MUTEX_LOCK(&(EmailMsgMng.emailmutex));
	if(NULL!=EmailMsg)
	{
		return -1;
	}
	if(EmailMsgMng.msgnum==0)
	{
		return 0;
	}
	//free(EmailMsg->text);
	//free(EmailMsg->attchment);
	ret=HandleManageDelHandle(&(EmailMsgMng.headlist),EmailMsg);
	free(EmailMsg);
	EmailMsgMng.msgnum--;
	MUTEX_LOCK(&(EmailMsgMng.emailmutex));
	return 0;
}
static int EmailMsgClearBuff()
{
	EmailMsgInfo_T*pushmsg=NULL;
	int i=0;
	MUTEX_LOCK(&(EmailMsgMng.emailmutex));
	for(i=EmailMsgMng.msgnum;i>0;i--)
	{
		pushmsg=HandleManageGetNextHandle(&(EmailMsgMng.headlist));
		//先清空内容 buff
		free(pushmsg->attchment);
		free(pushmsg->text);
		HandleManageDelHandle(&(EmailMsgMng.headlist),pushmsg);
		free(pushmsg);
		EmailMsgMng.msgnum--;
	}
	MUTEX_LOCK(&(EmailMsgMng.emailmutex));
	return 0;
}
int GetEmailMsgAddr(EmailMsgPartSize_T*EmailMsgSize)
{
	
	 EmailMsgSize->textAddr=(char*)malloc(EmailMsgSize->textLen);
	 if(EmailMsgSize->textAddr==NULL)
	 {
		return -1;
	 }
	 memset( EmailMsgSize->textAddr,0,EmailMsgSize->textLen);
	 EmailMsgSize->attchmentAddr=(char*)malloc(EmailMsgSize->attchmentLen);
	 if(EmailMsgSize->textAddr==NULL)
	 {
	 	free( EmailMsgSize->textAddr);
		return -1;
	 }
	memset(EmailMsgSize->attchmentAddr,0,EmailMsgSize->attchmentLen);
	return 0;
}

#if 0
static int SendEmail(char *data,int len)//异步发送邮件
{
	//buffer缓存
	//HandleManageAddHandle(HandleManage_T * head,void * handle)
	
	return KEY_TRUE;
}
#endif

int EmailManageRegist(EmailManageRegistHandle_T *handle)
{
	handle->pSendEmail = SendEmail;
	handle->pSendEmailSync = SendEmailSync;
	MsgResult=handle->sendEmailResult;
	return KEY_TRUE;
}

int EmailManageUnRegist(EmailManageRegistHandle_T *handle)
{
	handle->pSendEmail=NULL;
	handle->pSendEmailSync=NULL; 
	MsgResult=NULL;
	return KEY_TRUE;
}
static void* EmailSendProc(void*arg)
{
	EmailMsgInfo_T EmailMsg;
	int ret=0;
	//标志
	while(EmailModEnable)
	{
		//链表
		memset(&EmailMsg,0,sizeof(EmailMsgInfo_T));
		ret=EmailMsgOut(&EmailMsg);
		if(ret)
		{
			usleep(1000*1000*1);
			continue;
		}
		ret=SendEmailSync(&EmailMsg);
		if(MsgResult!=NULL)
		{
			MsgResult(ret);
		}
		EmailMsgDel(&EmailMsg);
		EmailMsg.text=NULL;
		EmailMsg.text=NULL;
		usleep(1000*1000*1);
	}
	return;
}

//做异步的邮件发送线程
static int EmailMsgSendThread()
{
	pthread_t EmailThread;
	pthread_attr_t EmailThreadAttr;
	int ret=0;
	EmailModEnable=1;
	pthread_attr_init(&EmailThreadAttr);
	pthread_attr_setdetachstate(&EmailThreadAttr,PTHREAD_CREATE_DETACHED);
	ret=pthread_create(&EmailThread,&EmailThreadAttr,EmailSendProc,NULL);
	pthread_attr_destroy(&EmailThreadAttr);
	if(ret)
	{
		EmailModEnable=0;
		
		return KEY_FALSE;
	}
	return KEY_TRUE;


}

int EmailManageInit(EmailManageHandle_T *handle)
{
	EmailParam_T emailparam = {0};
	int ret=0;
	GetEmailParam(&emailparam);
	handle->pGetEmailParam = GetEmailParam;
	handle->pSetEmailParam = SetEmailParam;

	//链表
	HandleManageInitHead(&(EmailMsgMng.headlist));
	MUTEX_INIT(&(EmailMsgMng.emailmutex));
	//线程等待
	ret=EmailMsgSendThread();
	if(ret)
	{
		return KEY_FALSE;
	}
	
	DF_DEBUG("EmailManageInit Success");
	return KEY_TRUE;
}

int EmailManageUnInit(EmailManageHandle_T *handle)
{
	EmailModEnable=0;
	//清空buff如果没有获取的消息没有取完就全部清除
	EmailMsgClearBuff();
	return KEY_TRUE;
}




