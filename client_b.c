#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#define BUFSIZE 1024

void error_handling(char *message);

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

	while(1){
		fputs("INPUT message: ",stdout);
		fgets(message,BUFSIZE,stdin);

		if(!strcmp(message,"q\n")||!strcmp(message,"Q\n"))
			break;
		
		write(sock,message,strlen(message));
		str_len=read(sock,message,BUFSIZE-1);
		message[str_len]='\0';
		printf("SERVER : %s",message);
	}
	close(sock);
	return 0;
}

void error_handling(char *message){
	fputs(message,stderr);
	fputs("\n",stderr);
	exit(1);
}
