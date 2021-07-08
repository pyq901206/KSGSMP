#include "common.h"
#include "mosquitto.h"
#include "cictapi.h"
#include <errno.h>
#include "adpapi.h"
#include "cjson.h"
#include "mqttapi.h"

#define MQTT_QOS 0
#define Volmeter_Number 4

typedef struct GZ_Volmeter_Parameter_T
{
		int  deviceID;
		char  dataU[12];
		char  dataI[12];
		char  dataP[12];
}Volmeter_Parameter_T;

static  Volmeter_Parameter_T electpar[Volmeter_Number]={0};

static 	int JsonMapPushCICRealDataMake(char *msg)
{
	if(NULL == msg)
	{
		return KEY_FALSE;
	}
	
	char *out = NULL;
	int item = 0;
	char time[64] = {0};
	cJSON *Realdata = cJSON_CreateObject();
	cJSON *cJPresetArry = cJSON_CreateArray();	
	cJSON_AddStringToObject(Realdata, "cmd", "pushcicrealdata");	
	cJSON_AddItemToObject(Realdata, "data", cJPresetArry);	
	for (item = 0; item < Volmeter_Number; item++)
	{
	
		cJSON *cJPresetItem = cJSON_CreateObject();
		
		cJSON_AddItemToObject(cJPresetArry,"Realdata",cJPresetItem);
		cJSON_AddNumberToObject(cJPresetItem, "deviceID", electpar[item].deviceID + 1);
		cJSON_AddStringToObject(cJPresetItem, "U", electpar[item].dataU);
		cJSON_AddStringToObject(cJPresetItem, "I", electpar[item].dataI);
		cJSON_AddStringToObject(cJPresetItem, "P", electpar[item].dataP);

	}
	
	out = cJSON_PrintUnformatted(Realdata);
	memcpy(msg, out, strlen(out));
	free(out);
	return KEY_TRUE;
}

static int ParesCICRealData(struct mosquitto *mosq, char *msgBuf)
{
	char *p = NULL;
	char *q = NULL;
	char Buff[256] = {0};
	char time[24] = {0};
	char deviceID[24] = {0};
	char sendMsg[256] = {0};
	int len = 0;
	int i = 0;
	int a = 0;
	int Tag = 0;
	snprintf(Buff, 256, "%s,", msgBuf);
	
	p = strstr(Buff,",");
	p = p+1;
	q = strstr(p,",");	
	len = strlen(p) - strlen(q);
	snprintf(time, len+1, "%s", p);
	p = q;
	while(strstr(p, ",") != NULL)
	{	
		p = p+1; 
		q = strstr(p,",");
		
		if(q == NULL)
		{
			break;
		}
		len = strlen(p) - strlen(q);
		snprintf(time, len+1, "%s", p);
		Tag = atoi(time);
		a = atoi(time)%3;
		if(Tag > 0 && Tag < 4)
		{
			i = 0;
			electpar[i].deviceID = i;
		}
		if(Tag > 3 && Tag < 7)
		{
			i = 1;
			electpar[i].deviceID = i;
		}
		if(Tag > 6 && Tag < 10)
		{
			i = 2;
			electpar[i].deviceID = i;
		}
		if(Tag > 9 && Tag < 11)
		{
			i = 3;
			electpar[i].deviceID = i;
		}

		p = q + 1; 
		q = strstr(p,",");	
		len = strlen(p) - strlen(q);
		if(q == NULL)
		{
			break;
		}
				
		if(a == 0)
		{
			snprintf(electpar[i].dataP, len+1, "%s", p);
 		}
		if(a == 1)
		{
			snprintf(electpar[i].dataU, len+1, "%s", p);
		}

		if(a == 2)
		{
			snprintf(electpar[i].dataI, len+1, "%s", p);
		}
		
		p = q;
	
	}

	JsonMapPushCICRealDataMake(sendMsg);

	mosquitto_publish(mosq, NULL, MAIN_SERVER_PUSH_TOPIC, strlen(sendMsg), sendMsg, MQTT_QOS, false);	

	return KEY_TRUE;	
}

void CICTMQ_Connect_Cb(struct mosquitto *mosq, void *userdata,  int result)
{
	int mqtt_mid = 0;
	mosquitto_subscribe(mosq, &mqtt_mid, "GS12/Data", MQTT_QOS);

	return;
}

void CICTMQ_Message_Cb(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	DF_DEBUG("topic:%s  payload:%s", message->topic, (char *)message->payload);
	int ret = -1;
	if(0 == strcmp(message->topic,"GS12/Data"))
	{
		ParesCICRealData(mosq, (char *)message->payload);
	}
	return;
}

void CICTMQ_Subscribe_Cb(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	return ;
}

void *CICTApiTask(void *mqttServerIp)
{
	prctl(PR_SET_NAME, (unsigned long)"CICTApiTaskThead", 0,0,0);	
    int rc;
	struct mosquitto *mosq = NULL;
	char mqtt_host[128] = {0};
	memcpy(mqtt_host,(char *)mqttServerIp,strlen((char *)mqttServerIp));
	int mqtt_port = 1883;
	bool clean_session = true;
	int mqtt_keepalive = 25; //Tencent's wechat set heartbreak time span is 30 senconds

	mosq = mosquitto_new(NULL, clean_session, NULL);
	if (!mosq) {
		DF_ERROR("Error: can not create mosq structure! ");
		return NULL;
	}
	
	mosquitto_connect_callback_set(mosq, CICTMQ_Connect_Cb);
	mosquitto_message_callback_set(mosq, CICTMQ_Message_Cb);
	mosquitto_subscribe_callback_set(mosq, CICTMQ_Subscribe_Cb);
    rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, mqtt_keepalive);
    if (rc) {
 		 DF_ERROR("CICTMQ connect Failure! ");
    } else {
    	DF_DEBUG("CICTMQ connect Succeed! ");		 
	}
	rc = mosquitto_loop_forever(mosq, -1, 1);
	if(rc == MOSQ_ERR_ERRNO && errno == EPROTO)
	{
		
	}
	mosquitto_destroy(mosq);
	return NULL;
}

int CICTApiStart(char *ipaddr)
{
    mosquitto_lib_init();
	pthread_t cictTask;	
	if (pthread_create(&cictTask, NULL, CICTApiTask, (void *)ipaddr)){
		DF_ERROR("pthread_create CICTApiStart  is err \n");
		assert(0);
	}
	pthread_detach(cictTask);
	return KEY_TRUE;
}

int CICTApiStop(void)
{
	mosquitto_lib_cleanup();
	return KEY_TRUE;
}
