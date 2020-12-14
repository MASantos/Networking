#include <sys/socket.h>
#include <unistd.h>    //requiredby: read 	on:archlinux
#include <netinet/in.h>
#include <arpa/inet.h> //requiredby: inet_pton  on:archlinux
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE         4096    /* max text line length */
#define bzero(ptr,n)            memset(ptr, 0, n)
#define SA      struct sockaddr
#define LISTENQ         1024    /* 2nd argument to listen() */

int main(int argc, char **argv){
	int sockfd, n;
	char receivline[MAXLINE+1];
	struct sockaddr_in servaddr;

	if ( argc != 2 ){
		printf("usage: daytime-client server-IP\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if ( sockfd < 0) {
		printf("ERROR: socket error\n");
		exit(2);
	}
	printf("reading server ip...\n");
	
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(13);
	
	int sad = inet_pton(AF_INET,argv[1],&servaddr.sin_addr);

	if( sad < 0 ){
		printf("ERROR: inet_pton error for %s\n",argv[1]);
		exit(3);
	}
	
	printf("set server address and port ...\n");
	
	int con = connect(sockfd,(SA*) &servaddr, sizeof(servaddr));
	if ( con < 0){
		printf("ERROR: connect error\n");
		exit(4);
	}

	printf("connection to server established ...\nstart reading ...\n");	

	while ( (n=read(sockfd,receivline,MAXLINE))>0 ){
		receivline[n] = 0 ; //null-terminate
		
		if( fputs(receivline, stdout) == EOF ) {
			printf("ERROR: fputs errorn");
			exit(5);
		}
	}
	
	if( n<0){
		printf("ERROR: read error\n");
		exit(6);
	}
	exit(0);
}

