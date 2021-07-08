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
	int  abrenable;  	 //��������
    int  servport;   	 //�˿�
  	char servaddr[128];  //��������ַ
    char streamurl[128]; //����������
    char name[32];		 //�������û���
    char passwd[32];	 //����
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
