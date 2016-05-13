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
#include<sys/shm.h>
#include<time.h>

#define BUFSIZE 1024
#define MAXLEN 100
#define MEMSIZE 1024
#define KEY 1005
struct PID{
    int pid;
    int state;
    float runningTime;
    char file[MAXLEN];
};
typedef struct SHM{
    int size;
    struct PID pidA[MAXLEN];
    pthread_mutex_t mutex;
}SHM;
int fileNum;
//shared memory struct
int mem_id;//shared memory id
extern char **environ;
void error_handling(char *message);
void read_childproc(int sig);
void handleClient(int clnt);
void getFileFromC(int clnt);
void getStatus(int clnt);
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
    if(listen(serv_sock,101)==-1){
        error_handling("listen error");
    }
    struct sigaction act;
    act.sa_handler=read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    state=sigaction(SIGCHLD,&act,0);

    /*make shared memory*/
    mem_id=shmget(KEY,sizeof(SHM),0777|IPC_CREAT);
    if(mem_id==-1)
        error_handling("shm error!");
    SHM *client=(SHM*)shmat(mem_id,NULL,0);
    //client->mutex=PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&(client->mutex),NULL);
   // printf("%d\n",client->size);
    while(1){
        adr_sz=sizeof(clnt_adr);
        clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_adr,&adr_sz);
        if(clnt_sock==-1)
            continue;
        else
            puts("new client connected...");
        pthread_mutex_lock(&(client->mutex));
        fileNum++;
        pthread_mutex_unlock(&(client->mutex));
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
    sprintf(fileName,"%s_%d",fileName,fileNum);
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
        //printf("read size : %d\n",str_len);
        getFile+=str_len;
        if(sizeofFile<getFile+tempBufSize){
            tempBufSize=sizeofFile-getFile;
        }
        if(tempBufSize==0){
            close(fd);
            break;
        }
    }
    pid_t pid;
    sprintf(message,"./%s",fileName);
    /*make pipe for save result!*/
    int result;
    int pipefd[2];
    result=pipe(pipefd);
    /*shared memory setting*/
    SHM *client=(SHM*)shmat(mem_id,NULL,0);
    pthread_mutex_lock(&(client->mutex));
    printf("%d\n",client->size);
    if(client->size>MAXLEN){
        sprintf(message,"too many process running wait or take result plz");
        write(sock,message,strlen(message));
        close(sock);
    }
    pthread_mutex_unlock(&(client->mutex));
    
    /*time check*/
    time_t startTime=0, endTime=0;
    startTime=clock();
   
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
        /*shared memory use*/
        pthread_mutex_lock(&(client->mutex));
        client->size++;
        client->pidA[client->size-1].pid=pid;
        strcpy(client->pidA[client->size-1].file,fileName);
        client->pidA[client->size-1].state=1;
        printf("-----------size - %d\n",client->size);

        pthread_mutex_unlock(&(client->mutex));
        int status;
        sprintf(message,"%d",pid);
        write(sock,message,strlen(message));
        wait(&status);
        
        endTime=clock();

        pthread_mutex_lock(&(client->mutex));
        int i;
        for(i=0; i<MAXLEN; i++){
            if(client->pidA[i].pid==pid){
                client->pidA[i].state=2;
                client->pidA[i].runningTime=(float)(endTime-startTime)/(CLOCKS_PER_SEC);
                break;
            }
            printf("%d:[%d]-state:%d-fileName:%s\n",i,client->pidA[i].pid,client->pidA[i].state,client->pidA[i].file);
        }
        pthread_mutex_unlock(&(client->mutex));
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
    /*shared memory setting*/
    SHM *client=(SHM*)shmat(mem_id,NULL,0);
    pthread_mutex_lock(&(client->mutex));
    float runningTime;
    int i,j,k;
    //good queuing in this setting
    for(i=0; i<client->size; i++){
        printf("%d : checking\n",i);
        if(client->pidA[i].pid==pid){
            state=client->pidA[i].state;
            strcpy(fileName,client->pidA[i].file);
            runningTime=client->pidA[i].runningTime;
            if(state==2){
                for(j=i;j<client->size-1; j++){
                    client->pidA[j].pid=client->pidA[j+1].pid;
                    client->pidA[j].state=client->pidA[j+1].state;
                    client->pidA[j].runningTime=client->pidA[j+1].runningTime;
                    strcpy(client->pidA[j].file,client->pidA[j+1].file);
                }
                client->size--;
            }
            break;
        }
    }
    pthread_mutex_unlock(&(client->mutex));

    sprintf(message,"%d",state);
    write(sock,message,strlen(message));
    read(sock,message,BUFSIZE-1);   

    sprintf(message,"running time : %f",runningTime);
    write(sock,message,strlen(message));
    read(sock,message,BUFSIZE-1);

    if(state==2){
        sprintf(message,"%s.txt",fileName);
        printf("%s read\n",message);
        int fd=open(message,O_RDONLY);
        while((str_len=read(fd,message,BUFSIZE-1))!=0){
            write(sock,message,str_len);
            message[str_len]='\0';
            printf("%s\n",message);
        }
        close(fd);
        close(sock);
    }
    else{
        close(sock);
        return;
    }
}
void handleClient(int sock){
    int str_len;
    char message[BUFSIZE];
    str_len=read(sock,message,BUFSIZE-1);
    message[str_len]='\0';
    printf("%s \n",message);
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
        case '3':
            printf("%d select menu 3\n",getpid());
 //           read(sock,message,BUFSIZE-1);
            close(sock);
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
