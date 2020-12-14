// sends stdin to peer 
// simultaneously prints what receives from peer
//
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/err.h>

#include <stdio.h>
#include <stdlib.h>
#include <err.h>

typedef int SOCKET;
#define IOBUFSIZE 2048

void quit(int e, char* logm, const char* em){
	fprintf(stderr,"ERROR: %d : %s\n",e,logm);
	errx(e,"%s",em);
}
struct addrinfo* getHostAddr(char* host, char* port){
	struct addrinfo hint, *peeraddr;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	int e = getaddrinfo(host, port, &hint, &peeraddr);
	if ( e) {
		char* logm ;
		asprintf(&logm,"failed getaddrinfo for %s:%s\n",host,port);
		quit(e,logm,gai_strerror(e));
		free(logm);
	}
	return peeraddr;
}

SOCKET getHostSocket(struct addrinfo* hai){
	SOCKET s = socket(hai->ai_family, hai->ai_socktype, hai->ai_protocol);
	if(s < 0 ){
		quit(s,"socket failed\n","couldn't setup socket");
	}
	return s;
}

void tcp_connect(SOCKET hs, struct addrinfo* hai){
	int e = connect(hs, hai->ai_addr, hai->ai_addrlen);
	if ( e ){
		quit(errno,"Connection failed\n",strerror(errno));
	}
	freeaddrinfo(hai);
}

int main(int argc, char** argv){
	if(argc < 3 ){
		fprintf(stderr,"Usage: %s host port\n",argv[0]);
		return 1;
	}
	char* host = argv[1];
	char* port = argv[2];

	//getaddrinfo
	struct addrinfo* peerai = getHostAddr(host,port);
	//socket
	SOCKET peer_s = getHostSocket(peerai);
	//SSL init PRNG
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	SSL_CTX* sslctx = SSL_CTX_new(TLS_client_method());//offer all alg; set PRNG
	//connect
	tcp_connect(peer_s, peerai);
	//ssl - host
	SSL* ssl = SSL_new(sslctx); // enc engine
	if( !SSL_set_tlsext_host_name(ssl,host) ){ //SNI
		quit(-1,"ERROR: SSL_set_tlsext_host_name failed\n","Quitting");
	}
	if( !SSL_set_fd(ssl,peer_s) ){
		quit(-1,"ERROR: SSL_set_fd failed","QUIT!");
	}
	//ssl connect
	int e;
	if( (e=SSL_connect(ssl)) <= 0 ){
		quit(e,"ERROR: SSL_connect failed\n","Handshake could not complete");
	}
	printf("Cipher: %s\n",SSL_get_cipher(ssl));
	
	while(1){
	//init FD_set
		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(STDIN_FILENO,&reads);
		FD_SET(peer_s, &reads);
		//select
		if( select(peer_s+1,&reads,0,0,0) <=0 ){
			quit(errno,"ERROR: select failed\n",strerror(errno));
		}
			//ssl read
			//ssl write
		if( FD_ISSET(peer_s,&reads) ){
			char iobuf[IOBUFSIZE+1];
			int rcvd = SSL_read(ssl, iobuf, IOBUFSIZE);
			if( rcvd <1){
				printf("\nConnection reset by peer\n");
				break;
			}
			iobuf[rcvd]=0;
			printf("%s",iobuf);
		}
		if(FD_ISSET(STDIN_FILENO, &reads)){
			char iobuf[IOBUFSIZE+1];
			if( !fgets(iobuf,IOBUFSIZE,stdin) ){
				quit(-1,"ERROR: couldn't read from stdin\n","Quitting");	
			}
			e = SSL_write(ssl, iobuf, strlen(iobuf));
			if( !e ) {
				char* logm;
				asprintf(&logm,"ERROR: SSL_write failure : iobuf[%lu]=%s\n",strlen(iobuf),iobuf);
				quit(e,logm,"QUITTING!");
			} 
		}
	}
	//SSL free,shutdown & close
	SSL_shutdown(ssl);
	return 0;
}
