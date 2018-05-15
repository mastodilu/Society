#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
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
        if( sprintf(myself.name, "%s", argv[2]) <0 )
            errExit("A sprintf name parameter");
        myself.genome = (unsigned long)atol(argv[3]);
    int sem_init_people, sem_init_people2;//semaphores to start children
        sem_init_people = atoi(argv[4]);
        sem_init_people2 = atoi(argv[5]);
    int msgq = atoi(argv[6]);//id of main message queue
    struct mymsg msg_in, love_letter, response, msg_gestore1, msg_gestore2;
    int queue_of_love, engaged = -1;
    pid_t possible_partner;

    
    //tell parent you're ready to live
	if( releaseSem(sem_init_people, 0) != 0 )
		errExit("B releaseSem sem_init_people child process");

	//wait for parent permission to start
	if( reserveSem(sem_init_people2, 0) != 0 )
		errExit("B reserveSem sem_init_people2 child process");
	

    printf("%c:%d name:%s gen:%lu sem1:%d sem2:%d msgq:%d\n",
        myself.type, (int)getpid(), myself.name, myself.genome, sem_init_people, sem_init_people2, msgq );
    

    while(engaged != 0){

        //read first message from queue with mtype < OFFSET
        if( msgrcv(msgq, &msg_in, sizeof(msg_in), -OFFSET, 0) < 1 )
            errExit("B msgrcv 1");
        print_rcvd_msg(msg_in);


        queue_of_love = msg_in.mtxt.key_of_love;
        possible_partner = msg_in.mtxt.pid;


        //send love_letter to A
        love_letter.mtype = possible_partner;
        love_letter.mtxt.pid = getpid();
        love_letter.mtxt.type = 'B';
        if( sprintf(love_letter.mtxt.name, "%s", myself.name) < 0 )
            errExit("B sprintf love_letter.mtxt.name");
        love_letter.mtxt.genome = myself.genome;
        love_letter.mtxt.key_of_love = -1;
        love_letter.mtxt.partner = 0;

#if 0
        if( msgsnd(queue_of_love, &love_letter, sizeof(love_letter), 0) == -1 )
            errExit("B msgsnd love_letter to A");
        print_sent_msg(love_letter);
#else
        if( msgsnd(queue_of_love, &love_letter, sizeof(love_letter), 0) == -1 ){
            if(errno == EACCES)         printf("B msgsnd EACCESS\n");
            else if(errno == EAGAIN)    printf("B msgsnd EAGAIN\n");
            else if(errno == EFAULT)    printf("B msgsnd EFAULT\n");
            else if(errno == EIDRM)     printf("B msgsnd EIDRM\n");
            else if(errno == EINTR)     printf("B msgsnd EINTR\n");
            else if(errno == EINVAL)    printf("B msgsnd EINVAL id:%d\n", queue_of_love);
            else if(errno == ENOMEM)    printf("B msgsnd ENOMEN\n");
        }
        print_sent_msg(love_letter);
#endif


        //printf("B key:%d\n", queue_of_love);


        //receive response from A
        if( msgrcv(queue_of_love, &response, sizeof(response), getpid(), 0) < 1 ){
            if( errno == ENOMSG )       errExit("B msgrcv response ENOMSG");
            else                        errExit("B msgrcv 2");
        }
        print_rcvd_msg(response);


        engaged = response.mtxt.key_of_love;//exit while or loop again
    }//-while

    //here if B is engaged with A, so tell gestore you're engaged
    
    //message with info of A
    msg_gestore2.mtype = response.mtxt.pid + OFFSET; //pid_A + OFFSET
    msg_gestore2.mtxt.pid = response.mtxt.pid;
    msg_gestore2.mtxt.type = 'A';
    if( sprintf(msg_gestore2.mtxt.name, "%s", response.mtxt.name) < 0 )
            errExit("B sprintf msg_gestore2.mtxt.name");
    msg_gestore2.mtxt.genome = response.mtxt.genome;
    msg_gestore2.mtxt.key_of_love = engaged;
    msg_gestore2.mtxt.partner = getpid();

    //message with info of B
    msg_gestore1.mtype = getppid() + OFFSET; //pid_gestore + OFFSET
    msg_gestore1.mtxt.pid = getpid();
    msg_gestore1.mtxt.type = 'B';
    if( sprintf(msg_gestore1.mtxt.name, "%s", myself.name) < 0 )
            errExit("B sprintf msg_gestore1.mtxt.name");
    msg_gestore1.mtxt.genome = myself.genome;
    msg_gestore1.mtxt.key_of_love = engaged;
    msg_gestore1.mtxt.partner = response.mtxt.pid;

    if( msgsnd(msgq, &msg_gestore1, sizeof(msg_gestore1), 0) == -1 ){
        if(errno == EINTR)	        perror("B engagement details 1 to gestore, caught a signal and failed msgsnd");
        else     			        perror("B engagement details 1 to gestore, msgsnd"); 
        exit(EXIT_FAILURE);
    }
    if( msgsnd(msgq, &msg_gestore2, sizeof(msg_gestore2), 0) == -1 ){
        if(errno == EINTR)	        perror("B engagement details 2 to gestore, caught a signal and failed msgsnd");
        else     			        perror("B engagement details 2 to gestore, msgsnd"); 
        exit(EXIT_FAILURE);
    }

    printf("B:%d engaged and paused with A:%d\n", (int)getpid(), (int)response.mtxt.pid);

        
    pause();
    return EXIT_SUCCESS;
}


/*
 * print received message
 */
void print_rcvd_msg(struct mymsg msg)
{
    printf("B:%d received mtype:%lu pid:%d type:%c name:%s gen:%lu key<3:%d pid<3:%d\n",
        (int)getpid(),
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
    printf("B:%d sent mtype:%lu pid:%d type:%c name:%s gen:%lu key<3:%d pid<3:%d\n",
        (int)getpid(),
        msg.mtype,
        (int)msg.mtxt.pid,
        msg.mtxt.type,
        msg.mtxt.name,
        msg.mtxt.genome,
        msg.mtxt.key_of_love,
        (int)msg.mtxt.partner );
}