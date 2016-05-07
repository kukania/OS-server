#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<fcntl.h>

#define BUFSIZE 1024
#define MAXLEN 100
extern char **environ;
struct CLIENT{
    int pidA[5];
    void* pid[5];
    int pidIndex;
};
struct CLIENT c;
void error_handling(char *message);
void read_childproc(int sig);
void handleClient(int clnt);
void getFileFromC(int clnt);
int parseitems(char *cmdln,char items[][MAXLEN]){
	int i=0;
	char * pch=NULL;
	pch=strtok(cmdln," \n");
	while(pch != NULL && i<MAXLEN){
		strcpy(items[i],pch);
		pch=strtok(NULL," \n");
		i++;
	}
	return i;
}
int main(int argc, char *argv[]){
	int serv_sock,clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	pid_t pid;
	socklen_t adr_sz;
	int str_len,state;
	char buf[BUFSIZE];
	if(argc!=2){
		printf("Usage : %s <port>\n",argv[0]);
		exit(1);
	}

	serv_sock=socket(PF_INET,SOCK_STREAM,0);
	memset(&serv_adr,0,sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));

	if(bind(serv_sock,(struct sockaddr*)&serv_adr,sizeof(serv_adr))==-1){
		error_handling("bind error");
	}
	if(listen(serv_sock,5)==-1){
		error_handling("listen error");
	}
	struct sigaction act;
	act.sa_handler=read_childproc;
	sigemptyset(&act.sa_mask);
	act.sa_flags=0;
	state=sigaction(SIGCHLD,&act,0);

	while(1){
		adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_adr,&adr_sz);
		if(clnt_sock==-1)
			continue;
		else
			puts("new client connected...");
		pid=fork();
		if(pid==-1){
			close(clnt_sock);
			continue;
		}
		if(pid==0){
			close(serv_sock);
			
			handleClient(clnt_sock);

			close(clnt_sock);
			puts("client disconnected...");
			return 0;
		}
		else
			close(clnt_sock);
	}
	close(serv_sock);
	return 0;
}
void getFileFromC(int sock){
	int str_len;
	char fileName[BUFSIZE]="temp_";
	char message[BUFSIZE];
	char items[MAXLEN][MAXLEN];

	str_len=read(sock,message,BUFSIZE-1);
	message[str_len]=0;
	printf("%d > send file %s\n",getpid(),message);
	strcat(fileName,message);
	printf("make %s to %s\n",message,fileName);
	
	int fd=open(fileName,O_WRONLY|O_CREAT,0777);
	if(fd==-1){
		write(sock,"0",strlen("0"));
		error_handling("file open error!");
	}
	else write(sock,"1",strlen("1"));

    /*get file size*/
    str_len=read(sock,message,BUFSIZE);
    message[str_len]=0;
    int sizeofFile=atoi(message);
    printf("file size: %d\n",sizeofFile);

    int getFile=0;
    int tempBufSize=1024;
	while((str_len=read(sock,message,tempBufSize))!=0){
		write(fd,message,str_len);
        printf("read size : %d\n",str_len);
        getFile+=str_len;
        if(sizeofFile<getFile+tempBufSize){
            tempBufSize=sizeofFile-getFile;
        }
        if(tempBufSize==0){
            close(fd);
            break;
        }
	}
    printf("read complete!\n");
	
	memset(message,0,sizeof(message));
	write(sock,"input the argument option one :",strlen("input the argument option one:"));
	str_len=read(sock,message,BUFSIZE-1);
	message[str_len]=0;
	int itemNum=parseitems(message,items);
	int i;
    printf("read argument = %d\n",itemNum);
	for(i=0; i<itemNum; i++){
		printf("%s\n",items[i]);
	}
    /*setting for execv*/
    char *myArgv[MAXLEN];
    myArgv[0]=&fileName;
    pid_t pid;
    for(i=1; i<itemNum; i++)
        myArgv[i]=&items[i];
    if(c.pidIndex>4){
        sprintf(message,"too many process running now wait:%d",c.pidIndex);
        write(sock,message,strlen(message));
        return;
    }
    else{
        if((pid=fork())==0){
            sprintf(message,"./%s",fileName);
            int errorCode= execvpe(message,myArgv,environ);
            printf("%d running result : %d",getpid(),errorCode);
            exit(1);
        }
        else{
            c.pidA[c.pidIndex++]=pid;
        }
    }
}
void handleClient(int sock){
	int str_len;
	char message[BUFSIZE];
	while((str_len=read(sock,message,BUFSIZE-1))!=0){
		message[str_len]='\0';
		char input=message[0];
		switch(input){
			case '1':
				printf("%d select menu 1\n",getpid());
				write(sock,"menu 1 selected",strlen("menu 1 selected"));
				getFileFromC(sock);
				break;
			case '2':
				printf("%d select menu 2\n",getpid());
				write(sock,"menu 2 selected",strlen("menu 2 selected"));
				break;
		}
	}
}
void read_childproc(int sig){
	pid_t pid;
	int status;
	pid=waitpid(-1,&status,WNOHANG);
	printf("removed proc id :%d\n",pid);
}
void error_handling(char *message){
	fputs(message, stderr);
	fputs("\n",stderr);
	exit(1);
}
