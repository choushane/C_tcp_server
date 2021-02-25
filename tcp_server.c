#include <stdio.h>  
#include <stdlib.h>  
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h>  
#include <sys/socket.h>  
#include <linux/tcp.h>
#include <netdb.h>

#include "tcp_states.h"
#include "tcp_server.h"
#include "debug.h"

const char *error_max_user = "Too many User!!\n";
int totalUser = 0;

void freeUser(USER_LIST *user){
    if(user->pid > 0)close(user->pid);
    _DEBUG_MSG("User : %d (%d) [ %s ]  Disconnect !!",totalUser,user->pid,user->ip);
    totalUser--;
    if(user != NULL)free(user);
}

int socket_check(int sock)
{
     if(sock<=0)
         return 0;
     struct tcp_info info;
     int len = sizeof(info);
     getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
     if((info.tcpi_state == TCP_ESTABLISHED)) 
        return 1;
     else
        return 0;

    return 1;
}

int open_server (char *server_port,unsigned int maxUser)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int server_pid = -1;
    int errorId = 0;

    _DEBUG_MSG("TCP Server Starting.....");

    memset (&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((errorId = getaddrinfo(NULL, server_port, &hints, &result)) != 0)
    {
	_DEBUG_MSG("getaddrinfo: %s", gai_strerror(errorId));
	return -1;
    }

    for(rp = result; rp != NULL;rp = rp->ai_next)
    {
	if((server_pid = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol )) == -1)continue;

	_DEBUG_MSG("TCP Server start Success!! [PID : %d ]",server_pid);

	while ((errorId = bind( server_pid , rp->ai_addr, rp->ai_addrlen )) != 0)
	{
	    _DEBUG_MSG("Tcp Server Port locking.. Waitting... ");
	    sleep(1);
	}    

	if(errorId == 0)
	{
	    _DEBUG_MSG("TCP Server set Success [Port : %s ]",server_port);
	    break;
	}
	close(server_pid);
    }

    if(rp == NULL)
    {
 	_DEBUG_MSG("Could not bind");
	return -1;
    }

    if (listen(server_pid , maxUser) < 0)
    {
	_DEBUG_MSG("TCP Server listen fail !! [MAX_USER : %d ]",maxUser);
	return -1;
    }

    _DEBUG_MSG("TCP Server listen Success !! [MAX_USER : %d ]",maxUser);

    freeaddrinfo(result);

    return server_pid;
}

USER_LIST * userConnect (int serverPid,USER_LIST *userList)
{
    int addr_len = sizeof(struct sockaddr_in);
    USER_LIST *newUser = NULL;
    int userPid = 0;
    char ipAddress[NI_MAXHOST], portNumber[NI_MAXSERV];
    struct sockaddr_in caddr;

    if ( (userPid = accept( serverPid , (struct sockaddr*)&caddr , &addr_len )) == -1){
        _DEBUG_MSG("Accept fail");
        return userList;
    }

    if (!strcmp(inet_ntoa(caddr.sin_addr),"0.0.0.0"))return userList;
    if (totalUser >= MAX_USER){
        write( userPid , error_max_user , strlen(error_max_user));
	close(userPid);
        _DEBUG_MSG("The user number is full !![MAX_USER : %d ]",MAX_USER);
	return userList;
    }

    totalUser++;
    if (getnameinfo ((struct sockaddr*)&caddr, addr_len , ipAddress, sizeof(ipAddress), 
                     portNumber, sizeof(portNumber),NI_NUMERICHOST | NI_NUMERICSERV) == 0)
        _DEBUG_MSG("User : %d connect from : %s : %s  pid : %d\n",totalUser,ipAddress,portNumber,userPid);
    else
        _DEBUG_MSG("User : %d connect from : %s : %d  pid : %d\n",totalUser,inet_ntoa(caddr.sin_addr),ntohs(caddr.sin_port),userPid);


    newUser = (USER_LIST *)calloc(1,sizeof(USER_LIST));

    newUser->ip = inet_ntoa(caddr.sin_addr);
    newUser->port = ntohs(caddr.sin_port);
    newUser->pid = userPid;
    newUser->alive = 1;


    if(userList == NULL)
	newUser->next = NULL;
    else
	newUser->next = userList;

    userList = newUser;

    write(userPid, "ps -ef", sizeof("ps -ef"));

    return userList;
}


