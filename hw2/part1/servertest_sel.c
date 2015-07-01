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


#define pipeNsize 5000
#define BUFFERSIZE 4000
#define SERV_TCP_PORT	8888
char **environ;

fd_set tempset, savedset;  // descriptor set to be monitored

int count [pipeNsize][2];
char pipebuffer[pipeNsize][10240];
int pn=0;
int OptionValue = 1;
void    SigChgChildHandler(int n)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

typedef struct usr
{
    char name[20] , msgbuffer[BUFFERSIZE],pibuffer[31][BUFFERSIZE],dest[20],mypath[100];
    int id , skfd , port;

} user;

user u[31];

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


void broadcast(char* msg){
	int i;
	for(i=1;i<31;i++){
		if(u[i].id>0)
			write(u[i].skfd,msg,strlen(msg));
	}
}
void broadcastwithoutme(char*msg , int id){
	int i;
	for(i=1;i<31;i++){
		if(u[i].id>0 && u[i].id != id )
			write(u[i].skfd,msg,strlen(msg));
	}
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
    for(j=0; j<pipeNsize; j++)
        count[j][0]=-1;
    memset(pipebuffer,'\0',sizeof(pipebuffer));
    chdir("ras");
    setenv("PATH", "bin:.", 1);


}


void parseandexec( char *ins ,int id)
{
	char allins [10240];
	strcpy(allins,ins);
	int breakthisins=0;
	int readpipeid =0, writepipeid =0;
	int myid = id;
    char buffer[10240];
    char *command;
    char *tmp;
    char a[2048] ;      //store normal buffer
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
        ispipeN = 0;
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
					char errmsg[100];
                    n++;
                    writepipeid = atoi(n);
                    if(u[writepipeid].id <= 0)
                    {
                        sprintf(errmsg,"*** Error: user #%d does not exist yet. ***\n",writepipeid);
						write(u[myid].skfd,errmsg,strlen(errmsg));
                        writepipeid=0;
						breakthisins = 1;
                        break;
                    }
                    if(u[writepipeid].pibuffer[myid][0]!='\0')
                    {
                        sprintf(errmsg,"*** Error: the pipe #%d->#%d already exists. ***\n",myid,writepipeid);
						write(u[myid].skfd,errmsg,strlen(errmsg));
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
					char errmsg[100];
                    n++;
                    readpipeid = atoi(n);
                    if(u[myid].pibuffer[readpipeid][0]=='\0')
                    {
                        sprintf(errmsg,"*** Error: the pipe #%d->#%d does not exist yet. ***\n",readpipeid,myid);
						write(u[myid].skfd,errmsg,strlen(errmsg));
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
		if(breakthisins == 1){
			breakthisins =0;
			break;
		}

        /**********setenv case**************/
//	printf("%s",arg[0]);
        if(strcmp(arg[0],"setenv")==0)
        {
			strcpy(u[myid].mypath,arg[2]);
            setenv(arg[1], arg[2], 1);
			break;

        }

        /**********printenv case**************/
        else if(strcmp(arg[0],"printenv")==0)
        {
            if((env1 = getenv(arg[1])) != 0)
            {
				char envmsg[100];
                //		fflush(stdout);
                sprintf(envmsg,"%s=%s\n",arg[1],env1);
				write(u[myid].skfd,envmsg,strlen(envmsg));
                //	fflush(stdout);
            }
        }
        /************exit case**************/
        else if(strcmp(arg[0],"exit")==0)
        {
			dup2(2,STDERR_FILENO); 
			char exitmsg[200];
            sprintf(exitmsg,"*** User '%s' left. ***\n",u[myid].name);
            broadcast(exitmsg);
			memset(u[myid].pibuffer,'\0',sizeof(u[myid].pibuffer));
			u[myid].id = -1;
			FD_CLR(u[myid].skfd, &savedset);
            close(u[myid].skfd);
        }

       /************tell case*************/
        else if(strcmp(arg[0],"tell")==0)
        {
            int tmpid = atoi(arg[1]);
            if(u[tmpid].id <= 0)
            {
				char errmsg[200];
                sprintf(errmsg,"*** Error: user #%d does not exist yet. ***\n",tmpid);
				write(u[myid].skfd,errmsg,strlen(errmsg));
                break;
            }
            else
            {
                char tellmsg[200];
                strcpy(tellmsg,"*** ");
                strcat(tellmsg,u[myid].name);
                strcat(tellmsg," told you ***: ");
                int aaa = 2;
                while(arg[aaa]!=NULL)
                {
                    strcat(tellmsg,arg[aaa]);
                    strcat(tellmsg," ");
                    aaa++;
                }
                write(u[tmpid].skfd,tellmsg,strlen(tellmsg));
                break;
            }

        }
        /*************yell case ***********/
        else if(strcmp(arg[0],"yell")==0)
        {
            char yellmsg[200];
            strcpy(yellmsg,"*** ");
            strcat(yellmsg,u[myid].name);
            strcat(yellmsg," yelled ***: ");
            int aaa = 1;
            while(arg[aaa]!=NULL)
            {
                strcat(yellmsg,arg[aaa]);
                strcat(yellmsg," ");
                aaa++;
            }
            broadcast(yellmsg);
            break;
        }
        /************who case*************/
        else if(strcmp(arg[0],"who")==0)
        {
            int i = 1;
			char whomsg1[100],whomsg2[100],whomsg3[50];

            strcpy(whomsg1,"<ID>	<nickname>	<IP/port>	<indicate me>\n");
			write(u[myid].skfd,whomsg1,strlen(whomsg1));
            for(i=1; i<=30; i++)
            {
                if(u[i].id > 0)
                {
                    sprintf(whomsg2,"%-d	%-10s	%s/%4d	",u[i].id,u[i].name,u[i].dest,u[i].port);
					write(u[myid].skfd,whomsg2,strlen(whomsg2));
                    if(u[i].id==myid){
                        strcpy(whomsg3,"<-me\n");
						write(u[myid].skfd,whomsg3,strlen(whomsg3));
					}
                    else{
                        strcpy(whomsg3,"\n");
						write(u[myid].skfd,whomsg3,strlen(whomsg3));
					}
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
                if(u[aaa].id > 0)
                    if(strcmp(u[aaa].name,arg[1])==0)
                        used=1;
            }
            if(used == 0)
            {
                strcpy(u[myid].name,arg[1]);
                char namemsg[200];
                char nameport[10];

                sprintf(namemsg,"*** User from %s/%d is named '%s'. ***\n",u[myid].dest,u[myid].port,arg[1]);
                broadcast(namemsg);
                break;
            }
            else
            {	char errmsg[100];
                sprintf(errmsg,"*** User '%s' already exists. ***\n",arg[1]);
				write(u[myid].skfd,errmsg,strlen(errmsg));
                break;
            }
        }


        /**********other case***************/
        else
        {
            pipe(pipepatoch);
            pipe(pipechtopa);
            int a1=0;
            for(a1=0; a1<pipeNsize; a1++)
            {
                if(count[a1][0]!=-1 && count[a1][1] == myid)
                    count[a1][0]--;
                if(count[a1][0]==0 && count[a1][1]== myid)
                {
                        write(pipepatoch[1],pipebuffer[a1],strlen(pipebuffer[a1]));
                }
            }

			if(readpipeid !=0 )
            {
				char test[1000];
                write(pipepatoch[1],u[myid].pibuffer[readpipeid],strlen(u[myid].pibuffer[readpipeid]));
				strcpy(test,u[myid].pibuffer[readpipeid]);
	//			printf("%s\n",u[myid].pibuffer[readpipeid]);
	//			printf("piped : %d\n",ispipe[1]);
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
				dup2(u[myid].skfd,STDERR_FILENO);
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
                memset(a,'\0',2048);
                close(pipechtopa[1]);
                read(pipechtopa[0],a,2048);
                close(pipechtopa[0]);
//		  printf("%d\n",ispipe[0]);
                if(strncmp(a,"Unknown",7)==0)
                {
                    for(a1=0; a1<pipeNsize; a1++)
                    {
                        if(count[a1][0]!= -1 && count[a1][1]==myid)
                            count[a1][0]++;
                    }
                    write(u[myid].skfd,a,strlen(a));
                    break;
                }

                if(ispipe[0] != 1 && ispipeN == 0 && wri == NULL && writepipeid ==0)
                {
                    //			flush(stdout);
                    write(u[myid].skfd,a,strlen(a));
                    //			flush(stdout);
                    memset(a,'\0',2048);
                }
				if(readpipeid != 0){
                 char readmsg[200];
                 sprintf(readmsg,"*** %s (%d) just received from %s (#%d) by '%s' ***\n",u[myid].name,myid,u[writepipeid].name,writepipeid,allins);
                 broadcast(readmsg);
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
					count[pn][1]=myid;
                    count[pn][0]=N;
                    strcpy(pipebuffer[pn],a);
                    pn++;
                }
                if(wri != NULL)
                {
                    int fp = open(wri->data, O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR);
                    write(fp,a,strlen(a));
                }
				if(writepipeid!=0)
                {	
				//	printf("%s\n",a);
                    strcpy(u[writepipeid].pibuffer[myid],a);
                    char writemsg[200];
                    sprintf(writemsg,"*** %s (#%d) just piped '%s' to %s (#%d) ***\n",u[myid].name,myid,allins,u[writepipeid].name,writepipeid);
                    broadcast(writemsg);
					writepipeid = 0;
                }
            }
        }
    }
}




















int main(int argc, char** argv, char **envp)
{


    int sockfd, newsockfd, childpid, ServerPortNumber = SERV_TCP_PORT;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;
    struct sigaction    SignalAction;
    int maxfd,maxi;
    int i , j ,k =0;
    int numready;

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
    maxfd = sockfd;
    maxi = -1;
    for(i=0; i<31; i++)
    {
        u[i].id = -1;
		strcpy(u[i].mypath,"bin:.");
    }
    FD_ZERO(&savedset); // initialize the descriptor set to be monitored to empty
    FD_SET(sockfd, &savedset);

    /* reap all dead processes */
    SignalAction.sa_handler = SigChgChildHandler;
    sigemptyset(&SignalAction.sa_mask);
    SignalAction.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &SignalAction, NULL) == -1)
    {
        perror("sigaction");
        exit(-1);
    }
	char wtf[] = "% ";
    fprintf(stderr,"Service started on port %d !\n",ServerPortNumber);
	init();
    for(;;)
    {
        tempset = savedset;
        numready = select(maxfd+1, &tempset, NULL, NULL, NULL); // pass max descriptor and wait indefinitely until data arrives

        /* Wait for a connection from a client process.*/
        char buffer[10240];
        if(FD_ISSET(sockfd, &tempset))  // new client connection
        {

            clilen = sizeof(cli_addr);
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		//	char welmsg[]="****************************************\n** Welcome to the information server. **\n****************************************\n";
	
		//	write(newsockfd,welmsg,sizeof(welmsg));
	//		write(newsockfd,"%% ",3);
            if(newsockfd < 0)
                perror("server: accept error!");
            fprintf(stderr, "-%16s:%d connected\n",
                    inet_ntoa(cli_addr.sin_addr),
                    ntohs(cli_addr.sin_port));
			char entermsg[200];
            for (j=1; j<31; j++)
            {
                if(u[j].id <0)
                {
                    u[j].id=j;
					strcpy(u[j].dest,inet_ntoa(cli_addr.sin_addr));
					strcpy(u[j].name,"(no name)");
					u[j].port = ntohs(cli_addr.sin_port);
                    u[j].skfd=newsockfd;
                    break;
                }

            }
			 char welmsg[200]="****************************************\n** Welcome to the information server. **\n****************************************\n";    
		//	write(u[j].skfd,wtf,strlen(wtf));  
			sprintf(entermsg,"*** User '(no name)' entered from %s/%d. ***\n",u[j].dest,u[j].port);
			strcat(welmsg,entermsg);
			strcat(welmsg,wtf); 
		//	write(u[j].skfd,wtf,strlen(wtf));
			write(newsockfd,welmsg,sizeof(welmsg));
			broadcastwithoutme(entermsg,j);
            FD_SET(newsockfd,&savedset);
            if(newsockfd > maxfd)
                maxfd = newsockfd;
            if(j>maxi)
                maxi=j;

        }

        for(k=1; k<=maxi; k++)
        {
            if(u[k].id>0)
            {	
                if(FD_ISSET(u[k].skfd,&tempset))
                {	
					memset(buffer,'\0',sizeof(buffer));
					setenv("PATH",u[k].mypath,1);
                    int numbyte;
                    numbyte = readline(u[k].skfd,buffer,10240);
					rstrip(buffer);
                   // write(u[k].skfd,buffer,strlen(buffer));
					if(buffer[0]!='\0')
						parseandexec(buffer,k);
					write(u[k].skfd,wtf,strlen(wtf));  
//                    if(--numready <=0) // num of monitored descriptors returned by select call
//                      break;

                }

            }

        }


    }

    fprintf(stderr, "Service Terminated!\n");
    return 0;
}
