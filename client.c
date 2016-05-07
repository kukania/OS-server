#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<fcntl.h>

#define BUFSIZE 1024

void error_handling(char *message);
void showMenu();
void sendFile(int sock);
void checkProc(int sock);
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
    str_len=read(sock,message,BUFSIZE-1);
    message[str_len]='\0';
    printf("Server : %s  - %c\n",message,input);
    switch(input){
        case '1':
            //	printf("you selected menu 1\n");		
            sendFile(sock);
            break;
        case '2':
            checkProc(sock);
            break;
    }

    //write(sock,message,strlen(message));
    //str_len=read(sock,message,BUFSIZE-1);
    //message[str_len]='\0';
    //printf("SERVER : %s",message);

    close(sock);
    return 0;
}
void checkProc(sock){
    char message[BUFSIZE];
    int str_len;
 //   str_len=read(sock,message,BUFSIZE-1);
    message[str_len]='\0';
    printf("Input pid :");

    int pid;
    scanf(" %d",&pid);
    printf("%d selected",pid);
    sprintf(message,"%d",pid);
    write(sock,message,strlen(message));
    
    str_len=read(sock,message,BUFSIZE-1);
    message[str_len]='\0';
    int state=atoi(message);

    if(state==1){
        printf("[%d]running \n",pid);
    }
    else if(state==0){
        printf("no process in server [%d]\n",pid);
    }
    else if(state==2){
        printf("test\n");
    }
}
void sendFile(int sock){
    char message[BUFSIZE];
    int str_len;

    fputs("input the send executable :",stdout);
    scanf("%s",message);
    fflush(stdin);
    int fd=open(message,O_RDONLY);
    struct stat st;
    stat(message,&st);
    printf("file size : %d\n\n",(int)st.st_size);

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
        printf("read size: %d\n",str_len);
    }
    close(fd);

    write(sock,message,strlen(message));
    str_len=read(sock,message,BUFSIZE-1);
    message[str_len]='\0';
    printf("Server : %s\n",message);
}
void showMenu(){
    fputs("1. send program and argument!\n",stdout);
    fputs("2. check program state\n",stdout);
    fputs("3. input 'q' or 'Q' for exit program\n",stdout);
}
void error_handling(char *message){
    fputs(message,stderr);
    fputs("\n",stderr);
    exit(1);
}
