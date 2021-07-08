#ifndef _LIBEVENTSOCKET_H_
#define _LIBEVENTSOCKET_H_
#define MAX_RECV_MSG  1024 * 5
#define BATTERY_SERVER_PORT 8001
typedef struct _NetServer_T{
	char ipaddr[16];
	int socket;	
	char recvMsg[MAX_RECV_MSG];
}NetServer_T;

typedef int (*LibEventNetTcpSerReadCbFun)(NetServer_T *cli);
typedef int (*LibEventNetTcpSerCloseCbFun)(NetServer_T *cli);

typedef struct _LibEventNetWorkSerHandle_T{
	LibEventNetTcpSerReadCbFun 	readcb;
	LibEventNetTcpSerCloseCbFun 	closecb;
}LibEventNetWorkSerHandle_T;


int LibEventTcpNetSer(LibEventNetWorkSerHandle_T *handle);
int TcpNetSendMsg(int socket, char *msg, int len);

#endif
