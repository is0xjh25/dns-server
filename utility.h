/*
COMP30023 Project Two (utility.h)
Created by Yun-Chi Hsiao (student ID: 1074004)
May 20th, 2021

This is the header file for utility.c.
It contains the struct of packet (header, question and anwser).
The struct and functions in this library would be applied in phase1 and phase2.
The more customized functions for cache & non-blocking requirement
would be provided in phase2.h.
*/

#ifndef UTILITYH
#define UTILITYH
#define DOT '.'
#define TERMINATOR '\0'
#define BYTES_OF_SIZE 2
#define ONE_BYTE 1
#define TWO_BYTES 2
#define QUERY 0
#define RESPONSE 1
#define INVALID_RESPONSE -1
#define IPV6 28 
#define UNIMPLEMENTED_QUERY -1
#define NO_ANSWER -999
#define CACHE_ARRAY_SIZE 5
#define CACHE_NOT_FOUND -1
#define CACHE_OVERTIME -2
#define TRUE 1
#define FALSE 0

#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef struct packet {
    struct header* header;
    struct question* question;
    struct answer** answer;
} pack;

typedef struct header {
    uint8_t qr;
    uint8_t op_code;
    uint8_t aa;
    uint8_t tc;
    uint8_t rd;
    uint8_t ra;
    uint8_t z;
    uint8_t r_code;
    uint16_t id;
    uint16_t qd_count;
    uint16_t an_count;
    uint16_t ns_count;
    uint16_t ar_count;
} head;

typedef struct question {
    char* q_name;
    uint16_t q_type;
    uint16_t q_class;
} ques;

typedef struct answer {
    char* ip;
    uint8_t* rd_data;
    uint16_t a_type;
    uint16_t a_class;
    uint16_t rd_length;
    uint32_t ttl;
} ans;

typedef struct cache_data {
    uint8_t* raw;
    char* name;
    char* ip;
    time_t record_time;
    time_t expire_time;
} cache;

cache* create_cache(uint8_t* raw, pack* packet);
pack* init_packet(uint8_t* raw);
uint8_t* read_massage(int sockfd);
uint8_t* handle_nonAAAA_query(pack* query_packet, uint8_t* query);
int check_packet(pack* packet);
void cache_expire_log(FILE* fp, cache* current);
void cache_replace_log(FILE* fp, char* name_1, char* name_2);
void free_cache(cache* current);
void free_cache_array(cache** caches);
void free_packet(pack* packet);
void init_caches(int caches_size, cache** caches);
void read_two_bytes(uint8_t* raw, int pos, uint16_t* des);
void standard_log(FILE* fp, pack* packet, int request);
void store_answer(pack* packet, uint8_t* raw, int* pos);
void store_header(pack* packet, uint8_t* raw, int* pos);
void store_question(pack* packet, uint8_t* raw, int* pos);

#endif
