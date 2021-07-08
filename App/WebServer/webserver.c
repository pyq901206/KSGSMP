#include "common.h"
#include "webs.h"
#include "userdefine.h"
#include "polarssl/config.h"
#include "polarssl/base64.h"
#include "polarssl/error.h"
#include "polarssl/net.h"
#include "polarssl/ssl.h"
#include "polarssl/havege.h"
#include "polarssl/certs.h"
#include "polarssl/x509.h"
#include "tsocket.h"
#include "handlemanage.h"
#include "jsonmap.h"
#include "hkManage.h"

#define BUFFER_SIZE 1024
#define RESPONSE_HEADER_LEN_MAX 1024

#define MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

typedef struct _FramHead_T{
    char fin;
    char opcode;
    char mask;
	int  headLen;
    unsigned int payload_length;
    char masking_key[4];
}FramHead_T;

typedef struct _Inner_WebServerHandle_T
{
	HandleManage_T 	   linkListHead;
	NetWorkSerHandle_T webNetSer; 
}Inner_WebServerHandle_T;
Inner_WebServerHandle_T g_webHandle;
static Inner_WebServerHandle_T *GetWebServerHandle()
{
	return &g_webHandle;
}

typedef struct _LinkInfo_T
{
	int  socket;
	int  channel;
	int  enable;
	char SerialNumber[64]; 
}LinkInfo_T;

typedef struct _LinkManage_T
{
	pthread_mutex_t 	selfMutex;
	LinkInfo_T			linkInfo;
}LinkManage_T;

static int ParasingFrameRequest(char *msg, LinkInfo_T *linkInfo)
{
	if(NULL == msg){
		return -1;
	}

	cJSON *json= cJSON_Parse(msg);
	if(NULL != json){
		linkInfo->enable  = cJSON_GetObjectItemValueInt(json,"enable");
		linkInfo->channel = cJSON_GetObjectItemValueInt(json,"channel");
		cJSON_GetObjectItemValueString(json, "SerialNumber", linkInfo->SerialNumber, sizeof(linkInfo->SerialNumber));
	}
	cJSON_Delete(json);
	return 0;
}

static int ParsingFrameHeader(char *recvMsg, int recvLen, FramHead_T* head)
{
    int readCount = 0;
    /*read fin and op code*/
    if (recvLen <= 0)
    {
        perror("read fin");
        return -1;
    }
    head->fin = (recvMsg[readCount] & 0x80) == 0x80;
    head->opcode = recvMsg[readCount] & 0x0F;
	readCount ++;
    head->mask = (recvMsg[readCount] & 0x80) == 0X80;
    /*get payload length*/
    head->payload_length = recvMsg[readCount] & 0x7F;
	readCount ++;
    if (head->payload_length == 126)
    {
		char *extern_len = &recvMsg[readCount];
		head->payload_length = (extern_len[0]&0xFF) << 8 | (extern_len[1]&0xFF);
		readCount += 2;
    }
    else if (head->payload_length == 127)
    {
    	char temp = 0;
		char *extern_len = &recvMsg[readCount];
        int i;
        for(i = 0; i < 4; i++)
        {
            temp = extern_len[i];
            extern_len[i] = extern_len[7-i];
            extern_len[7-i] = temp;
        }
        memcpy(&(head->payload_length),extern_len,8);	
		readCount += 8; 
    }

    /*read masking-key*/
	char *extern_len = &recvMsg[readCount];
	memcpy(&head->masking_key, extern_len, 4);	
	readCount += 4; 
	head->headLen = readCount;	
    return 0;
}

static void webSocket_getRandomString(unsigned char *buf, unsigned int len)
{
    unsigned int i;
    unsigned char temp;
    srand((int)time(0));
    for(i = 0; i < len; i++)
    {
        temp = (unsigned char)(rand()%256);
        if(temp == 0)   // 随机数不要0, 0 会干扰对字符串长度的判断
            temp = 128;
        buf[i] = temp;
    }
}

static int webSocket_enPackage(unsigned char *data, unsigned int dataLen, unsigned char *package, unsigned int packageMaxLen, bool isMask, Websocket_CommunicationType type)
{
    unsigned char maskKey[4] = {0};    // 掩码
    unsigned char temp1, temp2;
    int count;
    unsigned int i, len = 0;
 
    if(packageMaxLen < 2)
        return -1;
 
    if(type == WCT_MINDATA)
        *package++ = 0x00;
    else if(type == WCT_TXTDATA)
        *package++ = 0x81;
    else if(type == WCT_BINDATA)
        *package++ = 0x82;
    else if(type == WCT_DISCONN)
        *package++ = 0x88;
    else if(type == WCT_PING)
        *package++ = 0x89;
    else if(type == WCT_PONG)
        *package++ = 0x8A;
    else
        return -1;
    //
    if(isMask)
        *package = 0x80;
    len += 1;
    //
    if(dataLen < 126)
    {
        *package++ |= (dataLen&0x7F);
        len += 1;
    }
    else if(dataLen < 65536)
    {
        if(packageMaxLen < 4)
            return -1;
        *package++ |= 0x7E;
        *package++ = (char)((dataLen >> 8) & 0xFF);
        *package++ = (unsigned char)((dataLen >> 0) & 0xFF);
        len += 3;
    }
    else if(dataLen < 0xFFFFFFFF)
    {
        if(packageMaxLen < 10)
            return -1;
        *package++ |= 0x7F;
        *package++ = 0; //(char)((dataLen >> 56) & 0xFF);   // 数据长度变量是 unsigned int dataLen, 暂时没有那么多数据
        *package++ = 0; //(char)((dataLen >> 48) & 0xFF);
        *package++ = 0; //(char)((dataLen >> 40) & 0xFF);
        *package++ = 0; //(char)((dataLen >> 32) & 0xFF);
        *package++ = (char)((dataLen >> 24) & 0xFF);        // 到这里就够传4GB数据了
        *package++ = (char)((dataLen >> 16) & 0xFF);
        *package++ = (char)((dataLen >> 8) & 0xFF);
        *package++ = (char)((dataLen >> 0) & 0xFF);
        len += 9;
    }
    //
    if(isMask)    // 数据使用掩码时, 使用异或解码, maskKey[4]依次和数据异或运算, 逻辑如下
    {
        if(packageMaxLen < len + dataLen + 4)
            return -1;
        webSocket_getRandomString(maskKey, sizeof(maskKey));    // 随机生成掩码
        *package++ = maskKey[0];
        *package++ = maskKey[1];
        *package++ = maskKey[2];
        *package++ = maskKey[3];
        len += 4;
        for(i = 0, count = 0; i < dataLen; i++)
        {
            temp1 = maskKey[count];
            temp2 = data[i];
            *package++ = (char)(((~temp1)&temp2) | (temp1&(~temp2)));  // 异或运算后得到数据
            count += 1;
            if(count >= sizeof(maskKey))    // maskKey[4]循环使用
                count = 0;
        }
        len += i;
        *package = '\0';
    }
    else    // 数据没使用掩码, 直接复制数据段
    {
        if(packageMaxLen < len + dataLen)
            return -1;
        memcpy(package, data, dataLen);
        package[dataLen] = '\0';
        len += dataLen;
    }
    //
    return len;
}

static void Webumask(char *data,int len,char *mask)
{
    int i;
    for (i = 0;i < len; ++i)
        *(data+i) ^= *(mask+(i%4));
}

static int WebSocketSendList(void *handle, int argc, void *argv[])
{
	int sendLen = 0, channel = 0;

	int *sendLenBack = (int *)argv[0];
	sendLen = *sendLenBack;

	char *sendMsg = (char *)argv[1];
	char *SerialNumber = (char *)argv[2];

		
	LinkManage_T *linkListInfo = (LinkManage_T *)handle;
	char *buff = NULL;
	int buffLen = 0;
	FramHead_T head = {0};
	head.payload_length = sendLen;
	buff = (char *)malloc(sendLen + 128);
	memset(buff, 0, sendLen + 128);
	buffLen = webSocket_enPackage(sendMsg, sendLen, buff, sendLen + 1024, false, WCT_BINDATA);
	//DF_DEBUG("send to socket: %d and len = %d ,sendmsg = %s",linkListInfo->linkInfo.socket,buffLen,buff);
	NetTcpSendMsg(linkListInfo->linkInfo.socket, buff, buffLen);
	free(buff);
	return KEY_TRUE;
}

static int GetHttpWebSocketKey(char *str,char *host)
{
	if(NULL == str || NULL == host) return KEY_FALSE;
	char *pHead = strstr(str, "Sec-WebSocket-Key");
	if(NULL == pHead) return KEY_FALSE;
	STR_GETVALUE_BTPOINT(pHead+strlen("Sec-WebSocket-Key")+1,str+strlen(str),host);
	return KEY_TRUE;
}

static int WebSocketAuthentication(int socket, char *webSocketKey)
{
	char wsKeyAndMagic[1024] = {0};
    unsigned char sha1Data[20 + 1]={0};
    char wsAccept[32] = {0};
	int wsAcceptLen = 0;
    char response[BUFFER_SIZE] = {0};
	snprintf(wsKeyAndMagic, 1024, "%s%s", webSocketKey, MAGIC);
	sha1((unsigned char*)&wsKeyAndMagic,strlen(wsKeyAndMagic),(unsigned char*)&sha1Data);
    base64_encode(wsAccept, (size_t *)&wsAcceptLen, (unsigned char *)sha1Data, (size_t)strlen(sha1Data));
	sprintf(response, "HTTP/1.1 101 Switching Protocols\r\n" \
							"Upgrade: websocket\r\n" \
							"Connection: Upgrade\r\n" \
							"Sec-WebSocket-Accept: %s\r\n" \
							"\r\n",wsAccept);
	write(socket, response, strlen(response));
	return KEY_TRUE;
}

int WebSocketSend(char *sendMsg, int sendLen, char *SerialNumber, int channel)
{
#if 0
	//使用链表管理
	int argc = 2;
	void *argv[2];
	argv[0] = (void *)&sendLen;
	argv[1] = (void *)sendMsg;
	HandleManagePostLoop(&GetWebServerHandle()->linkListHead, WebSocketSendList, argc, argv);
#else
	int buffLen = 0;
	char *buff = NULL;
	void *result = NULL;
	FramHead_T head = {0};
	LinkManage_T *linkListInfo = NULL;

	HandleManage_T *cur = NULL;
	HandleManage_T *listhead = &GetWebServerHandle()->linkListHead;

	for(cur = listhead->next; cur != NULL; cur = cur->next)
	{
		linkListInfo = (LinkManage_T *)cur->handle;
		if(!strcmp(linkListInfo->linkInfo.SerialNumber, SerialNumber) && (linkListInfo->linkInfo.channel == channel))
		{
			head.payload_length = sendLen;
			buff = (char *)malloc(sendLen + 128);
			memset(buff, 0, sendLen + 128);
			buffLen = webSocket_enPackage(sendMsg, sendLen, buff, sendLen + 1024, false, WCT_BINDATA);
			NetTcpSendMsg(linkListInfo->linkInfo.socket, buff, buffLen);
			free(buff);
			break;
		}
	}
#endif
	return KEY_TRUE;
}

static int AddNodeToLink(LinkInfo_T info)
{
	int iret = -1;
	void *result = NULL;
	LinkManage_T *linkListInfo = NULL;
	HandleManageGetHandleByInt(&GetWebServerHandle()->linkListHead, linkInfo.socket, info.socket, result, LinkManage_T);
	if (NULL != result){
		return iret;
	}

	linkListInfo = (LinkManage_T *)malloc(sizeof(LinkManage_T));
	memset(linkListInfo, 0, sizeof(LinkManage_T));
	MUTEX_INIT(&linkListInfo->selfMutex);
	memcpy(&linkListInfo->linkInfo, &info, sizeof(LinkInfo_T));
	HandleManageAddHandle(&GetWebServerHandle()->linkListHead, (void *)linkListInfo);
	return KEY_TRUE;
}

static int DelNodeToLink(LinkInfo_T info)
{
	int iret = -1;
	void *result = NULL;
	LinkManage_T *linkListInfo = NULL;
	HandleManageGetHandleByInt(&GetWebServerHandle()->linkListHead, linkInfo.socket, info.socket, result, LinkManage_T);
	if (NULL == result){
		return iret;
	}

	HandleManageDelHandle(&GetWebServerHandle()->linkListHead, result);
	free(result);
	return KEY_TRUE;
}

static int WebRequst(NetCliInfo_T *cli)
{
	FramHead_T head = {0};
	char webSocketKey[512] = {0};
	char *payload = NULL;
	LinkInfo_T linkInfo = {0};
	GetHttpWebSocketKey(cli->recvmsg, webSocketKey);
	if (0 != strlen(webSocketKey))
	{
		WebSocketAuthentication(cli->recvSocket, webSocketKey);
		return KEY_TRUE;
	}
	
	ParsingFrameHeader(cli->recvmsg, cli->recvLen, &head);
	Webumask(&cli->recvmsg[head.headLen], head.payload_length, head.masking_key);
	linkInfo.socket = cli->recvSocket;
	ParasingFrameRequest(&cli->recvmsg[head.headLen], &linkInfo);

	return KEY_TRUE;
}

static int WebHttpChecked(NetCliInfo_T *cli)
{
	return KEY_TRUE;
}

static int WebHttpClose(int sockfd)
{
	DF_DEBUG("remove link fd = %d", sockfd);

	close(sockfd);
	LinkInfo_T info;
	info.socket = sockfd;
	DelNodeToLink(info);
	return KEY_TRUE;
}

static int UnInitHandleList()
{
	HandleManage_T*	head = &GetWebServerHandle()->linkListHead;
	HandleManage_T *cur = NULL;
	MUTEX_LOCK(&head->selfMutex);
	for(cur = head->next;cur != NULL;cur = cur->next){
		free(cur->handle);
		if(cur->pre != head && cur->pre != NULL){
			free(cur->pre);
		}
	}
	MUTEX_UNLOCK(&head->selfMutex);
	return KEY_TRUE;	
}

static int WebServerHttpStart(int port)
{
	NetWorkSerHandle_T *netHandle = &(GetWebServerHandle()->webNetSer);
	netHandle->readcb = WebRequst;
	netHandle->checkendcb = WebHttpChecked;
	netHandle->closecb = WebHttpClose;
	netHandle->serverport = port;
	NetCreateTcpSer(netHandle);
	//初始化连接管理链表
	Inner_WebServerHandle_T *gHandle =  GetWebServerHandle();
	HandleManageInitHead(&gHandle->linkListHead);
	return KEY_TRUE;
}

static int WebServerHttpStop()
{
	UnInitHandleList();
	return KEY_TRUE;
}

int WebServerHttpInit(int port)
{
	WebServerHttpStart(port);
	return KEY_TRUE;
}

int WebServerHttpUnInit()
{
	WebServerHttpStop();
	return KEY_TRUE;
}

