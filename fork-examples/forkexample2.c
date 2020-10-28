#include <stdio.h>
#include <unistd.h> //fork()
#include <stdlib.h> //exit()
#include <string.h> //strcpy

int gVar = 2;

int main() {
	char pName[80] = "";
	int lVar = 20;
	
	pid_t pID = fork();
	if(pID == 0) {	//child
		// Code executed only by child process
		strcpy(pName,"Child:  ");	
		++gVar;
		++lVar;	
	}
	else if(pID < 0) { //failed to fork
		perror("Failed to fork\n");
		exit(1); 
	}
	else {		//parent
		//Code executed only by parent process
		strcpy(pName,"Parent: ");
	}
	
	//Code executed by both parent and child process
	printf("%s",pName);
	printf("Global: %d ",gVar);
	printf("Local: %d\n",lVar);
}
