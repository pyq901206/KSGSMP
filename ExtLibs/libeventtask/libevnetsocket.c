#include "common.h"
#include <event2/event-config.h>
#include <event.h>
#include "libevnetsocket.h"
#include<sys/socket.h>
#include<arpa/inet.h>


#define MAX_LINE    1024 
static LibEventNetWorkSerHandle_T g_netHandle;

int TcpNetSendMsg(int socket, char *msg, int len)
{
	int size = send(socket, msg, len, 0); 
	return size;
}

void read_cb(struct bufferevent *bev, void *arg)
{  
	struct sockaddr_in sa;
	int len = sizeof(sa);
	NetServer_T param = {0};
    struct evbuffer *buf = (struct evbuffer *)arg;  
    char line[MAX_LINE+1];  
    int n;  
    evutil_socket_t fd = bufferevent_getfd(bev);  
    while (n = bufferevent_read(bev, line, MAX_LINE), n > 0) {  
        line[n] = '\0';  
        //将读取到的内容放进缓冲区  
        evbuffer_add(buf, line, n);  
 #if 0
        //搜索匹配缓冲区中是否有==，==号来分隔每次客户端的请求  
        const char *x = "==";  
        struct evbuffer_ptr ptr = evbuffer_search(buf, x, strlen(x), 0);     
        if (ptr.pos != -1)
		{  
            bufferevent_write_buffer(bev, buf); //使用buffer的方式输出结果
        }
#endif
    }
	//消息回调出去，带socket，ip
//	DF_DEBUG("fd = [%d] buf = [%s]",bufferevent_getfd(bev), line);
	getpeername(bufferevent_getfd(bev), (struct sockaddr *)&sa, &len);
	snprintf(param.ipaddr, 16, "%s", inet_ntoa(sa.sin_addr));
	memcpy(param.recvMsg, line, MAX_LINE);
	param.socket = bufferevent_getfd(bev);
	g_netHandle.readcb(&param);
	return;
}  

void write_cb(struct bufferevent *bev, void *arg) {}  
void error_cb(struct bufferevent *bev, short event, void *arg)
{  
	NetServer_T param = {0};
    evutil_socket_t fd = bufferevent_getfd(bev);  
	param.socket = (int)fd;
    DF_DEBUG("fd = %u, ", fd);  
    if (event & BEV_EVENT_TIMEOUT) {  
        DF_DEBUG("Timed out..");	
		g_netHandle.closecb(&param);
	//	return;
    } else if (event & BEV_EVENT_EOF) {  
        DF_DEBUG("connection closed..");  
		g_netHandle.closecb(&param);
    } else if (event & BEV_EVENT_ERROR) {  
        DF_DEBUG("some other error..");  
		g_netHandle.closecb(&param);
    }  
    //清空缓冲区  
    struct evbuffer *buf = (struct evbuffer *)arg;  
    evbuffer_free(buf);  
    bufferevent_free(bev);  
}  
  
//回调函数，用于监听连接进来的客户端socket  
void do_accept(evutil_socket_t fd, short event, void *arg)
{  
    int client_socketfd;//客户端套接字      
    struct sockaddr_in client_addr; //客户端网络地址结构体     
    int in_size = sizeof(struct sockaddr_in);    
    //客户端socket   
    client_socketfd = accept(fd, (struct sockaddr *) &client_addr, &in_size); //等待接受请求，这边是阻塞式的    
    if (client_socketfd < 0)
	{    
        DF_ERROR("accpet error..");    
        return;  
    }
	DF_DEBUG("event accept is fd %d", client_socketfd);
  
    //类型转换  
    struct event_base *base_ev = (struct event_base *) arg;  
  
    //socket发送欢迎信息    
    //char * msg = "Welcome to My socket";    
    //int size = send(client_socketfd, msg, strlen(msg), 0);    
  
    //创建一个事件，这个事件主要用于监听和读取客户端传递过来的数据  
    //持久类型，并且将base_ev传递到do_read回调函数中去  
    //struct event *ev;  
    //ev = event_new(base_ev, client_socketfd, EV_TIMEOUT|EV_READ|EV_PERSIST, do_read, base_ev);  
    //event_add(ev, NULL);  
  
    //创建一个evbuffer，用来缓冲客户端传递过来的数据  
    struct evbuffer *buf = evbuffer_new();  
    //创建一个bufferevent  
    struct bufferevent *bev = bufferevent_socket_new(base_ev, client_socketfd, BEV_OPT_CLOSE_ON_FREE);  
    //设置读取方法和error时候的方法，将buf缓冲区当参数传递  
    bufferevent_setcb(bev, read_cb, NULL, error_cb, buf);    
    //设置类型  
    bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);  
    //设置水位  
    bufferevent_setwatermark(bev, EV_READ, 0, 0);
	//设置超时，心跳长链接
  //  struct timeval tv = {20,0};
  //  bufferevent_set_timeouts (bev, &tv, NULL);
	return;
}  

static void *LibEventNetServerThead(void *argv)
{
	int server_socketfd; //服务端socket    
    struct sockaddr_in server_addr;   //服务器网络地址结构体      
    memset(&server_addr,0,sizeof(server_addr)); //数据初始化--清零      
    server_addr.sin_family = AF_INET; //设置为IP通信      
    server_addr.sin_addr.s_addr = INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上      
    server_addr.sin_port = htons(BATTERY_SERVER_PORT); //服务器端口号      
    
    //创建服务端套接字    
    server_socketfd = socket(PF_INET,SOCK_STREAM,0);    
    if (server_socketfd < 0) {    
        DF_ERROR("socket error");    
        return NULL;    
    }    
  
    evutil_make_listen_socket_reuseable(server_socketfd); //设置端口重用  
    evutil_make_socket_nonblocking(server_socketfd); //设置无阻赛  
    
    //绑定IP    
    if (bind(server_socketfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))<0) {    
        DF_ERROR("bind error");    
        return NULL;    
    }    
  
    //监听,监听队列长度 5    
    listen(server_socketfd, 10);    
      
    //创建event_base 事件的集合，多线程的话 每个线程都要初始化一个event_base  
    struct event_base *base_ev;  
    base_ev = event_base_new();   
    const char *x =  event_base_get_method(base_ev); //获取IO多路复用的模型，linux一般为epoll  
    DF_DEBUG("METHOD:%s\n", x);  
  
    //创建一个事件，类型为持久性EV_PERSIST，回调函数为do_accept（主要用于监听连接进来的客户端）  
    //将base_ev传递到do_accept中的arg参数  
    struct event *ev;  
    ev = event_new(base_ev, server_socketfd, EV_TIMEOUT|EV_READ|EV_PERSIST, do_accept, base_ev);  
    //注册事件，使事件处于 pending的等待状态  
    event_add(ev, NULL);  
  
    //事件循环  
    event_base_dispatch(base_ev);  
  
    //销毁event_base  
    event_base_free(base_ev);    
	return NULL;
}

int LibEventTcpNetSer(LibEventNetWorkSerHandle_T *handle)
{	
	pthread_t pthid;
	g_netHandle.readcb = handle->readcb;  //不能用全局，用全局的话，此模型只能使用一次
	g_netHandle.closecb = handle->closecb;
	pthread_create(&pthid, NULL, LibEventNetServerThead, (void *)NULL);
    pthread_detach(pthid);
	return KEY_TRUE;
}


