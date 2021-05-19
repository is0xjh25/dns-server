/*
COMP30023 Project Two (phase1.c)
Created by Yun-Chi Hsiao (student ID: 1074004) 
May 20th, 2021
*/

#include "utility.h"
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************/
int main(int argc, char** argv) {

    int status;
    uint8_t* raw;
    pack* packet = NULL;

    // Create file pointer to work with files
    FILE *fp;
    // Open file in writing mode
    fp = fopen("dns_svr.log", "w");

    raw = read_massage(0);
    
    packet = init_packet(raw);
    status = check_packet(packet);
    standard_log(fp, packet, status);

    fclose(fp);
    
    free(raw);
    free_packet(packet);
    

    return 0;
}






