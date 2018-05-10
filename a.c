#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#include "header.h"

void print_rcvd_msg(struct mymsg msg);
void print_sent_msg(struct mymsg msg);
int similar(unsigned long, unsigned long);

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
    struct mymsg welcome, love_letter;
    int count_refused = 0; //count refused love requests
    int engaged = -1;

        
    //tell parent you're ready to live
	if( releaseSem(sem_init_people, 0) != 0 )
		errExit("A releaseSem sem_init_people child process");

	//wait for parent permission to start
	if( reserveSem(sem_init_people2, 0) != 0 )
		errExit("B reserveSem sem_init_people2 child process");


    printf("%c name:%c gen:%lu sem1:%d sem2:%d msgq:%d\n",
        myself.type, myself.name, myself.genome, sem_init_people, sem_init_people2, msgq );


    //create personal message queue of love
	love_msg_queue = msgget(IPC_PRIVATE, 0666|IPC_CREAT|IPC_EXCL);
	if( love_msg_queue == -1 ){
		if( errno == EEXIST ){
			if( msgctl(love_msg_queue, IPC_RMID, &msq) == -1 )
				errExit("rmid queue of love");
		}else
			errExit("msgget queue of love");
	}
    printf("A <3queue<3:%d\n", love_msg_queue);


    while(engaged != 0){
        //create message to introduce A to others
        welcome.mtype = (long)myself.genome;
        welcome.mtxt.pid = getpid();
        welcome.mtxt.type = 'A';
        welcome.mtxt.name = myself.name;
        welcome.mtxt.genome = myself.genome;
        welcome.mtxt.key_of_love = love_msg_queue;
        welcome.mtxt.partner = -1;

        if( msgsnd(msgq, &welcome, sizeof(welcome), 0) == -1 ){
            if( errno == EINTR)	    perror("A-welcome caught a signal and failed msgsnd");
            else     			    perror("A-welcome msgsnd"); 
            exit(EXIT_FAILURE);
        }


        //read love_letter from B
        if( msgrcv(love_msg_queue, &love_letter, sizeof(love_letter), 0, 0) == -1 )
            errExit("A msgrcv love_letter from B");
        print_rcvd_msg(love_letter);

        engaged = -1;

        //accept or reject?
        if(myself.genome%love_letter.mtxt.genome == 0){
            printf("A must accept - 1\n");
            engaged = 0;
        }else if( similar(myself.genome, love_letter.mtxt.genome) == 0 ){
            printf("A must accept - 2\n");
            engaged = 0;
        }else if( count_refused >= 2 ){
            printf("A must accept - 3\n");
            engaged = 0;
        }else{
            count_refused++;
            printf("A rejected %d\n", (int)love_letter.mtxt.pid);
        }

        
        //send B a response
        love_letter.mtype = getpid();
        love_letter.mtxt.pid = getpid();
        love_letter.mtxt.type = 'A';
        love_letter.mtxt.name = myself.name;
        love_letter.mtxt.genome = myself.genome;
        love_letter.mtxt.partner = getpid();
        love_letter.mtxt.key_of_love = engaged; //0 means accepted

        if( msgsnd(love_msg_queue, &love_letter, sizeof(love_letter), 0) == -1 ){
            if( errno == EINTR)	perror("A-love_letter to B caught a signal and failed msgsnd");
            else     			perror("A-love_letter to B msgsnd"); 
            exit(EXIT_FAILURE);
        }
    }

    pause();
    return EXIT_SUCCESS;
}



/*
 * return 0 if the difference between m and n is less than 10% of m
 * */
int similar(unsigned long m, unsigned long n)
{
    long diff = labs((long)m-(long)n);
    
    //if difference is little
    if( diff < (long)m/10 )
        return 0;

    return -1;
}


/*
 * print received message
 */
void print_rcvd_msg(struct mymsg msg)
{
    printf("A received mtype:%lu pid:%d type:%c name:%c gen:%lu key<3:%d pid<3:%d\n",
        msg.mtype,
        (int)msg.mtxt.pid,
        msg.mtxt.type,
        msg.mtxt.name,
        msg.mtxt.genome,
        msg.mtxt.key_of_love,
        (int)msg.mtxt.partner );
}


/*
 * print sent message
 */
void print_sent_msg(struct mymsg msg)
{
    printf("A sent mtype:%lu pid:%d type:%c name:%c gen:%lu key<3:%d pid<3:%d]\n",
        msg.mtype,
        (int)msg.mtxt.pid,
        msg.mtxt.type,
        msg.mtxt.name,
        msg.mtxt.genome,
        msg.mtxt.key_of_love,
        (int)msg.mtxt.partner );
}