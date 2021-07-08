#include "common.h"
#include "rtmpjsonmap.h"

cJSON* JsonMapRtmpParamMake(RtmpConfParm_T *info)
{
	if(NULL == info){
		return NULL;
	}
	
	cJSON *cJrtmp = cJSON_CreateObject();
	cJSON_AddNumberToObject(cJrtmp, "abrenable", info->abrenable);
	cJSON_AddNumberToObject(cJrtmp,"port",info->servport);
	cJSON_AddStringToObject(cJrtmp,"servaddr",info->servaddr);
	cJSON_AddStringToObject(cJrtmp,"streamurl",info->streamurl);
	cJSON_AddStringToObject(cJrtmp,"username",info->name);
	cJSON_AddStringToObject(cJrtmp,"password",info->passwd);
	return cJrtmp;
}

int JsonMapRtmpParamPares(cJSON *conf, RtmpConfParm_T *info)
{
	int iret = -1;
	
	if(conf == NULL|| info == NULL)
	{
		return -1;
	}
	
	info->abrenable = cJSON_GetObjectItemValueInt(conf, "abrenable");
	info->servport  = cJSON_GetObjectItemValueInt(conf, "port");
	cJSON_GetObjectItemValueString(conf, "servaddr", info->servaddr, 128);
	cJSON_GetObjectItemValueString(conf, "streamurl",info->streamurl, 128);
	cJSON_GetObjectItemValueString(conf, "username", info->name, 32);
	cJSON_GetObjectItemValueString(conf, "password", info->passwd, 32);
	return 0;
}

