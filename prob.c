#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ITEMS	10
#define MAX_LEN		256

extern char **environ;

int parseitems (char* cmdln, char items[][MAX_LEN])
{
	int i = 0;
	char* pch = NULL;

	pch = strtok (cmdln, " \n");
	while (pch != NULL && i < MAX_ITEMS) {
		strcpy (items[i], pch);
		//items[i][strlen(items[i])]=0;
		pch = strtok (NULL, " \n");
		i++;
	}

	return i;
}

int main (int argc, char* argv[])
{
	char cmdln[MAX_LEN];
	char items[MAX_ITEMS][MAX_LEN];
	int nr_items;

	pid_t pidA[3];
	pid_t pid;
	int pidC=0;
	//strcpy (cmdln, "Test string");
	while(1){
		fgets(cmdln,MAX_LEN,stdin);
		fflush(stdin);
		nr_items = parseitems (cmdln, items);
		int i;
		if(strcmp(items[0],"exit")==0){
			printf("Good bye\n");
			return 0;
		}
		int child_status;
		char *myArgv[MAX_ITEMS];
		for(i=0; i<nr_items; i++){
			myArgv[i]=&items[i];
		}
		myArgv[nr_items]=NULL;

		if(strcmp(items[nr_items-1],"&")==0){
			printf("back ground gogo\n");
			char *myArgv2[MAX_ITEMS];
			for(i=0;i<nr_items-1;i++){
				myArgv2[i]=&items[i];
			}
			myArgv[nr_items-1]=NULL;
			pidA[pidC]=fork();
			if(pidC==3){
				printf("process is three, kill another\n");
				continue;
			}
			if(pidA[pidC]==0){
				printf("command : %s\n",items[0]);
				//printf("a:%s I:%s\n",argv[1],items[nr_items]);
				int rV=execvpe(items[0],myArgv2,environ);
				//printf("%d\n",rV);
				exit(0);
			}
			else{
				printf("[%d] running",pidA[pidC]);
				pidC++;
			}
		}
		else{
			if(strcmp(items[0],"close")==0){
				int num=atoi(items[1]);
				i=0;
				int flag=0;
				for(; i<pidC; i++){
					if(pidA[i]==num){
						flag=1; break;
					}
				}
				if(flag==0){
					printf("no pid\n");
					continue;
				}
				waitpid(pidA[i],&child_status,0);
				int j=0;
				for(j=i; j<3; j++){
					pidA[j]=pidA[j+1];
				}
				pidC--;
			}
			else if(strcmp(items[0],"status")==0){
				for(i=0;i<pidC; i++){
					printf("[%d]\n",pidA[i]);
				}
				continue;
			}
			pid=fork();
			if(pid==0)
				execvpe(items[0],myArgv,environ);
			else{
				wait(&child_status);
			}
		}
	}
	return 0;
}
