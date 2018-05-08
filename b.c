#include <stdlib.h>
#include <stdio.h>
#include "header.h"

int main(int argc, char* argv[])
{
    if(argc < 6)
        errExit("B too few arguments");

    printf("I'm B\n");
    
    return EXIT_SUCCESS;
}