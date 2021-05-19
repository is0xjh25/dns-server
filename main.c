/*
COMP30023 Project Two (main.c)
Created by Yun-Chi Hsiao (student ID: 1074004) 
May 20th, 2021
*/

#include "utility.h"
#include "phase2.h"
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************/
int main(int argc, char** argv) {

    // Create file pointer to work with files
    FILE *fp;
    // Open file in writing mode
    fp = fopen("dns_svr.log", "w");
    
    init_server(argc, argv, fp);

    fclose(fp);

    return 0;
}
