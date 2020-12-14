#include <netdb.h>		//getaddrinfo
#include <sys/socket.h>	//socket, connect, send, recv
#include <stdlib.h>		//calloc
#include <unistd.h>		//close
#include <stdio.h>
#include <string.h>

#define BUFSIZE 1024

int main(int argc, char** argv){
	if( argc != 3 ){
		printf("Usage: %s IP port\n",argv[0]);
		return 0;
	}
	//getaddrinfo
	struct addrinfo hint, *r_addr;
	memset(&hint,0,sizeof(hint));	//w/o it is OK OSX Catalina, but not in Linux
	hint.ai_family = AF_INET ;
	hint.ai_socktype = SOCK_STREAM ;
	getaddrinfo(argv[1], argv[2], &hint, &r_addr);	

	//socket
	int r_sock = socket(r_addr->ai_family, r_addr->ai_socktype, r_addr->ai_protocol);

	//connection
	connect(r_sock, r_addr->ai_addr, r_addr->ai_addrlen);
	freeaddrinfo(r_addr);

	//send / receive
	char* buf = (char*) calloc(BUFSIZE,sizeof(char));
	sprintf(buf,
		"GET / HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: MA client\r\n"
		"Accept: */*\r\n\r\n"
		,argv[1]
	);
	send(r_sock, buf, strlen(buf), 0);
	
	int rcvd=1;
	while( rcvd >0 ){
		*buf='\0';
		rcvd = recv(r_sock, buf, BUFSIZE, 0);
		if( rcvd < BUFSIZE-1) buf[rcvd]='\0';
		printf("%s",buf);
	}
	close(r_sock);	
	//close
	return 0;
}
