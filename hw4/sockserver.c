#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>         /* for read, write, open, close ,etc*/
#include <ctype.h>
#include <signal.h>
#define SERV_TCP_PORT	9500
#define BUFSIZE 50000
	
typedef struct source
{
    char vn,cd;
    char destport[2],destip[4];
    char usrID[50];

} sourceinfo;


int checkfw(char *destip){
	//printf("destip : %s\n",destip);
	int filefd;
	int result;
	char buffer[200];
	int defa; //0 permit 1 deny
	memset(buffer,0,sizeof(buffer));
	filefd = open("sock.conf",O_RDONLY);
	while( readline(filefd,buffer,sizeof(buffer)) > 0){
		if(strstr(buffer,"default") != NULL){
			if(strcmp(buffer,"permit") == 0){
				defa = 0;
				continue;
			}
			if(strcmp(buffer,"deny") == 0){
				defa = 1;
				continue;
			}
		}
		if(strncmp(buffer,"permit",6) == 0 ){
			char *tmp = buffer+7;
			char *tmp2 = strstr(buffer,"*");
			strtok(buffer,"*");
		//	printf("tmp %s\n",tmp);
			if(tmp2 != NULL)
				result = strncmp(tmp,destip,strlen(tmp));
			else 
				result = strcmp(tmp,destip);
			if(result == 0 )
				return 0;
			

		}
		if(strncmp(buffer,"deny",4) == 0 ){
			char *tmp = buffer + 5;
                        char *tmp2 = strstr(buffer,"*");
			strtok(buffer,"*");
			printf("tmp %s\n",tmp);
                        if(tmp2 != NULL){
                                result = strncmp(tmp,destip,strlen(tmp));
				//printf("1111111111111111111111\n");
			}
                        else
                                result = strcmp(tmp,destip);
                        if(result == 0 )
                                return 1;


		}		
	}
	return defa;
	
	

}

sourceinfo source;

int readline(int fd,unsigned char *ptr,int maxlen)
{
    int n,rc;
    unsigned char c;
    *ptr = 0;
    for(n=1; n<maxlen; n++)
    {
        fflush(stdout);
        if((rc=read(fd,&c,1)) == 1)
        {

            *ptr++ = c;
            if(c=='\n')  break;
        }
        else if(errno == EAGAIN )
        {
            if(n==1)     return(0);
            else         break;
        }
        else
            return(-1);
    }
    return(n);
}

char* rstrip(char* str)
{
    int i = strlen(str)-1;
    while(isspace(str[i]))
        str[i--]='\0';
    return str;
}


int communication(int rsock,int ssock)
{
	char *buffer;
	int n;
	int nfds;
	fd_set rs,rfds,ws,wfds;
	nfds=getdtablesize();
    bzero(&rs,sizeof(fd_set));
    bzero(&rfds,sizeof(fd_set));
	buffer=(char *)malloc(BUFSIZE);
	bzero(buffer,BUFSIZE*sizeof(char));
	FD_SET(ssock,&rs);
	FD_SET(rsock,&rs);

	while(1){
		memcpy(&rfds,&rs,sizeof(fd_set));
		//memcpy(&wfds,&ws,sizeof(fd_set));
		bzero(buffer,BUFSIZE*sizeof(char));
		select(nfds,&rfds,(fd_set*)0,(fd_set*)0,(struct timeval *)0);
		if(FD_ISSET(rsock,&rfds)){
			n=read(rsock,buffer,1000);
			if(n<0){
				perror("read line error:");
				close(rsock);
				break;
			}
			else if(n>0){
				write(ssock,buffer,n);
			}
			else{
				close(rsock);
				break;
			}
		}
		else if(FD_ISSET(ssock,&rfds)){
			n=read(ssock,buffer,1000);
			if(n<0){
				perror("read line error:");
				close(ssock);
				break;
			}
			else if(n>0){
				write(rsock,buffer,n);
			}
			else{
				close(ssock);
				break;
			}			
		}
		else{
			close(rsock);
			close(ssock);
			break;
		}
	}
	free(buffer);
	return 0;

}




int OptionValue = 1;
void    SigChgChildHandler(int n)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void socks_redirect(int sourcefd)
{
    int destfd;
    int rbyte;
    unsigned char buffer[3000];
    char hostname[100];
    char destip[20];
    char *tmp;
    int skfd;
    int conn;
    int bexit=0;
    int pid;
    int permit;   
    struct sockaddr_in dest;
    struct in_addr inaddr;
    memset(hostname,'\0',sizeof(hostname));
    memset(buffer,'\0',sizeof(buffer));
    memset(destip,'\0',20);
    fflush(stdout);

    rbyte = read(sourcefd,buffer,200);
//	buffer[2]=4;buffer[3]=210;
//	buffer[4]=127;buffer[5]=0;buffer[6]=0;buffer[7]=1;
    if(buffer[1] == 1)
    {
        unsigned char VN = buffer[0] ;
        unsigned char CD = buffer[1] ;
        unsigned int DST_PORT = buffer[2] << 8 | buffer[3] ;
        unsigned long int DST_IP = buffer[4] << 24 | buffer[5] << 16 | buffer[6] << 8 | buffer[7] ;
        char* USER_ID = buffer + 9 ;
//	DST_PORT = 8899;
	sprintf(destip,"%u.%u.%u.%u",buffer[4],buffer[5],buffer[6],buffer[7]);
        printf("vn = %u cd = %u dst_port = %u dst_ip = %u.%u.%u.%u id = %s\n",VN,CD,DST_PORT,buffer[4],buffer[5],buffer[6],buffer[7],USER_ID);
//	printf("ip = %s",inet_ntoa(DST_IP));
//	printf("%16s\n",inet_ntoa(ntohl(DST_IP)));
	permit = checkfw(destip);
	if(permit == 1){
	    printf("this is deny\n");
	    unsigned char package[8];
            package[0] = 0;
            package[1] = 91 ; // 90 or 91
            package[2] = DST_PORT/256;
            package[3] = DST_PORT % 256;
            package[4] = buffer[4];
            package[5] = buffer[5];
            package[6] = buffer[6];
            package[7] = buffer[7];
            write(sourcefd, package, 8);
	    exit(1);

	}
        skfd = socket(AF_INET, SOCK_STREAM, 0);
        bzero(&dest,sizeof(dest));
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = ntohl(DST_IP);
        dest.sin_port = htons(DST_PORT);
        if( (conn = connect(skfd,(struct sockaddr *)&dest,sizeof(dest))) < 0)
        {

            printf("connerr");
	    exit(0);
        }
        else
        {
            unsigned char package[8];
            package[0] = 0;
            package[1] = 90 ; // 90 or 91
            package[2] = DST_PORT/256;
            package[3] = DST_PORT % 256;
            package[4] = buffer[4];
            package[5] = buffer[5];
            package[6] = buffer[6];
            package[7] = buffer[7];
            write(sourcefd, package, 8);
            int readbyte = 0;
     //       printf("reply\n");
            usleep(3000);
	    communication(sourcefd,skfd);

        }

    } 
    if(buffer[1] == 2)
    {
        int destsockfd;
        struct sockaddr_in client_addr;
        socklen_t clientlen;
	socklen_t len = sizeof(dest);
 	int port;
        unsigned char VN = buffer[0] ;
        unsigned char CD = buffer[1] ;
        unsigned int DST_PORT = buffer[2] << 8 | buffer[3] ;
        unsigned long int DST_IP = buffer[4] << 24 | buffer[5] << 16 | buffer[6] << 8 | buffer[7] ;
        char* USER_ID = buffer + 9 ;
	sprintf(destip,"%u.%u.%u.%u",buffer[4],buffer[5],buffer[6],buffer[7]);
	permit = checkfw(destip);
        if(permit == 1){
	    printf("this is deny\n");
            unsigned char package[8];
            package[0] = 0;
            package[1] = 91 ; // 90 or 91
            package[2] = DST_PORT/256;
            package[3] = DST_PORT % 256;
            package[4] = buffer[4];
            package[5] = buffer[5];
            package[6] = buffer[6];
            package[7] = buffer[7];
            write(sourcefd, package, 8);
            exit(1);

        }

    	struct sockaddr_in sin;
	int sockfd,type,skfd;
	bzero((char *)&sin,sizeof(sin));
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=htonl(INADDR_ANY);
	type=SOCK_STREAM;
	sockfd=socket(PF_INET,type,0);
	if(sockfd<0){
		//errexit("usage:connect server error");
		printf("socket fail\n");
		exit(1);
	}
	if(bind(sockfd,(struct sockaddr *)&sin,sizeof(sin))<0){
		//errexit("usage:connect server error");
		printf("bind fail\n");
		exit(1);
	}
	if(type==SOCK_STREAM&&listen(sockfd,32)<0){
		//errexit("usage:connect server error");
		printf("listen fail\n");
		exit(1);
	}
	clientlen = sizeof(sin);	

	getsockname(sockfd,(struct sockaddr *)&sin,&clientlen);
	port = ntohs(sin.sin_port);
	
	unsigned char package[8];
        package[0] = 0; 
        package[1] = 90 ; // 90 or 91
        package[2] = port /256;
        package[3] = port % 256;
        package[4] = 140;
        package[5] = 113;
        package[6] = 235;
        package[7] = 165;
	write(sourcefd, package, 8);
	if((destsockfd=accept(sockfd,(struct sockaddr *)&sin,&clientlen))<0){

			exit(1);					

	}
	
        write(sourcefd, package, 8);
        usleep(3000);
        printf("bind\n");

	communication(sourcefd,destsockfd);
              
    }








}















int main(int argc, char** argv, char **envp)
{

    int sockfd, newsockfd, childpid, ServerPortNumber = SERV_TCP_PORT;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;
    struct sigaction    SignalAction;


    /* Open a TCP socket (an Internet stream socket) */

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror("server: can't open stream socket!");

    /* SOLVE THE "bind: Address already in use" PROBLEM!!!" */
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                  (const void *)&OptionValue, sizeof(socklen_t)) == -1)
    {
        perror("server: setsockopt failed! ");
        exit(errno);
    }


    /* Bind our local address so that the client can send to us*/

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons(ServerPortNumber);

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        perror("server: can't bind local address!");

    /* Listening*/
    listen(sockfd, 5);

    /* reap all dead processes */
    SignalAction.sa_handler = SigChgChildHandler;
    sigemptyset(&SignalAction.sa_mask);
    SignalAction.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &SignalAction, NULL) == -1)
    {
        perror("sigaction");
        exit(-1);
    }

    fprintf(stderr,"Service started on port %d !\n",ServerPortNumber);

    for(;;)
    {
        /* Wait for a connection from a client process.*/

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if(newsockfd < 0)
            perror("server: accept error!");

        fprintf(stderr, "-%16s:%d connected\n",inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        if((childpid = fork()) < 0)
            perror("server: fork error!");

        else if (childpid == 0)       /* child process */
        {

            close(sockfd);            /* close original socket */
            socks_redirect(newsockfd);
			close(newsockfd);
            exit(1);
        }

        close(newsockfd);
    }
}
