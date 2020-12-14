/* tcp_client.c
Does following
stdin => server
recvd => stdout
until user ctrl+C or server closes connection
*/
//#include "networking.h"
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <err.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PROGRAM "tcp_client"
#define READBUFLEN 4096
#define TIMEOUTSEC 0
#define TIMEOUTUSEC 100000 //100 msec

int main(int argc, char** argv){
if ( argc < 3){
	fprintf(stderr,"Usage: %s IP port\n",PROGRAM);
	return 1;
}

// getaddrinfo	
struct addrinfo hints, *server_addrs;
memset(&hints,0,sizeof(hints));
hints.ai_socktype = SOCK_STREAM;
int err = getaddrinfo(argv[1],argv[2],&hints,&server_addrs);

if ( err ){
	fprintf(stderr,"Error: getaddrinfo : %s\n",gai_strerror(err) );
	return 2;
}

char addrbuf[100], servbuf[100];
getnameinfo(server_addrs->ai_addr, 
			server_addrs->ai_addrlen,
			addrbuf, sizeof(addrbuf),
			servbuf, sizeof(servbuf),
			NI_NUMERICHOST
);
printf("Server: %s:%s\n",addrbuf,servbuf);

// socket
int socket_server = socket(server_addrs->ai_family,
						   server_addrs->ai_socktype,
						   server_addrs->ai_protocol
					);
if ( socket_server < 0 ){
		fprintf(stderr,"Error: failed to open socket \n");
	return 3;
}

// connect
int con = connect(socket_server, server_addrs->ai_addr, server_addrs->ai_addrlen);
if ( con ){
	fprintf(stderr,"Error: couldn't connect to server \n");
	return 4;
}
freeaddrinfo(server_addrs);

printf("Connection to server established.\n");
printf("To send data, enter text followed by enter.\n");

while(1){
	fd_set reads;
	FD_ZERO(&reads);
	FD_SET(0, &reads);
	FD_SET(socket_server, &reads);
	
	struct timeval timeout;
	timeout.tv_sec = TIMEOUTSEC;
	timeout.tv_usec = TIMEOUTUSEC ;

	if( select(socket_server+1, &reads, 0,0, &timeout) < 0 ){
		fprintf(stderr,"Error: while listening to server\n");
		return 5;
	}
	
	if( FD_ISSET(socket_server, &reads) ){
		char* read = (char*) calloc(READBUFLEN+1,sizeof(char));;
		int bytes_rcvd = recv(socket_server, read, READBUFLEN, 0);
		if ( bytes_rcvd < 1 ){
			printf("Connection reset by peer\n");
			break;
		}
		read[bytes_rcvd]='\0';
		printf("%s",read);
	}
	
	if( FD_ISSET(STDIN_FILENO, &reads) ){
		char* read = (char*) calloc(READBUFLEN+1,sizeof(char));
		if( !fgets(read,READBUFLEN,stdin) ) {
			fprintf(stderr,"Error: couldn't read stdin\n");
			printf("DEBUG: read :\n---\n%s\n---\n",read);
			return 6;
		}
		int bytes_sent = send(socket_server, read, strlen(read), 0);
	}
}
printf("Closing socket...\n");
close(socket_server);
printf("Finished\n");

return 0;
}
