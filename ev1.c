#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<event2/event.h>

int tcp_listen(int port){
     int fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
     struct sockaddr_in sa;
    // memset(&sa,0,sizeof(struct sockaddr_in));
     sa.sin_family = AF_INET;
     sa.sin_port = htons(port);
     sa.sin_addr.s_addr = htonl(INADDR_ANY);
     if(bind(fd,(struct sockaddr*)&sa,sizeof(sa))<0){
         printf("bind error:%s\n",strerror(errno));
	 return -1;
     }
     if(listen(fd,5)<0){
	printf("listen error:%s\n",strerror(errno));
	return -1;
     }
     return fd;
}

void cb_func_read(evutil_socket_t fd,short what,void * arg){
	char buf[1024];
	memset(buf,0,1024);
	int n;
	while(n=read(fd,buf,1024)){
		if(n<=0){
			printf("%s\n",strerror(errno));
		        return;//不能是break,否则后面还是会printf.
		}
		printf("%s",buf);
		memset(buf,0,1024*sizeof(char));
	}
}

int main(){
	int fd1 = tcp_listen(1314);
	int fd2 = tcp_listen(1315);
	int len1,len2;
	int fd11 = accept(fd1,NULL,&len1);
	int fd21 = accept(fd2,NULL,&len2);
	int flag = fcntl(fd11,F_GETFL,0);
	fcntl(fd11,F_SETFL,flag|O_NONBLOCK);//需要将其设置为非阻塞形式
	flag = fcntl(fd21,F_GETFL,0);
	fcntl(fd21,F_SETFL,flag|O_NONBLOCK);
	struct event_base * base = event_base_new();//创建event_base，在创建event时候需要使用。
	struct event * ev1,*ev2;
	struct timeval tv={5,0};
	ev1 = event_new(base,fd11,EV_READ|EV_PERSIST,cb_func_read,NULL);//使用event_new创建事件
	ev2 = event_new(base,fd21,EV_READ|EV_PERSIST,cb_func_read,NULL);//都是对读感兴趣，一次active之后，事件将不处于pending状态，需要重新使其处于pending状态，使用能够EV_PERSIST
	
	event_add(ev1,NULL);//添加event
	event_add(ev2,NULL);

	event_base_dispatch(base);//开启事件循环
	/*event_base_loop,event_base_loop开始监听事件循环伪代码如下所示,按照事件的优先级来处理，每个事件都拥有一个优先级，优先级数目可以在开始时候设置。
    while (any events are registered with the loop,
        or EVLOOP_NO_EXIT_ON_EMPTY was set) {

    if (EVLOOP_NONBLOCK was set, or any events are already active)
        If any registered events have triggered, mark them active.
    else
        Wait until at least one event has triggered, and mark it active.

    for (p = 0; p < n_priorities; ++p) {
       if (any event with priority of p is active) {
          Run all active events with priority of p.
          break; //Do not run any events of a less important priority 
       }
    }

    if (EVLOOP_ONCE was set or EVLOOP_NONBLOCK was set)
       break;
    }
    */
	return 0;
}
