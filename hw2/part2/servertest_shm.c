#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>         /* for read, write, open, close ,etc*/
#include <ctype.h>
#include <signal.h>


#define BUFFERSIZE 10240
#define SERV_TCP_PORT	8888
#define SHM_KEY (key_t)2222
#define USRNUM 31

char **environ;
int k=0,shmid;
int isthisprocess = 0;
int count [4000];
char pipebuffer[4000][BUFFERSIZE];


typedef struct usr
{

    char name[20] , msgbuffer[BUFFERSIZE],pibuffer[31][BUFFERSIZE],dest[20];
    int id , pid , port;

} user;
user *u;
int myid;

int OptionValue = 1;
void    SigChgChildHandler(int n)
{
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

    for( n=1; n<maxlen; n++)
    {
        if((rc = read(fd, &c, 1)) == 1)
        {
            *ptr++ = c;
            if(c == '\n')
                break;
        }
        else if (rc == 0)
        {
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

void broadcastall(char* msg )
{
    int i ;
    for(i=1; i<=30; i++)
    {
        if(u[i].id!=0)
        {
            strcpy(u[i].msgbuffer,msg);
            kill(u[i].pid,SIGUSR1);
        }
    }

}

void broadcastwithoutme(char*msg  , int myid)
{
    int i ;
    for(i=1; i<=30; i++)
    {
        if(i==myid)
            continue;
        if(u[i].id!=0)
        {
            strcpy(u[i].msgbuffer,msg);
            kill(u[i].pid,SIGUSR1);
        }
    }

}


void  sigusr_handler(int sig)
{
    if(isthisprocess == 0)
        printf("%s\n",u[myid].msgbuffer);
    else
        printf("%s\n",u[myid].msgbuffer);
//	printf("\naaa\n%% ");
    fflush(stdout);
}
void sigint_handler(int sig)
{
    shmctl(shmid,IPC_RMID,NULL);
	fprintf(stderr, "Service Terminated!\n"); 
	exit(0);
}


/*****************************************************
****************linked list section*******************
*****************************************************/

typedef struct nd
{
    char data[256];
    struct nd* next;
} node;

node* create_node(char* a)
{
    node* n = (node*)malloc(sizeof(node));
    strcpy(n->data,a);
    n->next = NULL;
    return n;

}

node* insert(node* end, node* n2)
{
    end->next = n2;
    n2->next = NULL;
    return n2;
}

int hasnext(node* n1)
{
    if(n1->next == NULL)
        return 0;
    else
        return 1;
}

void printall(node *n)
{
    node * current = n;
    while(1)
    {
        printf("%s\n",current->data);
        if(!(hasnext(current)))
            break;
        else
            current = current -> next;
    }

}

node* findpipe(node *n)
{
    node *current = n;
    do
    {
        if(hasnext(current))
            current = current -> next;

        if(strcspn(current->data,"|")==0)
        {
            return current;
        }

        else if (!hasnext(current))
        {
            return NULL;
        }


    }
    while(1);
}

/********************section end**************************/

void init()
{
    int j;
    for(j=0; j<1000; j++)
        count[j]=-1;
    memset(pipebuffer,'\0',sizeof(pipebuffer));
    chdir("ras");
    setenv("PATH", "bin:.", 1);


}


void parseandexec( char *ins ,int usrID,user *u)
{
    myid = usrID;
    char allins[10240];
    strcpy(allins,ins);
    char buffer[10240];
    int writepipeid=0,readpipeid=0;
    char *command;
    char *tmp;
	int breakthisins = 0;
    char a[BUFFERSIZE] ;      //store normal buffer
    node *end,*current,*nextins,*head,*wri = NULL;
    int commandcount = 1,inputcount = 0,pipecount;
    int ispipeN = 0,ispipe[2] = {0,0}  , pid;
    char* arg[500]={'\0'};
    int pipepatoch[2];
    int N;
    int pipechtopa[2];
    char **env1;
    /***********************init****************************/



//	 printf("1111111111111111111111111111111111111111111111111\n");

    strcpy(buffer,ins);
    tmp = strtok(buffer," ");
    current = create_node(tmp);
    nextins = current;
    head = current;
    end = current;

//creat list by " "
    while(tmp!=NULL)
    {
        tmp=strtok(NULL," ");
        if(tmp!=NULL)
        {
            node *n = create_node(tmp);
            end = insert(end,n);
        }
    }


//printf("%s\n",current->data);
//	printall(head);

    while(nextins != NULL  && current != NULL)
    {
        wri = NULL;
        if(current!=NULL)
        {
            nextins = findpipe(current); //find "|" in which node
        }
        if(nextins!=NULL) 				 //have pipe
        {

            if(nextins->data[1] != '\0' )  //pipe N case
            {
                ispipeN = 1;
                char *t = nextins -> data;
                N = atoi(++t);

            }
            else 							//normal pipe case
            {
                ispipe[0] = 1;
            }
        }

        else 							//no pipe no next instruction
        {
            ispipe[0]=0;
        }


        /***********set argu in arg ******************/

        if(current != NULL)
        {

            int i=0;
            while(current != nextins && current != NULL)
            {
                if(strcmp(current -> data,">")==0)
                {
//				writetofile =1;
//				printf("this is write case");
                    wri = current -> next;
                    current = nextins;
                    break;
                }
                if(current->data[0]=='>' && current->data[1]!='\0')
                {
                    char *n = current->data;
                    n++;
                    writepipeid = atoi(n);
                    if(u[writepipeid].id == 0)
                    {
                        printf("*** Error: user #%d does not exist yet. ***\n",writepipeid);
                        writepipeid=0;
						breakthisins = 1;
                        break;
                    }
                    if(u[writepipeid].pibuffer[myid][0]!='\0')
                    {
                        printf("*** Error: the pipe #%d->#%d already exists. ***\n",myid,writepipeid);
                        writepipeid=0;
						breakthisins = 1; 
                        break;
                    }
                    //          printf("%s\n",arg[i]);
                    i++;
                    current = current -> next;
                    continue;

                }
                if(current->data[0]=='<' && current->data[1]!='\0')
                {
                    char *n = current->data;
                    n++;
                    readpipeid = atoi(n);
                    if(u[myid].pibuffer[readpipeid][0]=='\0')
                    {
                        printf("*** Error: the pipe #%d->#%d does not exist yet. ***\n",readpipeid,myid);
                        readpipeid=0;
						breakthisins = 1; 
                        break;
                    }
                    //          printf("%s\n",arg[i]);
                    i++;
                    current = current -> next;
                    continue;

                }

                arg[i] = current -> data;
//			printf("%s\n",arg[i]);
                i++;
                current = current -> next;

            }
            arg[i]=	NULL;
        }
		if(breakthisins == 1)
			break;
        /**********setenv case**************/
//	printf("%s",arg[0]);
        if(strcmp(arg[0],"setenv")==0)
        {
            setenv(arg[1], arg[2], 1);


        }

        /**********printenv case**************/
        else if(strcmp(arg[0],"printenv")==0)
        {
            if((env1 = getenv(arg[1])) != 0)
            {
                //		fflush(stdout);
                printf("%s=%s\n",arg[1],env1);
                //	fflush(stdout);
            }
        }
        /************exit case**************/
        else if(strcmp(arg[0],"exit")==0)
        {
            char exitmsg[200];
            sprintf(exitmsg,"*** User '%s' left. ***",u[myid].name);
            broadcastall(exitmsg);
            memset(u[myid].pibuffer,'\0',sizeof(u[myid].pibuffer));
			int aaa;
			for(aaa=1;aaa<31;aaa++){
				memset(u[aaa].pibuffer[myid],'\0',BUFFERSIZE);
			}
            u[myid].id = 0;
            exit(0);
        }
        /************tell case*************/
        else if(strcmp(arg[0],"tell")==0)
        {
            int tmpid = atoi(arg[1]);
            if(u[tmpid].id==0)
            {
                printf("*** Error: user #%d does not exist yet. ***\n",tmpid);
                break;
            }
            else
            {
                char tellmsg[200];
                strcpy(tellmsg,"*** ");
                strcat(tellmsg,u[usrID].name);
                strcat(tellmsg," told you ***: ");
                int aaa = 2;
                while(arg[aaa]!=NULL)
                {
                    strcat(tellmsg,arg[aaa]);
                    strcat(tellmsg," ");
                    aaa++;
                }
                strcpy(u[tmpid].msgbuffer,tellmsg);
                kill(u[tmpid].pid,SIGUSR1);
                break;
            }

        }
        /*************yell case ***********/
        else if(strcmp(arg[0],"yell")==0)
        {
            char yellmsg[200];
            strcpy(yellmsg,"*** ");
            strcat(yellmsg,u[usrID].name);
            strcat(yellmsg," yelled ***: ");
            int aaa = 1;
            while(arg[aaa]!=NULL)
            {
                strcat(yellmsg,arg[aaa]);
                strcat(yellmsg," ");
                aaa++;
            }
            broadcastall(yellmsg);
            break;
        }
        /************who case*************/
        else if(strcmp(arg[0],"who")==0)
        {
            int i = 1;
            printf("<ID>	<nickname>	<IP/port>	<indicate me>\n");
            for(i=1; i<=30; i++)
            {
                if(u[i].id!=0)
                {
                    printf("%-d	%-10s	%s/%4d	",u[i].id,u[i].name,u[i].dest,u[i].port);
                    if(u[i].id==myid)
                        printf("<-me\n");
                    else
                        printf("\n");
                }
            }
            break;
        }
        /************name case***********/
        else if(strcmp(arg[0],"name")==0)
        {
            int aaa;
            int used=0;
            for(aaa=1; aaa<=30; aaa++)
            {
                if(u[aaa].id != 0)
                    if(strcmp(u[aaa].name,arg[1])==0)
                        used=1;
            }
            if(used == 0)
            {
                strcpy(u[myid].name,arg[1]);
                char namemsg[200];
                char nameport[10];

                sprintf(namemsg,"*** User from %s/%d is named '%s'. ***",u[myid].dest,u[myid].port,arg[1]);
                broadcastall(namemsg);
                break;
            }
            else
            {
                printf("*** User '%s' already exists. ***\n",arg[1]);
                break;
            }
        }

        /**********other case***************/
        else
        {
            pipe(pipepatoch);
            pipe(pipechtopa);
            int a1=0;
			char pipeNinput[10240];
			memset(pipeNinput,'\0',sizeof(pipeNinput));
            for(a1=0; a1<4000; a1++)
            {
                if(count[a1]!=-1)
                    count[a1]--;
                if(count[a1]==0)
                {
		//				strncat(pipeNinput,pipebuffer[a1],strlen(pipebuffer[a1]));
		//				printf("%d output: %s\n",count[1],pipebuffer[1]);
                        write(pipepatoch[1],pipebuffer[a1],strlen(pipebuffer[a1]));
                }
            }
	//		if(pipeNinput[0]!='\0')
	//			write(pipepatoch[1],pipeNinput,strlen(pipeNinput));
            if(readpipeid !=0 )
            {
                write(pipepatoch[1],u[myid].pibuffer[readpipeid],strlen(u[myid].pibuffer[readpipeid]));
                memset(u[myid].pibuffer[readpipeid],'\0',BUFFERSIZE);
            }

            if(ispipe[1] == 1)
            {
                write(pipepatoch[1],a,strlen(a));
            }
            close(pipepatoch[1]);

            pid = fork();
            if(pid < 0)
            {
                printf("exeerr");
                exit(0);
            }
            else if(pid == 0)
            {
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
            else
            {
                wait(0);
                close(pipepatoch[0]);
                memset(a,'\0',BUFFERSIZE);
                close(pipechtopa[1]);
                read(pipechtopa[0],a,BUFFERSIZE);
                close(pipechtopa[0]);
//		  printf("%d\n",ispipe[0]);
                if(strncmp(a,"Unknown",7)==0)
                {
                    for(a1=0; a1<4000; a1++)
                    {
                        if(count[a1]!= -1)
                            count[a1]++;
                    }
                }

                if(ispipe[0] != 1 && ispipeN == 0 && wri == NULL && writepipeid==0)
                {
                    //			flush(stdout);
                    printf("%s",a);
                    //			flush(stdout);
                    memset(a,'\0',BUFFERSIZE);
                }
				if(readpipeid !=0){
					char readmsg[200]; 
	                sprintf(readmsg,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",u[myid].name,myid,u[readpipeid].name,readpipeid,allins);        
		            broadcastall(readmsg);
		            readpipeid=0;

				}
                if(nextins != NULL)
                {
                    nextins = nextins-> next;
                    current = nextins;
                    ispipe[1] = ispipe[0];
                    ispipe[0] = 0;

                }
                else
                {
                    current = NULL;
                }
                if(ispipeN==1)
                {
                    count[k]=N;
                    strcpy(pipebuffer[k],a);
                    k++;
					ispipeN = 0;
                }
                if(wri != NULL)
                {
                    int fp = open(wri->data, O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR);
                    write(fp,a,strlen(a));
                }
                if(writepipeid!=0)
                {
                    strcpy(u[writepipeid].pibuffer[myid],a);
                    char writemsg[200];
                    sprintf(writemsg,"*** %s (#%d) just piped '%s' to %s (#%d) ***",u[myid].name,myid,allins,u[writepipeid].name,writepipeid);
                    broadcastall(writemsg);
					writepipeid = 0;
                }
                ispipeN=0;
            }
        }
    }
}




















int main(int argc, char** argv, char **envp)
{

    int usrID = 1;
    int sockfd, newsockfd, childpid, ServerPortNumber = SERV_TCP_PORT;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;
    struct sigaction    SignalAction;
    /*********************** creat shared memory*********************/
    shmid = shmget(SHM_KEY,31*sizeof(user),0666 | IPC_CREAT);
    u =  shmat(shmid,NULL,0);
    memset(u,'\0',31*sizeof(user));
    int s=1;
    for(s=1; s<=30; s++)
        u[s].id=0;


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
	signal(SIGINT,sigint_handler);
    fprintf(stderr,"Service started on port %d !\n",ServerPortNumber);

    for(;;)
    {
        /* Wait for a connection from a client process.*/

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if(newsockfd < 0)
            perror("server: accept error!");

        fprintf(stderr, "-%16s:%d connected\n",
                inet_ntoa(cli_addr.sin_addr),
                ntohs(cli_addr.sin_port));
        /***************set usr info ********************/
        for(usrID=1; usrID<=30; usrID++)
        {
            if(u[usrID].id ==0)
            {
                u[usrID].id = usrID;
                break;
            }
        }
        u[usrID].port = ntohs(cli_addr.sin_port);
        strcpy(u[usrID].name,"(no name)");
        strcpy(u[usrID].dest,inet_ntoa(cli_addr.sin_addr));

        if((childpid = fork()) < 0)
            perror("server: fork error!");

        else if (childpid == 0)       /* child process */
        {

            close(sockfd);            /* close original socket */
            /* writefd to stdin and stdout */
            if(sockfd != STDERR_FILENO)
            {
                close(STDERR_FILENO);
                dup2(newsockfd,STDERR_FILENO);
            }

            if(newsockfd!=STDIN_FILENO)
            {

                close(STDIN_FILENO);
                dup2(newsockfd,STDIN_FILENO);

            }
            if(sockfd != STDOUT_FILENO)
            {

                close(STDOUT_FILENO);
                dup2(newsockfd,STDOUT_FILENO);

            }
            signal(SIGUSR1, sigusr_handler);
            char buffer[10240];
            fflush(stdout);
            printf("****************************************\n** Welcome to the information server. **\n****************************************\n");
            fflush(stdout);
            init();
            myid = usrID;
            char entermsg[200];
            sprintf(entermsg,"*** User '(no name)' entered from %s/%d. ***",u[myid].dest,u[myid].port);
            isthisprocess = 1;
            broadcastall(entermsg);
            isthisprocess = 0;
            while(1)
            {
                fflush(stdout);
                printf("%% ");
                fflush(stdout);
                int n=0;
                n = readline(newsockfd, buffer, 10000);
                rstrip(buffer);
                if(buffer[0] != '\0')
                    parseandexec(buffer,usrID,u);
            }

            fprintf(stderr, "-%16s:%d disconnected\n",
                    inet_ntoa(cli_addr.sin_addr),
                    ntohs(cli_addr.sin_port));

            close(newsockfd);		  /* close the client socket on the forked process */
            exit(0);
        }

        else
        {
            u[usrID].pid = childpid;
            close(newsockfd);
        }

    }
//	shmctl(shmid,IPC_RMID,NULL);
//   fprintf(stderr, "Service Terminated!\n");
    return 0;
}
