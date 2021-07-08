/*******************************************************************
|  �汾: 1.0 
|  ����: ��˼��
|  ����: 2018��05��02��
|  ˵��: EMAIL �����ñ�
|
******************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jsonmap.h"
#include "emailconfig.h"

int EmailParamDefault()
{
	EmailParam_T info = {0};
	
	info.enabled = 0;	
	info.EncodeType = 1;
	info.servicePort = 465;
	sprintf(info.serverAddress,"%s","smtp.gmail.com");
	sprintf(info.userName,"%s","user@gmail.com");
	sprintf(info.passWord,"%s","123456");
	sprintf(info.sender,"%s","user@gmail.com");
	sprintf(info.receiver,"%s","to@gmail.com");
	sprintf(info.title,"%s","Alarm");
	EmailParamSet(&info);
	return KEY_TRUE;
}

int EmailParamSet(EmailParam_T *info)
{
	cJSON *confemail = JsonMapEmailParamMake(info);
	if(NULL == confemail){
		return KEY_FALSE;
	}
	return ConfigManageSet(confemail,D_CONFEMAILPARAM_STR);
}

int EmailParamGet(EmailParam_T *info)
{
	if(KEY_FALSE == JsonMapEmailParamPares(ConfigManageGet(D_CONFEMAILPARAM_STR),info))
	{
		EmailParamDefault();
		return JsonMapEmailParamPares(ConfigManageGet(D_CONFEMAILPARAM_STR),info);
	}
	return KEY_TRUE;
}



