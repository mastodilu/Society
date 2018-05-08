#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#include "header.h"

#ifndef MAX_PEOPLE
#define MAX_PEOPLE 5
#endif

#define SIZE_NUMBER 15

void person_params(struct person);

unsigned int init_people;
char * args[8];
char * envs[1];
char * child_name;
char * child_genome;
char * child_sem; //contain name of semaphore 1
char * child_sem2;//contain name of semaphore 2
char * child_msgq_a; //contain name of message queue
int sem_init_people; //semaphore that stops parent process and makes it wait for init_people children
int sem_init_people2; //semaphore that tells init_people children to start living
struct msqid_ds msq; //struct associated with msg queue
int msgq_a; //id of message queue to share info for processes of type A

int main(int argc, char * argv[])
{
    unsigned int i = 0;
    init_people = 0;
    time_t t;//for srand
    struct person person;

    //initialize random number generator
    srand((unsigned) time(&t));

    //assign a value to init_people
	init_people = generate_first_people((unsigned int)2, (unsigned int)MAX_PEOPLE);
    printf("init_people %d\n", init_people);

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

    //create children
    for(i = 0; i < init_people; i++){
        
        person = create_person();
        print_person(person);

        //set parameters for execve
        person_params(person);
        
    }

    //TODO delete semaphores and queues

    if( msgctl(msgq_a, IPC_RMID, &msq) != 0 )
        perror("gestore main_msg_queue RMID");

    if( semctl(sem_init_people, 0, IPC_RMID, NULL) == -1 )
		perror("remove sem_init_people");

	if( semctl(sem_init_people2, 0, IPC_RMID, NULL) == -1 )
		perror("remove sem_init_people2");

    return EXIT_SUCCESS;
}


/*
 * set parameters for execve
 */
void person_params(struct person person)
{
    int i = 0;
    child_sem = calloc(SIZE_NUMBER, sizeof(char));
    child_sem2 = calloc(SIZE_NUMBER, sizeof(char));
    child_name = calloc(SIZE_NUMBER, sizeof(char));
    child_genome = calloc(SIZE_NUMBER, sizeof(char));
    child_msgq_a = calloc(SIZE_NUMBER, sizeof(char));

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
    
    //TYPE
    if(person.type == 'A')
        args[i++] = "./A";
    else
        args[i++] = "./B";
    
    //NAME
    if( sprintf(child_name, "%d", person.name) < 0 )
        errExit("child_name sprintf");
    args[i++] = child_name;
    
    //GENOME
    if( sprintf(child_genome, "%lu", person.genome) < 0 )
        errExit("child_genome sprintf");
    args[i++] = child_genome;

    //SEMAPHORE 1
    if( sprintf(child_sem, "%d", sem_init_people) < 0 )
        errExit("child_sem sprintf");
    args[i++] = child_sem;

    //SEMAPHORE 2
    if( sprintf(child_sem2, "%d", sem_init_people2) < 0 )
        errExit("child_sem2 sprintf");
    args[i++] = child_sem2;

    //MESSAGE QUEUE
    if( sprintf(child_msgq_a, "%d", msgq_a) < 0 )
        errExit("child_msgq_a sprintf");
    args[i++] = child_msgq_a;

    free(child_name);
    free(child_genome);
    free(child_sem);
    free(child_sem2);
    free(child_msgq_a);
}