#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<string.h>
#define BUFSIZE 1024
#define PORT 7890
void main(int argc, char *argv[]){
	int s,n;
	char *haddr;
	struct sockaddr_in server_addr;
	char buf[BUFSIZE];

	if(argc!=2){
		printf("Usage : %s IP\n",argv[0]);
		exit(0);
	}

	haddr=argv[1];

	if((s=socket(PF_INET,SOCK_STREAM,0))<0){
		printf("can't create socket");
		exit(0);
	}

	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(argv[1]);
	server_addr.sin_port=htons(PORT);
	
	if(connect(s,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
		printf("connect error!\n");
		exit(0);
	}

	while((n=read(s,buf,BUFSIZE)>0)){
		buf[n]='\0';
		printf("Server : %s\n",buf);
	}
	close(s);
}
