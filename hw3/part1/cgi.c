#include <sys/types.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>


#define F_CONNECTING 0	/* connecting to server and ready to read msg */
#define F_READING 1	/* reading msg from server */
#define F_WRITING 2	/* writing command to server */
#define F_DONE 3

typedef struct USR
{
    int port,skfd,fd,status,bsuccess;
    char ip[200],fname[200];
    struct sockaddr_in sin;
    struct hostent *he;
    FILE *file_p;
} user;

user u[100];
int clinum = 0;

int readline(int fd,char *ptr,int maxlen)
{
    int n,rc;
    char c;
    *ptr = 0;
    for(n=1; n<maxlen; n++)
    {
        if((rc=read(fd,&c,1)) == 1)
        {

            *ptr++ = c;
            if(c=='\n')  break;
    /*        if(c=='%')
            {
                *ptr++ = ' ';
                n++;
                *ptr++ = '\n';
                n++;
                break;
            }
   */
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


void sendheader()
{
    int i;
    printf("Content-Type: text/html\n\n");
    printf("<html>\n");
    printf("<head>\n");
    printf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />\n");
    printf("<TITLE>Network Programming Homework 3</TITLE>\n");
    printf("</head>\n");
    printf("<body bgcolor=#336699>\n");
    printf("<font face=\"Courier New\" size=2 color=#FFFF99>\n");
    printf("<table width=\"800\" border=\"1\">\n");
    printf("<tr>\n");
    for( i=1; i <= 5; i++)
        printf("<td>%s</td>",u[i].ip);
    //printf("<td>140.113.210.145</td><td>140.113.210.145</td><td>140.113.210.145</td></tr>");
    printf("<tr>\n");
    for( i=1; i <= 5; i++)
        printf("<td valign=\"top\" id=\"m%d\"></td>",i);
    printf("</tr>\n");
    printf("</table>\n");
    fflush(stdout);
    //printf("<td valign=\"top\" id=\"m0\"></td><td valign=\"top\" id=\"m1\"></td><td valign=\"top\" id=\"m2\"></td></tr>");

}


void parser(char *str)
{
    char *c;
    int num;
    int i;
    int len;
    char *tmp;
    while((tmp = strsep(&str,"&"))!=NULL)
    {
        switch(*tmp)
        {
        case 'h':
            num = atoi(++tmp);
            tmp++;
            tmp++;
            if(*tmp=='\0')
                break;
            strcpy(u[num].ip,tmp);
            break;
        case 'p':
            num = atoi(++tmp);
            tmp++;
            tmp++;
            if(*tmp=='\0')
                break;
            u[num].port=atoi(tmp);
            break;
        case 'f':
            num = atoi(++tmp);
            tmp++;
            tmp++;
            if(*tmp=='\0')
                break;
            strcpy(u[num].fname,tmp);
            break;
        }//end of switch

    }//end of while

}


void setsocket(fd_set *rs,fd_set *ws)
{
    int flags = 0;
    int i;
    int conn;
    //printf("setsocket\n");
    fflush(stdout);
    for(i=1; i<=5; i++)
    {
        u[i].bsuccess = 0;
        if(u[i].ip[0]!= '\0')
        {
            //	u[i].fd = open(u[i].fname,O_RDONLY);
            //	u[i].he = gethostbyname(u[i].ip);

            if((u[i].fd = open(u[i].fname,O_RDONLY))<0)
            {
                fprintf(stderr,"Not Found File\n");
                continue;
            }

            //fprintf(stderr,"user[i].ip=%s\n",user[i].ip);

            if((u[i].he = gethostbyname(u[i].ip)) == NULL)
            {
                fprintf(stderr,"GetHostByName Error\n");
                continue;
            }
            u[i].skfd = socket(AF_INET, SOCK_STREAM, 0);
            bzero(&u[i].sin,sizeof(struct sockaddr_in));
            u[i].sin.sin_family = AF_INET;
            u[i].sin.sin_addr = *((struct in_addr *)u[i].he -> h_addr);
            u[i].sin.sin_port = htons (u[i].port);
            flags = fcntl(u[i].skfd,F_GETFL,0);
            fcntl(u[i].skfd , F_SETFL , flags | O_NONBLOCK);
            if( (conn = connect(u[i].skfd,(struct sockaddr *)&u[i].sin,sizeof(u[i].sin))) < 0)
            {
                if(errno != EINPROGRESS)
                {
                    fprintf(stderr,"NONBLOCK I/O set Error\n");
                    continue;
                }

            }
            u[i].bsuccess = 1;
            u[i].status = F_CONNECTING;
            FD_SET(u[i].skfd,rs);
            FD_SET(u[i].skfd,ws);
            clinum++;
            usleep(3000);
        }

    }

}



int setselect(fd_set *rs, fd_set *ws, fd_set *rfds, fd_set *wfds)
{
    char wrimsg[4000];
    char cmd_buffer[4000];
    int cmd_len;
    int nfds = FD_SETSIZE;
    int i;
    int u_index;
    char *tmp;
    int n = 0;
    int k;
    int bexit=0;
    int error = 0;
    //printf("setselect\n");
    fflush(stdout);
    while(clinum > 0)
    {
        memcpy(rfds,rs,sizeof(fd_set));
        memcpy(wfds,ws,sizeof(fd_set));
        fflush(stdout);
        if(select(nfds,rfds,wfds,(fd_set*)0,(struct timeval *)0) < 0)
        {
            //	printf("<script>document.all['m1'].innerHTML += \"aaa<br>\";</script>\n");
            fflush(stdout);
            return -1;
        }
        for(u_index=1; u_index<=5; u_index++)
        {
            if(u[u_index].bsuccess == 1)
            {	
                if( (u[u_index].status == F_CONNECTING ) && ( FD_ISSET(u[u_index].skfd,rfds) || FD_ISSET(u[u_index].skfd,wfds)) )
                {
                    if( (getsockopt(u[u_index].skfd,SOL_SOCKET,SO_ERROR,&error,&n) < 0) || (error!=0) )
                    {
                        //non-Blocking connect failure
                        fprintf(stderr, "user[%d].skfd:%d non-Blocking connect failure", i, u[u_index].skfd);
                        return -1;
                    }
                    FD_CLR(u[u_index].skfd,ws);
                    u[u_index].status = F_READING;
                    FD_SET(u[u_index].skfd,rs);
                }
                else if (u[u_index].status == F_WRITING && FD_ISSET(u[u_index].skfd, wfds) )
                {
                    memset(cmd_buffer,'\0',sizeof(cmd_buffer));
                    cmd_len = readline(u[u_index].fd,cmd_buffer,sizeof(cmd_buffer));
                    if(strlen(cmd_buffer)>0)
                    {
                        	printf("send to server : %s",cmd_buffer);
                        //	cmd_buffer[cmd_len]=='\0';
                        write(u[u_index].skfd,cmd_buffer,strlen(cmd_buffer));
                        tmp = strtok(cmd_buffer,"\r\n");
			if(strstr(tmp,"exit")!= NULL) bexit=1;
                        sprintf(wrimsg,"<script>document.all['m%d'].innerHTML += \"<b>%s</b><br>\";</script>\n",u_index,tmp);
                        printf("%s",wrimsg);
                        fflush(stdout);
                    }
                    else
                    {
                        fflush(stdout);
                        FD_CLR(u[u_index].skfd,ws);
                        close(u[u_index].skfd);
                        close(u[u_index].fd);
                        u[u_index].bsuccess = 0;
                        u[u_index].status = F_DONE ;
                        fprintf(stderr, "u[%d].status = F_DONE\n", u_index);
                        clinum--;
                        continue;
                    }
                    FD_CLR(u[u_index].skfd, ws);
                    u[u_index].status = F_READING;
                    FD_SET(u[u_index].skfd, rs);
		    usleep(30000);

                }
                else if (u[u_index].status == F_READING && FD_ISSET(u[u_index].skfd, rfds) )
                {
           	
                    fflush(stdout);
                    char msg_buf[4000];
                    char allmsg[4000];
		    int j;
                    int msglen;
                    char *tmp;
                    while(1)
                    {
			tmp = NULL;
                        memset(msg_buf,'\0',sizeof(msg_buf));
                        memset(allmsg,'\0',sizeof(allmsg));
         
                        //			printf("777\n");
                        fflush(stdout);
                        msglen = readline(u[u_index].skfd,msg_buf,sizeof(msg_buf));
                       	memset(wrimsg,'\0',sizeof(wrimsg));
                        fflush(stdout);
                        //		printf("reclen: %d\n",msglen);
                        //                printf("rec msg :");
                        //		for(k=0; k<msglen; ++k) printf("%c", msg_buf[k]);
                        //               printf("\n");
                        //			printf("rec msg : %s\n",msg_buf);
                        fflush(stdout); 
			if(msglen <= 0 && bexit == 1 )
                        {
                            FD_CLR(u[u_index].skfd,rs);
                            u[u_index].status = F_WRITING;
                            FD_SET(u[u_index].skfd,ws);
                            break;
                        }
			if(msglen == 0 ) continue;
			tmp = strtok(msg_buf,"\n");
			//printf("%d\n",strlen(tmp));
			if(tmp == NULL ){
			printf("<script>document.all['m%d'].innerHTML += \"<br>\";</script>\n",u_index);
			fflush(stdout);
                            break;
			}
			char *www;
			www = strstr(tmp,"\r");
			if(www != NULL){
			 *www = '\0';
			}
			
                        if(strncmp(tmp,"% ",2) == 0)
                        {
                            sprintf(wrimsg,"<script>document.all['m%d'].innerHTML += \"<b>%s</b>\";</script>\n",u_index,tmp);
                            printf("%s",wrimsg);
			    fflush(stdout);
                            FD_CLR(u[u_index].skfd,rs);
                            u[u_index].status = F_WRITING;
                            FD_SET(u[u_index].skfd,ws);
                            break;
                        }
			j = 0;
                        for(i=0; i<msglen; i++)
                        {
                            /* if messages from server contain some special character */

                            if(tmp[i] == '\"') 	/* if message contains '"' */
                            {
                                allmsg[j] = '\\';
                                allmsg[j+1] = '\"';
                                j = j+2;
                            }

                            else if(tmp[i] == '<')
                            {
                                /* if message contains '<' */
                                allmsg[j] = '&';
                                allmsg[j+1] = 'l';
                                allmsg[j+2] = 't';
                                allmsg[j+3] = ';';
                                j = j+4;
                            }
                            else if(tmp[i] == '>')
                            {
                                /* if message contains '>' */

                                allmsg[j] = '&';
                                allmsg[j+1] = 'g';
                                allmsg[j+2] = 't';
                                allmsg[j+3] = ';';
                                j = j+4;
                            }
                            else
                            {
                                allmsg[j] = tmp[i];
                                j = j +1;
                            }

                        }
                        memset(wrimsg,'\0', sizeof(wrimsg));
		//	printf("allmsg :");
		//	for(k=0;k<strlen(allmsg);k++)printf("%c",allmsg[k]);
		//	printf("\n");
                        sprintf (wrimsg,"<script>document.all['m%d'].innerHTML += \"%s<br>\";</script>\n", u_index,allmsg);
                        printf("%s",wrimsg);
			fflush(stdout);
			usleep(3000);
                    }

                }

            }
        }
    }
}


int main()
{
    fd_set rs,ws,rfds,wfds;
    memset(u,'\0',sizeof(u));
    bzero(&rs, sizeof(fd_set));
    bzero(&ws, sizeof(fd_set));
    bzero(&rfds, sizeof(fd_set));
    bzero(&wfds, sizeof(fd_set));

    char teststr[]="h1=127.0.0.1&p1=8899&f1=t5.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=&h5=&p5=&f5=";
//    parser(teststr);
    parser(getenv("QUERY_STRING"));
    sendheader();
    setsocket(&rs,&ws);
    setselect(&rs,&ws,&rfds,&wfds);
//	sleep(10);
    return 0;
}
