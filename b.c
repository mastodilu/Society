#include <stdlib.h>
#include <stdio.h>
#include "header.h"


void print_rcvd_msg(struct mymsg msg);
void print_sent_msg(struct mymsg msg);


struct msqid_ds msq;
int love_msg_queue; //id of private message queue used to meet people

int main(int argc, char* argv[])
{
    //argv: cmd type name genome sem1 sem2 msgQueue
    if(argc < 6)
        errExit("B too few arguments");
    
    struct person myself;
        myself.type = 'B';
        myself.name = atoi(argv[2]);
        myself.genome = (unsigned long)atol(argv[3]);
    int sem_init_people, sem_init_people2;//semaphores to start children
        sem_init_people = atoi(argv[4]);
        sem_init_people2 = atoi(argv[5]);
    int msgq = atoi(argv[6]);//id message queue
    struct mymsg msg_in, love_letter;

    
    //tell parent you're ready to live
	if( releaseSem(sem_init_people, 0) != 0 )
		errExit("B releaseSem sem_init_people child process");

	//wait for parent permission to start
	if( reserveSem(sem_init_people2, 0) != 0 )
		errExit("B reserveSem sem_init_people2 child process");
	

    printf("%c name:%c gen:%lu sem1:%d sem2:%d msgq:%d\n",
        myself.type, myself.name, myself.genome, sem_init_people, sem_init_people2, msgq );
    

    //read message from queue
    if( msgrcv(msgq, &msg_in, sizeof(msg_in), -OFFSET, 0) < 1 )
        errExit("msgrcv");
    print_rcvd_msg(msg_in);


    //send love_letter to A
    love_letter.mtype = (long)getpid();
    love_letter.mtxt.pid = (int)getpid();
    love_letter.mtxt.type = 'B';
    love_letter.mtxt.name = myself.name;
    love_letter.mtxt.genome = myself.genome;
    love_letter.mtxt.key_of_love = -1;
    love_letter.mtxt.partner = -1;
    
    if( msgsnd(msg_in.mtxt.key_of_love, &love_letter, sizeof(love_letter), 0) == -1 )
        errExit("B msgsnd love_letter to A");
    print_sent_msg(love_letter);
    

        
    pause();
    return EXIT_SUCCESS;
}


/*
 * print received message
 */
void print_rcvd_msg(struct mymsg msg)
{
    printf("B received mtype:%lu pid:%d type:%c name:%c gen:%lu key<3:%d pid<3:%d\n",
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
    printf("B sent mtype:%lu pid:%d type:%c name:%c gen:%lu key<3:%d pid<3:%d]\n",
        msg.mtype,
        (int)msg.mtxt.pid,
        msg.mtxt.type,
        msg.mtxt.name,
        msg.mtxt.genome,
        msg.mtxt.key_of_love,
        (int)msg.mtxt.partner );
}