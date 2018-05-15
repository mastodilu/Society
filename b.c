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
        //myself.name = argv[2];
        if( sprintf(myself.name, "%s", argv[2]) <0 )
            errExit("A sprintf name parameter");
        myself.genome = (unsigned long)atol(argv[3]);
    int sem_init_people, sem_init_people2;//semaphores to start children
        sem_init_people = atoi(argv[4]);
        sem_init_people2 = atoi(argv[5]);
    int msgq = atoi(argv[6]);//id message queue
    struct mymsg msg_in, love_letter, response;
    int queue_of_love, engaged = -1;

    
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
            errExit("msgrcv");
        print_rcvd_msg(msg_in);


        queue_of_love = msg_in.mtxt.key_of_love;


        //send love_letter to A
        love_letter.mtype = getpid();
        love_letter.mtxt.pid = (int)getpid();
        love_letter.mtxt.type = 'B';
        //love_letter.mtxt.name = myself.name;
        if( sprintf(love_letter.mtxt.name, "%s", myself.name) < 0 )
            errExit("B sprintf love_letter.mtxt.name");
        love_letter.mtxt.genome = myself.genome;
        love_letter.mtxt.key_of_love = -1;
        love_letter.mtxt.partner = 0;
        
#if 1
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
        if( msgrcv(queue_of_love, &response, sizeof(response), 0, 0) < 1 ){
            if( errno == ENOMSG )       errExit("B msgrcv response ENOMSG");
            else                        errExit("B msgrcv");
        }
        print_rcvd_msg(response);


        engaged = response.mtxt.key_of_love;//exit while or loop again
    }

    printf("B:%d engaged and paused\n", (int)getpid());

        
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
    printf("B:%d sent mtype:%lu pid:%d type:%c name:%s gen:%lu key<3:%d pid<3:%d]\n",
        (int)getpid(),
        msg.mtype,
        (int)msg.mtxt.pid,
        msg.mtxt.type,
        msg.mtxt.name,
        msg.mtxt.genome,
        msg.mtxt.key_of_love,
        (int)msg.mtxt.partner );
}