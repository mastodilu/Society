#ifndef _HEAD_H
#define _HEAD_H

#include <unistd.h>
#include <sys/msg.h>

#define OFFSET 1000000
#define GENES 1000000

#define SLEEP sleep(1);

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

struct person{
    char type;
    char name[64];
    unsigned long genome;
};

struct msg_text {
    pid_t pid;
    char type;
    char name[64];
    unsigned long genome;
    int key_of_love;
    pid_t partner;
};

struct mymsg { 
    long mtype;
    struct msg_text mtxt;
};



/*
 * print person struct
 */
void print_person(struct person person);


/*
 * print error and exit
 */
void errExit(char *);

/*
 * assign a value to init_people between 2 and MAX_PEOPLE (included)
 */
unsigned int generate_first_people(unsigned int, unsigned int);


/*
 * return the mcd between 2 numbers
 * */
unsigned long mcd(unsigned long, unsigned long);


/*
 * return a random unsigned long number between min and max
 * */
unsigned long random_ulong(unsigned long);


/*
 * handle semaphores
 */
int initSemAvailable(int, int);
int initSemInUse(int, int);
int reserveSem(int, int);
int releaseSem(int, int);

#endif