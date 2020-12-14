// HTTPS client
// OSX: gcc src.c -L/usr/local/opt/openssl@1.1/lib -I/usr/local/opt/openssl@1.1/include -lssl -lcrypto
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/err.h>

#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#define DEFPORT "443"
#define IOBUFSIZE 2048

typedef int SOCKET;

struct addrinfo* gethostaddr(char* host, char* port){
	struct addrinfo hint, *srvadd;
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM ; //tcp
	int e = getaddrinfo(host, port, &hint, &srvadd);
	if( e != 0 ) {
		fprintf(stderr,"ERROR: getaddrinfo %d: %s\n",e,gai_strerror(e));
		errx(e,"QUITTING!");
	}
	return srvadd;
}

void get_requested_server(struct sockaddr* hostaddr, socklen_t hostaddrlen){
	char hostname[100], servicename[100];
	getnameinfo(hostaddr, hostaddrlen, hostname,100,servicename, 100, NI_NUMERICHOST);
	printf("Requested connecting to server:port %s:%s\n",hostname,servicename);
}
int main(int argc, char** argv){
	if( argc < 2 ){
		//printf("Usage: %s hostname [port(443)]\n",argv[0]);
		printf("Usage: %s hostname [path(/)] [-p port]\n",argv[0]);
		return 1;
	}
	char* host=argv[1];
	//char* port = (argc>2)? argv[2]: DEFPORT;
	char port[6] ;
	snprintf(port, strlen(DEFPORT)+1,"%s",DEFPORT);
	char path[2048] ;
	path[0]=0;
	printf("#DEBUG: command : %s %s ",argv[0],argv[1]);
	if(argc>2){
		switch(argc){
			case 3:
				snprintf(path,strlen(argv[2])+1,"%s",argv[2]);
				printf("%s\n",argv[2]);
				break;
			case 4:
				if( strcmp(argv[2],"-p")!=0 ){
					fprintf(stderr,"ERROR: expected port option -p : found %s instead\n",argv[2]);
					errx(-1,"Wrong usage");
				}
				snprintf(port,strlen(argv[3])+1,"%s",argv[3]);
				printf("%s %s\n",argv[2],argv[3]);
				break;
			case 5:
				snprintf(path,strlen(argv[2])+1,"%s",argv[2]);
				if( strcmp(argv[3],"-p")!=0 ){
					fprintf(stderr,"ERROR: expected port option -p : found %s instead\n",argv[2]);
					errx(-1,"Wrong usage");
				}
				snprintf(port,strlen(argv[4])+1,"%s",argv[4]);
				printf("%s %s %s\n",argv[2],argv[3],argv[4]);
				break;
			default:
				fprintf(stderr,"Wrong number of arguments\n");
				errx(-1,"Check usage: ./%s\n",argv[0]);
		}
	}
	if(argc<3) printf("\n");
	printf("#DEBUG: Setting up connection to %s:%s (default port: %s[%lu])\n",host,port,DEFPORT,strlen(DEFPORT));

	struct addrinfo* hostadd = gethostaddr(host,port);
	get_requested_server(hostadd->ai_addr, hostadd->ai_addrlen);
	SOCKET host_s =  socket(hostadd->ai_family, hostadd->ai_socktype, hostadd->ai_protocol);

	printf("#DEBUG: host socket set\n");

	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	SSL_CTX* sslctx = SSL_CTX_new(TLS_client_method()); //agree on best algo w/ client

	int e = connect(host_s, hostadd->ai_addr, hostadd->ai_addrlen);
	freeaddrinfo(hostadd);

	printf("#DEBUG: SSL context set; handshake w/ host return value:%d\n",e);

	// SSL connection
	SSL* ssl = SSL_new(sslctx);

	if( !SSL_set_tlsext_host_name(ssl, host) ){  // use SNI
		fprintf(stderr, "ERROR: SSL_set_tlsext_host_name failed: Couldn't find valid certificate\n");
		ERR_print_errors_fp(stderr);
		return 7;
	}	
	
	SSL_set_fd(ssl,host_s);

	printf("#DEBUG: TCP connection wrapped under TLS\n");
	printf("#DEBUG: Trying SSL connect...\n");

	e = SSL_connect(ssl);
	printf("#DEBUG: SSL connect returned...\n");

	if( e < 1 ){
		fprintf(stderr,"ERROR: Failed to establish TLS/SSL connection : e=%d\n",e);
		ERR_print_errors_fp(stderr);
		return 8;
	}
	printf("Using TLS/SSL cypher %s\n",SSL_get_cipher(ssl));

	// Cetificate
	X509* cert = SSL_get_peer_certificate(ssl);
	if( !cert) {
		fprintf(stderr,"ERROR: could not retrieve peer certificate!\n");
		return 9;
	}
	printf("#DEBUG: Retrieved TSL/SSL X509 certificate from host\n");

	// 			Certificate info
	char* info;
	info = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
	if( info ){
		printf("Certificate's subject name:%s\n",info);
		OPENSSL_free(info);
	}
	info = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
	if( info ){
		printf("Certificate's issuer's name:%s\n",info);
		OPENSSL_free(info);
	}
	printf("#DEBUG: Last seen error code: %lu\n",ERR_get_error());
	X509_free(cert);

	/* 			Certificate validation
	if( !SSL_CTX_load_verify_locations(sslctx,"trusted_certificates.pem", 0) ){
		fprintf(stderr,"ERROR: failed to load trusted certificates\n");
		ERR_print_errors_fp(stderr);
		return 10;
	}
	long vp = SSL_get_verify_result(ssl);
	if( vp == X509_V_OK) {printf("Validated certificate\n");}
	else {printf("WARNING: coudln't validate certificate\n");}
	*/

	printf("#DEBUG: Proceeding with HTTP(S) request\n");
	// HTTP(S)
	char iobuf[IOBUFSIZE+1];
	sprintf(iobuf,"GET /%s HTTP/1.1\r\n",path);
	sprintf(iobuf+strlen(iobuf),"Host: %s:%s\r\n",host,port);
	sprintf(iobuf+strlen(iobuf),"Connection: close\r\n");
	sprintf(iobuf+strlen(iobuf),"User-Agent: myHTTPSclient\r\n");
	sprintf(iobuf+strlen(iobuf),"\r\n");
	
	printf("#DEBUG: request sending:\n%s\n\n",iobuf);
	
	// send	
	SSL_write(ssl,iobuf,strlen(iobuf));
	
	// receive
	printf("#DEBUG: Receiving...\n");
	while( 1 ){
		int rcvd = SSL_read(ssl,iobuf,IOBUFSIZE);
		if( rcvd < 1 ){
			printf("\nConnection closed by peer.\n");
			break;
		}
		iobuf[rcvd]=0;
		//printf("#DEBUG:Received-%d-bytes:\n",rcvd);
		printf("%s",iobuf);
	}

	SSL_shutdown(ssl);
	close(host_s);
	SSL_free(ssl);
	SSL_CTX_free(sslctx);

	printf("\n\nConnection ended\n");
	return 0;
}
