#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "header.h"

#define SIZE_NUMBER 15


void handle_signal(int);
void remove_all();
void terminate_children();
void free_all();
void person_params(struct person);
void print_rcvd_msg(struct mymsg msg);
void make_children(char*, char*, pid_t, pid_t, unsigned long, unsigned long);
void print_newborn(struct person);
struct person create_person();
void update_world_record(struct person);
void print_world_records(); 


unsigned int init_people = 0, count_A = 0, count_B = 0;
char * args[8];
char * envs[] = {NULL};
char * child_name;
char * child_genome;
char * child_sem;
char * child_sem2;
char * child_msgq_a;
char * temp_string;
char * longest_name;
int sem_init_people;
int sem_init_people2;
struct msqid_ds msq;
int msgq_a;
pid_t * children;
struct mymsg msg, msg2;
struct sigaction sa;
struct person long_name, big_genome;
sigset_t my_mask;
pid_t pidA, pidB;
unsigned long genomeA = 0, genomeB = 0, average_genome = 0;
char * name_A, * name_B;
FILE * my_file;
unsigned int max_people = 0, sim_time = 0, birth_death = 0;


//int main(int argc, char * argv[])
int main(void)
{
    unsigned int i = 0;
    time_t t;//for srand
    struct person person;
    pid_t child = 0;
    int flag = 0;

    child_sem = (char*)calloc(SIZE_NUMBER, sizeof(char));
    child_sem2 = (char*)calloc(SIZE_NUMBER, sizeof(char));
    child_name = (char*)calloc(64, sizeof(char));
    child_genome = (char*)calloc(SIZE_NUMBER, sizeof(char));
    child_msgq_a = (char*)calloc(SIZE_NUMBER, sizeof(char));
    name_A = (char*)calloc(64, sizeof(char));
    name_B = (char*)calloc(64, sizeof(char));
    temp_string = (char*)calloc(64, sizeof(char));
    longest_name = (char*)calloc(64, sizeof(char));

    
    if(name_A == NULL)          errExit("name_A is null");
    if(name_B == NULL)          errExit("name_B is null");
    if(temp_string == NULL)     errExit("temp_string is null");
    if(longest_name == NULL)    errExit("longest_name is null");


    big_genome.type = 'C';
    big_genome.genome = (unsigned long)0;
    if( sprintf(big_genome.name, "%s", "zz") < 0 )
        errExit("sprintf big_genome name");
    


    
    //read parameters from file
    my_file = fopen("config.txt", "r");
    if(my_file == NULL)
        errExit("gestore fopen");
    
    if( fscanf(my_file, "%s %u", temp_string, &max_people) < 0 )
        errExit("fscanf max_people");
    if( fscanf(my_file, "%s %u", temp_string, &sim_time) < 0 )
        errExit("fscanf sim_time");
    if( fscanf(my_file, "%s %u", temp_string, &birth_death) < 0 )
        errExit("fscanf birth_death");

    if(max_people < 2)              max_people = 2;
    if(max_people > 100)            max_people = 100;
    if(birth_death < 1)             birth_death = 1;
    if(birth_death > 10)            birth_death = 10;
    if(sim_time < birth_death)      sim_time = 2*birth_death;
    
    printf("max_people:%u sim_time:%u birth_death:%u\n", max_people, sim_time, birth_death);
    

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
	init_people = generate_first_people((unsigned int)2, max_people);
#else
    init_people = 2;
#endif
    printf("init_people:%d\n", init_people);

    children = calloc(init_people, sizeof(pid_t));
    if(children == NULL)
        errExit("children NULL");

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

    //initialize sem_init_people to 0 (reserved)
	if( initSemInUse(sem_init_people, 0) == -1 ){
		perror("initSemInUse for sem_init_people");
		exit(EXIT_FAILURE);
	}

	//initialize sem_init_people2 to 0 (reserved)
	if( initSemInUse(sem_init_people2, 0) == -1 ){
		perror("initSemInUse for sem_init_people");
		exit(EXIT_FAILURE);
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
        if(i == 0)      person.type = 'A';
        if(i == 1)      person.type = 'B';

        if(person.type == 'A')
            count_A++;
        else
            count_B++;

        //set parameters for execve
        person_params(person);
        
        switch( child = fork() ){
            
            case -1:{
                errExit("fork");
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
				children[i] = child;
                print_newborn(person);
                update_world_record(person);
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


    //shut system down after sim_time seconds
	alarm(sim_time);


    for(i = 0; i <= sim_time/birth_death; i++){ //100 to avoid infinite loop

        sleep(birth_death);
        
        //printf("---> GESTORE is reading messages\n");
        do{
            flag = 0;
            //read the first message (info of B)
            //and wait for the second message with info of A
            if( msgrcv(msgq_a, &msg, sizeof(msg), ((long)OFFSET+getpid()), (int)IPC_NOWAIT) == -1){
                if( errno != ENOMSG ){
                    perror("Gestore can't receive any message");
                    flag = -2;
                    
                }
                else 
                    flag = -1;
            }
            //flag unchanged so first message received
            if(flag == 0){
                
                //PARAMETERS
                if( sprintf(name_B, "%s", msg.mtxt.name) < 0 )
                    errExit("gestore sprintf name_B");
                genomeB = msg.mtxt.genome;
                pidB = msg.mtxt.pid;
                pidA = msg.mtxt.partner;
                
                //print_rcvd_msg(msg);
                
                //read second message
                if( msgrcv(msgq_a, &msg, sizeof(msg), (long)OFFSET+pidA, 0) == -1 )
                    perror("gestore can't receive any message");
                
                //PARAMETERS
                if( sprintf(name_A, "%s", msg.mtxt.name) < 0 )
                    errExit("gestore sprintf name_B");
                genomeA = msg.mtxt.genome;
                
                //print_rcvd_msg(msg);

                //terminate A and B
                if( kill(pidA, 0) == 0 ){
                    if( kill(pidA, SIGTERM) == -1 ){
                        perror("kill sigterm to pidA");
                    }
                }
                if( kill(pidB, 0) == 0 ){
                    if( kill(pidB, SIGTERM) == -1 ){
                        perror("kill sigterm to pidB");
                    }
                }
                
                //create two children
                make_children(name_A, name_B, pidA, pidB, genomeA, genomeB);
            }    
        }while(flag == 0);
    }


    return EXIT_SUCCESS;
}



/*
 * creates 2 children
 * */
void make_children(char* name_A, char* name_B, pid_t pid_A, pid_t pid_B, unsigned long genome_A, unsigned long genome_B)
{
    struct person first, second;
    unsigned long i, n = mcd(genome_A, genome_B);
    int flag = -1;
    unsigned int character;
    pid_t child;
    
    character = (unsigned int)(65 + rand() % 26);

    first.type = 'A';
    if(strlen(first.name) < 63){
        if( sprintf(first.name, "%s%c", name_A, character) < 0 )
            errExit("gestore sprintf first.name");
    }else{
        if( sprintf(first.name, "%s", name_A) < 0 )
            errExit("gestore sprintf first.name");
    }
    first.genome = random_ulong(n);
    
    second.type = 'B';
    if(strlen(second.name) < 63){
        if( sprintf(second.name, "%s%c", name_B, character) < 0 )
            errExit("gestore sprintf second.name");
    }else{
        if( sprintf(second.name, "%s", name_B) < 0 )
            errExit("gestore sprintf second.name");
    }
    second.genome = random_ulong(n);

    //set parameters of A for execve
    person_params(first);

    //create A
    switch( child = fork() ){
            case -1:{ errExit("fork error"); }

            case 0:{
                    if( execve(args[0], args, envs) == -1 ){
                        perror("gestore execve");
                    }
                    //we're here if execve didnt't work
                    exit(EXIT_FAILURE);
            }
            default:{
                //replace pid in pids_array
				for(i = 0; i < init_people; i++){
                    if(children[i] == pid_A){
                        children[i] = child;
                        flag = 0;
                    }
                }
                //if pid not found
                if(flag == -1){
                    printf("gestore pid_A not found");
                    exit(EXIT_FAILURE);
                }
                print_newborn(first);

                update_world_record(first);

                //allow child to start
                if( releaseSem(sem_init_people2, 0) != 0 )
			        errExit("releaseSem sem_init_people2");
                break;
            }
        }//-switch

    //set parameters of B for execve
    person_params(second);

    //create B
    switch( child = fork() ){
        case -1:{ errExit("fork error"); }

        case 0:{
                if( execve(args[0], args, envs) == -1 ){
                    perror("gestore execve");
                }
                //we're here if execve didnt't work
                exit(EXIT_FAILURE);
        }
        default:{
            //replace pid in pids_array
            flag = -1;
            for(i = 0; i < init_people; i++){
                if(children[i] == pid_B){
                    children[i] = child;
                    flag = 0;
                }
            }
            if(flag == -1){
                printf("gestore pid_B not found");
                exit(EXIT_FAILURE);
            }
            
            print_newborn(second);

            update_world_record(second);

            //allow child to start
            if( releaseSem(sem_init_people2, 0) != 0 )
                errExit("releaseSem sem_init_people2");
            break;
        }
    }//-switch
    printf("---> GESTORE creted 2 new children\n");

    if(first.type == 'A')
        count_A++;
    else
        count_B++;
    
    if(second.type == 'A')
        count_A++;
    else
        count_B++;
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
    if( sprintf(child_name, "%s", person.name) < 0 )
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
    
    
    //update longest name
    if( sprintf(longest_name, "%s", person.name) < 0 )
        errExit("sprintf longest_name");

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
            print_world_records();
			
            exit(EXIT_SUCCESS);
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

    if( fclose(my_file) != 0 )
        perror("gestore fclose");
}


//free allocated memory
void free_all()
{
    free(child_name);
    free(child_genome);
    free(child_sem);
    free(child_sem2);
    free(child_msgq_a);
    free(temp_string);
}


/*
 * terminate every children
 */
void terminate_children()
{
	unsigned int i = 0;

	for(i = 0; i < init_people; i++){
		if( kill(children[i], 0) == 0 ){
			if( kill(children[i], SIGTERM) == -1 ){
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
    printf("Gestore received mtype:%lu pid:%d type:%c name:%s gen:%lu key<3:%d pid<3:%d\n",
        msg.mtype,
        (int)msg.mtxt.pid,
        msg.mtxt.type,
        msg.mtxt.name,
        msg.mtxt.genome,
        msg.mtxt.key_of_love,
        (int)msg.mtxt.partner );
}



void print_world_records()
{
    printf("\n\nGUINNES WORLD RECORDS\n\n");
    printf("Count A:%u, count B:%u, average_genome:%lu\n", count_A, count_B, average_genome/(count_A+count_B));
    printf("The person with the longest name: %c:%s genome:%lu\n", long_name.type, long_name.name, long_name.genome);
    printf("The person with the most big genome: %c:%s genome:%lu\n", big_genome.type, big_genome.name, big_genome.genome);
}


/*
 * print newborn person struct
 */
void print_newborn(struct person person)
{
    printf("+ newborn type:%c name:%s genome:%lu\n",
        person.type,
        person.name,
        person.genome );
}


/*
 * create a new struct person
 */
struct person create_person()
{
    struct person person;

    if(rand()%2 == 0){
        person.type = 'B';
    }else{
        person.type = 'A';
    }

    //initial name has 3 characters
    person.name[0] = 65 + rand()%26;
    person.name[1] = 65 + rand()%26;
    person.name[2] = 65 + rand()%26;
    person.name[3] = '\0';

    person.genome = 2 + ((unsigned long)(rand())%(GENES+2));
    average_genome += person.genome;

    return person;
}




/*
 * update the person with the longest name and the person with the most big genome
 * */
void update_world_record(struct person p)
{
    //update longest name
    if(strlen(p.name) > strlen(long_name.name)){
        long_name.type = p.type;
        if( sprintf(long_name.name, "%s", p.name) < 0 )
            errExit("long_name name sprintf");
        long_name.genome = p.genome;
    }

    //update most big genome
    if(p.genome > big_genome.genome){
        big_genome.genome = p.genome;
        big_genome.type = p.type;
        if( sprintf(big_genome.name, "%s", p.name) < 0 )
            errExit("big_genome name sprintf");
    }
    
}