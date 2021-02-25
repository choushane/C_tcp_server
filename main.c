#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#include "debug.h"
#include "tcp_server.h"

static USER_LIST *userList = NULL;

char* getNowTime (void)
{
    time_t t1 = time(NULL);
    struct tm *ptr = localtime(&t1);
    int day = ptr->tm_mday;
    int hour = ptr->tm_hour;
    char *date = (char *)calloc(sizeof(char),64);

    sprintf(date,"%d-%d %d:%d:%d",( 1 + ptr-> tm_mon), ptr->tm_mday, ptr->tm_hour, ptr->tm_min, ptr->tm_sec);
    return date;
}


void userEcho(fd_set *fds){
    USER_LIST *now = NULL;
    char data[MAX_BUFF];
    int dataLength = 0;
    char *time;
    char *send = "ACK";
    FILE *fp = NULL;

    //fp = fopen("log.txt","a");
    for(now = userList;now != NULL;now = now->next){
	if(now->pid <= 0 || now->alive == 0 || !socket_check(now->pid))
	    now->alive = 0;
	else if(FD_ISSET(now->pid, fds)){	
	    memset(data,0,sizeof(char) * MAX_BUFF);
	    while((dataLength = read(now->pid, data, sizeof(data))) != 0) {
	        if(dataLength > 0){
		    time = getNowTime();
		    data[ strlen(data) ] = '\0';
	    	    _DEBUG_MSG("[%s] (%s) %d %d> %s",time,now->ip,dataLength,strlen(data),data); 
	            //if(fp)fwrite(data,1,dataLength,fp);
		    //close(now->pid);
		    free(time);
	    	    memset(data,0,sizeof(char) * MAX_BUFF);
	        }
	    }
		now->pid = -1;
	}
    }
    //if(fp)fclose(fp);
}

void delUser(void){
    USER_LIST *now = NULL;
    USER_LIST *previous = NULL;

    now = userList;
    while(now != NULL){
	if(now->alive == 0){
	    if(previous != NULL ){
	        previous->next = now->next;
		freeUser(now);
	        now = previous->next;
	    }else{
		userList = now->next;
		freeUser(now);
		now = userList;
	    } 
	}else{
	    previous = now;
	    now = now->next;
	}
    }
}

void addUserSelect(int *maxfd,fd_set *fds){
    USER_LIST *now = NULL;

    for(now = userList;now != NULL;now = now->next){
	if(now->pid > 0 && now->alive == 1){
	    FD_SET(now->pid, fds);	
	    *maxfd = MAXNO(now->pid, *maxfd);
	}
    }
}

void checkUserAlive(void){
    USER_LIST *now = NULL;

    if(userList == NULL)return;

    for(now = userList;now != NULL;now = now->next){
        if(now->pid <= 0 ||  now->alive == 0 || !socket_check(now->pid))
            now->alive = 0;
    }
}

int main( int argc, char *argv[] ){

    int serverPid = 0,maxfd = 0;
    int dataLength = 0;
    fd_set fds;

    if(argc < 2){
        _DEBUG_MSG("please,input the port number !!");
	goto end;
    }
    if(atoi(argv[1]) < 1 || atoi(argv[1]) > 65535){
        _DEBUG_MSG("please,input the correct port number(1~65535) !!");
	goto end;
    }

    serverPid = open_server(argv[1],MAX_USER);

    if(serverPid < 0){
        _DEBUG_MSG("Socket open error!!");
	goto end;
    }
    
    _DEBUG_MSG("Main Loop Start...");

    while(1)
    {
	FD_ZERO(&fds);
	maxfd = 0;
	FD_SET(serverPid, &fds);
	maxfd = MAXNO(serverPid, maxfd);

	checkUserAlive();

	delUser();
	addUserSelect(&maxfd,&fds);

	if(!select(maxfd+1, &fds, NULL, NULL, NULL)) continue; 

	if(FD_ISSET(serverPid, &fds))
	    userList = userConnect(serverPid,userList);
	else{
	    userEcho(&fds);
	}
    }
end:
    if(serverPid > 0)close(serverPid);

    return 0;
}
