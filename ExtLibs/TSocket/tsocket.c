/*******************************************************************
|  Copyright(c)  GaoZhi Technology Development Co.,Ltd
|  All rights reserved.
|
|  版本: 1.0 
|  作者: 谭帆 [tanfan0406@163.com]
|  日期: 2018年2月22日
|  说明:
|  修改:
|	
******************************************************************/
#include "tsocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <net/if.h>       /* ifreq struct */
#include <net/if_arp.h>
#include <pthread.h>
#include "list.h"
#include "common.h"
#include <sys/prctl.h>
#include <netdb.h>
#include <fcntl.h>

typedef enum _SOCKETTYPE_E{
	SOCKETTYPE_ENUM_NULL   = 0x00,
	SOCKETTYPE_ENUM_MAIN	= 0x01,
	SOCKETTYPE_ENUM_SUB		= 0x02,
}SOCKETTYPE_E;

typedef struct _SocketList_S{
	int 					sockfd;
	SOCKETTYPE_E 			socketType; 
	char					ip[16];
	int						port;
	struct					listnode _list;
}SocketList_S;

typedef struct _ExtParam_S{
	int mainSockfd;
    char *mulicastIp;
	NetGetNetCab		getNetCabInfo;
}ExtParam_S;



typedef struct _NetServerParam_S{
	int mainSockfd;
	NetTcpSerReadCbFun readcb;
	NetTcpSerCheckEndCbFun checkendcb;
	NetTcpSerCloseCbFun closecb;
}NetServerParam_S;

static int bTsockFlag=KEY_TRUE;//note by fan 此变量会导致Tsocket一个模块退出全体退出

int GetSockInfo(int  socked, char *ipaddr,int *port)
{
	struct sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));

	socklen_t nSockAddrLen = sizeof(sockAddr);
	int iResult = getsockname(socked, (struct sockaddr *)&sockAddr, &nSockAddrLen);
	if (0 == iResult){
		*port = ntohs(sockAddr.sin_port);
		sprintf(ipaddr,"%s",inet_ntoa(sockAddr.sin_addr));
		return KEY_TRUE;
	}
	return KEY_FALSE;
} 


int CheckSelectExcept(fd_set *exceptfds,SocketList_S *sockInfo)
{
	if(0 == FD_ISSET(sockInfo->sockfd, exceptfds)){
		return KEY_FALSE;
	}
	list_remove(&(sockInfo->_list));
	close(sockInfo->sockfd);
	free(sockInfo);
	return KEY_TRUE;
}

int CheckSelectRead(fd_set *readfds,SocketList_S *head,SocketList_S *sockInfo,NetServerParam_S *param)
{
	//static NetCliInfo_T cliInfo = {0};
	//memset(&cliInfo, 0, sizeof(NetCliInfo_T));
	//NetCliInfo_T *cli = &cliInfo;
	NetCliInfo_T *cli = (NetCliInfo_T *)malloc(sizeof(NetCliInfo_T));
	memset(cli, 0, sizeof(NetCliInfo_T));
	int cliSockfd = -1;
	struct sockaddr_in client_addr;
	SocketList_S *newnode = NULL;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int ret = -1;

	fd_set readfd,errorfd;
	int maxfd = 0;
	int iret = 0;
	struct timeval timeout = {0};
	int timeOutCount;

	if(0 == FD_ISSET(sockInfo->sockfd, readfds)){
		free(cli);
		return KEY_FALSE;
	}

	if(SOCKETTYPE_ENUM_MAIN == sockInfo->socketType){//主套接字
		cliSockfd = accept(sockInfo->sockfd, (struct sockaddr*)&client_addr, &addr_len);
		if(cliSockfd <= 0){
			free(cli);
			return KEY_FALSE;
		}
		
		newnode =  (SocketList_S *)malloc(sizeof(SocketList_S));
		if(NULL == newnode){
			free(cli);
			return KEY_FALSE;
		}
		newnode->sockfd = cliSockfd;
		newnode->socketType = SOCKETTYPE_ENUM_SUB;
		newnode->port = ntohs(client_addr.sin_port);
		inet_ntop(AF_INET, (void *)(&client_addr.sin_addr.s_addr), newnode->ip, 16);
		//snprintf(newnode->ip,16,"%s",inet_ntoa(client_addr.sin_addr));
		list_add_tail(&(head->_list),&(newnode->_list));	
	}else if(SOCKETTYPE_ENUM_SUB == sockInfo->socketType){//从套接字
		//cli = (NetCliInfo_T *)malloc_checkout(sizeof(NetCliInfo_T));
		//if(NULL == cli){
		//	return KEY_FALSE;
		//}
		memset(cli,0,sizeof(NetCliInfo_T));
		memset(cli->recvmsg,0,sizeof(cli->recvmsg));
		//DF_DEBUG("socket read");
		timeOutCount = 0;
		while(    bTsockFlag==KEY_TRUE){
			FD_ZERO(&readfd);
			FD_ZERO(&errorfd);
			FD_SET(sockInfo->sockfd,&readfd); //添加描述符
			FD_SET(sockInfo->sockfd,&errorfd); 
		 	maxfd = sockInfo->sockfd;
			timeout.tv_sec = 0;
			timeout.tv_usec = 100000;//select函数会不断修改timeout的值，所以每次循环都应该重新赋值[windows不受此影响]	
			ret  = select (maxfd + 1, &readfd, NULL, &errorfd, &timeout);
			if (ret > 0){
				if(0 == FD_ISSET(sockInfo->sockfd, &readfd)){
					goto READ_ERROR;
				}
				ret = recv(sockInfo->sockfd,cli->recvmsg+cli->recvLen,sizeof(cli->recvmsg)-cli->recvLen,0);//获取网络消息
				if(ret >  0){
					//收到正确消息处理
					memcpy(cli->fromIP,sockInfo->ip,16);
					//DF_DEBUG("cli->fromIP = %s",cli->fromIP);
					cli->fromPort= sockInfo->port;
					//DF_DEBUG("cli->fromPort = %d",cli->fromPort);
					cli->recvLen += ret;
					cli->recvSocket = sockInfo->sockfd;
					GetSockInfo(cli->recvSocket,cli->recvIp,&(cli->recvPort));
					if(KEY_TRUE == param->checkendcb(cli)){
						struct timeval timeout = {0,300 * 1000};    
						setsockopt(cli->recvSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(struct timeval));

						param->readcb(cli);
						break;
					}
					//if(ret != DF_NET_MSGBUFLEN){
					//	break;
					//}
					//DF_DEBUG("recv no end");
				}else{
					//客户端主动关闭套接字或链接错误关闭套接字
				//	DF_DEBUG("clinet close socked,ret = %d",ret);
				goto READ_ERROR;
				}
			}else if (ret == 0){
				timeOutCount++;
				if(timeOutCount > 5){
					goto READ_ERROR;
				}
				//DF_DEBUG("select time out");
				continue;
			}else {
				//DF_DEBUG("select error");
				goto READ_ERROR;
			}
			SleepMs(10);//程序空跑 CPU不至于100%		
		}
	}
	if (NULL != cli)
	{
		free(cli);
		cli = NULL;
	}
	return KEY_TRUE;
READ_ERROR:
	if(NULL != cli->usrdef){
		free(cli->usrdef);
	}
	if (NULL != cli)
	{
		free(cli);
		cli = NULL;
	}
#if 1
	list_remove(&(sockInfo->_list));
	close(sockInfo->sockfd);
	param->closecb(sockInfo->sockfd);	
	free(sockInfo);	
#endif
	return KEY_TRUE;
}

void *NetServerThead(void *arg)
{
	SocketList_S *head = NULL;
	SocketList_S *newnode = NULL;
	NetServerParam_S *param = NULL;
	struct listnode *node = NULL;
	struct listnode *nextnode = NULL;
	SocketList_S *sockInfo = NULL;

	fd_set readfd,errorfd;
	int maxfd = 0;
	int ret = 0;
	struct timeval timeout = {0};

	prctl(PR_SET_NAME, (unsigned long)"NetServerThead", 0,0,0);
	if(NULL == arg){
		return NULL;
	}
	param = (NetServerParam_S *)arg;
	if(0 >= param->mainSockfd){
		return NULL;
	}
	head = (SocketList_S *)malloc(sizeof(SocketList_S));
	if(NULL == head){
		close(param->mainSockfd);
		return NULL;
	}
	memset(head,0,sizeof(SocketList_S));
	head->socketType = SOCKETTYPE_ENUM_NULL;
	list_init(&(head->_list));
	newnode =  (SocketList_S *)malloc(sizeof(SocketList_S));
	if(NULL == newnode){
		free(head);
		close(param->mainSockfd);
		return NULL;
	}
	newnode->sockfd = param->mainSockfd;
	newnode->socketType = SOCKETTYPE_ENUM_MAIN;
	list_add_tail(&(head->_list),&(newnode->_list));
	while(    bTsockFlag==KEY_TRUE){
		FD_ZERO(&readfd);
		FD_ZERO(&errorfd);
		maxfd = -1;
		list_for_each(node,&(head->_list)){
			sockInfo = (SocketList_S *)node_to_item(node,SocketList_S,_list);
			FD_SET(sockInfo->sockfd,&readfd); //添加描述符
			FD_SET(sockInfo->sockfd,&errorfd); 
			if(maxfd < sockInfo->sockfd) maxfd = sockInfo->sockfd;
		}
		timeout.tv_sec = 1;
		timeout.tv_usec = 500000;//select函数会不断修改timeout的值，所以每次循环都应该重新赋值[windows不受此影响]	
		ret  = select (maxfd + 1, &readfd, NULL, &errorfd, &timeout);
		if (ret > 0){
			//list_for_each(node, &(head->_list)){
			nextnode = NULL;
	 		for (node = head->_list.next; node != &(head->_list); ){
				sockInfo = (SocketList_S *)node_to_item(node,SocketList_S,_list);
				if (0 >= sockInfo->sockfd) {
					DF_DEBUG("why");
					continue;
				}
				nextnode = node->next;
				if(KEY_TRUE == CheckSelectExcept(&errorfd,sockInfo));
				else if(KEY_TRUE == CheckSelectRead(&readfd,head,sockInfo,param));
				node = nextnode;
			}
		}else if (ret == 0){
			//DF_DEBUG("Service Select Timeout");
			continue;
		}else {
			DF_DEBUG("select error");
			list_for_each(node, &(head->_list)){
				sockInfo = (SocketList_S *)node_to_item(node, SocketList_S, _list);
				struct stat tStat;
				if (0 != fstat(sockInfo->sockfd, &tStat)) {
					//DF_DEBUG("fstat %d error:%s\n", sockInfo->sockfd, strerror(errno));					
					if(param != NULL && param->closecb != NULL)
					{
						param->closecb(sockInfo->sockfd);
					}
					list_remove(&(sockInfo->_list));
					close(sockInfo->sockfd);
					free(sockInfo);
					break;
				}
			}
		}
		SleepMs(10);//程序空跑 CPU不至于100%
	}	
}
 
static int NetTcpServerSocketInit(int  serverport)
{
	unsigned short port = serverport;
	int connfd = 0,bindfd = 0;
	int listenfd = 0;
	int sockfd = 0;
	int opt = 1;
	
	struct sockaddr_in server_addr;	
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	//server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0)
	{
		DF_DEBUG("sockfd error!!!");
		return 0;
	}
	//设置端口复用
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
	bindfd = bind(sockfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr));
	if(bindfd != 0)
	{
		DF_DEBUG("bind error!!!");
		close(sockfd);
		return 0;
	}
	
	listenfd = listen(sockfd,10);
	if(listenfd != 0)
	{
		DF_DEBUG("listen error!!!!");		
		close(sockfd);
		return 0;
	}
	return sockfd;
}

int NetCreateTcpSer(NetWorkSerHandle_T *handle)
{
	pthread_t pthid;
	NetServerParam_S *param;
    bTsockFlag=KEY_TRUE;

	param = (NetServerParam_S *)malloc(sizeof(NetServerParam_S));
	param->mainSockfd = NetTcpServerSocketInit(handle->serverport);
	param->readcb = handle->readcb;
	param->checkendcb = handle->checkendcb;
	param->closecb = handle->closecb;
	if(param->mainSockfd <= 0){
		free(param);
		return KEY_FALSE;
	}
	handle->sockfd = param->mainSockfd;
	DF_DEBUG("NetCreateTcpSer");	
	pthread_create(&pthid, NULL, NetServerThead, (void *)param);
    pthread_detach(pthid);
	return KEY_TRUE;
}

/*
static int GetNetWorkCab(NetCab_T *info)
{
  struct ifaddrs *ifap0,*ifap;
  struct sockaddr_in *addr4;
  struct sockaddr_in6 *addr6;
  int ret;
  int i = 0;
  char buf[NI_MAXHOST];
  
  if(getifaddrs(&ifap0))
  {
    return -1;
  }
  
  for(ifap = ifap0;ifap!=NULL;ifap=ifap->ifa_next)
  {
    if(ifap->ifa_addr == NULL) continue;
    if((ifap->ifa_flags & IFF_UP) == 0) continue;
    if(AF_INET !=ifap->ifa_addr->sa_family) continue;
    if(AF_INET == ifap->ifa_addr->sa_family){ 
	  addr4 = (struct sockaddr_in *)ifap->ifa_addr;
      if(NULL != inet_ntop(ifap->ifa_addr->sa_family,(void *)&(addr4->sin_addr),buf,NI_MAXHOST))
      {
	  	if(0 == strcmp("127.0.0.1",buf) || 0 == strcmp("192.168.99.1",buf))continue;
 		sprintf(info->ethip[i],buf);   
		//printf("address =  %s\n",buf);
		i++;   
      }
    }
  }
  info->ethCount = i;
  //printf("count = %d\r\n",i);
  freeifaddrs(ifap0);
  return 0;
}*/

void *addMulicastTimerTask(void *arg)
{
	ExtParam_S *param = NULL;
	param = (ExtParam_S *)arg;
	struct ip_mreq mreq;
	prctl(PR_SET_NAME, (unsigned long)"addMulicastTimerTask", 0,0,0);
	while(    bTsockFlag==KEY_TRUE){
		NetCab_T info = {0};
		int i = 0;
		SleepMs(5000);
		param->getNetCabInfo(&info);
		for(i = 0;i < info.ethCount;i++){
		//	DF_DEBUG("ethip = %s\r\n",info.ethip[0]);
		//	DF_DEBUG("mulicastIp = %s\r\n",param->mulicastIp);
			memset(&mreq, 0,sizeof(struct ip_mreq));
			mreq.imr_multiaddr.s_addr = inet_addr(param->mulicastIp);
			mreq.imr_interface.s_addr = inet_addr(info.ethip[i]);
	//		DF_DEBUG("5S Checked Start\r\n");
			if (setsockopt(param->mainSockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq,sizeof(struct ip_mreq)) < 0) {
	//		  perror("Add Membership\r\n");
	//		  DF_DEBUG("setsockopt\n");
		    }
	//		DF_DEBUG("5S Checked End\r\n");
		}
	}

	free(param);
	return 0;
}

void *udpServerThead(void *arg)
{
	prctl(PR_SET_NAME, (unsigned long)"udpServerThead", 0,0,0);
	unsigned int socklen, n;
	/* 循环接收网络上来的组播消息 */
	NetCliInfo_T cliinfo;
	memset(&cliinfo,0,sizeof(NetCliInfo_T));
	NetCliInfo_T *cli = &cliinfo;
	NetServerParam_S *param = NULL;
	struct sockaddr_in peeraddr;
	param = (NetServerParam_S *)arg;
	socklen = sizeof(struct sockaddr_in);
	while(    bTsockFlag==KEY_TRUE) {
		//cli = (NetCliInfo_T *)malloc_checkout(sizeof(NetCliInfo_T));
		//if(NULL == cli){
		//	SleepMs(100);
		//	continue;
		//}
		memset(cli->recvmsg,0,sizeof(cli->recvmsg));
		
		n = recvfrom(param->mainSockfd, cli->recvmsg, DF_NET_MSGBUFLEN, 0,(struct sockaddr *) &peeraddr, &socklen);
		if (n < 0) {
			DF_DEBUG("recvfrom err in udptalk!\n");
		//	free(cli);
			continue;
		}else {
			/* 成功接收到数据报 */
			cli->fromPort = ntohs(peeraddr.sin_port);
			inet_ntop(AF_INET,&peeraddr.sin_addr,cli->fromIP,16); 
			//DF_DEBUG("SUCCESS RECV MSG\r\n");
			cli->recvLen = n;
			param->readcb(cli);
		//	free(cli);
		}
		
	}

	free(param);
}

int NetMulicastServerSocketInit(char *mulicastip,int mulicastPort)
{
	struct sockaddr_in peeraddr;
//	struct in_addr ia;
	int sockfd;
	unsigned int socklen;
//	struct hostent *group;
	struct ip_mreq mreq;
	int ret = -1;
	unsigned  char  one = 1; 
    char  sock_opt = 1; 
	
	/* 创建 socket 用于UDP通讯 */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		DF_DEBUG("socket creating err in udptalk\n");
		return KEY_FALSE;
	}

	/* 设置要加入组播的地址 */
	memset(&mreq, 0,sizeof(struct ip_mreq));
	mreq.imr_multiaddr.s_addr = inet_addr(mulicastip);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);

 	if((setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR, ( void  *) &sock_opt,  sizeof  (sock_opt)))== -1) { 
		DF_DEBUG("setsockopt\n");
     }  // 设置 允许重用本地地址和端口
	 
    if  ((setsockopt(sockfd, IPPROTO_IP,IP_MULTICAST_LOOP,&one,  sizeof  (unsigned  char ))) == -1){ 
		DF_DEBUG("setsockopt\n");
     }  //	
	/* 把本机加入组播地址，即本机网卡作为组播成员，只有加入组才能收到组播消息 */


	if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq,sizeof(struct ip_mreq)) < 0) {
		perror("Add Membership");
		DF_DEBUG("setsockopt\n");
		return -1;
	}

	socklen = sizeof(struct sockaddr_in);
	memset(&peeraddr, 0, socklen);
	peeraddr.sin_family = AF_INET;
	peeraddr.sin_port = htons(mulicastPort);
	peeraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//inet_pton(AF_INET, mulicastip, &peeraddr.sin_addr);
	/* 绑定自己的端口和IP信息到socket上 */
	if (bind(sockfd, (struct sockaddr *) &peeraddr,sizeof(struct sockaddr_in)) != 0 ){
		DF_DEBUG("Bind error\n");
		//assert(0);
		return -1;
	}
	
	return sockfd;
}


int NetCreateUdpMulicast(NetWorkSerHandle_T *handle)
{
	pthread_t pthid;
	pthread_t pthaddMulicast;
	NetServerParam_S *param;
	ExtParam_S *extParam = NULL;
	param = (NetServerParam_S *)malloc(sizeof(NetServerParam_S));
	extParam = (ExtParam_S *)malloc(sizeof(ExtParam_S));
	param->mainSockfd = NetMulicastServerSocketInit(handle->mulicastip,handle->mulicastPort);
	param->readcb = handle->readcb;
	if(param->mainSockfd <= 0){
		free(param);
		free(extParam);
		return KEY_FALSE;
	}
    bTsockFlag=KEY_TRUE;
	DF_DEBUG("NetCreateUdpMulicast!!!!");	
	handle->sockfd = param->mainSockfd;	
	extParam->mainSockfd = param->mainSockfd;	
	extParam->mulicastIp = handle->mulicastip;
	extParam->getNetCabInfo = handle->getNetCabInfo;
	
	pthread_attr_t attr;
		
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&pthaddMulicast, &attr, addMulicastTimerTask, (void *)extParam);
	pthread_attr_destroy (&attr);

	pthread_create(&pthid, NULL, udpServerThead, (void *)param);
	//pthread_create(&pthaddMulicast, NULL, addMulicastTimerTask,(void *)extParam);
    pthread_detach(pthid);
    //pthread_detach(pthaddMulicast);
	return 0;
}


int NetCloseSerSocket(NetWorkSerHandle_T *handle)
{
    bTsockFlag=KEY_FALSE;

	if(handle->sockfd <= 0){
		return KEY_FALSE;
	}
	close(handle->sockfd);
	handle->sockfd = -1;
	return KEY_TRUE;
}

int Packet_length(int iVideoLen ,int iMtu)
{
	int iLen = 0 , remainder = 0;
	iLen = iVideoLen / iMtu;
	remainder = iVideoLen % iMtu;
	if(remainder != 0)
	{
		iLen = iLen + 1;
	}
	return iLen;
}

int NetTcpMtuSendMsg(int sockfd, char *msg,int len)
{
	int i = 0;
	int ret = -1;
	int iSend = 0;
	int lenSend = 0;
	struct timeval tv; 
	fd_set writeset,errorfd; 
	int mtu = 1460;
	int iPackCount = 0;
	int i_payload = 0;

	iPackCount = Packet_length(len, mtu);
	for (i = 0; i < iPackCount; i++)
	{
		i_payload = ((len < mtu) ? len : mtu);
		tv.tv_sec = 0;//3;	 
		tv.tv_usec = 0;//500;	
		FD_ZERO(&errorfd);
		FD_ZERO(&writeset);    
		FD_SET(sockfd, &writeset);
		FD_SET(sockfd, &errorfd); 
		iSend = select(sockfd + 1, NULL, &writeset, &errorfd, &tv);
		if (iSend > 0)//3.5秒之内可以send，即socket可以写入	 
		{	
			if(0 != FD_ISSET(sockfd, &errorfd)){
				close(sockfd);
				return -1;
			}
			lenSend = send(sockfd,msg,i_payload,0);    
			if(lenSend == -1)	
			{	
				return -1;
			}	
		}
		else if (iSend == 0)
		{
			return -2;
		}
		else  //3.5秒之内socket还是不可以写入，认为发送失败	
		{			
			return -1;
		}
		msg += i_payload;
		len -= i_payload;			
	}
	return KEY_TRUE;
}

#if 1
int NetTcpSendMsg(int sockfd, char *msg,int len)
{
	int ret = -1;
	int iSend = 0;
	int Total = 0;    
	int lenSend = 0;
	int cont = 0;
	struct timeval tv;    
	fd_set wset,errorfd; 
	
	while(1)
	{
		FD_ZERO(&wset);    
		FD_ZERO(&errorfd);
		FD_SET(sockfd, &wset);
		FD_SET(sockfd, &errorfd);
		tv.tv_sec = 3;//3;	 
		tv.tv_usec = 0;//500;	
//		DF_DEBUG("select start");
		iSend = select(sockfd + 1, NULL, &wset, &errorfd, &tv);
//		DF_DEBUG("select end");
		if (iSend > 0)//3.5秒之内可以send，即socket可以写入	 
		{	
			if(0 != FD_ISSET(sockfd, &errorfd)){
				return -1;
			}
			if(0 == FD_ISSET(sockfd, &wset)){
				return -1;
			}
			lenSend = send(sockfd,msg + Total,len - Total,0);    
			if(lenSend <= 0)	
			{	
				ret = -1;
				break;
			}
			
			Total += lenSend;	
			if(Total == len)	
			{	
				ret = len;	
				break;
			}
		}
		else if (iSend == 0)
		{
			DF_ERROR("select Time out !");
			cont ++;
			if (2 == cont)
			{
				return -1;
			}
			continue;
		}
		else  //3.5秒之内socket还是不可以写入，认为发送失败	
		{
			DF_ERROR("select err !");
			ret = -1;
			if (sockfd > 0)
			{
			}
			break;
		}	
	}
//	DF_DEBUG("ret = %d",ret);
	return ret;
}
#else
int NetTcpSendMsg(int sockfd,char *msg,int len)
{
	int iret = 0;
	if(sockfd <= 0){
		return KEY_FALSE;
	}
	iret = send(sockfd, msg, len, 0);
	return iret;
}
#endif

int NetUdpSendMsg(char *ip, int port,char *msg,int len,char *ethname)
{
	struct sockaddr_in peeraddr;
	unsigned int socklen;
	int sockfd;
	/* 创建 socket 用于UDP通讯 */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd <= 0){
		return KEY_FALSE;
	}
	
	socklen = sizeof(struct sockaddr_in);
	memset(&peeraddr, 0, socklen);
	peeraddr.sin_family = AF_INET;
	peeraddr.sin_port = htons(port);
	inet_pton(AF_INET,ip, &peeraddr.sin_addr);
	
	//struct sockaddr_in addrSelf;//本地地址  
    //addrSelf.sin_addr.S_un.S_addr = inet_addr(ethIP);//指定网卡的地址  
    //addrSelf.sin_family = AF_INET;  
	//bind(sockfd , (struct sockaddr *)&addrSelf , sizeof(struct sockaddr_in));

#if defined(WIN32)
	
#else
	struct ifreq inter;
 	memset(&inter, 0x00, sizeof(inter));
	strncpy(inter.ifr_ifrn.ifrn_name,ethname,strlen(ethname));
//	DF_DEBUG("ethname = %s",ethname);
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE,(char *)&inter, sizeof(inter))  < 0) {
       perror("SO_BINDTODEVICE failed");
      /* Deal with error... */
	}
#endif

	sendto(sockfd,msg,len,0,(struct sockaddr *)&peeraddr,socklen);
	close(sockfd);
	return KEY_TRUE;
}


int NetUdpSendMsgBySocket(int sockfd,char *ip, int port,char *msg,int len)
{
	struct sockaddr_in peeraddr;
	unsigned int socklen;
	/* 创建 socket 用于UDP通讯 */
	if(sockfd <= 0){
		return KEY_FALSE;
	}
	
	socklen = sizeof(struct sockaddr_in);
	memset(&peeraddr, 0, socklen);
	peeraddr.sin_family = AF_INET;
	peeraddr.sin_port = htons(port);
	inet_pton(AF_INET,ip, &peeraddr.sin_addr);
	sendto(sockfd,msg,len,0,(struct sockaddr *)&peeraddr,socklen);
	return KEY_TRUE;
}

static int NonbConnect(int sockfd, const char *addr, const int port)
{
	int ret = 0;
	int flags = 0;
	int falgsbak = 0;
	int iRet = 0;
	int len = -1;
    int error = -1;
	fd_set wds;
	struct timeval tv;
	struct sockaddr_in SockAddr = {0};
	bzero(&SockAddr, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = htons(port);
	SockAddr.sin_addr.s_addr = inet_addr(addr); 
	if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1){
        printf("fcntl(F_GETFL, O_NONBLOCK)");
    }
	falgsbak = flags;
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1){
        printf("fcntl(F_SETFL, O_NONBLOCK)");
    }
	
	int sock_opt = 1;
	if((setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR, ( void  *) &sock_opt,  sizeof  (sock_opt)))== -1) { 
		DF_DEBUG("setsockopt\n");
     }  // 设置 允许重用本地地址和端口

	ret = connect(sockfd, (struct sockaddr *)&SockAddr, sizeof(SockAddr));
	if(0 > ret){    
        FD_ZERO(&wds);
        FD_SET(sockfd,&wds);
        tv.tv_sec = 1;
        tv.tv_usec = 3000 * 1000;
        iRet = select(sockfd + 1, NULL, &wds, NULL, &tv);
        if(iRet>0 && FD_ISSET(sockfd,&wds)){
            len = sizeof(error);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, (socklen_t *)&len);
			if(0 != error){
				ret =  -1;
			} else {
				ret = 0;
			}
       	}
	}
	return ret;
}

//NetTcpGetMsg 返回值 KEY_FALSE 为出错,返回 >0 为recv 长度。 = 0 为套接字关闭
//此接口为简单网络接口,请readBUF,尽可能大
//timeOut ms
int NetTcpGetMsg(char *ip,int port,char *sendBuf,int sendLen,char *readBuf,int readBufLen,int timeOut,CheckDataComplet checkdata) 
{
    int sockfd = 0;
    int cLen = 0;
	int countLen = 0;
	int errnoCount = 0;
	int on = 1;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	fd_set readfd,errorfd;
	struct timeval timeout = {0};
	int ret = -1;
    if(sockfd < 0)
    {
        DF_DEBUG("socket() failure!\n");
        return KEY_FALSE; 
    }

#if 0
    if(NonbConnect(sockfd, ip, port) < 0)
    {
        DF_DEBUG("connect() failure!\n");
		close(sockfd);
        return KEY_FALSE;
    }
#else
	struct hostent *host;
	struct sockaddr_in SockAddr = {0};
	host = gethostbyname(ip);
	if(host == NULL)
	{
		DF_ERROR("get host name fialed!,%s\n",ip);
		return -1;
	}	
	bzero(&SockAddr, sizeof(SockAddr));
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(port);
	memcpy(&(SockAddr.sin_addr.s_addr),host->h_addr_list[0],4);
	int sock_opt = 1;
	struct timeval time = {3, 0};
    socklen_t len = sizeof(time);
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &time, len);
	if((setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR, ( void  *) &sock_opt,  sizeof  (sock_opt)))== -1) { 
		DF_DEBUG("setsockopt\n");
     }  // 设置 允许重用本地地址和端口
	ret = connect(sockfd, (struct sockaddr *)&SockAddr, sizeof(SockAddr));
	if (0 > ret)
	{
		DF_ERROR("connect() failure!");
		close(sockfd);
		return KEY_FALSE;
	}
#endif
	
	int tempLen = 0;
	while(sendLen - tempLen > 0)
	{
		cLen = send(sockfd,sendBuf + tempLen, sendLen - tempLen, 0);
		if(-1 != cLen)
		{
			tempLen = tempLen + cLen;
		}
		else
		{			
			printf("Error: %s \n", strerror(errno));
			if (errnoCount >= 5) // 5秒发送不出去默认关闭
			{
			//	break;
			}
			
			if(errno == EINTR)//发送缓冲区已满，或者被中断了
			{
				continue;
			}
			else if (errno == EAGAIN)
			{
				errnoCount ++;
				usleep(1 * 1000 * 1000);
				continue;
			}
			
			if (errno == ECONNRESET || errno == EPIPE)
			{
				//对方已经断开了！！
			} 
			break;
		}
		//printf("sendlen1 = %d tempLen = %d cLen = %d\n",sendLen,tempLen,cLen);
	}
	//printf("tempLen = %d\n",tempLen);
	if (NULL == readBuf || 0 >= readBufLen)
	{
		close(sockfd);
        return cLen;
	}
	cLen = 0;
	memset(readBuf, 0, readBufLen);
	while(    bTsockFlag==KEY_TRUE)
	{
		FD_ZERO(&readfd);
		FD_ZERO(&errorfd);
		FD_SET(sockfd,&readfd); //添加描述符
		FD_SET(sockfd,&errorfd); 
		timeout.tv_sec = timeOut/1000;
		timeout.tv_usec = (timeOut%1000)*1000;//select函数会不断修改timeout的值，所以每次循环都应该重新赋值[windows不受此影响]	
		ret = select (sockfd + 1, &readfd, NULL, &errorfd, &timeout);
		if (ret > 0)
		{
			if(0 != FD_ISSET(sockfd, &errorfd)){
				DF_DEBUG("errorfd");
				close(sockfd);
				return KEY_FALSE;
			}		
			
			if(0 != FD_ISSET(sockfd, &readfd))
			{
				//DF_DEBUG("recv start");
				cLen = recv(sockfd, readBuf+countLen, readBufLen - countLen,0);  
				countLen += cLen;
				//DF_DEBUG("recv end");
				if(NULL != checkdata)
				{
					//DF_DEBUG("checkdata 1");
					if(KEY_TRUE == checkdata(readBuf,countLen))
					{
						//DF_DEBUG("checkdata KEY_TRUE");
						close(sockfd);
						return countLen;
					}
				//DF_DEBUG("checkdata 2");
				}
				if(cLen < 0 || cLen == 0)
				{
					DF_DEBUG("cLen error");
					break;
				}
				if(NULL == checkdata)
				{					
					close(sockfd);
					return countLen;
				}
				///DF_DEBUG("not enough");
			}
			DF_DEBUG("why?????");
		}
		else if (ret == 0)
		{
			DF_DEBUG("Client  Recv Select Timeout");
			break;
		}
		else 
		{
			DF_DEBUG("select error");
			break;
		}
	}
    close(sockfd);
    return KEY_FALSE;
}

int NetTcpConnect(char *ip,int port) 
{
    int sockfd = 0;
    int cLen = 0;
	int countLen = 0;
    struct sockaddr_in cli;
	int on = 1;
	struct hostent *host;
	host = gethostbyname(ip);
	if(host == NULL)
	{
		DF_DEBUG("get host name fialed!,%s\n",ip);
		return -1;
	}
    cli.sin_family = AF_INET;
    cli.sin_port = htons(port);
	
	memcpy(&(cli.sin_addr.s_addr),host->h_addr_list[0],4);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        DF_DEBUG("socket() failure!\n");
        return KEY_FALSE; 
    }
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	int keepAlive = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));
    if(connect(sockfd, (struct sockaddr*)&cli, sizeof(cli)) < 0)
    {
      //  DF_DEBUG("connect() failure!\n");
		close(sockfd);
        return KEY_FALSE;
    }
    return sockfd;
}

