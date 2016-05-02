#ifndef STDIO
#define STDIO
#include<string.h>
#include<stdio.h>
#endif

#ifndef FILEH
#define FILEH
#include<fcntl.h>
#endif


#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>


#define BUFSIZE 1024
#define PORT 7890

char buffer[10]="hello";

int main(int argc, char *argv[]){
	int c_socket,s_socket;
	struct sockaddr_in c_addr,s_addr;
	int len;
	int n;

	//make the socket for connect client
	s_socket=socket(PF_INET,SOCK_STREAM,0);

	//setting server
	memset(&s_addr,0,sizeof(s_addr));
	s_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	s_addr.sin_family=AF_INET;
	s_addr.sin_port=htons(PORT);

	if(bind(s_socket,(struct sockaddr*)&s_addr, sizeof(s_addr))==-1){
		printf("can not bind\n");
		return -1;
	}
	
	if(listen(s_socket,5)==-1){
		printf("listen fail\n");
		return -1;
	}

	len=sizeof(c_addr);
	c_socket=accept(s_socket,(struct sockaddr *)&c_addr,&len);
	printf("server: client connected\n");
	n=strlen("hello");
	write(c_socket,"hello",n);
	char buf[BUFSIZE];
	int rd;
	while(1){
		rd=read(c_socket,buf,BUFSIZE);
		buf[rd]='\0';
		switch(buf[0]){
			case '1':
				break;
			case '2':
				break;
			default:
				write(c_socket,"Server :select menu\n",sizeof("Server :select Menu\n"));
				break;
		}
	}
	close(c_socket);
	close(s_socket);
}
