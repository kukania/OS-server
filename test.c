#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#define BUFSIZE 1024
int main(){
	char message[BUFSIZE];
	scanf("%s",message);
	int fd=open(message,O_RDONLY);
	if(fd==-1)
		printf("error\n");
	else 
		printf("success\n");
}
