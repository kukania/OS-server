#ifndef STDIO
#define STDIO
#include<stdio.h>
#include<string.h>
#endif

#ifndef FILEH
#define FILEH
#include<fcntl.h>
#endif

#include"file.h"

#define BUFSIZE 1024
int fileReadOpen(char* input){
	int fd=open(input,O_RDWR);
	if(fd>0){
		return fd;
	}
	else{
		printf("fail open %s check %s \n",input,input);
		return -1;
	}
}
int fileWriteOpen(char *input){
	int fd=open(input,O_WRONLY|O_CREAT|O_EXCL,0644);
	if(fd>0)return fd;
	else {
		printf("fail opne %s check %s \n",input,input);
		return -1;
	}
}
int closeFile(int fd){
	close(fd);
}
