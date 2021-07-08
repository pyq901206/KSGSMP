#ifdef __cplusplus
extern "C" {
#endif


#ifndef __TSOCKET_H__
#define __TSOCKET_H__
#define DF_NET_MSGBUFLEN (5 * 1024 * 1024)//(200*1024)	

typedef struct _NetCliInfo_T{
	char 	fromIP[16];
	int		fromPort;
	int		recvSocket;
	int 	recvLen;
	char	recvmsg[DF_NET_MSGBUFLEN];
	char	recvIp[16];
	int		recvPort;
	char	ethName[64];
	void	*usrdef;
}NetCliInfo_T;

typedef struct _NetCab_T{
	int					ethCount;
	char				ethip[10][16];//Íø¿¨IP
	char				ethname[10][16];//Íø¿¨Ãû×Ö
}NetCab_T;

typedef int (*NetTcpSerReadCbFun)(NetCliInfo_T *cli);
typedef int (*NetTcpSerCheckEndCbFun)(NetCliInfo_T *cli);
typedef int (*NetTcpSerCloseCbFun)(int sockfd);
typedef int (*CheckDataComplet)(char *readbuf,int len);
typedef int (*NetGetNetCab)(NetCab_T *info);



typedef struct _NetWorkSerHandle_T{
	char				mulicastip[16];//×é²¥µØÖ·
	int					mulicastPort;
	int 				serverport;
	int					sockfd;
	NetTcpSerReadCbFun 	readcb;
	NetTcpSerCloseCbFun 	closecb;
	NetTcpSerCheckEndCbFun checkendcb;
	NetGetNetCab		getNetCabInfo;
}NetWorkSerHandle_T;

//TCP
int NetCreateTcpSer(NetWorkSerHandle_T *handle);
int NetCloseSerSocket(NetWorkSerHandle_T *handle);
int NetTcpSendMsg(int sockfd,char *msg,int len);
int NetTcpMtuSendMsg(int sockfd, char *msg,int len);

//UDP
int NetCreateUdpMulicast(NetWorkSerHandle_T *handle);
int NetUdpSendMsg(char *ip, int port,char *msg,int len,char *ethname);
int NetUdpSendMsgBySocket(int sockfd,char *ip, int port,char *msg,int len);
int NetTcpGetMsg(char *ip,int port,char *sendBuf,int sendLen,char *readBuf,int readBufLen,int timeOut,CheckDataComplet checkdata);



#ifdef __cplusplus
}
#endif

#endif

