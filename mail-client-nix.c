
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>		//va_start, va_list, va_end
#include <ctype.h>		//isdigit
#include <stdlib.h>		//strtol
#include <time.h>

typedef int SOCKET;

#define SMTPP "25"

#define INPBUFSIZE 512
#define OUTBUFSIZE 1024

void _get_input(const char* prmt, char* buf){
	printf("%s",prmt);
	buf[0]=0;
	fgets(buf,INPBUFSIZE,stdin);
	int read = strlen(buf);
	if ( read > 0) buf[read-1]=0;		//override newline at the end.
}

void _sendfmt(SOCKET srv, char* txt, ...){
	char buf[INPBUFSIZE+1];
	va_list args;
	va_start(args, txt);
	vsnprintf(buf, INPBUFSIZE+1, txt, args);
	va_end(args);

	send(srv, buf,strlen(buf), 0);
	
	printf("C: %s",buf);
}

int paresp_code(const char* resp){
	const char* k = resp;
	if( !k[0] || !k[1] || !k[2] ) return 0;
	for( ; k[3] ; k++){
		if( k == resp || k[-1] == '\n'){
			if( isdigit(k[0]) && isdigit(k[1]) && isdigit(k[2]) &&
				k[3] != '-' && strstr(k,"\r\n") 
			) return strtol(k,0,10);
		}
	}
	return 0;
}

void buffer_srvresp(SOCKET srv , int exp_code){
	char buffer[INPBUFSIZE+1] ;
	char* p = buffer ;
	char* end = buffer + INPBUFSIZE ;
	int code = 0;
	
	do{
		int rcvd = recv(srv, p, end-p, 0);
		if( rcvd <=0 ){
			const char* msg = "ERROR: Connection close or broken";
			fprintf(stderr,"%s\n",msg);
			perror(msg);
			errx(errno,"connection closed or broken");
		}
		p += rcvd;
		*p = 0;
		if( p == end ) {
			fprintf(stderr,"ERROR: Message from server too long:\n%s\n",buffer);
			errx(1,"Message too long");
		}
		code = paresp_code(buffer);
	} while( !code );

	if ( code != exp_code ){
		fprintf(stderr,"ERROR: SMTP server send a wrong code:\n%s\ncode:",buffer);
		errx(code, "Wrong code received from server");
	}
	printf("S: %s", buffer);
}

SOCKET _conn(const char* host, const char* port){
	struct addrinfo hint, *srvadd;
	memset(&hint,0,sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;

	int e = getaddrinfo(host, port, &hint, &srvadd);
	if( e ){
		fprintf(stderr,"ERROR: getaddrinfo failed %s\n",gai_strerror(e));
		errx(e,"getaddrinfo failed");
	}
	SOCKET srv_s = socket(srvadd->ai_family, srvadd->ai_socktype, srvadd->ai_protocol);
	if ( srv_s <=0 ){
		fprintf(stderr,"ERROR: failed to setup socket\n");
		perror("");
		errx(2,"socket failed");
	}
	e = connect(srv_s, srvadd->ai_addr, srvadd->ai_addrlen) ;
	if( e <- 0 ){
		fprintf(stderr,"ERROR: couldn't connect to SMTP server\n");
		perror("Connection failed");
		errx(errno, "connection to server failed");
	} 
	freeaddrinfo(srvadd);
	return srv_s;
}

int main(int argc, char** argv){	
	char mserver[INPBUFSIZE] ;
	_get_input("Mail server:",mserver);

	printf("Connecting to host: %s:25\n",mserver);
	SOCKET srv_s = _conn(mserver, "25");
	buffer_srvresp(srv_s, 220);
	
	_sendfmt(srv_s, "HELO %s\r\n", mserver);
	buffer_srvresp(srv_s, 250);

	char sender[INPBUFSIZE];
	char recipient[INPBUFSIZE];
	char subject[INPBUFSIZE];
	_get_input("From: ", sender);
	_get_input("To: ", recipient);
	_get_input("Subject: ", subject);
	_sendfmt(srv_s,"MAIL FROM:<%s>\r\n",sender);
	buffer_srvresp(srv_s, 250);
	_sendfmt(srv_s,"RCPT TO:<%s>\r\n",recipient);
	buffer_srvresp(srv_s, 250);
	_sendfmt(srv_s, "DATA\r\n");
	buffer_srvresp(srv_s, 354);

	//Mail headers
	time_t zeit;
	time(&zeit);
	struct tm *zeitinf = gmtime(&zeit);
	char date[128];
	strftime(date,128,"%a, %d %b %Y %H:%M:%S  +0000",zeitinf);

	_sendfmt(srv_s, "From:<%s>\r\n",sender);
	_sendfmt(srv_s, "To:<%s>\r\n",recipient);
	_sendfmt(srv_s, "Subject:%s\r\n",subject);
	_sendfmt(srv_s, "Date:%s\r\n",date);
	_sendfmt(srv_s, "\r\n");

	//Mail body
	while( 1 ){
		char body_chunk[INPBUFSIZE];
		_get_input("> ",body_chunk);
		_sendfmt(srv_s, "%s\r\n",body_chunk);
		if( strcmp(body_chunk, ".")==0) break;
	}
	buffer_srvresp(srv_s, 250);

	_sendfmt(srv_s, "QUIT\r\n");
	buffer_srvresp(srv_s, 221);

	close(srv_s);
	printf("Closing connection with server\nFinished\n");
	
	return 0;
}
