#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h> 	//memset
#include <unistd.h>		//close

#include <stdio.h>		//printf
#include <stdlib.h>		//calloc

#define PORT "8080"
#define RDBUFLEN 1024
#define WRBUFLEN RDBUFLEN

int main(){
	struct addrinfo hint;
	memset(&hint,0,sizeof(hint) );
	hint.ai_family = AF_INET6 ;
	hint.ai_socktype = SOCK_STREAM ;
	hint.ai_flags = AI_PASSIVE ; //for use w/ bind

	//getaddrinfo
	struct addrinfo *sadd ;
	getaddrinfo(0, PORT, &hint, &sadd);

	//socket
	int server_s = socket(sadd->ai_family,
				sadd->ai_socktype,
				sadd->ai_protocol
	);
 
	//bind
	bind(server_s, sadd->ai_addr, sadd->ai_addrlen);

	//listen
	listen(server_s,3);

	//accept: blocking : waits for client to connect
	struct sockaddr cadd;
	socklen_t cadd_len = sizeof(cadd);
	int client_s = 1;
	int n=0;
	while( (client_s=accept(server_s, &cadd, &cadd_len) )>0 ){

		//recv
		char*  buf = (char*) calloc(RDBUFLEN,sizeof(char));

		size_t rcvd = recv(client_s, buf, RDBUFLEN,0);
		*(buf+rcvd)='\0';
		printf("Received:\n%s\n",buf);	

		//send
		char response[WRBUFLEN] =
			"HTTP/1.1 200 OK\r\n"
			"Connection: close\r\n"
			"Content-type: text/html\r\n\r\n";
		send(client_s,response,WRBUFLEN,0);

		sprintf(response,
			"<!doctype html>\r\n"
			//"<html><head><meta charset='utf-8'><title>Lemon Inc.</title></head>\r\n"  	//Firefox sends 1 request only
			//"<html><head><title>Lemon Inc.</title></head>\r\n"  						//Firefox sends 1 request only
			"<html><head><title>Lemon Inc.</title><meta charset='utf-8'></head>\r\n"  //Buggy Firefox sends 2 requests! 
			"<body style='background-color:#000000;color:#fff'>\r\n"
			"<h1 style='font-size:48;color:#ffff33'>Lemon Inc.</h1>\r\n" 
			"<p>Your browser sent me following details:</p>\r\n" 
			"<div style='text-align:left;width:50%%;margin:auto;'>\r\n" 
		);
		send(client_s,response,strlen(response),0);

		int sent=send(client_s, buf , rcvd,0);

		sprintf(response,"</div><br>Number of connections today: %d\r\n",++n);
		send(client_s, response , strlen(response), 0);

		sprintf(response , "</body></html>\r\n");
		send(client_s, response , strlen(response), 0);

		close(client_s);
	}	
	//close
	close(server_s);
	printf("\n\nServer shutdown\n");
	return 0;
}
