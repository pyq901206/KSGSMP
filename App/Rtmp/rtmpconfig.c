#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rtmpconfig.h"
#include "common.h"
#include "cjson.h"
#include "rtmpjsonmap.h"
#include "configmanage.h"

static int RtmpConfigDefault()
{
	RtmpConfParm_T info = {0};

	info.abrenable = 0;
	info.servport  = 1935;
	snprintf(info.servaddr,128,"113.247.222.69");
	snprintf(info.streamurl,128,"/live/");
	snprintf(info.name,32,"admin");
	snprintf(info.passwd,32,"admin");
	RtmpParamSetToConfig(&info);
	return KEY_TRUE;	
}

int RtmpParamSetToConfig(RtmpConfParm_T *info)
{
	cJSON* confonvif = JsonMapRtmpParamMake(info);
	if(NULL == confonvif){
		return KEY_FALSE;
	}
	 
	return ConfigManageSet(confonvif, D_RTMPCONF_STR);
}

int RtmpParamGetFromConfig(RtmpConfParm_T *info)
{
    int s32Ret=KEY_FALSE;
    cJSON *cjtmp=NULL;
    cjtmp=ConfigManageGet(D_RTMPCONF_STR);
    if(NULL==cjtmp)
    {
        RtmpConfigDefault();
        cjtmp=ConfigManageGet(D_RTMPCONF_STR);
	}
    s32Ret=JsonMapRtmpParamPares(cjtmp, info);
	cJSON_Delete(cjtmp);cjtmp=NULL;
	return s32Ret;
}

