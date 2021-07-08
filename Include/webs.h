#ifndef _WEB_S_
#define _WEB_S_

#ifdef __cplusplus
extern "C" {
#endif


typedef enum{
    WCT_MINDATA = -20,      // 0x0：标识一个中间数据包
    WCT_TXTDATA = -19,      // 0x1：标识一个txt类型数据包
    WCT_BINDATA = -18,      // 0x2：标识一个bin类型数据包
    WCT_DISCONN = -17,      // 0x8：标识一个断开连接类型数据包
    WCT_PING = -16,     // 0x8：标识一个断开连接类型数据包
    WCT_PONG = -15,     // 0xA：表示一个pong类型数据包
    WCT_ERR = -1,
    WCT_NULL = 0
}Websocket_CommunicationType;

int WebServerHttpInit(int port);
int WebSocketSend(char *sendMsg, int sendLen, char *SerialNumber, int channel);
int WebServerHttpUnInit();


#ifdef __cplusplus
}
#endif


#endif
