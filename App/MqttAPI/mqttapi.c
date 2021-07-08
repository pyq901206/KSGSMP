#include "mosquitto.h"
#include "common.h"
#include "mqttapi.h"
#include <errno.h>
#include "adpapi.h"
#include "jsonswitch.h"
#include "jsonmap.h"
#include "userdefine.h"
#include "usrmanage.h"

static AdpAPIHandle_T innerAdpHandle = {0};
struct mosquitto *g_pushmosq = NULL;
static int g_pushStatus = ENUM_ENABLED_FALSE;

#define MQTT_QOS 0
void my_connect_callback(struct mosquitto *mosq, void *userdata,  int result)
{
   //mode是MSGMODE_STDIN_FILE和MSGMODE_NULL时发布消息
	int mqtt_mid = 0;
	mosquitto_subscribe(mosq, &mqtt_mid, MAIN_SERVER_PULL_TOPIC, MQTT_QOS);	
	mosquitto_publish(mosq, NULL, MAIN_SERVER_PUSH_TOPIC, 5, "hello", MQTT_QOS, true);
	//上下线判断
	//mosquitto_subscribe(mosq, &mqtt_mid, "$SYS/brokers/+/clients/#", MQTT_QOS);
	return;
}

static int JsonMapPushDeviceStatusMake(char *msg, DeviceStorageInfo_T *deviceInfo)
{
	cJSON *pJsonRoot = NULL, *pJsonData = NULL;
	pJsonRoot = cJSON_CreateObject();
	cJSON_AddStringToObject(pJsonRoot, "cmd", "pushdevicestatus");	
	cJSON_AddStringToObject(pJsonRoot, "deviceid", deviceInfo->deviceInfo.deviceID);	
	cJSON_AddNumberToObject(pJsonRoot, "status", deviceInfo->deviceInfo.onlineStatus);	
	cJSON_AddStringToObject(pJsonRoot, "devicesoftwarever", deviceInfo->deviceInfo.deviceSoftwareVer);	
	cJSON_AddStringToObject(pJsonRoot, "deviceipaddr", deviceInfo->deviceInfo.deviceIpaddr);	
	char *out = cJSON_PrintUnformatted(pJsonRoot);
	sprintf(msg, "%s", out);
	free(out);
	cJSON_Delete(pJsonRoot);
	return KEY_TRUE;
}

static int FunGetDeviceStatus(int status, char *msg)
{
	char clientid[128] = {0};
	char flag[128] = {0};
	char deviceId[128] = {0};
	char sendMsg[512] = {0};
	int iret = -1;
	DeviceStorageInfo_T deviceInfo = {0};
	if (NULL == innerAdpHandle.pGetDeviceInfo || NULL == innerAdpHandle.pSetDeviceInfo)
	{
		return KEY_FALSE;
	}	
	cJSON *root = cJSON_Parse(msg);
	if (NULL == root)
	{
		return KEY_FALSE;
	}
	cJSON_GetObjectItemValueString(root, "clientid", clientid, sizeof(clientid));
	sscanf(clientid, "%[^/]/%s", deviceId,flag);
	if (0 == STRCASECMP(flag, "station"))
	{
		iret = innerAdpHandle.pGetDeviceInfo(deviceId, &deviceInfo);
		if (KEY_TRUE == iret)
		{
			deviceInfo.deviceInfo.onlineStatus = status;
			JsonMapPushDeviceStatusMake(sendMsg, &deviceInfo);
			mosquitto_publish(g_pushmosq, NULL, "station/web/pullmsg", strlen(sendMsg), (char *)sendMsg, MQTT_QOS, false);
			innerAdpHandle.pSetDeviceInfo(deviceId, &deviceInfo);
		}
	}
	cJSON_Delete(root);
	return KEY_TRUE;
}

static int ParesMqttHead(cJSON *data, JsonMsg_T *head)
{
	if(data == NULL || head == NULL){
		return KEY_FALSE;
	}
	cJSON_GetObjectItemValueString(data,"cmd",head->cmd,sizeof(head->cmd));
	cJSON_GetObjectItemValueString(data,"method",head->mothod,sizeof(head->mothod));
	cJSON_GetObjectItemValueString(data,"msgid",head->msgID,sizeof(head->msgID));
	cJSON_GetObjectItemValueString(data,"sessionid",head->ssionID,sizeof(head->ssionID));
	cJSON_GetObjectItemValueString(data,"clitopic",head->srctopic,sizeof(head->srctopic));
	return KEY_TRUE;
}

static int GroupMqttTopic(JsonMsg_T headMsg, char *msg, int msgLen)
{
	if (NULL == msg){
		return KEY_FALSE;
	}
	cJSON *root = cJSON_Parse(msg);
	if (NULL == root){
		return KEY_FALSE;
	}
	
	char *out = cJSON_PrintUnformatted(root);
	memset(msg, 0, msgLen);
	memcpy(msg, out, strlen(out));
	free(out);
	out = NULL;
	cJSON_Delete(root);
	return KEY_TRUE;
}

void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	int iret = -1;	
	JsonMsg_T headMsg = {0};
	cJSON *headData = NULL;
	char sendMsg[MQTTMSG_MAXLEN] = {0};

	DF_DEBUG("topic:%s  payload:%s", message->topic, (char *)message->payload);
	if (!strcmp(message->topic, MAIN_SERVER_PULL_TOPIC))
	{
		headData = cJSON_Parse((char *)message->payload);
		ParesMqttHead(headData,&headMsg);
		cJSON_Delete(headData);
    	if (!STRCASECMP(headMsg.mothod, "request"))
		{
			iret = JsonSwitch((char *)message->payload, sendMsg, MQTTMSG_MAXLEN);
			if (KEY_CMDNOTFOUND != iret)
			{
				if (0 < strlen(sendMsg) && 0 != strlen(headMsg.srctopic))
				{
					GroupMqttTopic(headMsg, sendMsg, sizeof(sendMsg));
					iret = mosquitto_publish(mosq, NULL, headMsg.srctopic, strlen(sendMsg), (unsigned char *)(sendMsg), MQTT_QOS, false);
					DF_DEBUG("iret: %d  sendMsg: %s", iret, sendMsg);
				}
			}
		}
		else
		{
			DF_DEBUG("request [%s] [%s]",message->topic, message->payload);
			JsonSwitch((char *)message->payload, sendMsg, MQTTMSG_MAXLEN);
		}
	}
	return;
}

void my_disconnect_callback(struct mosquitto *mosq)    
{
}

void my_publish_callback(struct mosquitto *mosq, short mid)        
{
}


void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
}

void *MqttApiTask(void *mqttServerIp)
{
	prctl(PR_SET_NAME, (unsigned long)"MqttApiTaskThead", 0,0,0);	
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
	
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
	mosquitto_will_clear(mosq);
	mosquitto_will_set(mosq, MAIN_SERVER_PUSH_TOPIC, 3, "bye", 0, 1);
		
    rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, mqtt_keepalive);
    if (rc) {
 		 DF_ERROR("MQTT connect Failure! ");
    } else {
    	DF_DEBUG("MQTT connect Succeed! ");		 
	}
	
    rc = mosquitto_loop_forever(mosq, -1, 1);
	if(rc == MOSQ_ERR_ERRNO && errno == EPROTO)
	{
		
	}
	
	mosquitto_destroy(mosq);
	return NULL;
}

int pushMqttMsg(char * topic, char *resMsg, int qos)
{
	
	for (;g_pushmosq == NULL || ENUM_ENABLED_FALSE == g_pushStatus;){
		sleep(1);
		continue;
	}
	DF_DEBUG("push %s %s",topic, resMsg);
	mosquitto_publish(g_pushmosq, NULL, topic, strlen(resMsg), (unsigned char *)(resMsg), qos, false);
	return KEY_TRUE;
}

int pushMqttUnsubscribe(char * topic)
{
	int mqtt_mid = 0;
	mosquitto_unsubscribe(g_pushmosq, &mqtt_mid, topic);
	return KEY_TRUE;
}

static void *PushMqttApiTask(void *mqttServerIp)
{
	prctl(PR_SET_NAME, (unsigned long)"PushMqttApiTaskThead", 0,0,0);	
	int rc;
	char mqtt_host[128] = {0};
	memcpy(mqtt_host,(char *)mqttServerIp,strlen((char *)mqttServerIp));
	int mqtt_port = 1883;
	bool clean_session = true;
	int mqtt_keepalive = 25; //Tencent's wechat set heartbreak time span is 30 senconds
	g_pushmosq = mosquitto_new(NULL, clean_session, NULL);
	if (!g_pushmosq) {
		DF_ERROR("Error: can not create mosq structure! ");
		return NULL;
	}
	
    rc = mosquitto_connect(g_pushmosq, mqtt_host, mqtt_port, mqtt_keepalive);
    if (rc) {
 		 DF_ERROR("Push MQTT connect Failure! ");
    } else {
    	DF_DEBUG("Push MQTT connect Succeed! ");	
		g_pushStatus = ENUM_ENABLED_TRUE;
	}
	
    rc = mosquitto_loop_forever(g_pushmosq, -1, 1);
	if(rc == MOSQ_ERR_ERRNO && errno == EPROTO)
	{
		
	}
	mosquitto_destroy(g_pushmosq);
	return NULL;
}

static int MqttHighTempFunc(DeviceAlarm_T *info)
{
	char sendMsg[512] = {0};
	if(NULL == info){
		return KEY_FALSE;
	}
	JsonMapPushAlarmMake(sendMsg, info);
	pushMqttMsg(MAIN_SERVER_PUSH_TOPIC, sendMsg, 0);
	return KEY_TRUE;
}

int MqttStart(char *mqttServerIp)
{
    mosquitto_lib_init();
	innerAdpHandle.highTempFun = MqttHighTempFunc;
 	if (KEY_FALSE == AdpRegistHandle(&innerAdpHandle, INNER_USER_NAME, INNER_USER_PASSWORD)){
		DF_ERROR("AdpRegistHandle fault");
		return KEY_UNAUTHORIZED;
	}
	
	pthread_t mqttTask;	
	if (pthread_create(&mqttTask, NULL, MqttApiTask, (void *)mqttServerIp)){
		DF_ERROR("pthread_create MqttStart  is err \n");
		assert(0);
	}
	pthread_detach(mqttTask);

	pthread_t pushMqttTask;	
	if (pthread_create(&pushMqttTask, NULL, PushMqttApiTask, (void *)mqttServerIp)){
		DF_ERROR("pthread_create MqttStart  is err \n");
		assert(0);
	}
	pthread_detach(pushMqttTask);
	return KEY_TRUE;
}

int MqttStop(void)
{
	mosquitto_lib_cleanup();
	AdpUnRegistHandle(&innerAdpHandle);
	return KEY_TRUE;
}
