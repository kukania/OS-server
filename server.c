#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<pthread.h>

#define BUFSIZE 1024
#define MAXLEN 100
#define MEMSIZE 1024

struct PID{
    int pid;
    int state;
    char file[MAXLEN];
};
struct SHM{
    int pidIndex;
    int size;
    struct PID pidA[MAXLEN];
    pthread_mutex_t mutex = PTHREAD_COND_INITIALIZER; //MUTEX for CLIENT c
};
int mem_id;
extern char **environ;
pthread_mutex_t mutex = PTHREAD_COND_INITIALIZER; //MUTEX for CLIENT c
struct CLIENT{
    int pidA[5];
    int size;
    int pidIndex;
    char *pidExe[5][MAXLEN];
};
struct CLIENT c;
void error_handling(char *message);
void read_childproc(int sig);
void handleClient(int clnt);
void getFileFromC(int clnt);
void getStatus(int clnt);
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

    mem_id=shmget(IPC_PRIVATE,sizeof(SHM),SHM_R|SHM_W);
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

            printf("%d processing client...\n",getpid());
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
    if(size>MAXLEN-1){
        write(sock,"num over max size wait plz",strlen("num over max size wait plz"));
        close(sock);
        return;
    }
    
    pid_t pid;
    sprintf(message,"./%s",fileName);
    /*make pipe for save result!*/
    int result;
    int pipefd[2];
    result=pipe(pipefd);
    /*shared memory setting*/
    SHM *client=(SHM*)shmat(mem_id,NULL,0);
    if(result<0){
        error_handling("pipe error");
    }
    if((pid=fork())==0){
        close(sock);
        dup2(pipefd[1],STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        char *myArg[2];
        myArg[0]=&message;
        int errorCode=execvpe(message,myArg,environ);
        exit(1);
    }
    else{
        close(pipefd[1]);
        FILE *resultFd=fdopen(pipefd[0],"r");
        sprintf(message,"%s.txt",fileName);
        int fd2=open(message,O_WRONLY|O_CREAT,0777);
        char fileBuf[BUFSIZE];

        while(fgets(fileBuf,BUFSIZE,resultFd)){
            write(fd2,fileBuf,strlen(fileBuf));
        }

        pthread_mutex_lock(&mutex);
        size++;
        pidA[pidIndex].pid=pid;
        strcpy(pidA[pidIndex].file,fileName);
        pidA[pidIndex].state=1;
        pidIndex=(pidIndex+1)%MAXLEN;
        printf("size - %d\n",size);
        pthread_mutex_unlock(&mutex);
        
        int status;
        sprintf(message,"[%d] do your job!",pid);
        write(sock,message,strlen(message));
        wait(&status);

        pthread_mutex_lock(&mutex);
        int i;
        for(i=0; i<size; i++){
            if(pidA[i].pid==pid){
                pidA[i].state=2;
            }
            printf("%d:[%d]-state:%d-fileName:%s\n",i,pidA[i].pid,pidA[i].state,pidA[i].file);
        }
        pthread_mutex_unlock(&mutex);
    }
}
void getStatus(int sock){
    char message[BUFSIZE];
    int str_len;
 //   sprintf(message,"input the process id");
 //   write(sock,message,strlen(message));

    str_len=read(sock,message,BUFSIZE-1);
    message[str_len]='\0';
    int pid=atoi(message);
    printf("client find for -%d\n",pid);
    int state=0;
    char fileName[MAXLEN];
    pthread_mutex_lock(&mutex);
    int i;
    for(i=0; i<size; i++){
        if(pidA[i].pid==pid){
            state=pidA[i].state;
            strcpy(fileName,pidA[i].file);
            if(state==2)
                size--;
        }
    }
    pthread_mutex_unlock(&mutex);

    sprintf(message,"%d",state);
    write(sock,message,strlen(message));
    
    if(state==2){
        printf("test");
    }
    else return;
}
void handleClient(int sock){
    int str_len;
    char message[BUFSIZE];
    str_len=read(sock,message,BUFSIZE-1);
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
            getStatus(sock);
            break;
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
