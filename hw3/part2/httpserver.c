#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>	/* BSD */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9988

int optval = 1;
extern char **environ;
void    SigChgChildHandler(int n){
	while(waitpid(-1, NULL, WNOHANG) > 0);

}


void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


void parseandexec(char* req){
	/*token[0]: GET,POST...
	  token[1]: url
	  token[2]: HTTP
	*/
	int fd;
	FILE reqfile;
	char token[10][300];
	char *quenv,*filename;
	memset(token,'\0',sizeof(token));
	char *tmp;
	tmp = strtok(req," \t\n");
	strcpy(token[0],tmp);
	tmp = strtok(NULL," \t");
	strcpy(token[1],tmp);
	tmp = strtok(NULL , " \t\n");
	strcpy(token[2],tmp);
//	fprintf(stderr,"token[0] : %s\n token[1] : %s\n token[2] : %s\n",token[0],token[1],token[2]);
	filename = strtok(token[1],"?");
	quenv = filename + strlen(filename)+1;
	fprintf(stderr,"filename : %s\nquenv : %s\n" ,filename,quenv);

	 if ( strncmp( token[2], "HTTP/1.0", 8)!=0 && strncmp( token[2], "HTTP/1.1", 8)!=0 )
	{
		 printf("HTTP/1.0 400 Bad Request\n");
		 fflush(stdout);
		 return;
	}
	 if(strcmp(token[0], "GET") == 0 || strcmp(token[0], "POST") == 0)
	{
		printf("HTTP/1.1 200 OK\n");
		fflush(stdout); 
	}
	else {
		printf("HTTP/1.0 400 Bad Request\n");
		fflush(stdout);
		return;
	}
	/*cgi case */
	if(strstr(filename,".cgi") != NULL){
		fprintf(stderr,"in cgicase\n");
		if(strlen(quenv)>0 )
			setenv("QUERY_STRING", quenv, 1);
			filename++;
			
		 if(execlp(filename, filename, NULL) == -1)
             {
                 perror("execlp");
                 exit(1);
			}
	}
	/* html case*/
	if(strstr(filename,".htm")!=NULL || strstr(filename,".html")){
		int byteread;
		filename++;
		char htm_buf[10240];
		memset(htm_buf,'\0',sizeof(htm_buf));
		if ( (fd=open(filename, O_RDONLY))!=-1 )    //FILE FOUND
			{
			//	send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);
				while ( (byteread = read(fd, htm_buf, sizeof(htm_buf)))>0 )
					printf("%s\n",htm_buf);
			}
			else{
				printf("HTTP/1.0 404 Not Found\n");
				fflush(stdout);
				return;
			}


		
	}
	
	

	


}






int main(){
	struct sigaction    SignalAction; 
	int ServerSockfd;
	int ClientSockfd;
	int port = PORT;
	int reqlen=0;
	char req_buf[5000];
	struct sockaddr_in ServerAddr;
	struct sockaddr_in ClientAddr;
	socklen_t sin_size = sizeof(struct sockaddr_in);
		if((ServerSockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Server: socket open error!");
		exit(errno);
	}

		/* solves the "bind: Address already in use" problem */
	if(setsockopt(ServerSockfd, SOL_SOCKET, SO_REUSEADDR, 
			(const void *)&optval, sizeof(socklen_t)) == -1)
	{
		perror("Server: setsockopt error!");
		exit(errno);
	}

	/* setup Server Address */
	bzero((char *) &ServerAddr, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(port);
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&ServerAddr.sin_zero, 0, sizeof ServerAddr.sin_zero);

	/* bind */
	if(bind(ServerSockfd, (struct sockaddr *)&ServerAddr,
			sizeof(struct sockaddr)) == -1)
	{
		perror("Server: bind error!");
		exit(errno);
	}

	/* listen */
	if(listen(ServerSockfd, 5) == -1)
	{
		perror("Server: listen error!");
		exit(errno);
	}
	setenv("PATH",".",1);
    setenv("CONTENT_LENGTH","16000",1);
    setenv("REQUEST_METHOD","GET",1);
    setenv("SCRIPT_NAME","~/public_html/hw3.cgi",1);
    setenv("REMOTE_HOST","java.csie.nctu.edu.tw",1);
    setenv("REMOTE_ADDR","140.113.185.117",1);
    setenv("AUTH_TYPE","everybody",1);
    setenv("REMOTE_USER","csie",1);
    setenv("REMOTE_IDENT","9988",1);


	fprintf(stderr, "Sever started on port %d\n", port);
     SignalAction.sa_handler = SigChgChildHandler;
     sigemptyset(&SignalAction.sa_mask);
     SignalAction.sa_flags = SA_RESTART;
	 if(sigaction(SIGCHLD, &SignalAction, NULL) == -1)
     {
         perror("sigaction");
         exit(-1);
     }
	for(;;){
		if((ClientSockfd = accept(ServerSockfd, (struct sockaddr*)&ClientAddr,&sin_size )) == -1)
		{
			perror("Server: accept error!");
			continue;
		}
		fprintf(stderr, "-%16s:%d connected\n",inet_ntoa(ClientAddr.sin_addr),
                ntohs(ClientAddr.sin_port));
		
			if(fork() == 0)
		{
			close(ServerSockfd);
			dup2(ClientSockfd, STDIN_FILENO);
			dup2(ClientSockfd, STDOUT_FILENO);
			close(ClientSockfd);
			memset(req_buf,'\0',sizeof(req_buf));
			/* process http request */
			read(0, req_buf, sizeof(req_buf));
		//	fprintf(stderr,"%s",req_buf);
			parseandexec(req_buf);
			//method_GET(ClientAddr);
			/* after, close connection */
			fprintf(stderr, "-%16s:%d disconnected\n",
					inet_ntoa(ClientAddr.sin_addr),ntohs(ClientAddr.sin_port));
			/*usleep(1000000);*/
			fflush(stdout);
			/*shutdown(ClientSockfd, SHUT_RDWR);*/
			exit(0);
		}
		close(ClientSockfd);


	}


}
