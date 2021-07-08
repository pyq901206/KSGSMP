#ifndef __RTMPSTART_H__
#define __RTMPSTART_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif     /* __cpluscplus */
#endif  /* __cpluscplus */

#define RTMP_USR_NUM 256

typedef struct _RtmpConfParm_T
{
	int  abrenable;  	 //码率适配
    int  servport;   	 //端口
  	char servaddr[128];  //服务器地址
    char streamurl[128]; //服务器名称
    char name[32];		 //服务器用户名
    char passwd[32];	 //密码
}RtmpConfParm_T;


int RtmpInit();
int RtmpDeInit();
int RtmpSetSerconfig(RtmpConfParm_T *rtmp);
int RtmpGetSerconfig(RtmpConfParm_T *rtmp);

#ifdef __cplusplus
#if __cplusplus
}
#endif     /* __cpluscplus */
#endif  /* __cpluscplus */


#endif
