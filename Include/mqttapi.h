#ifndef __MOTTAPI_H__
#define __MOTTAPI_H__

//web端接受消息 CCSMP/xxxxxxxx/pullmsg
#define MAIN_SERVER_PULL_TOPIC  "CCSMP/mainserver/pullmsg"
#define MAIN_SERVER_PUSH_TOPIC  "CCSMP/mainserver/pushmsg"
#define MQTTMSG_MAXLEN	(4 * 1024 * 1024)
#define MQTTRESTOPICLEN	 128	

int MqttStart(char *mqttServerIp);
int MqttStop(void);

int pushMqttMsg(char * topic, char *resMsg, int qos);
int pushMqttUnsubscribe(char * topic);

#endif

