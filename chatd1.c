#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/select.h>

#include <string.h>
#include <stdio.h>
#include <err.h>

#include <ctype.h>
#include <stdlib.h>

#define PORT "18080"
#define HOSTINADR NULL		//listen to any valid interface, including localhost

#define ERRLOG stderr
#define STDLOG stdout

#define RDBUFLEN 1024

static int errno ;

int main(){
	//getaddrinfo
	struct addrinfo hint, *srvad;
	hint.ai_family = AF_INET ;
	hint.ai_socktype = SOCK_STREAM ;
	hint.ai_flags =  AI_PASSIVE ;
	int err = getaddrinfo(HOSTINADR, PORT, &hint, &srvad);
	if( err ) {
		fprintf(ERRLOG,"ERROR: getaddrinfo : %d : %s\n",err,gai_strerror(err));
		//errx(err," Exiting\n");
		perror(" Failure getaddrinfo ");
	}
	//socket
	int srv_sock = socket(srvad->ai_family, srvad->ai_socktype, srvad->ai_protocol);
	if( srv_sock < 0 ) {
		fprintf(ERRLOG,"ERROR: socket : couldn't set up host socket\n");
		return 1;
	}
	int soval=1;
	if( setsockopt(srv_sock,SOL_SOCKET,SO_REUSEADDR,&soval,sizeof(soval)) < 0 )
		perror(" failed to set socketoption SO_REUSEADDR ");
	//bind
	err = bind(srv_sock, srvad->ai_addr, srvad->ai_addrlen);
	if( err < 0 ){
		fprintf(ERRLOG, "ERROR: Couldn't bind address to host socket : %d\n",errno);
		//errx(errno," Exiting\n");
		perror(" bind failed ");
	}
	freeaddrinfo(srvad);
	//listen
	err = listen(srv_sock, 10);
	if( err < 0 ){
		fprintf(ERRLOG, "ERROR: Couldn't host socket to listen: %d\n",errno);
		//errx(errno," Exiting\n");
		perror(" listen failed ");
	}

	//Main loop: accepting, receiving and replying to (new) connections	
	fd_set masterfds,readfds;
	FD_ZERO(&masterfds);
	FD_SET(srv_sock, &masterfds);
	int max_sockets = srv_sock;

	while(1){	
		readfds = masterfds;
	//select
		err = select(max_sockets+1, &readfds, 0, 0, 0);
		if( err <= 0 ){
			fprintf(ERRLOG,"ERROR: select read : %d\n",errno);
			//errx(errno," Exiting\n");
			perror(" select failed ");
		}

		//which connection is ready: if new=>accept; else, read/send
		int client_s;
		int tfds = err, cfds = 0;
		for( client_s = 1 ; client_s <= max_sockets ; client_s++){	

		//a connection is read ready
			if( FD_ISSET(client_s,&readfds) )  {
				if( client_s == srv_sock ){
		//accept

					struct sockaddr client_addr;
					socklen_t client_addrlen = sizeof(client_addr);
					int new_client_s = accept(srv_sock, &client_addr, &client_addrlen);
					if ( new_client_s < 0 ){
						fprintf(ERRLOG,"ERROR: couldn't accept new clients connection: %d\n",errno);
						//errx(errno," Exiting\n");
						perror(" new client accept failed ");
					}
					char chost[NI_MAXHOST], cport[NI_MAXSERV];
					err = getnameinfo(&client_addr, client_addrlen,
									chost, sizeof(chost), cport, sizeof(cport),
									NI_NUMERICHOST | NI_NUMERICSERV
					);
					if( err < 0 ){
						fprintf(ERRLOG,"WARNING: couldn't get client's numeric address %s\n",gai_strerror(err));
					}
					else fprintf(STDLOG,"Socket %d : New connection from %s:%s\n",new_client_s,chost,cport);
			
					max_sockets = (max_sockets < new_client_s)? new_client_s : max_sockets;
					
					FD_SET(new_client_s, &masterfds);
				}else{
		//recv & send

					//char rbuf[RDBUFLEN+1];
					char* rbuf = (char*) calloc(RDBUFLEN+1,sizeof(char));
					int rcvd = recv(client_s, rbuf,RDBUFLEN,0);
					if ( rcvd <= 0 ) {
						fprintf(ERRLOG,"WARNING: While reading, connection problem or closed by peer\n");
						FD_CLR(client_s,&masterfds);
						continue;
					}
					rbuf[rcvd]='\0';

					for(int c = 0 ; c < rcvd ; c++){
						rbuf[c] = toupper(rbuf[c]);
					}

					int oclient_s;	
					for( oclient_s=1; oclient_s <= max_sockets ; oclient_s++){
						if( oclient_s != client_s && oclient_s != srv_sock && FD_ISSET(oclient_s,&masterfds) ){
							char* hdr = (char*) calloc(100,sizeof(char));
							int sent ;
							sprintf(hdr,"Client %5d says: ",client_s);
							send(oclient_s,hdr,strlen(hdr), 0);
							while( ((sent=send(oclient_s, rbuf, strlen(rbuf), 0)) > 0) && (sent < rcvd) );
						}
					}
				} //if client_s == listen (new client) or else current client
			} //if client_s is ready
		} //loop for client_s from 1 to max_sockets
	} //main loop : while(1) : select
	//close
	close(srv_sock);
	printf("Closed server listening socket\nExiting...\n");
	return 0;
}
