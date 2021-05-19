/*
COMP30023 Project Two (utility.c)
Created by Yun-Chi Hsiao (student ID: 1074004) 
May 20th, 2021
*/

#include "utility.h"
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/******************************************************************************/
// Phase2
cache* create_cache(uint8_t* raw, pack* packet) {
	
	time_t current_time = time(NULL);

	cache* data = (cache*)calloc(1, sizeof(cache));
	assert(data);
    
	int raw_size = (raw[0] << 8 | raw[1]) + BYTES_OF_SIZE;
	data->raw = (uint8_t*)malloc(raw_size*sizeof(uint8_t));
	assert(data->raw);
	memcpy(data->raw, raw, raw_size);
	assert(data->raw);

	int name_len = strlen(packet->question->q_name) + 1;
	data->name = (char*)calloc(name_len, sizeof(char));
	assert(data->name);
	memcpy(data->name, packet->question->q_name, name_len);
	assert(data->name);

	int ip_len = strlen(packet->answer[0]->ip) + 1;
	data->ip = (char*)calloc(ip_len, sizeof(char));
	assert(data->ip);
	memcpy(data->ip, packet->answer[0]->ip, ip_len);
	assert(data->ip);

    

	data->record_time = current_time;
	data->expire_time = current_time + packet->answer[0]->ttl;

	return data;
}

// Phase1
pack* init_packet(uint8_t* raw) {
    
    pack* packet = (pack*)calloc(1, sizeof(pack));
    assert(packet);

    // Ignore two starting bytes (size of packet)
    int pos = 2;

    store_header(packet, raw, &pos);
     
    store_question(packet, raw, &pos);
    
    store_answer(packet, raw, &pos);

    return packet;    
}

// Phase1
uint8_t* read_massage(int sockfd) {

    int pack_size, tot_n, n;
    uint8_t buffer[2];

    memset(buffer, 0, 2);
    tot_n = 0;
    
    n = read(sockfd, buffer, 2);
    tot_n += n;
    
    // Read size of packet
    while (tot_n < BYTES_OF_SIZE) {
        if (n < 0) {
            perror("read");
		    exit(EXIT_FAILURE);
        }
        n = read(sockfd, buffer+tot_n, BYTES_OF_SIZE-tot_n);
        tot_n += n;
    }
	
    // Allocate memory for the raw
    pack_size = (buffer[0] << 8 | buffer[1]) + BYTES_OF_SIZE;
	
    uint8_t* raw = (uint8_t*)calloc(pack_size, sizeof(uint8_t));
    assert(raw);
	
	raw[0] = buffer[0];
	raw[1] = buffer[1];
	
	n = read(sockfd, raw+tot_n, pack_size-tot_n);
	tot_n += n;

    // Read missing bytes
    while (tot_n < pack_size) {
        n = read(sockfd, raw+tot_n, pack_size-tot_n);
        tot_n += n;
        if (n < 0) {
            perror("read");
		    exit(EXIT_FAILURE);
        }
    }

    return raw;
}

// Phase2
uint8_t* handle_nonAAAA_query(pack* query_packet, uint8_t* query) {

    uint16_t temp = 0;
    uint8_t* response = NULL;
    int response_size;

    response_size = strlen(query_packet->question->q_name) + 18 + BYTES_OF_SIZE;

    response = (uint8_t*)calloc(response_size, sizeof(uint8_t));
    assert(response);

    for (int i = 0; i < response_size; i++) {
        response[i] = query[i];
    }

    temp = response_size - BYTES_OF_SIZE;
    response[0] = temp >> 8;
    response[1] = temp & ~(1 << 8);

    read_two_bytes(response, 4, &temp);
    // Change QR from query to response
    temp |= 1 << 15;
    // Set recursive to none
    temp &= ~(1 << 8);
    temp &= ~(1 << 9);

    // Set RCode to 4;
    temp &= ~(1 << 3);
    temp &= ~(1 << 2);
    temp &= ~(1 << 1);
    temp &= ~(1 << 0);
    temp |= 1 << 2;

    response[4] = temp >> 8;
    response[5] = temp & ~(1 << 8);

    // Set ANCOUNT
    response[8] = 0;
    response[9] = 0;
    // Set NSCOUNT
    response[10] = 0;
    response[11] = 0;
    // Set ARCOUNT
    response[12] = 0;
    response[13] = 0;
    
    return response;
}

// Phase1
int check_packet(pack* packet) {
    
    if (packet->header->qr == QUERY) {
        if (packet->question->q_type == IPV6) {
            return QUERY;
        } else if (packet->question->q_type != IPV6) {
            return UNIMPLEMENTED_QUERY;
        }
    } else if (packet->header->qr == RESPONSE) {
        if (packet->answer[0] != NULL) {
            if (packet->answer[0]->a_type == IPV6) {
                return RESPONSE;
            }
            return INVALID_RESPONSE;
        }
        return NO_ANSWER;
    }

    return NO_ANSWER;
}

// Phase2
void cache_expire_log(FILE* fp, cache* current) {
	
	// Time
    time_t timer = time(NULL);
    struct tm* tm_info = localtime(&timer);
    char buffer_0[26];
    strftime(buffer_0, 26, "%FT%T%z", tm_info);
	struct tm* tm_expire = localtime(&current->expire_time);
	char buffer_1[26];
    strftime(buffer_1, 26, "%FT%T%z", tm_expire);
	
	// File not found 
    if (fp == NULL) {
        printf("NO FILE EXIST");
        exit(1);
    }

	fprintf(fp, "%s %s expires at %s\n", buffer_0, current->name, buffer_1);

	fflush(fp);
}

// Phase2
void cache_replace_log(FILE* fp, char* name_1, char* name_2) {
	
	// Time
    time_t timer = time(NULL);
    struct tm* tm_info = localtime(&timer);
    char buffer[26];
    strftime(buffer, 26, "%FT%T%z", tm_info);

	// File not found 
    if (fp == NULL) {
        printf("NO FILE EXIST");
        exit(1);
    }

	fprintf(fp, "%s replacing %s by %s\n", buffer, name_1, name_2);

	fflush(fp);
}

// Phase2
void free_cache(cache* current) {
	free(current->raw);
	free(current->name);
	free(current->ip);
	free(current);
}

// Phase2
void free_cache_array(cache** caches) {
	for (int i = 0; i < CACHE_ARRAY_SIZE; i++) {
		if (caches[i] != NULL) {
			free_cache(caches[i]);
		}
	}
	free(caches);
}

// Phase1
void free_packet(pack* packet) {
    
    for (int i = 0; i < packet->header->an_count; i++) {
        free(packet->answer[i]->ip);
        free(packet->answer[i]->rd_data);
        free(packet->answer[i]);
    }

    free(packet->answer);
    free(packet->question->q_name);
    free(packet->question);
    free(packet->header);
    free(packet);
}

// Phase2
void init_caches(int caches_size, cache** caches) {
	for (int i=0; i < caches_size; i++) {
		caches[i] = NULL;
	}
}

// Phase1
void read_two_bytes(uint8_t* raw, int pos, uint16_t* des) {
    *des = (raw[pos] << 8) | raw[pos + 1];
}

// Phase1
void standard_log(FILE* fp, pack* packet, int request) {
    
    // Time
    time_t timer = time(NULL);
    struct tm* tm_info = localtime(&timer);
    char buffer[26];
    strftime(buffer, 26, "%FT%T%z", tm_info);

    // File not found 
    if (fp == NULL) {
        printf("NO FILE EXIST");
        exit(1);
    }

    if (request == QUERY) {
        fprintf(fp, "%s requested %s\n", buffer, packet->question->q_name);
    } else if (request == UNIMPLEMENTED_QUERY) {
        fprintf(fp, "%s requested %s\n", buffer, packet->question->q_name);
        fprintf(fp, "%s unimplemented request\n", buffer);
    } else if (request == RESPONSE) {
        if (packet->header->an_count > 0) {
            // Only record first answer
            fprintf(fp, "%s %s is at %s\n", 
                buffer, packet->question->q_name, packet->answer[0]->ip);
        }
    }

    fflush(fp);
}

// Phase1
void store_answer(pack* packet, uint8_t* raw, int* pos) {

    packet->answer = (ans**)calloc(1, sizeof(ans*));
    assert(packet->answer);

    for (int i = 0; i < packet->header->an_count; i++) {
        
        packet->answer[i] = (ans*)calloc(1, sizeof(ans));
        assert(packet->answer[i]);

        // Skip a_name
        *pos += TWO_BYTES;

        read_two_bytes(raw, *pos, &(packet->answer[i]->a_type));
        *pos += TWO_BYTES;

        read_two_bytes(raw, *pos, &(packet->answer[i]->a_class));
        *pos += TWO_BYTES;

        // TTL 32 bytes special case
        uint16_t ttl_1, ttl_2;
        read_two_bytes(raw, *pos, &(ttl_1));
        *pos += TWO_BYTES;
        read_two_bytes(raw, *pos, &(ttl_2));
        *pos += TWO_BYTES;
        packet->answer[i]->ttl = ttl_1 << 16 | ttl_2;

        read_two_bytes(raw, *pos, &(packet->answer[i]->rd_length));
        *pos += TWO_BYTES;
        
        packet->answer[i]->rd_data = 
            (uint8_t*)calloc(packet->answer[i]->rd_length, sizeof(uint8_t));
        assert(packet->answer[i]->rd_data);

        for (int j = 0; j < packet->answer[i]->rd_length; j++) {
            packet->answer[i]->rd_data[j] = raw[*pos];
            (*pos)++;
        }   
    
        // Convertion of ip address
        packet->answer[i]->ip = (char*)calloc(INET6_ADDRSTRLEN, sizeof(char));
        assert(packet->answer[i]->ip);
        inet_ntop(AF_INET6, packet->answer[i]->rd_data, packet->answer[i]->ip, 
                    INET6_ADDRSTRLEN);    
    }   
}

// Phase1
void store_header(pack* packet, uint8_t* raw, int* pos) {
    
    packet->header = (head*)calloc(1, sizeof(head));
    assert(packet->header);

    // Parise packet fields - DNS header
    read_two_bytes(raw, *pos, &(packet->header->id));
    (*pos) += TWO_BYTES;

    uint16_t temp;
    read_two_bytes(raw, *pos, &temp);
    packet->header->qr = temp >> 15 & 1;
    packet->header->op_code = temp >> 11 & 15;    
    packet->header->aa = temp >> 10 & 1;
    packet->header->tc = temp >> 9 & 1;
    packet->header->rd = temp >> 8 & 1;
    packet->header->ra = temp >> 7 & 1;
    packet->header->z = temp >> 4 & 7;
    packet->header->r_code = temp & 15;
    (*pos) += TWO_BYTES;

    read_two_bytes(raw, *pos, &(packet->header->qd_count));
    (*pos) += TWO_BYTES;

    read_two_bytes(raw, *pos, &(packet->header->an_count));
    (*pos) += TWO_BYTES;

    read_two_bytes(raw, *pos, &(packet->header->ns_count));
    (*pos) += TWO_BYTES;

    read_two_bytes(raw, *pos, &(packet->header->ar_count));
    (*pos) += TWO_BYTES;
}

// Phase1
void store_question(pack* packet, uint8_t* raw, int* pos) {
    
    packet->question = (ques*)calloc(1, sizeof(ques));
    assert(packet->question);

    // Recognize domain name
    int str_pos = *pos;
    int str_count = 0;
    int str_length = 0;

    while (raw[str_pos] != 0) {
        str_count++;
        str_pos += raw[str_pos] + ONE_BYTE;
    }
    
    int name_size = str_pos - (*pos);
    
    packet->question->q_name = (char*)calloc(name_size, sizeof(char));
    assert(packet->question->q_name);
    
    // Reset
    str_pos = *pos;
    
    // Enter characters
    str_length = raw[str_pos];
    str_pos++;

    for (int i = 0; i < name_size - ONE_BYTE; i++) {
        if (str_length > 0) {
            packet->question->q_name[i] = raw[str_pos];
            str_length--;
            str_pos++;
        } else {
            packet->question->q_name[i] = DOT;
            str_length = raw[str_pos];
            str_pos++;
        }
    }

    // String end
    packet->question->q_name[name_size-1] = TERMINATOR;
    
    str_pos += ONE_BYTE;

    read_two_bytes(raw, str_pos, &(packet->question->q_type));
    str_pos += TWO_BYTES;

    read_two_bytes(raw, str_pos, &(packet->question->q_class));
    str_pos += TWO_BYTES;
    
    *pos = str_pos;
}











