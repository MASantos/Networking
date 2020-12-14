#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h> 
#include <unistd.h> 
#include <errno.h>

#include <stdio.h> 
#include <string.h>
#include <time.h>

//should use errno w/ fprint(stderr.... but...I'll use exit(...) here
#include <stdlib.h>

int main() {
	printf("Ready to use socket API.\n");

	struct addrinfo adinfo;
	
	memset(&adinfo , 0 , sizeof(adinfo) );
	adinfo.ai_family = AF_INET6;
	adinfo.ai_socktype = SOCK_STREAM;
	adinfo.ai_flags = AI_PASSIVE ;

	struct addrinfo *bind_address;
	getaddrinfo(0, "8080", &adinfo, &bind_address);

	int socket_listen ;

	socket_listen = socket(bind_address->ai_family, 
							bind_address->ai_socktype,
							bind_address->ai_protocol
					);
	if( socket_listen < 0 ) {
		printf("Error: can't listen\n");
		exit(1);
	}

	int binded = bind(socket_listen, 
						bind_address->ai_addr,
						bind_address->ai_addrlen
					);
	if (binded != 0 ){
		printf("Error: addr bind error\n");
		exit(2);
	}

	freeaddrinfo(bind_address);

	int lsn = listen(socket_listen, 10) ; //queue up to 10 conn: OS rejects further conn until queue has less than that limit

	if ( lsn < 0) {
		printf("Error: listening erorr\n");
		exit(3);
	}
	
	struct sockaddr_storage client_address; //can handle both ipv4 and ipv6

	socklen_t client_add_len = sizeof(client_address);

	int client_socket = accept(socket_listen,
								(struct sockaddr*) &client_address,
								&client_add_len
						);
	
	if (client_socket < 0){
		printf("Error: couldn't accept connection\n");
		exit(4);
	}
	
	printf("Client connected: ");

	char address_buf[100];	

	getnameinfo((struct sockaddr*)&client_address,
				client_add_len,
				address_buf,
				sizeof(address_buf),
				0,0,
				NI_NUMERICHOST
	);
	printf("%s\n",address_buf);
	
	char request[1024];

	int bytes_rcvd = recv(client_socket,	//recv blocks until it receives some request
							request,
							1024,
							0
					);
	if( bytes_rcvd <= 0 ) {
		printf("Warning: connection terminated by client\n");
	}
	else { 
		printf("Received %d bytes\n",bytes_rcvd);
		printf("%.*s",bytes_rcvd,request);
	}
	
	const char* response = 
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-type: text/plain\r\n\r\n"
		"Local time is: ";
	int bytes_sent = send(client_socket,response,strlen(response), 0);

	if( bytes_sent < strlen(response) ) printf("Warning: sent fewer bytes (%d) than expected (%d)\n",bytes_sent,(int)strlen(response));

	time_t timer;
	time(&timer);
	char *time_msg = ctime(&timer);
	bytes_sent = send(client_socket, time_msg, strlen(time_msg),0);

	close(client_socket); //otherwise client browser keeps waiting for more msg until it times out

	//now we could call accept to haver server accept a new connection; we'll stop here for now

	close(socket_listen);

	printf("Finished time server test\n");
	
	return 0; 
}

