
#ifndef _TCP_SWAP_H_
#define _TCP_SWAP_H_

#include "p2psocket.h"

#define LIVE_TCP_TEST 0

/*
#include "PPCS_Error.h"
#define  TCP_SUCCESSFUL                       ERROR_PPCS_SUCCESSFUL
#define  TCP_NOT_INITIALIZED                  ERROR_PPCS_NOT_INITIALIZED
#define  TCP_ALREADY_INITIALIZED              ERROR_PPCS_ALREADY_INITIALIZED
#define  TCP_TIME_OUT                         ERROR_PPCS_TIME_OUT
#define  TCP_INVALID_ID                       ERROR_PPCS_INVALID_ID
#define  TCP_INVALID_PARAMETER                ERROR_PPCS_INVALID_PARAMETER
#define  TCP_DEVICE_NOT_ONLINE                ERROR_PPCS_DEVICE_NOT_ONLINE
#define  TCP_FAIL_TO_RESOLVE_NAME             ERROR_PPCS_FAIL_TO_RESOLVE_NAME
#define  TCP_INVALID_PREFIX                   ERROR_PPCS_INVALID_PREFIX
#define  TCP_ID_OUT_OF_DATE                   ERROR_PPCS_ID_OUT_OF_DATE
#define  TCP_NO_RELAY_SERVER_AVAILABLE        ERROR_PPCS_NO_RELAY_SERVER_AVAILABLE
#define  TCP_INVALID_SESSION_HANDLE           ERROR_PPCS_INVALID_SESSION_HANDLE
#define  TCP_SESSION_CLOSED_REMOTE            ERROR_PPCS_SESSION_CLOSED_REMOTE
#define  TCP_SESSION_CLOSED_TIMEOUT           ERROR_PPCS_SESSION_CLOSED_TIMEOUT
#define  TCP_SESSION_CLOSED_CALLED            ERROR_PPCS_SESSION_CLOSED_CALLED
#define  TCP_REMOTE_SITE_BUFFER_FULL          ERROR_PPCS_REMOTE_SITE_BUFFER_FULL
#define  TCP_USER_LISTEN_BREAK                ERROR_PPCS_USER_LISTEN_BREAK
#define  TCP_MAX_SESSION                      ERROR_PPCS_MAX_SESSION
#define  TCP_UDP_PORT_BIND_FAILED             ERROR_PPCS_UDP_PORT_BIND_FAILED
#define  TCP_USER_CONNECT_BREAK               ERROR_PPCS_USER_CONNECT_BREAK
#define  TCP_SESSION_CLOSED_INSUFFICIENT_MEMORY                                     ERROR_PPCS_SESSION_CLOSED_INSUFFICIENT_MEMORY
#define  TCP_INVALID_APILICENSE               ERROR_PPCS_INVALID_APILICENSE
#define  TCP_FAIL_TO_CREATE_THREAD            ERROR_PPCS_FAIL_TO_CREATE_THREAD
*/
#define TCP_SUCCESSFUL						0
#define TCP_NOT_INITIALIZED					-1
#define TCP_ALREADY_INITIALIZED				-2
#define TCP_TIME_OUT							-3
#define TCP_INVALID_ID						-4
#define TCP_INVALID_PARAMETER				-5
#define TCP_DEVICE_NOT_ONLINE				-6
#define TCP_FAIL_TO_RESOLVE_NAME				-7
#define TCP_INVALID_PREFIX					-8
#define TCP_ID_OUT_OF_DATE					-9
#define TCP_NO_RELAY_SERVER_AVAILABLE		-10
#define TCP_INVALID_SESSION_HANDLE			-11
#define TCP_SESSION_CLOSED_REMOTE			-12
#define TCP_SESSION_CLOSED_TIMEOUT			-13
#define TCP_SESSION_CLOSED_CALLED			-14
#define TCP_REMOTE_SITE_BUFFER_FULL			-15
#define TCP_USER_LISTEN_BREAK				-16
#define TCP_MAX_SESSION						-17
#define TCP_UDP_PORT_BIND_FAILED				-18
#define TCP_USER_CONNECT_BREAK				-19
#define TCP_SESSION_CLOSED_INSUFFICIENT_MEMORY	-20
#define TCP_INVALID_APILICENSE				-21
#define TCP_FAIL_TO_CREATE_THREAD			-22

/*
int tcp_sys_init(void);
int tcp_sys_deinit(void);
int tcp_close(int fd);
int tcp_read(int fd, int chn, char *buf, int *data_size, int timeout);
int tcp_write(int fd, int chn,  char *buf, int data_size);
int tcp_check_fd(int fd);
int tcp_check_buffer(int fd, int chn, unsigned int *wsize, unsigned int *rsize);

int tcp_sys_client_init(void);
int tcp_connect(const char *devIp, char lan_search, unsigned short port);
*/
int P2P_TCP_Init(P2PHandle_T *handle);

int P2P_TCP_UnInit(P2PHandle_T *handle);

#endif
