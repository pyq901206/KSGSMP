#include <stdio.h>
#include <stdlib.h>


#if(defined LINUX )
//#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#elif(defined WIN32 )
#include "winsock2.h"
#include "windows.h"

#pragma commnet(lib,"ws2_32.lib")

#define close	(closesocket)
#define read 	(recv)
#define write	(send)
#define  MSG_NOSIGNAL	(0)
#define  MSG_DONTWAIT	(0)
#endif  /* LINUX */

#include "common.h"
#include "tcp_swap.h"

#ifndef printf_log
#define printf_log (printf("[ %s,%d]=> ",__FUNCTION__,__LINE__), printf)  
#endif

#ifndef printf_err
#define printf_err (printf("[ %s,%d]=>error:",__FUNCTION__,__LINE__), printf)  
#endif


static int  s_tcpFlag=-1;

int tcp_sys_init(void)
{
    int ret = 0;
	if(s_tcpFlag>0)
	{
		return 0;
	}
#if(defined WIN32 )
	{
		int err = 0;
		WORD wVersionRequested;
		WSADATA wsaData;

		wVersionRequested = MAKEWORD( 2, 2 );

		err = WSAStartup( wVersionRequested, &wsaData );
		if ( err != 0 ) 
		{
			return -1;
		}
		if ( LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 2 ) 
		{
			(VOID)WSACleanup( );
			return -1; 
		}
	}
#endif  /* WIN32 */
	s_tcpFlag=1;
    return 0;
}

int tcp_sys_deinit(void)
{
	if(s_tcpFlag<0)
	{
	    return 0;
	}

    #if(defined WIN32 )
    WSACleanup();
    #endif  /* WIN32 */
	
	s_tcpFlag=-1;
    return 0;
}

int tcp_close(int fd)
{
    return close(fd);
}

int tcp_read(int fd, int chn, char *buf, int *data_size, int timeout)
{
    int ret = 0;
    size_t len = (size_t)*data_size;
	fd_set stFdSet;
    struct timeval TimeoutVal; 
	
	FD_ZERO(&stFdSet);    
	FD_SET(fd, &stFdSet);
	TimeoutVal.tv_sec = 1;
	TimeoutVal.tv_usec = 0;
	if((ret = select(0, &stFdSet, NULL, NULL, &TimeoutVal)) < 0)
	{
		//int err = GetLastError();
		*data_size = 0;
		return TCP_SESSION_CLOSED_CALLED;
	}
	else if(ret == 0)
	{
		*data_size = 0;
		ret = TCP_SUCCESSFUL;
	}
	else
	{
	    ret = read(fd, (char *)buf, len,0);
	    if(ret < 0)
		{
			ret = TCP_SESSION_CLOSED_REMOTE;
		}
	    else
	    {
			*data_size = ret;
			ret = TCP_SUCCESSFUL;
	    }
	}
    return ret;
}

int tcp_write(int fd, int chn,  char *buf, int data_size)
{
    int ret = TCP_SUCCESSFUL;

    ret = write(fd, (char *)buf, data_size,0);
    if(ret < 0)
    {
		printf_log("write failed: %d!!\r\n",ret);
		ret = TCP_SESSION_CLOSED_REMOTE;
    }
    else if(ret!=data_size)
    {
		printf_log("write [%d] != data[%d]!\r\n",ret,data_size);
//		ret = TCP_SESSION_CLOSED_REMOTE;
    }
    return ret;
}

int tcp_connect(const char *devIp, char lan_search, unsigned short port)
{
    int sfd = -1;
    int ret;
    struct sockaddr_in servaddr;

    sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	printf_log("%s:connect sfd:(%d)\r\n", __FUNCTION__, sfd);
    if(sfd < 0)
    {
        printf_log("tcp make socket is not ok\r\n");
        return -1;
    }
//    bzero(&servaddr,sizeof(struct sockaddr_in));
    servaddr.sin_family =  AF_INET;
    servaddr.sin_port   =  htons(port);
    servaddr.sin_addr.s_addr   = inet_addr(devIp);

    ret = connect(sfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr));
    if(ret < 0)
    {
        printf_log("connect err:(%d)\r\n", ret);
        return TCP_NOT_INITIALIZED;
    }

	return sfd;
}

int tcp_check_buffer(int fd, int chn, unsigned int *wsize, unsigned int *rsize)
{
    int ret = 0;
    char buf[8]={0};
	fd_set readfd;    
	fd_set writefd;    
    struct timeval TimeoutVal;

	if(wsize!=NULL)
		{*wsize=0;}
	if(rsize!=NULL)
		{*rsize=0;}
	
	if(0!=chn)
	{
		return TCP_SUCCESSFUL;
	}

	if(rsize!=NULL)
	{
		TimeoutVal.tv_sec = 0;
		TimeoutVal.tv_usec = 5000;
		FD_ZERO(&readfd);    
	    FD_SET(fd,&readfd);   
	    ret = select(0,&readfd,NULL,NULL,&TimeoutVal);    
	    if(ret < 0)
	    {
	    	printf_log("!!!readfd [%d] is bad\r\n",fd);
	    	return TCP_NOT_INITIALIZED;
	    }
		if( FD_ISSET(fd, &readfd))
		{
			*rsize=32*1024;
		}
	}
	
	if(wsize!=NULL)
	{
		TimeoutVal.tv_sec = 0;
		TimeoutVal.tv_usec = 5000;
		FD_ZERO(&writefd);    
	    FD_SET(fd,&writefd);   
	    ret = select(0,NULL,&writefd,NULL,&TimeoutVal);    
	    if(ret < 0)
	    {
	    	printf_log("!!!writefd [%d] is bad\r\n",fd);
	    	return TCP_NOT_INITIALIZED;
	    }
		if( FD_ISSET(fd, &writefd))
		{
			*wsize=1024;
		}
	}
    return TCP_SUCCESSFUL;
}


/*
int tcp_listen(const char *mydid, int timeout, int port, int flag, const char *lisence)
{
	unsigned short servport = port;
	int connfd = 0,bindfd = 0;
	int listenfd = 0;
	int sockfd = 0;
	int opt = 1;
	
	struct sockaddr_in server_addr;	
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(servport);
	//server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0)
	{
		printf_log("sockfd error!!!");
		return 0;
	}
	//ÉèÖÃ¶Ë¿Ú¸´ÓÃ
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
	bindfd = bind(sockfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr));
	if(bindfd != 0)
	{
		printf_log("bind error!!!");
		close(sockfd);
		return 0;
	}
	
	listenfd = listen(sockfd,10);
	if(listenfd != 0)
	{
		printf_log("listen error!!!!");		
		close(sockfd);
		return 0;
	}
	return sockfd;
}
*/
static int sfd = -1;
static struct sockaddr_in servaddr;
int tcp_listen(const char *mydid, int timeout, int port, int flag, const char *lisence)
{

    if(sfd < 0)
    {
    	sfd = socket(AF_INET,SOCK_STREAM,0);
    	if(sfd < 0)
    	    return -1;
    	servaddr.sin_family = AF_INET;
    	servaddr.sin_port = htons(port);
    	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 //       printf("server ip:%u\n", servaddr.sin_addr.s_addr);
 //       SLOG(ERR,"server ip:%u\n",servaddr.sin_addr.s_addr);
    	if(bind(sfd,(const struct sockaddr *)&servaddr,sizeof(servaddr))<0)
    	    return -1;
        
    	if(timeout > 0)
    	{
    	    struct timeval tout;
    	    tout.tv_sec = timeout;
    	    tout.tv_usec = 0;
    	    setsockopt(sfd,SOL_SOCKET,SO_RCVTIMEO,&tout,sizeof(tout));
    	    setsockopt(sfd,SOL_SOCKET,SO_SNDTIMEO,&tout,sizeof(tout));
    	}
        
    	listen(sfd,5);
    }
    
    int client_fd = -1;
    struct sockaddr_in client_addr;
    socklen_t cliaddr_len;

    client_fd = accept(sfd,(struct sockaddr*)&client_addr,&cliaddr_len);
    if(client_fd < 0)
    	return -1; 

    return client_fd;
}


static int P2P_TCP_Regist(P2PHandle_T *handle)
{
	handle->initcb= tcp_sys_init;
	handle->deinitcb= tcp_sys_deinit;
	handle->listencb = tcp_listen;

	handle->connetcb = tcp_connect;
	handle->closecb = tcp_close;
	handle->readcb = tcp_read;

	handle->writecb = tcp_write;
	handle->checkbufcb = tcp_check_buffer;

	return KEY_TRUE;
}

static int P2P_TCP_UnRegist(P2PHandle_T *handle)
{
	handle = NULL;
	return KEY_TRUE;
}

_OUT_ int P2P_TCP_Init(P2PHandle_T *handle)
{
    tcp_sys_init();
	P2P_TCP_Regist(handle);
	return KEY_TRUE;
}

_OUT_ int P2P_TCP_UnInit(P2PHandle_T *handle)
{
	P2P_TCP_UnRegist(handle);
	tcp_sys_deinit();
	return KEY_TRUE;
}

