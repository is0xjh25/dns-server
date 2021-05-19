/*
COMP30023 Project Two (phase2.h)
Created by Yun-Chi Hsiao (student ID: 1074004)
May 20th, 2021

This is the header file for phase2.c.
It is implemented for standard, cahce and non-blocking task in phase2 section.
The functions which are relevent to TCP and multithreading are referenced to 
WS03-"thread1.c" & "thread2.c" and WS09-"server.c" & "client.c".
*/

#ifndef PHASE2H
#define PHASE2H
#define SERVER_PORT "8053"
#define CACHE 1
#define NON_BLOCKING 1

#include "utility.h"
#include <stdio.h>

typedef struct argument_data {
   FILE* fp; 
   cache** caches;
   uint8_t* query;
   int newsockfd;
   int argc;
   char** argv;   
} args;

args* init_args(FILE* fp, cache** caches, int argc, char** argv);
uint8_t* retrieve_cache(uint8_t* query, cache* current);
uint8_t* visit_server(int argc, char** argv, uint8_t* query);
int add_cache(uint8_t* raw, pack* packet, cache** caches);
int check_cache(pack* packet, cache** caches);
void* handle_query(void* parm);
void init_server(int argc, char** argv, FILE* fp);
void modify_TTL(uint8_t* response, cache* current);
void replace_cache(int pos, uint8_t* raw, pack* packet, cache** caches);

#endif
