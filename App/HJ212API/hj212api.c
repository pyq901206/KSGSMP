#include "common.h"
#include "hj212api.h"
#include "tsocket.h"
#include "cjson.h"
#include "mqttapi.h"

typedef struct _HJ212Api_T{
	NetWorkSerHandle_T netSer; 
}HJ212Api_T; 
static HJ212Api_T g_handle;
static HJ212Api_T *GeHj212Handle()
{
	return &g_handle;
}
#define Object_Number 5
static  char object_Name[Object_Number][24]={"a01001-Rtd", "a01002-Rtd", "a01006-Rtd", "a01007-Rtd", "a01008-Rtd"};
static  char object_Date[Object_Number][24]={"26.2℃", "39.9%RH", "100.7KPa", "0.1m/s", "东南风"};


static 	int JsonMapPushHJ212RealDataMake(char *msg)
{
	if(NULL == msg)
	{
		return KEY_FALSE;
	}
	
	char *out = NULL;
	int item = 0;
	char time[64] = {0};
	cJSON *Realdata = cJSON_CreateObject();
	cJSON *cJPresetArry = cJSON_CreateObject();	
	cJSON_AddStringToObject(Realdata, "cmd", "pushhj212realdata");	
	cJSON_AddItemToObject(Realdata, "data", cJPresetArry);	
	
	cJSON_AddStringToObject(cJPresetArry, "temperature", object_Date[0]);
	cJSON_AddStringToObject(cJPresetArry, "humidity", object_Date[1]);
	cJSON_AddStringToObject(cJPresetArry, "Atmospheric", object_Date[2]);
	cJSON_AddStringToObject(cJPresetArry, "rate", object_Date[3]);
	cJSON_AddStringToObject(cJPresetArry, "direction", object_Date[4]);
	
	out = cJSON_PrintUnformatted(Realdata);
	memcpy(msg, out, strlen(out));
	free(out);
	return KEY_TRUE;
}


static int ConfirmWindDirection(int value, int num)
{
	float rate = 0;

	if(value >= 348 && value < 360 && value > 0 && value < 11)
	{		
		sprintf(object_Date[num], "%s", "北风");
	}
	if(value >=11 && value < 78)
	{		
		sprintf(object_Date[num], "%s", "东北风");
	}
	if(value >= 78 && value < 101)
	{		
		sprintf(object_Date[num], "%s", "东风");
	}
	if(value >= 101 && value  < 169)
	{		
		sprintf(object_Date[num], "%s", "东南风");
	}
	if(value >= 169 && value < 191)
	{		
		sprintf(object_Date[num], "%s", "南风");
	}
	if(value >= 191 && value < 259)
	{		
		sprintf(object_Date[num], "%s", "西南风");
	}
	if(value >= 259 && value < 281)
	{		
		sprintf(object_Date[num], "%s", "西风");
	}
	if(value >= 281 && value < 348)
	{		
		sprintf(object_Date[num], "%s", "西北风");
	}
	rate = atof(object_Date[3]);
	
	DF_DEBUG("rate = %f", rate);
	if(rate <= 0.2)
	{
		sprintf(object_Date[num], "%s", "静风");
	}	
	return KEY_TRUE;
}

static int DealDate(char *string, int num)
{
	int number = -1;
	char value[24] = {0};
	if(num == 0)
	{
		sprintf(object_Date[num], "%s℃", string);
	}
	if(num == 1)
	{
		sprintf(object_Date[num], "%s%%RH", string);
	}
	if(num == 2)
	{
		sprintf(object_Date[num], "%sKPa", string);
	}
	if(num == 3)
	{
		sprintf(object_Date[num], "%sm/s", string);
	}
	if(num == 4)
	{
		sprintf(value, "%s", string);
		number = atoi(value);
		ConfirmWindDirection(number, num);
	}
	return KEY_TRUE;
}

static int HJ212RealDataPars(char *Msg)
{
	int i = -1;
	int len = -1;
	char *p = NULL;
	char *q = NULL;
	char value[24] = {0};
	char sendMsg[256] = {0};
	for(i = 0; i < Object_Number; i++)
	{
		if(strstr(Msg, object_Name[i]) != NULL)
		{
			if(strlen(object_Name[i]) == 0)
			{
				break;
			}
			p = strstr(Msg, object_Name[i]);
			p = p + strlen(object_Name[i]) + 1;
			char *q = strstr(p, ",");
			len = strlen(p) - strlen(q);
			snprintf(value, len+1, "%s", p);
			DealDate(value,i);
		}
		
	}

	for(i = 0; i < Object_Number; i++)
	{
		DF_DEBUG("object_Date[i] = [%s]", object_Date[i]);

	}
	JsonMapPushHJ212RealDataMake(sendMsg);
	
	pushMqttMsg(MAIN_SERVER_PUSH_TOPIC, sendMsg, 0);	

}

static int HJ212Close(int sockfd)
{
	if (0 < sockfd){
		close(sockfd);
	}
	return KEY_TRUE;
}

static int HJ212Checked(NetCliInfo_T *cli)
{
	return KEY_TRUE;
}

static int HJ212Requst(NetCliInfo_T *cli)
{
	if(	0 == strlen(cli->fromIP) || 
		0 >= cli->recvLen || 
		0 >= cli->fromPort || 
		0 >= cli->recvSocket){
		DF_DEBUG("%s %d %d %d\r\n",cli->fromIP,cli->recvLen,cli->fromPort,cli->recvSocket);
		DF_DEBUG("HTTP API Requst param error\r\n");
		return KEY_FALSE;
	}
	//DF_DEBUG("==>%s,==>%d",cli->recvmsg,cli->recvLen);
	HJ212RealDataPars(cli->recvmsg);
	return KEY_TRUE;
}

int HJ212ApiStart(int port)
{
 	NetWorkSerHandle_T *netHandle = &(GeHj212Handle()->netSer);
	netHandle->readcb =  HJ212Requst;
	netHandle->checkendcb =  HJ212Checked;
	netHandle->closecb =  HJ212Close;
	netHandle->serverport = port;
	NetCreateTcpSer(netHandle);
	return KEY_TRUE;
}

int HJ212ApiStop(void)
{
	return KEY_TRUE;
}
