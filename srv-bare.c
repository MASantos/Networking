#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h> 	//memset
#include <unistd.h>		//close

#define PORT "443"
#define RDBUFLEN 1024

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
	int client_s = accept(server_s, &cadd, &cadd_len);

	//recv
	char  buf[RDBUFLEN];
	recv(client_s, &buf, RDBUFLEN,0);
	
	//send
	char* response =
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-type: text/html\r\n\r\n"
		"<!doctype html>\r\n"
		"<html><head><title>Lemon Inc.</title></head>\r\n"
		"<body style='background-color:#000000;color:#fff'>\r\n"
		"<h1 style='font-size:48;color:#ffff33'>Lemon Inc.</h1>\r\n" 
		"<p>Your browser sent me following details:</p>\r\n" 
		"<div style='text-align:left;width:50%;margin:auto;'>\r\n" 
	;
	send(client_s,response,strlen(response),0);
	send(client_s, &buf , strlen(buf), 0);
	response = "</div></body></html>\r\n";
	send(client_s, response , strlen(response), 0);
	
	//close
	close(server_s);
	close(client_s);
	return 0;
}
