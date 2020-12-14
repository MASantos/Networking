#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <err.h>		//errx
#include <string.h>		//strlen
#include <stdio.h>		//printf
#include <stdlib.h> 	//calloc
#define BUFSIZE 1014

int main(int argc, char** argv){
	if( argc != 3 ){
		printf("Usage: %s server-IP port\n",argv[0]);
		return 1;
	}

	struct addrinfo hint, *srvadd;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM ; 
	
	int error = getaddrinfo(argv[1],argv[2],&hint,&srvadd);
	if( error ){
		errx(2,"Error: wrong server address/port : %s\n",gai_strerror(error) );	
	}

	int sw_srv = socket(srvadd->ai_family,
					srvadd->ai_socktype,
					srvadd->ai_protocol
	);
	
	int conn = connect(sw_srv, srvadd->ai_addr, srvadd->ai_addrlen );
	if( conn < 0) errx(3,"Error: connection failed\n");
	freeaddrinfo(srvadd);

	char buf[BUFSIZE] ;
	sprintf(buf,
		"GET / HTTP 1.0\r\n"
		"Host: %s\r\n"
		"Thanks.\r\n",
		argv[1]
	);
	int sent = send(sw_srv, buf, strlen(buf) , 0);

	int rcvd=1, msgn=0;
	while( rcvd > 0 ){
		rcvd = -10;
		msgn++;
		*buf = '\0';
		rcvd = recv(sw_srv, buf, BUFSIZE,0);
		if( rcvd < BUFSIZE-1) buf[rcvd]='\0';
		printf("Msg#: %d.  Received %d bytes: \n%s\n\n",msgn,rcvd,buf);
	}
	close(sw_srv);
	printf("Connection closed\n");
	return 0;
}
