#include <stdlib.h>
#include <stdio.h>
#include "header.h"

int main(int argc, char* argv[])
{
    if(argc < 6)
        errExit("A too few arguments");

    printf("I'm A\n");
    
    return EXIT_SUCCESS;
}