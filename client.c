#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<pthread.h>

#define BUFSIZE 1024
#define MAXLEN 100

void error_handling(char *message);
void showMenu();
int sendFile(int sock,char *file);
void checkProc(int sock,int pid);
void testBench(struct sockaddr_in serv_adr, char *fileName);
int main(int argc, char *argv[]){
    int sock;
    char message[BUFSIZE];
    int str_len;
    struct sockaddr_in serv_adr;
    if(argc!=3){
        printf("Usage : %s <IP> <port>\n",argv[0]);
        exit(1);
    }

    sock=socket(PF_INET,SOCK_STREAM,0);
    if(sock==-1){
        error_handling("sock error");	
    }

    memset(&serv_adr,0,sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_adr.sin_port=htons(atoi(argv[2]));

    if(connect(sock,(struct sockaddr*)&serv_adr,sizeof(serv_adr))==-1){
        error_handling("connect error!");
    }
    else 
        puts("connected ......");

    showMenu();
    printf("\nINPUT message: ");
    scanf("%s",message);

    if(!strcmp(message,"q")||!strcmp(message,"Q"))
        return 0;

    char input=message[0];
    write(sock,message,strlen(message));
    //str_len=read(sock,message,BUFSIZE-1);
    //message[str_len]='\0';
    //printf("Server : %s  - %c\n",message,input);
    int num;
    int i;
    char inputFileName[MAXLEN];
    switch(input){
        case '1':
            //	printf("you selected menu 1\n");		
            sendFile(sock,NULL);
            break;
        case '2':
            checkProc(sock,0);
            break;
        case '3':
            close(sock);
            printf("input the # of thread: ");
            scanf("%d",&num);
            printf("send file name: ");
            scanf("%s",inputFileName);
            for(i=0; i<num; i++)
                testBench(serv_adr,inputFileName);
            break;
    }

    //write(sock,message,strlen(message));
    //str_len=read(sock,message,BUFSIZE-1);
    //message[str_len]='\0';
    //printf("SERVER : %s",message);

    close(sock);
    return 0;
}
void testBench(struct sockaddr_in serv,char *fileName){
    int sock;
    sock=socket(PF_INET,SOCK_STREAM,0);
    if(sock<0){
       error_handling("sock make fail");
    }
    if(connect(sock,(struct sockaddr*)&serv,sizeof(serv))==-1){
        error_handling("connect error!");
    }
    else 
        puts("connected ......");
    write(sock,"1\0",strlen("1\0"));
    int pid=sendFile(sock,fileName);
    printf("%d\n",pid);
    sock=socket(PF_INET,SOCK_STREAM,0);
     if(connect(sock,(struct sockaddr*)&serv,sizeof(serv))==-1){
        error_handling("connect error!");
    }
    else 
        puts("connected ......"); 
    write(sock,"2",strlen("2"));
    checkProc(sock,pid);
    //close(sock);
}
void checkProc(int sock,int index){
    char message[BUFSIZE];
    int str_len=0;
    str_len=read(sock,message,BUFSIZE-1);
    message[str_len]='\0';
    printf("Input pid :");

    int pid;
    if(index==0){
        scanf(" %d",&pid);
        printf("%d selected\n",pid);
    }
    else{
        pid=index;
    }
    write(sock,message,strlen(message));
    
    sprintf(message,"%d",pid);
    str_len=read(sock,message,BUFSIZE-1);
    message[str_len]='\0';
    int state=atoi(message);
    printf("state : %d\n",state);
    write(sock,message,strlen(message));

    str_len=read(sock,message,BUFSIZE-1);
    message[str_len]='\0';
    printf("%s\n",message);
    write(sock,message,strlen(message));

    if(state==1){
        printf("[%d]running \n",pid);
    }
    else if(state==0){
        printf("no process in server [%d]\n",pid);
    }
    else if(state==2){
        printf("the job is done!\n");
        printf("the result is...\n");
        int flag=0;
        while((str_len=read(sock,message,BUFSIZE-1))!=0){
            if(str_len==1)
                break;
            message[str_len]='\0';
            printf("%s",message);
        }
    }
    printf("\n");
    close(sock);
}
int sendFile(int sock, char *fileName){
    char message[BUFSIZE];
    int str_len;
    str_len=read(sock,message,BUFSIZE-1);
    if(fileName==NULL){
        fputs("input the send executable :",stdout);
        scanf("%s",message);
        fflush(stdin);
    }
    else{
        strcpy(message,fileName);
    }
    int fd=open(message,O_RDONLY);
    struct stat st;
    stat(message,&st);
    usleep(100);
    printf("file size : %d\n\n",(int)st.st_size);
    message[strlen(message)]=0;
    write(sock,message,strlen(message));
    
    read(sock,message,BUFSIZE-1);
    if(fd==-1)
        error_handling("file error!");
    if(message[0]=='0'){
        error_handling("server error!");
    }
    /*send file size here*/
    int tempSize=(int)st.st_size;
    sprintf(message,"%d",tempSize);
    write(sock,message,strlen(message));
    while((str_len=read(fd,message,BUFSIZE))!=0){
        write(sock,message,str_len);
        message[str_len]='\0';
        //       printf("read size: %d\n",str_len);
    }
    close(fd);

    write(sock,message,strlen(message));
    str_len=read(sock,message,BUFSIZE-1);
    message[str_len]='\0';
    int pid=atoi(message);
    printf("Server : %s do your job\n",message);
    close(sock);
    return pid;
}
void showMenu(){
    fputs("1. send program and argument!\n",stdout);
    fputs("2. check program state\n",stdout);
    fputs("3. test bench!!\n",stdout);
    fputs("4. input 'q' or 'Q' for exit program\n",stdout);
}
void error_handling(char *message){
    fputs(message,stderr);
    fputs("\n",stderr);
    exit(1);
}
