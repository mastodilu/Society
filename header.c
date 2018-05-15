#include <sys/sem.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

#include "header.h"

/*
 * assign a value to init_people between 2 and MAX_PEOPLE
 */
unsigned int generate_first_people(unsigned int min, unsigned int max)
{
	if(max <= min)
		return min;
	return min + ((unsigned int)(rand()) % (max-min+1));
}


/*
 * return a random unsigned long number between min and max
 * */
unsigned long random_ulong(unsigned long min)
{
    if(min == GENES)
        return min;
    return min + (unsigned long)rand() % ((unsigned long)GENES - min + 1);
}




/*
 * initialize a new struct person
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

    return person;
}


/*
 * return the mcd between 2 numbers
 * */
unsigned long mcd(unsigned long a, unsigned long b) {
    if (b == 0)
        return a;  
    else
        return mcd(b, a % b);
}



/*
 * print error and exit
 */
void errExit(char * s)
{
    perror(s);
    exit(EXIT_FAILURE);
}


/*
 * print person struct
 */
void print_person(struct person person)
{
    printf("type:%c name:%s genome:%lu\n",
        person.type,
        person.name,
        person.genome );
}


/*
 * handle semaphores
 */

int initSemAvailable(int semId, int semNum)
{
    union semun arg;
    arg.val = 1;
    return semctl(semId, semNum, SETVAL, arg);
}

int initSemInUse(int semId, int semNum)
{
    union semun arg;
    arg.val = 0;
    return semctl(semId, semNum, SETVAL, arg);
}

int reserveSem(int semId, int semNum) {
    struct sembuf sops;
    sops.sem_num = (short unsigned int)semNum;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}

int releaseSem(int semId, int semNum) {
    struct sembuf sops;
    sops.sem_num = (short unsigned int)semNum;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}