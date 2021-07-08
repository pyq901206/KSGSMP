#ifndef __JSONSWITCH_H__
#define __JSONSWITCH_H__

#include "cjson.h"
#include "adpapi.h"

typedef struct _JsonMsg_T{
	char cmd[32];
	char mothod[32];
	char ssionID[32];
	char msgID[32];
	char deviceId[DF_UUID_LEN];
	char countrycode;
	char srctopic[128];
	char desttopic[128];
	ENUM_TERMINAL_TYPE_E terminaltype;
	cJSON *bodyData;
}JsonMsg_T;


int JsonSwitch(char *requst, char *response, int buffLen);
int JsonApiInit();
int out(cJSON *item);

#endif
