/*
*	Program:	server_web120.c
*   Author:		Paul Girard Ph.D., DIM, UQAC
*   Date:		Sept 2012
*	Version:     	1.2
*
*   Objective:    This basic web program supports http 1.0 and 1.1 :		
*               - GET method from a client commercial web navigator
*				- validates the request GET for error codes 400 and 404
*				- and generates a response following http protocol rules
*				(code 200).  html pages are coded in the program source.
*
*			This version doesn't support access to external files. 
*			Most http client header parameters are ignored :
*			(User_Agent, From, Referer, Connection, Accept, ...) 
*
*	Compilation and link options : (Solaris)          
*		gcc server_web120.c -lsocket -lnsl -o server_web120 or
*
*		gcc server_web120.c -lsocket -lnsl -w -o server_web120 
*						to suppress "warnings"

	Compilation and link options : (Linux)          
*		gcc server_web120.c -lnsl -o server_web120 or
*
*		gcc server_web120.c -lnsl -w -o server_web120 
*						to suppress "warnings"
*
*   Execution: 2 formal parameters to run the server :
*						 server_name and port #
*				dimensxcn1% server_web120 port# 
*				(note: port 80 is assigned to a commercial http server
*		ex. 	dimenszxcn1% server_web120 5001
*
*		The following html pages are coded directly in the source
*		: HOME_PAGE and TIME. They can be called  
*		like this by a telnet client or a web client navigator
*
*		1. with a telnet client using http 1.0 protocol:
*		 	telnet sunense.uqac.ca  port#	(no_port: port # used)
*			GET / HTTP/1.0 		to get HOME_PAGE and an empty
*			cr/lf			line to terminate the request
*		 	telnet sunense.uqac.ca  port#
*			GET /time HTTP/1.0	to get TIME
*			cr/lf
*
*		2. with telnet using HTTP/1.1 protocol and Apache:
*		 	telnet sunense.uqac.ca 80
*			GET / HTTP/1.1
*			Host: dimensxcn1.uqac.ca
*			cr/lf
*		 	telnet dimensxcn1.uqac.ca  80
*			Host: dimensxcn1.uqac.ca
*			GET /time HTTP/1.1
*			Host: dimensxcn1.uqac.ca
*			cr/lf
*
*		3. with Netscape or Explorer client
*			http://dimensxcn1.uqac.ca :no_port			to get Home_Page
*			http://dimensxcn1.uqac.ca :no_port/time		to get time
*/

/*
*       Files to include
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/stat.h>

#define BUFFSIZE	256
#define SERVER_NAME	"Mini web server  V1.0 (Paul Girard, Ph.D., UQAC)"

#define ERROR_400	"<head></head><body><html><h1>Error 400</h1><p>Th\
e server couldn't understand your request.</html></body>\n"

#define ERROR_404	"<head></head><body><html><h1>Error 404</h1><p>Do\
cument not found.</html></body>\n"

#define HOME_PAGE	"<head></head><body><html><h1>Welcome to the mini \
web server PG_web</h1><p>Let me invite you to visit : <ul><li><a href=\
\"http:\/\/wwwens.uqac.ca/~pgirard\"><font color=\"#0000ff\"> my personal \
site</font></a></li></ul></html></body>\n"

#define TIME_PAGE	"<head></head><body><html><h1>The actual date is : \
<br><br>%s</h1></html></body>\n"

#define TYPE_HTML "text/html"
#define TYPE_IMG_GIF "image/gif"
#define TYPE_IMG_JPG "image/jpeg"

int	recvln(int, char *, int);    	/* read a client http line  */
void send_header(int, int , int , char *);   	/* send a http header to client*/
int	wait_connection(short);			/* wait for a client connection */
void send_eof(int);					/* close a client connection */

int	init = 0;						/* initialization */
int	sock;							/* socket */
struct stat buf ;
/*Here is my private function*/
int html(int i,char * buff,char * url){
	//If the access is HTML use this function.
	FILE * file_fptr ;
	if(stat(url,&buf) !=0 )  
	{
		error(i , 404) ;
		return 0;
	}

	if((file_fptr = fopen(url,"r")) == NULL) 
	{
		error(i , 404) ;
		return 0;
	} 
	send_header(i, 200 , buf.st_size , TYPE_HTML);
	while(!feof(file_fptr))
	{
		fgets(buff,BUFFSIZE,file_fptr) ;
		send(i, buff, strlen(buff), 0);
	}	
	fclose(file_fptr) ;
	return buf.st_size ;
}

int img(int i , int type , char * buff){
	//If the access is image use this function.
	FILE * file_fptr ;
	char *url ;
	char *file_type ;
	int rval ;
	if(type == 0 )
	{
		url = "document/image.gif" ;
		file_type = TYPE_IMG_GIF;
	}else
	{
		url = "document/image.jpg" ;
		file_type = TYPE_IMG_JPG;
	}
	if(stat(url,&buf) !=0 )
	{
		error(i , 404) ;
		return 0;
	}

	if((file_fptr = fopen(url,"r")) == NULL)
	{
		error(i , 404) ;
		return 0;
	}
	send_header(i, 200,buf.st_size , file_type);
	while(!feof(file_fptr))
	{
		rval = fread(buff,sizeof(char) ,BUFFSIZE, file_fptr);
		if(rval)
		{
			send(i, buff, rval, 0);
		}
	}
	fclose(file_fptr) ;
	return buf.st_size ;
}

/* ***************************************************************************
*	Main Program
*  ***************************************************************************
*/
int main(int argc, char *argv[]) 	/* argv[0] pointer to program name
        							argv[1] pointer to port number */
{
	int size;
	int		conn;		/* socket descriptor */
	int		n;
	char	buff[BUFFSIZE], cmd[16], path[64], vers[16];
	char	*timestr;
	struct timeval	tv;		/* delay in sec since 1/1/1970 */
	time_t a;			/* function time */
	if (argc != 2) 
	{
		fprintf(stderr, "usage : %s <no_port_TCP>\n", argv[0]);
		return 1;
	}

	while(1) 
	{

/* 		Wait for a client connection on the known port # */

		conn = wait_connection(atoi(argv[1]));
		if (conn < 0) return 1;

/* 		Reads & validates the request client */

		n = recvln(conn, buff, BUFFSIZE);
		sscanf(buff, "%s %s %s", cmd, path, vers);
		printf("cmd : %s path : %s vers : %s\n",cmd, path, vers);

/* 		Ignore other client header parameters until \r\n on a line */

		while((n = recvln(conn, buff, BUFFSIZE)) > 0) 
		{
			if (n == 2 && buff[0] == '\r' && buff[1] == '\n')
				break;
		}

/* 		Check if there is an unexpected EOF */

		if (n < 1) 
		{
			send_eof(conn);
			continue;
		}
		
/* 		Validates the http client request  */
		
		if (strcmp(cmd, "GET") != 0 && 
			(strcmp(vers, "HTTP/1.0") != 0 ||
			 strcmp(vers, "HTTP/1.1") != 0))
		{		
			send_header(conn, 400, strlen(ERROR_400),strlen(ERROR_400));
			send(conn, ERROR_400, strlen(ERROR_400),0);
			send_eof(conn);
			continue;
		}

/* 		Transmit back the requested html page or 
*		the message "not found" error */

		if (strcmp(path, "/") == 0) 		/* HOME_PAGE by default*/
		{
			size = html(conn,buff,"page1.html");
			if(size){
				printf("File Size :%d octets\n", size ) ;
				printf("File content type html\n") ;
			}else
				continue;
		} 
		else{
			if(strstr(path, "..") != NULL){
				send_header(conn, 400, strlen(ERROR_400),strlen(ERROR_400));
				send(conn, ERROR_400, strlen(ERROR_400),0);
				send_eof(conn);  
				continue;
			}
			else if (strcmp(path, "/time") == 0) 	/* time page */
				 {
				/*	Solaris ==>		if (gettimeofday(&tv, NULL) == -1) 
						puts("Error with gettimeofday()");
						timestr = ctime(&tv.tv_sec);	*/

				/* testtime.c: test of time function*/
					time(&a);		/* sec since 1970 */
					timestr = ctime(&a);	/* formatted date-time */	
					sprintf(buff, TIME_PAGE, timestr);
					send_header(conn, 200, strlen(buff),TYPE_HTML);
					send(conn, buff, strlen(buff), 0);
				}
			else if(strstr(path , ".") != NULL)
			{
				if(strstr(path ,".html") != NULL)
				{
					size = html(conn , buff , path+1 ) ;
					if(size)
					{
						printf("File Size :%d octets\n", size ) ;
						printf("File content type html\n") ;
					}
					else
						continue ;
				}
				if((strstr(path ,".jpg") != NULL) ||
					(strstr(path ,".JPG") != NULL) ||
					(strstr(path ,".jpeg") != NULL) ||
					(strstr(path ,".JPEG") != NULL) )
				{
					size = img(conn , 1 , buff) ;
					if(size){
						printf("File Size :%d octets\n", size ) ;
						printf("File content type html\n") ;
					}
					else
						continue ;
				}
				if((strstr(path ,".gif") != NULL) || (strstr(path ,".GIF") != NULL))
				{
					size = img(conn , 0 , buff) ;
					if(size){
						printf("File Size :%d octets\n", size ) ;
						printf("File content type html\n") ;
					}
					else
						continue ;
				}
			}
			else
			{
				size = html(conn , buff , strcat(path+1,".html") ) ;
				if(size)
				{
					printf("File Size :%d octets\n", size ) ;
					printf("File content type html\n") ;
				}
				else
					continue ;
			}
		}      
		send_eof(conn);			/* close the client socket */
	}	/* end of infinite loop */
}		/* ==== end of main program */

/* *************************************************************************
*	Function wait_connection() returns a client connection 
*  *************************************************************************
*/

int wait_connection(short no_port)
{
	struct sockaddr_in sockaddr;	/* internet format */
    int newsock;		/* socket descriptor */
	size_t length;		/* #octets in sockadr_in structure*/ 

/*
* 	Tcp socket creation
*/

	switch (init)
	{
	case 0 :
        	sock = socket(AF_INET, SOCK_STREAM, 0);
        	if (sock < 0) 
	  		{
                	perror("Error in socket creation");
                	return 1;
          	} 

/*
*		2. Specify the local part of the address :
*		1)the local port number in network format and
*		2)the local IP address.  INADDR_ANY is used on a server
*		because many ip addresses may be used on the same machine.
*/
        	sockaddr.sin_family = AF_INET;
        	sockaddr.sin_addr.s_addr = INADDR_ANY;
        	sockaddr.sin_port = htons(no_port);
        
        	if (bind(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
	  		{
      			perror("The bind socket did not work");
         		close(sock);
         		return 1;
          	}

/*
*		Simple validation: find the name associated to this socket
*	  	and print its port number
*/
        	length = sizeof(sockaddr);
        	if (getsockname(sock, (struct sockaddr *)&sockaddr, &length ) < 0 )
	  		{
                	perror("Error in getsockname()");
                	exit(1);
          	}
        	printf("The socket tcp port # is  #%d\n", ntohs(sockaddr.sin_port));

/*
*		Fix the maximum number of clients waiting connection and 
*		leave the server in passive mode
*/	
			listen(sock, 5);
			puts("Server ready for client connection");
			init = 1;	/* end of initialization */
	
	case 1: 	/* after initialization continue here */
			newsock = accept(sock, (struct sockaddr *) 0, (size_t *) 0);
			return newsock;
	}
}
/* ==== End of wait_connection ================================================ */




/* *************************************************************************
*	Function recvlin() reading a socket connection with recv()
*	until newline (\n) or EOF is read. A flush of the remaining characters 
*	is done until newline or EOF if the buffer is full. It returns a full 
*	buffer. 
*
*	conn: socket # (input)
*	*buff: address of the buffer receiving a complete line (input)
*	buffsz: buffer size in octets (input)
*   ==>	recvln returns the length in octets of read characters in buff 
* **************************************************************************
*/

int recvln(int conn, char *buff, int buffsz)
{	
	char	*bp = buff, c;
	int	n;			/* #octets read by recv() */

/*  	While buffer not full and we can read characters 
*   	different from \n, octets are saved in buff
*/
	while (bp - buff < buffsz && 
	      (n = recv(conn, bp, 1, 0)) > 0)
	{
		if (*bp++ == '\n') return (bp - buff);
	}

	if (n < 0)		/* read has failed */
		return -1;
		
/* 	If the buffer is full, ignore the rest until \n */
	if (bp - buff == buffsz)
		while (recv(conn, &c, 1, 0) > 0 && c != '\n');
	return (bp - buff);
}

/* ==== End de recvln ====================================================== */




/* *************************************************************************
* 	Function send_header() sending the http 1.0 header to client  
*	with the status, server name, content type and content length
*  *************************************************************************
*/
 
void send_header(int conn, int stat ,  int len ,char * content_tpye)
{
	char	*statstr, buff[BUFFSIZE];

/* 	change the status code to a character string message */

	switch(stat) 
	{
	case 200:
		statstr = "OK";
		break;
	case 400:
		statstr = "Bad Request";
		break;
	case 404:
		statstr = "Not Found";
		break;
	default:
		statstr = "Unknown";
		break;
	}
	
/*
* 	send the parameter server header 
*	"HTTP/1.0 Server Content-Length Content-Type" to client
*/

	sprintf(buff, "HTTP/1.0 %d %s\r\n", stat, statstr);
	send(conn, buff, strlen(buff), 0);

	sprintf(buff, "Server: %s\r\n", SERVER_NAME); send(conn, buff, strlen(buff), 0);

	sprintf(buff, "Content-Length: %d\r\n", len);
	send(conn, buff, strlen(buff), 0);

	sprintf(buff, "Content-Type: %s\r\n",content_tpye);
	send(conn, buff, strlen(buff), 0);

	sprintf(buff, "\r\n");
	send(conn, buff, strlen(buff), 0);
}	
/* ===================== end of send_header ================================ */




/* ****************************************************************************
* 	Function send_eof() closes the connection from this end
*	by using a shutdown() call which shuts down all or part of a full-duplex
*	connection on the associated socket .
*   	0 : 	further receives will be disallowed, 
*		l :  	further sends will be disallowed (this mode is used),
*		2: 	further sends and receives will be disallowed
* *****************************************************************************
*/

void send_eof(int conn)
{					
	shutdown(conn,1);
	return; 						
}

/*===================== end of send_eof =================================== */
