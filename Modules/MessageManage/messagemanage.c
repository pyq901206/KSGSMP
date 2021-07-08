#include "common.h"
#include "messagemanage.h"
#include "timemanageout.h"
#include "handlemanage.h"
#include "rdkafka.h"
#include <ctype.h>

typedef struct _MsgManageInner_T{
	HandleManage_T listHead;
	rd_kafka_t *kafkaHandle;
}MsgManageInner_T;
static MsgManageInner_T g_msgInner;
static MsgManageInner_T *GetMsgListHandle()
{
	return &g_msgInner;
} 

static int KafkaInit(rd_kafka_t **handle, char *brokers)
{
	char errstr[512];		 /* librdkafka API error reporting buffer */
	const char *groupid = "-t";	 /* Argument: Consumer group id */
	rd_kafka_conf_t *conf;	 /* Temporary configuration object */
	//rd_kafka_t *cHandle = NULL;	
	conf = rd_kafka_conf_new();
	if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers,
								errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
	{
		  rd_kafka_conf_destroy(conf);
		  return -1;
	} 
	*handle = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
	if (!(*handle))
	{ 
		return -1;
	}	
	conf = NULL;
	return 0;
}

static int KafkaUnInit(rd_kafka_t *handle)
{
	DF_DEBUG("Closing Producev");
	rd_kafka_flush(handle, 10*1000 /* wait for max 10 seconds */);
	if (rd_kafka_outq_len(handle) > 0)
	{
        DF_ERROR("%% %d message(s) were not delivered",rd_kafka_outq_len(handle));
	}
	rd_kafka_destroy(handle);
	return KEY_TRUE;
}

static int KafkaProducev(rd_kafka_t *handle, char *topic, char *buf, int bufLen)
{
	int err = -1;
	if (0 >= strlen(buf))
	{
		return KEY_FALSE;
	}
	err = rd_kafka_producev(
			 /* Producer handle */
			 handle,
			 /* Topic name */
			 RD_KAFKA_V_TOPIC(topic),
			 /* Make a copy of the payload. */
			 RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
			 /* Message value and length */
			 RD_KAFKA_V_VALUE(buf, bufLen),
			 /* Per-Message opaque, provided in
			  * delivery report callback as
			  * msg_opaque. */
			 RD_KAFKA_V_OPAQUE(NULL),
			 /* End sentinel */
			 RD_KAFKA_V_END);
	 if (err) 
	 {
		 /*
		  * Failed to *enqueue* message for producing.
		  */
	     DF_ERROR("%% Failed to produce to topic %s: %s",topic, rd_kafka_err2str(err));
		 if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL)
		 {
			 /* If the internal queue is full, wait for
			  * messages to be delivered and then retry.
			  * The internal queue represents both
			  * messages to be sent and messages that have
			  * been sent or failed, awaiting their
			  * delivery report callback to be called.
			  *
			  * The internal queue is limited by the
			  * configuration property
			  * queue.buffering.max.messages */
			 rd_kafka_poll(handle, 1000/*block for max 1000ms*/);
			 return -2;
		 }
	 }
	 else 
	 {
		 fprintf(stderr, "%% Enqueued message (%d bytes) "
		 "for topic %s\n",
		 bufLen, topic);
	 }
	 rd_kafka_poll(handle, 0/*non-blocking*/);
	 return KEY_TRUE;
}

static int PushAlarmMsg(char *deviceId, AlarmMsg_T *msg)
{
	int iret = -1;
	char date[128] = {0};
	char buff[1024] = {0};
	char topic[128] = {0};
	snprintf(topic, 128, "%s", deviceId);
	snprintf(date, 64, "%04d-%02d-%02dT%02d:%02d:%02dZ",msg->date.year,msg->date.month,msg->date.day,msg->date.hour,msg->date.min,msg->date.sec);
	snprintf(buff, 1024, "{\"cmd\":\"pushalarm\",\"alarmtype\":%d,\"alarmtime\":\"%s\",\"deviceid\":\"%s\"}",msg->type, date, deviceId);
	iret = KafkaProducev(GetMsgListHandle()->kafkaHandle, topic, buff, strlen(buff));
	return iret;
}

int MsgManageInit(MsgManageHandle_T *handle)
{
	MsgManageRegist(handle);
	KafkaInit(&GetMsgListHandle()->kafkaHandle, "113.247.222.69");
	return KEY_TRUE;
}

int MsgManageUnInit(MsgManageHandle_T *handle)
{
	KafkaUnInit(GetMsgListHandle()->kafkaHandle);
	MsgManageUnRegist(handle);
	return KEY_TRUE;
}

int MsgManageRegist(MsgManageHandle_T *handle)
{
	handle->pPushAlarmMsg = PushAlarmMsg;
	return KEY_TRUE;
}

int MsgManageUnRegist(MsgManageHandle_T *handle)
{
	handle->pPushAlarmMsg = NULL;
	return KEY_TRUE;
}








