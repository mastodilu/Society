#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#include "header.h"

struct msqid_ds msq;
int love_msg_queue; //id of private message queue used to meet people

int main(int argc, char* argv[])
{
    //argv: cmd type name genome sem1 sem2 msgQueue
    if(argc < 6)
        errExit("A too few arguments");
    
    struct person myself;
        myself.type = 'A';
        myself.name = atoi(argv[2]);
        myself.genome = (unsigned long)atol(argv[3]);
    int sem_init_people, sem_init_people2;//semaphores to start children
        sem_init_people = atoi(argv[4]);
        sem_init_people2 = atoi(argv[5]);
    int msgq = atoi(argv[6]);//id message queue
    struct mymsg welcome;

        
    //tell parent you're ready to live
	if( releaseSem(sem_init_people, 0) != 0 )
		errExit("A releaseSem sem_init_people child process");

	//wait for parent permission to start
	if( reserveSem(sem_init_people2, 0) != 0 )
		errExit("B reserveSem sem_init_people2 child process");


    printf("%c name:%c gen:%lu sem1:%d sem2:%d msgq:%d\n",
        myself.type, myself.name, myself.genome, sem_init_people, sem_init_people2, msgq );


    //create message to introduce A to others
    welcome.mtype = (long)myself.genome;
    welcome.mtxt.pid = (int)getpid();
    welcome.mtxt.type = 'A';
    welcome.mtxt.name = myself.name;
    welcome.mtxt.genome = myself.genome;
    welcome.mtxt.key_of_love = -1;
    welcome.mtxt.partner = -1;

    if( msgsnd(msgq, &welcome, sizeof(welcome), 0) == -1 ){
        if( errno == EINTR)	perror("A-welcome caught a signal and failed msgsnd");
		else     			perror("A-welcome msgsnd"); 
		exit(EXIT_FAILURE);
    }
    
    pause();
    return EXIT_SUCCESS;
}