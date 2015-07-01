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
#define SERV_TCP_PORT	8888
 char **environ;


 int count [2000];                                                                                                                                                       
 char pipebuffer[2000][2048];


int OptionValue = 1;
void    SigChgChildHandler(int n){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}


char* rstrip(char* str)
{
    int i = strlen(str)-1;
    while(isspace(str[i]))
        str[i--]='\0';
    return str;
}

int readline(int fd, char *ptr, int maxlen)
{
	int    n, rc;
	char   c;

	for( n=1; n<maxlen; n++){
		if((rc = read(fd, &c, 1)) == 1){
			*ptr++ = c;
			if(c == '\n')
				break;
		}
		else if (rc == 0){
			if(n == 1)
				return 0;    /* EOF, no data read */
			else
				break;       /* EOF, some data was read */
		}
		else
			return -1;       /* error */
	}

	*ptr = 0;
	return n;
}

/*****************************************************
****************linked list section*******************
*****************************************************/

typedef struct nd{
	char data[256];
	struct nd* next;
} node;

node* create_node(char* a){
	node* n = (node*)malloc(sizeof(node));
	strcpy(n->data,a);
	n->next = NULL;
	return n;

}

node* insert(node* end, node* n2){
	end->next = n2;
	n2->next = NULL;
	return n2;
}

int hasnext(node* n1){
	if(n1->next == NULL)
		return 0;
	else
		return 1;
}

void printall(node *n){
	node * current = n;
	while(1){
		printf("%s\n",current->data);
		if(!(hasnext(current)))
			break;
		else
			 current = current -> next;
	}
		
}

node* findpipe(node *n){
	node *current = n;
	do{
	if(hasnext(current))
		current = current -> next;

		if(strcspn(current->data,"|")==0){
			return current;
		}
	
		else if (!hasnext(current)){
			return NULL;		
		}
	
	
	}while(1);
}

/********************section end**************************/

void init(){
 int j;
 for(j=0;j<2000;j++)
     count[j]=-1;
 memset(pipebuffer,'\0',sizeof(pipebuffer));
 chdir("ras");
 setenv("PATH", "bin:.", 1);


}


void parseandexec( char *ins ,int k){
	char buffer[10240];
	char *command; 
	char *tmp;
	char a[2048] ;      //store normal buffer
	node *end,*current,*nextins,*head,*wri = NULL;
	int commandcount = 1,inputcount = 0,pipecount;
	int ispipeN = 0,ispipe[2] ={0,0}  , pid;
	char* arg[500];
    int pipepatoch[2];
	int N;
    int pipechtopa[2];
	char **env1;
/***********************init****************************/



//	 printf("1111111111111111111111111111111111111111111111111\n"); 
/* 
writefd to stdin and stdout */
/*    if(sockfd!=STDIN_FILENO){
        close(STDIN_FILENO);
        dup2(sockfd,STDIN_FILENO);
    }
    if(sockfd != STDOUT_FILENO){
        close(STDOUT_FILENO);
        dup2(sockfd,STDOUT_FILENO);
    }
*/
//    printf("000000000000000000000@#$%()%&*(()(*&&\n");     
	strcpy(buffer,ins);
	tmp = strtok(buffer," "); 
	current = create_node(tmp);
	nextins = current;
	head = current;
	end = current;
//creat list by " "

	while(tmp!=NULL){
		tmp=strtok(NULL," ");
		if(tmp!=NULL && *tmp != '/0'){
			node *n = create_node(tmp);
			end = insert(end,n);
		}
	}


//printf("%s\n",current->data);
//	printall(head);

while(nextins != NULL  && current != NULL){
	wri = NULL;
	ispipeN = 0;
	if(current!=NULL){
		nextins = findpipe(current); //find "|" in which node
	}
	if(nextins!=NULL){				 //have pipe
		
		if(nextins->data[1] != '\0' ){ //pipe N case
			ispipeN = 1;
			char *t = nextins -> data;
		    N = atoi(++t);
			
		}
		else{							//normal pipe case
			ispipe[0] = 1;	
		}						
	}

	else{							//no pipe no next instruction
			ispipe[0]=0;			
	}
	
/***********set argu in arg ******************/

	if(current != NULL){

		int i=0;
		while(current != nextins && current != NULL){
			if(strcmp(current -> data,">")==0){
//				writetofile =1;
//				printf("this is write case");
				wri = current -> next;
				current = nextins;
				break;
			}

			arg[i] = current -> data;
//			printf("%s\n",arg[i]);
			i++;
			current = current -> next;
		
		}
		arg[i]=	NULL;
	}

/**********setenv case**************/
//	printf("%s",arg[0]);	
	if(strcmp(arg[0],"setenv")==0){
		setenv(arg[1], arg[2], 1);
		
		
	}

/**********printenv case**************/   
	else if(strcmp(arg[0],"printenv")==0){
            if((env1 = getenv(arg[1])) != 0){
		//		fflush(stdout);
                printf("%s=%s\n",arg[1],env1);
			//	fflush(stdout);
				}
	}
/************exit case**************/
     else if(strcmp(arg[0],"exit")==0){
         exit(0);
     }


/**********other case***************/
	else {
		pipe(pipepatoch);
	    pipe(pipechtopa);
		int a1=0;
		for(a1=0;a1<2000;a1++){
			if(count[a1]!=-1)
				count[a1]--;
			if(count[a1]==0){
			  if(ispipe[1] == 1)
				write(pipepatoch[1],pipebuffer[a1],strlen(pipebuffer[a1]));
			  else
				write(pipepatoch[1],pipebuffer[a1],strlen(pipebuffer[a1])); 
			}
		}

		if(ispipe[1] == 1){
			write(pipepatoch[1],a,strlen(a));
		}
		close(pipepatoch[1]);
		
	pid = fork();
		if(pid < 0){
			printf("exeerr"); 
		  exit(0);
		}
		else if(pid == 0){
			close(pipepatoch[1]);
			close(pipechtopa[0]);
			dup2(pipepatoch[0],STDIN_FILENO);
			dup2(pipechtopa[1],STDOUT_FILENO);
			close(pipechtopa[1]);
			close(pipepatoch[0]);
			execvp(arg[0],arg);
			printf("Unknown command: [%s].\n",arg[0]);
			exit(0);
			
		}
		else {
	      wait(0);
          close(pipepatoch[0]);
          memset(a,'\0',2048);
          close(pipechtopa[1]);
          read(pipechtopa[0],a,2048);
		  close(pipechtopa[0]);
//		  printf("%d\n",ispipe[0]);
		  if(strncmp(a,"Unknown",7)==0){
			for(a1=0;a1<2000;a1++){
				if(count[a1]!= -1)
					count[a1]++;
			}
			printf("%s",a);
			break;
		  }

          if(ispipe[0] != 1 && ispipeN == 0 && wri == NULL){
	//			flush(stdout);
				printf("%s",a);
	//			flush(stdout);
				memset(a,'\0',2048);
			} 
		 if(nextins != NULL){
			nextins = nextins-> next;
			current = nextins;
			ispipe[1] = ispipe[0];
			ispipe[0] = 0;
		 
		}
		else{
			current = NULL;
		}
		if(ispipeN==1){
			count[k]=N;
			strcpy(pipebuffer[k],a);
			k++;
		}
		if(wri != NULL){
			int fp = open(wri->data, O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR);
			write(fp,a,strlen(a));
		}
		}
	}
}
}





















int main(int argc, char** argv, char **envp){
			

	int sockfd, newsockfd, childpid, ServerPortNumber = SERV_TCP_PORT;
	socklen_t clilen;
	struct sockaddr_in cli_addr, serv_addr;
	struct sigaction    SignalAction;


	/* Open a TCP socket (an Internet stream socket) */

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror("server: can't open stream socket!");

	/* SOLVE THE "bind: Address already in use" PROBLEM!!!" */
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
		(const void *)&OptionValue, sizeof(socklen_t)) == -1){
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
    if(sigaction(SIGCHLD, &SignalAction, NULL) == -1){
        perror("sigaction");
        exit(-1);
    }

fprintf(stderr,"Service started on port %d !\n",ServerPortNumber);

	for(;;){
		/* Wait for a connection from a client process.*/

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		if(newsockfd < 0)
			perror("server: accept error!");

		fprintf(stderr, "-%16s:%d connected\n",
                inet_ntoa(cli_addr.sin_addr),
                ntohs(cli_addr.sin_port));

		if((childpid = fork()) < 0)
			perror("server: fork error!");

		else if (childpid == 0){      /* child process */
			
			close(sockfd);            /* close original socket */
		/* writefd to stdin and stdout */
    if(newsockfd!=STDIN_FILENO){

		 close(STDIN_FILENO);
         dup2(newsockfd,STDIN_FILENO);

     }
     if(sockfd != STDOUT_FILENO){
			
		close(STDOUT_FILENO);
         dup2(newsockfd,STDOUT_FILENO);

     }
      if(sockfd != STDERR_FILENO){
             
         close(STDERR_FILENO);
          dup2(newsockfd,STDERR_FILENO);
 
      }

 

             char buffer[10240];
			 fflush(stdout);
             printf("****************************************\n** Welcome to the information server. **\n****************************************\n");
			 fflush(stdout); 
             static int k=0 ; //index of pipebuffer
 //          static int count[1000];
 //          static int pipebuffer[1000][2048];
             init();
             while(1){
			 fflush(stdout); 
             printf("%% "); 
			 fflush(stdout); 
//             gets(buffer);
			int n=0;
            n = readline(newsockfd, buffer, 10000);
			rstrip(buffer);
             if(buffer[0] != '\0') 
                 parseandexec(buffer,k);
             }                              
			
			fprintf(stderr, "-%16s:%d disconnected\n",
                inet_ntoa(cli_addr.sin_addr),
                ntohs(cli_addr.sin_port));

			close(newsockfd);		  /* close the client socket on the forked process */
			exit(0);
         }
		
		close(newsockfd);

	}

		fprintf(stderr, "Service Terminated!\n");
		return 0;
}
