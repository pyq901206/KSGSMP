
#include <stdio.h>
#include <string.h>


#ifdef WIN32
#include <Winsock2.h>
#include <windows.h>
#include "thirdpart/ppcs/include/PPCS/PPCS_API.h"
#pragma comment(lib, "PPCS_API.lib")
#include "SwapOS.h"
#endif //// #ifdef WIN32
#include "common.h"

#include "p2p_swap.h"
#include "PPCS_API.h"
#include "p2psocket.h"


#ifndef printf_log
#define printf_log (printf("[ %s,%d]=> ",__FUNCTION__,__LINE__), printf)  
#endif

#ifndef printf_err
#define printf_err (printf("[ %s,%d]=>error:",__FUNCTION__,__LINE__), printf)  
#endif
static int  s_P2PFlag=-1;

int p2p_network_detect(void*arg)
{
    int ret;
    st_PPCS_NetInfo NetInfo;
    ret = PPCS_NetworkDetect(&NetInfo, 0);
    printf_log("PPCS_NetworkDetect() ret = %d\n", ret);

    printf_log("-------------- NetInfo: -------------------\n");
    printf_log("Internet Reachable     : %s\n", (NetInfo.bFlagInternet == 1) ? "YES":"NO");
    printf_log("P2P Server IP resolved : %s\n", (NetInfo.bFlagHostResolved == 1) ? "YES":"NO");
    printf_log("P2P Server Hello Ack   : %s\n", (NetInfo.bFlagServerHello == 1) ? "YES":"NO");

    char nat_type[64];
	memset(nat_type,0x0,sizeof(nat_type));
    switch(NetInfo.NAT_Type)
    {
		case 1: strncpy(nat_type,"IP-Restricted Cone type",sizeof(nat_type));break;
		case 2: strncpy(nat_type,"Port-Restricted Cone type",sizeof(nat_type));break;
		case 3: strncpy(nat_type,"Symmetric  Cone type",sizeof(nat_type));break;
		case 0:
		default:
			strncpy(nat_type,"unknown",sizeof(nat_type));
		break;
    }
    printf_log("NAT Type	: %s\n",nat_type);
    printf_log("My LAN IP	: %s\n",NetInfo.MyLanIP);
    printf_log("My WAN IP	: %s\n",NetInfo.MyWanIP);

    return ret;
}
int p2p_Check_LoginStatus( void )
{
	char loginStatus;
	if(( P2P_SUCCESSFUL == PPCS_LoginStatus_Check(&loginStatus))  )
	{
	    if(1==loginStatus)
	    {
    		 //printf("P2P login status :%d\n", loginStatus );
    		 return 1;
		}
	}
	 //printf("P2P login status :%d\n", loginStatus );
	return 0;
}
void p2p_server_reconnect()
{
	p2p_listen_break();
	p2p_network_detect(NULL);
}

//p2p func swap
int p2p_sys_client_init(void)
{
    int ret;
    unsigned int APIVersion = PPCS_GetAPIVersion();
    ret = PPCS_Initialize((CHAR*)"AAGDEIBIPJJNCKJDBEGBBABAHHJOGBNFCLEAAPCAEBIJPCLPHOBLGMLGHPPAIDKNFLNMKNCJKCNCEDDGNBMIMDELNGLLACCK");//ip
    printf_log("PPCS_API Version: %d.%d.%d.%d ret=%d\n", (APIVersion & 0xFF000000)>>24, (APIVersion & 0x00FF0000)>>16, (APIVersion & 0x0000FF00)>>8, (APIVersion & 0x000000FF) >> 0 ,ret);
	ret=p2p_network_detect(NULL);

    return ret;
}

int p2p_sys_init(void)
{
    int ret;
	if(s_P2PFlag>0)
	{
		return 0;
	}
    ret = p2p_sys_client_init();
    //PPPP_Share_Bandwidth(1);
    if(0==ret)
	{
		s_P2PFlag=1;
	}
    return ret;
}

int p2p_sys_deinit(void)
{
	if(s_P2PFlag<0)
	{
		return 0;
	}
	s_P2PFlag=-1;
	return PPCS_DeInitialize();
}

int p2p_close(int fd)
{
	printf_log("fd=%d !!\n",fd);
	return PPCS_ForceClose(fd);
}

int p2p_read(int fd, int chn, char *buf, int *data_size, int timeout)
{
    int ret;

    ret = PPCS_Read(fd, chn, buf, data_size, timeout);

    if(ret == P2P_SESSION_CLOSED_TIMEOUT)
    {
    	printf_log("Session TimeOUT!!\n");
    }
    else if(ret == P2P_SESSION_CLOSED_REMOTE)
    {
    	printf_log("Session Remote Close!!\n");
    }

	if (ret != P2P_SUCCESSFUL)
	{
		printf_err("p2p check buf[%d:%d] ret err %d\n",fd,chn ,ret);
	}
    return ret;
}

int p2p_write(int fd, int chn, char *buf, int data_size)
{
    int ret;
    unsigned int write_size;
__check_buf:

	ret = PPCS_Check_Buffer(fd, chn, &write_size, NULL);

    if(ret == P2P_SESSION_CLOSED_TIMEOUT)
    {
		printf_log("Session  TimeOUT!\n");
    }
    else if(ret == P2P_SESSION_CLOSED_REMOTE)
    {
		printf_log("Session Remote Close!\n");
    }   

	if (ret != P2P_SUCCESSFUL)
    {
		//printf_err("p2p check[%d:%d] buf ret err %d\n",fd,chn ,ret);
		return ret;
    }
    if (write_size > 256000)
    {
		printf_log("write buf if too big %d, wait!\n", write_size);
		SleepMs(10);
		goto __check_buf;
    }


    ret = PPCS_Write(fd, chn, buf, data_size);

    if(ret == P2P_SESSION_CLOSED_TIMEOUT)
    {
		printf_log("Session TimeOUT!!\n");
    }
    else if(ret == P2P_SESSION_CLOSED_REMOTE)
    {
		printf_log("Session Remote Close!!\n");
    }
    return ret;
}

int p2p_listen(const char *mydid, int timeout, int port, int flag, const char *lisence)
{
    int ret;
     ret=PPCS_Listen(mydid, timeout, port, flag, lisence);
	if((ret == P2P_TIME_OUT)||(ret ==P2P_USER_LISTEN_BREAK))
    {
//	    printf_log("p2p listen timeout\n");
    }    	    
	else if(ret == P2P_MAX_SESSION)
	{
	    printf_log("p2p listen max session!!!,ret = %d\n", ret);
		SleepMs(2000);
	}
	else if(ret == P2P_SESSION_CLOSED_REMOTE)
	{
		printf_log("p2p server dropped!!!,ret = %d\n", ret);
		p2p_server_reconnect();
		SleepMs(1000);
	}
	else if(ret < 0)
	{
	    printf_log("p2p listen failed,ret = %d\n", ret);
		p2p_server_reconnect();	
		SleepMs(1000);
	}
	return ret;
}
int p2p_listen_break(void)
{
    return PPCS_Listen_Break();
}
int p2p_check_fd(int fd)
{
    int ret;

    st_PPCS_Session Sinfo;
    ret = PPCS_Check(fd, &Sinfo);

    if(ret == P2P_SUCCESSFUL)
    {
		printf_log("-------Session Ready (%d): -%s------------------\n", ret,(Sinfo.bMode ==0)? "P2P":"RLY");
		printf_log("Socket : %d\n", Sinfo.Skt);
		//printf_log("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
    }
    return ret;
}

int p2p_connect(const char *dev_did, char lan_search, unsigned short port)
{
	return PPCS_Connect(dev_did, lan_search, port);
}


int p2p_check_buffer(int fd, int chn, unsigned int *wsize, unsigned int *rsize)
{
	return PPCS_Check_Buffer(fd, chn, wsize, rsize);
}

static int P2P_P2P_Regist(P2PHandle_T *handle)
{
	handle->initcb= p2p_sys_init;
	handle->deinitcb= p2p_sys_deinit;
	handle->listencb = p2p_listen;
	handle->listenbreakcb= p2p_listen_break;

	handle->connetcb = p2p_connect;
	handle->closecb = p2p_close;
	handle->readcb = p2p_read;

	handle->writecb = p2p_write;
	handle->checkbufcb = p2p_check_buffer;

	return KEY_TRUE;
}

static int P2P_P2P_UnRegist(P2PHandle_T *handle)
{
    memset(handle,0x0,sizeof(P2PHandle_T));
	return KEY_TRUE;
}

_OUT_ int P2P_P2P_Init()//P2PHandle_T *handle)
{
    p2p_sys_init();
//	P2P_P2P_Regist(handle);
	return KEY_TRUE;
}

_OUT_ int P2P_P2P_UnInit()//P2PHandle_T *handle)
{
//	P2P_P2P_UnRegist(handle);
	p2p_listen_break();
	p2p_sys_deinit();
	return KEY_TRUE;
}

