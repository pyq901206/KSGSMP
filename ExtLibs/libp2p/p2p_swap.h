
#ifndef _P2P_SWAP_H_
#define _P2P_SWAP_H_

//#include "p2psocket.h"

/*
#include "PPCS_Error.h"
#define  P2P_SUCCESSFUL                       ERROR_PPCS_SUCCESSFUL
#define  P2P_NOT_INITIALIZED                  ERROR_PPCS_NOT_INITIALIZED
#define  P2P_ALREADY_INITIALIZED              ERROR_PPCS_ALREADY_INITIALIZED
#define  P2P_TIME_OUT                         ERROR_PPCS_TIME_OUT
#define  P2P_INVALID_ID                       ERROR_PPCS_INVALID_ID
#define  P2P_INVALID_PARAMETER                ERROR_PPCS_INVALID_PARAMETER
#define  P2P_DEVICE_NOT_ONLINE                ERROR_PPCS_DEVICE_NOT_ONLINE
#define  P2P_FAIL_TO_RESOLVE_NAME             ERROR_PPCS_FAIL_TO_RESOLVE_NAME
#define  P2P_INVALID_PREFIX                   ERROR_PPCS_INVALID_PREFIX
#define  P2P_ID_OUT_OF_DATE                   ERROR_PPCS_ID_OUT_OF_DATE
#define  P2P_NO_RELAY_SERVER_AVAILABLE        ERROR_PPCS_NO_RELAY_SERVER_AVAILABLE
#define  P2P_INVALID_SESSION_HANDLE           ERROR_PPCS_INVALID_SESSION_HANDLE
#define  P2P_SESSION_CLOSED_REMOTE            ERROR_PPCS_SESSION_CLOSED_REMOTE
#define  P2P_SESSION_CLOSED_TIMEOUT           ERROR_PPCS_SESSION_CLOSED_TIMEOUT
#define  P2P_SESSION_CLOSED_CALLED            ERROR_PPCS_SESSION_CLOSED_CALLED
#define  P2P_REMOTE_SITE_BUFFER_FULL          ERROR_PPCS_REMOTE_SITE_BUFFER_FULL
#define  P2P_USER_LISTEN_BREAK                ERROR_PPCS_USER_LISTEN_BREAK
#define  P2P_MAX_SESSION                      ERROR_PPCS_MAX_SESSION
#define  P2P_UDP_PORT_BIND_FAILED             ERROR_PPCS_UDP_PORT_BIND_FAILED
#define  P2P_USER_CONNECT_BREAK               ERROR_PPCS_USER_CONNECT_BREAK
#define  P2P_SESSION_CLOSED_INSUFFICIENT_MEMORY                                     ERROR_PPCS_SESSION_CLOSED_INSUFFICIENT_MEMORY
#define  P2P_INVALID_APILICENSE               ERROR_PPCS_INVALID_APILICENSE
#define  P2P_FAIL_TO_CREATE_THREAD            ERROR_PPCS_FAIL_TO_CREATE_THREAD
*/

#define P2P_SUCCESSFUL						0
#define P2P_NOT_INITIALIZED					-1
#define P2P_ALREADY_INITIALIZED				-2
#define P2P_TIME_OUT							-3
#define P2P_INVALID_ID						-4
#define P2P_INVALID_PARAMETER				-5
#define P2P_DEVICE_NOT_ONLINE				-6
#define P2P_FAIL_TO_RESOLVE_NAME				-7
#define P2P_INVALID_PREFIX					-8
#define P2P_ID_OUT_OF_DATE					-9
#define P2P_NO_RELAY_SERVER_AVAILABLE		-10
#define P2P_INVALID_SESSION_HANDLE			-11
#define P2P_SESSION_CLOSED_REMOTE			-12
#define P2P_SESSION_CLOSED_TIMEOUT			-13
#define P2P_SESSION_CLOSED_CALLED			-14
#define P2P_REMOTE_SITE_BUFFER_FULL			-15
#define P2P_USER_LISTEN_BREAK				-16
#define P2P_MAX_SESSION						-17
#define P2P_UDP_PORT_BIND_FAILED				-18
#define P2P_USER_CONNECT_BREAK				-19
#define P2P_SESSION_CLOSED_INSUFFICIENT_MEMORY	-20
#define P2P_INVALID_APILICENSE				-21
#define P2P_FAIL_TO_CREATE_THREAD			-22

#define MAX_P2P_CHNNUM			(8)

int p2p_sys_client_init(void);
int p2p_sys_init(void);
int p2p_sys_deinit(void);
int p2p_close(int fd);
int p2p_read(int fd, int chn, char *buf, int *data_size, int timeout);

int p2p_write(int fd, int chn, char *buf, int data_size);
int p2p_listen(const char *mydid, int timeout, int port, int flag, const char *lisence);
int p2p_check_fd(int fd);
int p2p_detect_online(const char *dev_did);
int p2p_connect(const char *dev_did, char lan_search, unsigned short port);
int p2p_check_buffer(int fd, int chn, unsigned int *wsize, unsigned int *rsize);


int P2P_P2P_Init();//P2PHandle_T *handle);

int P2P_P2P_UnInit();//P2PHandle_T *handle);

#endif
