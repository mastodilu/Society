#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
    
    for(i = 0; i < init_people; i++){
        
        person = create_person();
        print_person(person);

        //set parameters for execve
        person_params(person);
        
    }

    return EXIT_SUCCESS;
}


/*
 * set parameters for execve
 */
void person_params(struct person person)
{
    int i = 0;
    child_name = calloc(SIZE_NUMBER, sizeof(char));
    child_genome = calloc(SIZE_NUMBER, sizeof(char));

    if(child_name == NULL)
        errExit("child_name NULL");
    if(child_genome == NULL)
        errExit("child_genome NULL");

    //TYPE
    if(person.type == 'A')
        args[i++] = "./A";
    else
        args[i++] = "./B";
    
    //NAME
    if( sprintf(child_name, "%d", person.name) < 0)
        errExit("child_name sprintf");
    args[i++] = child_name;
    
    //GENOME
    if( sprintf(child_genome, "%lu", person.genome) < 0)
        errExit("child_genome sprintf");
    args[i++] = child_genome;

    printf("\n");

    free(child_name);
    free(child_genome);
}