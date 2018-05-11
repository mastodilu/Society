#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "header.h"

#ifndef MAX_PEOPLE
#define MAX_PEOPLE 50
#endif

#define SIZE_NUMBER 15

#ifndef SIM_TIME
#define SIM_TIME 20
#endif

#ifndef BIRTH_DEATH
#define BIRTH_DEATH 5
#endif


void handle_signal(int);
void remove_all();
void terminate_children();
void free_all();
void person_params(struct person);
void print_rcvd_msg(struct mymsg msg);


unsigned int init_people;
char * args[8];
char * envs[] = {NULL};
char * child_name;
char * child_genome;
char * child_sem; //contain name of semaphore 1
char * child_sem2;//contain name of semaphore 2
char * child_msgq_a; //contain name of message queue
int sem_init_people; //semaphore that stops parent process and makes it wait for init_people children
int sem_init_people2; //semaphore that tells init_people children to start living
struct msqid_ds msq; //struct associated with msg queue
int msgq_a; //id of message queue to share info for processes of type A
pid_t * initial_children;//will contain pids of every child
struct mymsg msg;
struct sigaction sa;
sigset_t my_mask;
pid_t pidB;
unsigned long genomeA = 0, genomeB = 0;

//int main(int argc, char * argv[])
int main(void)
{
    unsigned int i = 0;
    init_people = 0;
    time_t t;//for srand
    struct person person;
    pid_t child = 0;
    int flag = 0;

    child_sem = calloc(SIZE_NUMBER, sizeof(char));
    child_sem2 = calloc(SIZE_NUMBER, sizeof(char));
    child_name = calloc(SIZE_NUMBER, sizeof(char));
    child_genome = calloc(SIZE_NUMBER, sizeof(char));
    child_msgq_a = calloc(SIZE_NUMBER, sizeof(char));

    //handle signals
	sa.sa_handler = &handle_signal;
	sa.sa_flags = 0; //No special behaviour
	sigemptyset(&my_mask); //empty mask, do not ignore any signal
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);

    //initialize random number generator
    srand((unsigned) time(&t));

    //assign a value to init_people
#if 1
	init_people = generate_first_people((unsigned int)2, (unsigned int)MAX_PEOPLE);
#else
    init_people = 2;
#endif
    printf("init_people %d\n", init_people);

    initial_children = calloc(init_people, sizeof(pid_t));
    if(initial_children == NULL)
        errExit("initial_children NULL");

    //create 2 semaphores to allow children to start living
	sem_init_people = semget(IPC_PRIVATE, 1, 0666|IPC_CREAT|IPC_EXCL);
	if( sem_init_people == -1 ){
		if(errno == EEXIST){ //delete semaphores if exist
			if( semctl(sem_init_people, 0, IPC_RMID, NULL) == -1 )
				errExit("remove sem_init_people");
        }else
            errExit("semget init_people");
	}
	
	sem_init_people2 = semget(IPC_PRIVATE, 1, 0666|IPC_CREAT|IPC_EXCL);
	if( sem_init_people2 == -1 ){
		if(errno == EEXIST){ //delete semaphores if exist
			if( semctl(sem_init_people2, 0, IPC_RMID, NULL) == -1 )
				errExit("remove sem_init_people2");
        }else
			errExit("semget sem_init_people2");
	}

    printf("sem1:%d sem2:%d\n", sem_init_people, sem_init_people2);

    //create message queue
	msgq_a = msgget(IPC_PRIVATE, 0666|IPC_CREAT|IPC_EXCL);//try to crete msg queue and return error if already exist
	if(msgq_a == -1){
		if( errno == EEXIST ){//if exists
			// delete message queue
			if( msgctl(msgq_a, IPC_RMID, &msq) == -1 )
				perror("rmid");
        }else
			perror("msgget queue A");
		exit(EXIT_FAILURE);
	}
    printf("msgq:%d\n", msgq_a);


    //RWX permissions for people processes
	if( chmod("./a", 0777) != 0 )
		errExit("chmod person A");
	if( chmod("./b", 0777) != 0 )
		errExit("chmod person B");

    //create children
    for(i = 0; i < init_people; i++){
        
        person = create_person();
#if 0
        if(i == 0)      person.type = 'A';
        else            person.type = 'B';
#endif

        //set parameters for execve
        person_params(person);
        
        switch( child = fork() ){
            
            case -1:{
                errExit("fork error");
            }

            case 0:{//child
                    if( execve(args[0], args, envs) == -1 ){
                        perror("gestore execve");
                    }

                    //we're here if execve didnt't work
                    exit(EXIT_FAILURE);
                break;
            }
            default:{//father
                //add every child in the array of children
				initial_children[i] = child;
                print_person(person);
            }
        }//-switch
    }//-for


	//wait for every child to be ready to start
	for(i = 0; i < init_people; i++){
		if( reserveSem(sem_init_people, 0) != 0 )
			errExit("reserveSem sem_init_people");
	}
    	
	//allow every child to start by releasing the second semaphore
	for(i = 0; i < init_people + 1; i++){
		if( releaseSem(sem_init_people2, 0) != 0 )
			errExit("releaseSem sem_init_people2");
	}


    //shut system down after N seconds
	alarm(SIM_TIME); //30 seconds


    for(;;){
        sleep(BIRTH_DEATH);
        printf("Gestore is reading messages\n");
        do{
            flag = 0;
            //read the first message (info of A)
            //and wait for the second message with info of B
            if( msgrcv(msgq_a, &msg, sizeof(msg), ((long)OFFSET+getpid()), (int)IPC_NOWAIT) == -1){
                if( errno != ENOMSG ){
                    perror("Gestore can't receive any message");
                    flag = -2;
                }
                else 
                    flag = -1;
            }
            //flag untouched, first message received
            if(flag == 0){
                
                pidB = msg.mtxt.partner;
                genomeA = msg.mtxt.genome;
                
                print_rcvd_msg(msg);
                
                //read second message
                if( msgrcv(msgq_a, &msg, sizeof(msg), (long)OFFSET+pidB, 0) == -1 )
                    perror("gestore can't receive any message");
                
                genomeB = msg.mtxt.genome;
                
                print_rcvd_msg(msg);
            }
                
        }while(flag == 0);
    }


    return EXIT_SUCCESS;
}


/*
 * set parameters for execve
 */
void person_params(struct person person)
{
    if(child_name == NULL)
        errExit("child_name NULL");
    if(child_genome == NULL)
        errExit("child_genome NULL");
    if(child_sem == NULL)
        errExit("child_sem NULL");
    if(child_sem2 == NULL)
        errExit("child_sem2 NULL");
    if(child_msgq_a == NULL)
        errExit("child_msgq_a NULL");
    
    //CMD and TYPE
    if(person.type == 'A'){
        args[0] = "./a";
        args[1] = "A";
    }else{
        args[0] = "./b";
        args[1] = "B";
    }
    
    //NAME
    if( sprintf(child_name, "%d", person.name) < 0 )
        errExit("child_name sprintf");
    args[2] = child_name;
    
    //GENOME
    if( sprintf(child_genome, "%lu", person.genome) < 0 )
        errExit("child_genome sprintf");
    args[3] = child_genome;

    //SEMAPHORE 1
    if( sprintf(child_sem, "%d", sem_init_people) < 0 )
        errExit("child_sem sprintf");
    args[4] = child_sem;

    //SEMAPHORE 2
    if( sprintf(child_sem2, "%d", sem_init_people2) < 0 )
        errExit("child_sem2 sprintf");
    args[5] = child_sem2;

    //MESSAGE QUEUE
    if( sprintf(child_msgq_a, "%d", msgq_a) < 0 )
        errExit("child_msgq_a sprintf");
    args[6] = child_msgq_a;

    //args[7] = "\0";
}


/*
 * handle signals
 */
void handle_signal(int signum)
{
    switch(signum){

        case SIGALRM:{ //terminate children and program
            
            terminate_children();
            remove_all();
			free_all();
			//TODO print_stats();
			
            exit(EXIT_SUCCESS);
            break;
        }

        default:{
            printf("parent default signal\n");
        }
    }//-switch
}


/*
 * delete semaphores and queues
 */
void remove_all()
{
    if( msgctl(msgq_a, IPC_RMID, &msq) != 0 )
        perror("gestore main_msg_queue RMID");

    if( semctl(sem_init_people, 0, IPC_RMID, NULL) == -1 )
		perror("remove sem_init_people");

	if( semctl(sem_init_people2, 0, IPC_RMID, NULL) == -1 )
		perror("remove sem_init_people2");
}


//free allocated memory
void free_all()
{
    free(child_name);
    free(child_genome);
    free(child_sem);
    free(child_sem2);
    free(child_msgq_a);
}


/*
 * terminate every children
 */
void terminate_children()
{
	unsigned int i = 0;

	for(i = 0; i < init_people; i++){
		if( kill(initial_children[i], 0) == 0 ){
			if( kill(initial_children[i], SIGTERM) == -1 ){
				perror("kill sigterm to child");
			}
		}
	}
}


/*
 * print received message
 */
void print_rcvd_msg(struct mymsg msg)
{
    printf("Gestore received mtype:%lu pid:%d type:%c name:%c gen:%lu key<3:%d pid<3:%d\n",
        msg.mtype,
        (int)msg.mtxt.pid,
        msg.mtxt.type,
        msg.mtxt.name,
        msg.mtxt.genome,
        msg.mtxt.key_of_love,
        (int)msg.mtxt.partner );
}