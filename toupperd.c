#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/select.h>

#include <string.h>
#include <stdio.h>
#include <err.h>

#include <ctype.h>

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
		errx(err," Exiting\n");
	}
	//socket
	int srv_sock = socket(srvad->ai_family, srvad->ai_socktype, srvad->ai_protocol);
	if( srv_sock < 0 ) {
		fprintf(ERRLOG,"ERROR: socket : couldn't set up host socket\n");
		return 1;
	}
	//bind
	err = bind(srv_sock, srvad->ai_addr, srvad->ai_addrlen);
	if( err < 0 ){
		fprintf(ERRLOG, "ERROR: Couldn't bind address to host socket : %d\n",errno);
		errx(errno," Exiting\n");
	}
	freeaddrinfo(srvad);
	//listen
	err = listen(srv_sock, 10);
	if( err < 0 ){
		fprintf(ERRLOG, "ERROR: Couldn't host socket to listen: %d\n",errno);
		errx(errno," Exiting\n");
	}

	//Main loop: accepting, receiving and replying to (new) connections	
	fd_set masterfds,readfds;
	FD_ZERO(&masterfds);
	FD_SET(srv_sock, &masterfds);
	int max_sockets = srv_sock;
//
//	fprintf(STDLOG,"#DEBUG: Server ready. srv_socket=%d...\n",srv_sock);

	while(1){	
		readfds = masterfds;
	//select
		err = select(max_sockets+1, &readfds, 0, 0, 0);
		if( err <= 0 ){
			fprintf(ERRLOG,"ERROR: select read : %d\n",errno);
			errx(errno," Exiting\n");
		}
//
//	fprintf(STDLOG,"#DEBUG: found %d connection(s) ready...\n",err);

		//which connection is ready: if new=>accept; else, read/send
		int client_s;
		int tfds = err, cfds = 0;
		for( client_s = 1 ; client_s <= max_sockets ; client_s++){	

	//fprintf(STDLOG,"#DEBUG: checking connection socket %d (%d/%d)...\n",client_s,cfds,tfds);

		//a connection is read ready
			if( FD_ISSET(client_s,&readfds) )  {
				if( client_s == srv_sock ){
		//accept
//
//	fprintf(STDLOG,"#DEBUG: type: new connection request...\n");

					struct sockaddr client_addr;
					socklen_t client_addrlen = sizeof(client_addr);
					int new_client_s = accept(srv_sock, &client_addr, &client_addrlen);
					if ( new_client_s < 0 ){
						fprintf(ERRLOG,"ERROR: couldn't accept new clients connection: %d\n",errno);
						errx(errno," Exiting\n");
					}
					char chost[NI_MAXHOST], cport[NI_MAXSERV];
					err = getnameinfo(&client_addr, client_addr.sa_len,
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
//
//	fprintf(STDLOG,"#DEBUG: type: old connection socket %d ...\n",client_s);

					char rbuf[RDBUFLEN+1];
					int rcvd = recv(client_s, rbuf,RDBUFLEN,0);
					if ( rcvd <= 0 ) {
						fprintf(ERRLOG,"WARNING: While reading, connection problem or closed by peer\n");
						FD_CLR(client_s,&masterfds);
						continue;
					}
//
//	fprintf(STDLOG,"#DEBUG: received text :\n%s\n\n",rbuf);

					for(int c = 0 ; c < rcvd ; c++){
						rbuf[c] = toupper(rbuf[c]);
					}
					rbuf[rcvd+1]='\0';
					int tsent = 0, sent;
//
//	fprintf(STDLOG,"#DEBUG: sending converted text...\n");

					while( tsent < rcvd ){
						sent = send(client_s, rbuf, strlen(rbuf), 0);
						if( sent <= 0 ){
							fprintf(ERRLOG,"WARNING: %d bytes sent : Failed to send to client socket : %d\n",sent,client_s);
							errx(errno," Exiting\n");
						}
						tsent += sent;
//
//	fprintf(STDLOG,"#DEBUG: sent %d out of %d bytes...\n",tsent,rcvd);

					}
//
//	fprintf(STDLOG,"#DEBUG: text sent\n");

				}
				
			}
		}
	}
	//close
	close(srv_sock);
	printf("Closed server listening socket\nExiting...\n");
	return 0;
}
