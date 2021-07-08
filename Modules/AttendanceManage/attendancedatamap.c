#include <stdio.h>
#include "jsonmap.h"
#include "base64.h"
#include "attendancemanage.h"
#include "attendancedatamap.h"
#include "dataanalysisout.h"

//Å×ËÍ¿¼ÇÚ
int JsonMapPushAttendance(char *msg, AttendanceAtt_T *info)
{
	if(NULL == msg || NULL == info)
	{
		DF_DEBUG("JsonMapGetAttendanceList param error\n");
		return -1;
	}
	
	char time[128] = {0};
	cJSON *root = cJSON_CreateObject();
	if (NULL != root)
	{
		cJSON_AddStringToObject(root, "cmd", "pushattendance");
		cJSON_AddStringToObject(root, "method", "request");
	    cJSON_AddStringToObject(root, "msgid", info->snapToken);

		cJSON *jsonData = cJSON_CreateObject();

		cJSON_AddStringToObject(jsonData, "name",info->name);
		cJSON_AddStringToObject(jsonData, "usrid",info->usrid);
		cJSON_AddStringToObject(jsonData, "phonenum",info->phonenum);
		sprintf(time, "%04d-%02d-%02d %02d:%02d", info->snapTime.year, info->snapTime.month, \
			info->snapTime.day, info->snapTime.hour,info->snapTime.min);
		cJSON_AddStringToObject(jsonData, "time", time);
		cJSON_AddStringToObject(jsonData, "plateimagepath",info->plateImagePath);
		cJSON_AddStringToObject(jsonData, "snapimagepath",info->snapImagePath);
		cJSON_AddStringToObject(jsonData, "snaptoken",info->snapToken);
		cJSON_AddItemToObject(root, "data", jsonData);
	       
		char *out = cJSON_PrintUnformatted(root);
		memcpy(msg, out, strlen(out));
		cJSON_Delete(root);
		free(out);
	}
	return KEY_TRUE;
}

//Å×ËÍ¹Ë¿Í
int JsonMapPushVisitors(char *msg, VIPCustomer_T *info)
{
	if(NULL == msg || NULL == info)
	{
		DF_DEBUG("JsonMapGetAttendanceList param error\n");
		return -1;
	}
	
	char time[128] = {0};
	cJSON *root = cJSON_CreateObject();
	if (NULL != root)
	{
		cJSON_AddStringToObject(root, "cmd", "pushvisitors");
		cJSON_AddStringToObject(root, "method", "request");
		cJSON_AddStringToObject(root, "msgid", info->snapToken);

		cJSON *jsonData = cJSON_CreateObject();

		cJSON_AddStringToObject(jsonData, "name",info->name);
		cJSON_AddNumberToObject(jsonData,"gradescore",GetGradeLevel(info->GradeScore));
		cJSON_AddNumberToObject(jsonData,"storefrequency",info->StoreFrequency);
		sprintf(time, "%04d-%02d-%02d %02d:%02d", info->snapTime.year, info->snapTime.month, \
			info->snapTime.day, info->snapTime.hour,info->snapTime.min);
		cJSON_AddStringToObject(jsonData, "time", time);
		cJSON_AddStringToObject(jsonData, "plateimagepath",info->plateImagePath);
		cJSON_AddStringToObject(jsonData, "snapimagepath",info->snapImagePath);
		cJSON_AddStringToObject(jsonData, "snaptoken",info->snapToken);
		cJSON_AddItemToObject(root, "data", jsonData);
		   
		char *out = cJSON_PrintUnformatted(root);
		memcpy(msg, out, strlen(out));
		cJSON_Delete(root);
		free(out);
	}
	return KEY_TRUE;
}


//Å×ËÍ¿ÍÁ÷Á¿
int JsonMapPushVolumeOfCommuters(char *msg, VolumeOfCommuters_T *info)
{
	if(NULL == msg || NULL == info)
	{
		DF_DEBUG("JsonMapGetAttendanceList param error\n");
		return -1;
	}
	
	char time[128] = {0};
	cJSON *root = cJSON_CreateObject();
	if (NULL != root)
	{
		cJSON_AddStringToObject(root, "cmd", "pushvolumeofcommuters");
		cJSON_AddStringToObject(root, "method", "request");

		cJSON *jsonData = cJSON_CreateObject();
		cJSON_AddNumberToObject(jsonData,"enternum",info->LeaveAndEnterAttr.dwEnterNum);
		cJSON_AddItemToObject(root, "data", jsonData);
		   
		char *out = cJSON_PrintUnformatted(root);
		memcpy(msg, out, strlen(out));
		cJSON_Delete(root);
		free(out);
	}
	return KEY_TRUE;
}



