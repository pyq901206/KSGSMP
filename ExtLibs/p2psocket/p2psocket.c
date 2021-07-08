
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
#include <sys/prctl.h>

#include "list.h"
#include "common.h"
#include "p2p_swap.h"
#include "p2psocket.h"


#ifndef MIN
#define MIN(x,y)	((x)<(y)?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y)	((x)>(y)?(x):(y))
#endif


#define P2P_SERVER_STATUS_RUN (1)
#define P2P_SERVER_STATUS_STOP (0)
#define P2P_SERVER_STATUS_ERR (-1)

#define P2P_SERVER_CRCKEY ":iNewCam"

typedef enum _P2PTYPE_E{
	P2P_ENUM_NULL   = 0x00,
	P2P_ENUM_P2P	= 0x01,
	P2P_ENUM_TCP	= 0x02,
	P2P_ENUM_BUTT	= (P2P_ENUM_TCP+1),

}P2PTYPE_E;


typedef struct _P2pSocketList_S{
	int 					sockfd;
	P2PTYPE_E 			socketType; 
	char					ip[16];
	int						port;
    P2PCliInfo_T            *cli;
	struct					listnode _list;
}P2pSocketList_S;



typedef struct _P2pServerParam_S{
    char aszfunname[32];
    char aszdid[32];
    char aszlisence[8];
	int mainSockfd;
	int  serverport;
    int servlistenstatus;
	pthread_t pthlistenid;

    P2PTYPE_E enP2Ptype;

	cbP2Pcheckcb 	checkendcb;
	cbP2PProccb     procdatacb;
    cbP2Pclosecb    closecb;

}P2pServerParam_S;

int p2pSendMsg(int fd, int chn,  char *buf, int data_size)
{
    return  p2p_write(fd,chn,buf,data_size);
}

int usr_Check_Buffer(int fd, int chn, unsigned int *wsize)
{
	return p2p_check_buffer(fd, chn, wsize,NULL);
}

//cli recv p2p 接收线程在p2pbreak后.链接也会断开
static void *P2PRecvThead(void *arg)
{

	if(NULL == arg)
	{
		return NULL;
	}

    int s32Ret = -1;
    int chn=0;
    P2PCliInfo_T    *cli = (P2PCliInfo_T *)arg;

    prctl(PR_SET_NAME, (unsigned long)"P2PRecvThead", 0,0,0);	    	
    DF_DEBUG("%s pthrd=%x fd=%x start!\n",__FUNCTION__,cli->pthrdrecvid,cli->recvSocket);

    while((0!=cli->CliStatus))
    {
        SleepMs(10);
        {     
    		int Readsize = 0;
           for(chn=0;chn<MAX_P2P_CHNNUM;chn++)
            {
    			Readsize = 0;
      			s32Ret = p2p_check_buffer(cli->recvSocket, chn, NULL, (unsigned int *)&Readsize);
      			if (P2P_SUCCESSFUL != s32Ret)
    			{
//    				DF_DEBUG("[%d:%d]=%d  fail!\n", cli->recvSocket, chn, s32Ret);
    				if(s32Ret != P2P_TIME_OUT)
    				{
        				DF_DEBUG("[%d:%d]=%d  fail!\n", cli->recvSocket, chn, s32Ret);
        				s32Ret=-1;
    					break;
    				}
				}
				else
    			{
    				if (Readsize>0)
    				{
        			    if(chn>cli->chnnum)
        			    {
             				DF_DEBUG("%s:[%d:%d:%d]=%d  XX!\n",__FUNCTION__,cli->recvSocket, chn, cli->chnnum,Readsize);
             				continue;
           			    }
           			    BufInfo_S *bufinf=&(cli->bufinfo);
           			    
         			    if(Readsize> 0 &&(bufinf->buflen == bufinf->writeLen))
  						{
             				DF_DEBUG("%s:[%d:%d]=%d  too MAX !\n",__FUNCTION__,cli->recvSocket, chn,Readsize);
						} 
						
						Readsize=MIN(Readsize,(bufinf->buflen - bufinf->writeLen));
                        // 接收数据 
            			s32Ret = p2p_read(cli->recvSocket, chn, (bufinf->buf+bufinf->writeLen), (unsigned int *)&Readsize,5000);
    					if (P2P_SUCCESSFUL != s32Ret)
    					{
            				DF_DEBUG("[%d:%d]=%d  fail!\n", cli->recvSocket, chn, s32Ret);
            				s32Ret=-1;
        					break;
        				}
    					if( (P2P_SUCCESSFUL == s32Ret)||(P2P_TIME_OUT == s32Ret))
    					{
    						bufinf->writeLen+=Readsize;
    					}
				         //检查数据是否有完整包
    					while(KEY_TRUE==cli->checkendcb(cli->recvSocket,cli->bufinfo.buf,cli->bufinfo.writeLen))
    					{
                         //处理数据
    					    s32Ret=cli->procdatacb(cli->recvSocket,cli->bufinfo.buf,&(cli->bufinfo.writeLen));
    					    if(KEY_FALSE==s32Ret)
    					    {
    					       DF_DEBUG("procdata fail=%d",s32Ret);
    					       break;
    					    }
    					}
    				}
    			}
			}
			
			if(KEY_FALSE==s32Ret)
			{
                DF_DEBUG("%s pthrd=%x fd=%x exit!\n",__FUNCTION__,cli->pthrdrecvid,cli->recvSocket);
			    cli->CliStatus=0;
			    break;
			}
		}
    }
    p2p_close(cli->recvSocket);
    cli->closecb(cli->recvSocket);
    free(cli);
    return 0;
}

//listen 线程
static void *P2PServerThead(void *arg)//static void *tcp_listen_pthread(void *arg)
{

	P2pServerParam_S *param = NULL;
	char aszdid[32];
	char aszlisence[32];
    int fd;
    int ret;
	if(NULL == arg)
	{
		return NULL;
	}
	param = (P2pServerParam_S *)arg;
//    P2PHandle_T *p2phandle=&(param->p2phandle);

    prctl(PR_SET_NAME, (unsigned long)"P2PServerThead", 0,0,0);	    	

    memcpy(aszdid,param->aszdid,strlen(param->aszdid));
    memcpy(aszlisence,param->aszlisence,strlen(param->aszlisence));
    strcat(aszlisence, P2P_SERVER_CRCKEY);

	DF_DEBUG(" %s  :%s\n",aszdid,aszlisence);

    while(P2P_SERVER_STATUS_RUN==param->servlistenstatus)
    {
        fd=p2p_listen(aszdid, 60, 0, 1, aszlisence);
    	if(fd < 0)
    	{
    	    DF_DEBUG(" %s err=%d!,status:%x\n",__FUNCTION__,fd,param->servlistenstatus);
    	}
    	else
    	{
            // 有新用户加入,添加到接收线程;
           	P2PCliInfo_T *cli =(P2PCliInfo_T *)malloc(sizeof(P2PCliInfo_T));
        	if(NULL == cli)
        	{
        	    DF_DEBUG("%s %d,malloc cli fail!\n",param->aszfunname,fd);
        	    p2p_close(fd);
        	}
        	//先写死,只处理一个通道;
        	memset(cli,0x0,sizeof(P2PCliInfo_T));
        	{
             	cli->chnnum=0;
             	cli->recvSocket=fd;
            	cli->bufinfo.buflen=DF_NETP2P_MSGBUFLEN;
            	cli->bufinfo.writeLen=0;
            	cli->CliStatus=1;
            	cli->checkendcb=param->checkendcb;
            	cli->procdatacb=param->procdatacb;
            	cli->closecb=param->closecb;
        	}
        	
        	pthread_attr_t pAttr;
        	pthread_attr_init(&pAttr);
        	pthread_attr_setdetachstate(&pAttr,PTHREAD_CREATE_DETACHED);
        	ret=pthread_create(&cli->pthrdrecvid, NULL, P2PRecvThead, (void *)cli);
        	pthread_attr_destroy(&pAttr);
        	if(0!=ret)
        	{
            	DF_DEBUG("%s %d,create cli pthread fail!\n",param->aszfunname,fd);
            	free(cli);cli=NULL;
            	p2p_close(fd);
        	}

     	    DF_DEBUG("%s %s recv fd=%d!\n",__FUNCTION__,param->aszfunname,fd);
   	    }

    }
    DF_DEBUG("%s %s exit!\n",__FUNCTION__,param->aszfunname);
    return 0;
}


int p2pCreatep2pSer(P2PSerHandle_T *handle)
{
	P2pServerParam_S *param=NULL;

	DF_DEBUG("%s","start");	
	param = (P2pServerParam_S *)malloc(sizeof(P2pServerParam_S));

	memset(param,0x0,sizeof(P2pServerParam_S));

	memcpy(param->aszdid,handle->aszdid,strlen(handle->aszdid));
	memcpy(param->aszlisence,handle->aszlisence,strlen(handle->aszlisence));
	snprintf(param->aszfunname,sizeof(param->aszfunname),__FUNCTION__);

	param->serverport=0;

    param->checkendcb=handle->checkendcb;
    param->procdatacb=handle->procdatacb;
    param->closecb=handle->closecb;

	param->enP2Ptype=P2P_ENUM_P2P;
	
	param->servlistenstatus=P2P_SERVER_STATUS_RUN;
	pthread_create(&param->pthlistenid, NULL, P2PServerThead, (void *)param);
	
	handle->servpara=param;
	DF_DEBUG("end");	

	return KEY_TRUE;
}

int p2pStopp2pSer(P2PSerHandle_T *handle)
{
	P2pServerParam_S *param=(P2pServerParam_S *)handle->servpara;
	param->servlistenstatus=P2P_SERVER_STATUS_STOP;

    p2p_listen_break();
    pthread_join(param->pthlistenid,NULL);

    free(handle->servpara);handle->servpara=NULL;
	DF_DEBUG("%s end",__FUNCTION__);	
	return KEY_TRUE;
}




