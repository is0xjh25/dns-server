/*
COMP30023 Project Two (phase2.c)
Created by Yun-Chi Hsiao (student ID: 1074004) 
May 20th, 2021
*/

#include "phase2.h"
#include "utility.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

pthread_mutex_t lock;

/******************************************************************************/
args* init_args(FILE* fp, cache** caches, int argc, char** argv) {
	
	args* arguments = (args*)calloc(1, sizeof(args));
	assert(arguments);

	arguments->fp = fp;
	arguments->caches = caches;
	arguments->argc = argc;
   	arguments->argv = argv;
	
	// default
	arguments->newsockfd = -1;
	arguments->query = NULL;

	return arguments;
}


uint8_t* retrieve_cache(uint8_t* query, cache* current) {

	uint8_t* response = NULL;

	// Copy response from cache			
	int size = (current->raw[0] << 8 | current->raw[1]) + BYTES_OF_SIZE;
	response = (uint8_t*)calloc(size, sizeof(uint8_t));
	assert(response);
	response = memcpy(response, current->raw, size);
	assert(response);

	// Set ID
	response[2] = query[2];
	response[3] = query[3];

	// Change TTL
	modify_TTL(response, current);

	return response;
}


uint8_t* visit_server(int argc, char** argv, uint8_t* query) {

    int sockfd, n;
	struct addrinfo hints, *servinfo, *rp;
	uint8_t* response;
	
	if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Create address
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	// Get addrinfo of server. From man page:
	// The getaddrinfo() function combines the functionality provided by the
	// gethostbyname(3) and getservbyname(3) functions into a single interface
	if ((getaddrinfo(argv[1], argv[2], &hints, &servinfo)) < 0) {
		perror("getaddrinfo");
		exit(EXIT_FAILURE);
	}

	// Connect to first valid result
	// Why are there multiple results? see man page (search 'several reasons')
	// How to search? enter /, then text to search for, press n/N to navigate
	for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1) {
			continue;
		}
		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break; // success
		}	
		close(sockfd);
	}

	if (rp == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		exit(EXIT_FAILURE);
	}
	
	// Send message to server
	n = write(sockfd, query, (query[0] << 8 | query[1])+BYTES_OF_SIZE);
	if (n < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Read message from server
    response = read_massage(sockfd);

	close(sockfd);
	freeaddrinfo(servinfo);
	
	return response;
}


int add_cache(uint8_t* raw, pack* packet, cache** caches) {
	
	time_t current_time = time(NULL);
	
	for (int i = 0; i < CACHE_ARRAY_SIZE; i++) {
		if (caches[i] == NULL) {		
			caches[i] = create_cache(raw, packet);
			return -1;
		}
	}
	
	for (int i = 0; i < CACHE_ARRAY_SIZE; i++) {	
		// Replace overtime cache
		if (current_time < caches[i]->expire_time) {
			free_cache(caches[i]);
			caches[i] = create_cache(raw, packet);
			return i;
		}
	}	

	// If there is no overtime cache, replace the first one
	free_cache(caches[0]);
	caches[0] = create_cache(raw, packet);

	return 0;
}


int check_cache(pack* packet, cache** caches) {

	time_t current_time = time(NULL);
	
	for (int i = 0; i < CACHE_ARRAY_SIZE; i++) {
		if (caches[i] != NULL) {
			if (strcmp(packet->question->q_name, caches[i]->name) == 0) {
				if (current_time < caches[i]->expire_time) {
					return i;
				} else {
					return CACHE_OVERTIME;
				}
			}
		}
	}
	
	return CACHE_NOT_FOUND;
}


void* handle_query(void* parm) {
	
	args* arguments = (args*)parm;
	FILE* fp = arguments->fp;
	cache** caches = arguments->caches;
	uint8_t* query = arguments->query;
	int newsockfd = arguments->newsockfd;
	int argc = arguments->argc;
   	char** argv = arguments->argv;  

	int i, n;
	uint8_t* response = NULL;
    pack* query_packet = NULL;
    pack* response_packet = NULL;

	int query_status = 0;
	int response_status = 0;
	int cache_status = 0;

	// Form a query packet
	query_packet = init_packet(query);
	// Check query
	query_status = check_packet(query_packet);

	pthread_mutex_lock(&lock);					
	standard_log(fp, query_packet, query_status);
	pthread_mutex_unlock(&lock);

						
	if (query_status == QUERY) {
		
		// Find in cache
		pthread_mutex_lock(&lock);
		cache_status = check_cache(query_packet, caches);
		
		// Cache found
		if (cache_status >= 0) {
			
			response = retrieve_cache(query, caches[cache_status]);
			cache_expire_log(fp, caches[cache_status]);
			pthread_mutex_unlock(&lock);

			response_packet = init_packet(response);
			
			// Check the response is existing and is AAAA
			response_status = check_packet(response_packet);
			if (response_status == RESPONSE) {
				pthread_mutex_lock(&lock); 
				standard_log(fp, response_packet, response_status);
				pthread_mutex_unlock(&lock);
			}

		// Cache not found
		} else if (cache_status == CACHE_NOT_FOUND) {

			pthread_mutex_unlock(&lock);

			response = visit_server(argc, argv, query);
			response_packet = init_packet(response);

			// Check the response is AAAA 
			response_status = check_packet(response_packet);
			if (response_status != NO_ANSWER) {
		
				// Insert cache
				pthread_mutex_lock(&lock);
				
				i = add_cache(response, response_packet, caches);

				if (i != -1) {
					cache_replace_log(fp, query_packet->question->q_name, 
					caches[i]->name);
				}

				if (response_status == RESPONSE) {
					standard_log(fp, response_packet, response_status);
				} else if (response_status == INVALID_RESPONSE) {
					//pass;
				}

				pthread_mutex_unlock(&lock);
			}
			
		// Cache found but overtime
		} else if (cache_status == CACHE_OVERTIME) {
			
			// Find overtime cache
			for (i = 0; i < CACHE_ARRAY_SIZE; i++) {
				if (caches[i] != NULL) {
					if (strcmp(query_packet->question->q_name, 
						caches[i]->name) == 0) {
						break;
					}
				}
			}
			pthread_mutex_unlock(&lock);

			response = visit_server(argc, argv, query);
			response_packet = init_packet(response);

			// Check the response is AAAA 
			response_status = check_packet(response_packet);
			if (response_status != NO_ANSWER) {
				
				// Replace cache
				pthread_mutex_lock(&lock);	
				replace_cache(i, response, response_packet, caches);
				
				if (response_status == RESPONSE) {
					cache_replace_log(fp, response_packet->question->q_name, 
									caches[i]->name);
					standard_log(fp, response_packet, response_status);
				} else if (response_status == INVALID_RESPONSE) {
					//pass;
				}

				pthread_mutex_unlock(&lock);
			}
		}

	} else if (query_status == UNIMPLEMENTED_QUERY) {
		response = handle_nonAAAA_query(query_packet, query);
	}

	// Write back to client
	n = write(newsockfd, response, 
		(response[0] << 8 | response[1])+BYTES_OF_SIZE);
		
	if (n < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}
		
	// Free memory
	if (response_packet != NULL) {
		free_packet(response_packet);
	}
	free_packet(query_packet);
	free(query);
	free(response);
	free(arguments);

	return 0;
}


void init_server(int argc, char* argv[], FILE* fp) {
    
    int sockfd, newsockfd, re;
	cache* caches[CACHE_ARRAY_SIZE];
	socklen_t client_addr_size;
	struct addrinfo hints, *res;
	struct sockaddr_storage client_addr;
	pthread_t tid;
	
	// Set a lock
	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("mutex init failed\n");
		exit(1);
	}
	
	// Ensure caches are starting with null
	init_caches(CACHE_ARRAY_SIZE, caches);

	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(EXIT_FAILURE);
	}

	// Create address we're going to listen on (with given port number)
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;       // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept
	// node (NULL means any interface), service (port), hints, res
	getaddrinfo(NULL, SERVER_PORT, &hints, &res);

	// Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Reuse port if possible
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	// Bind address to the socket
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	// Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued, man 3 listen
	if (listen(sockfd, 5) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	// Open for clients
	while (1) {
		// Accept a connection - 
		// blocks until a connection is ready to be accepted
		// Get back a new file descriptor to communicate on
		client_addr_size = sizeof client_addr;
		newsockfd =
			accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
		if (newsockfd < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		// Create an initial arguments for thread,
		// query and newsockfd are still missing
		args* arguments = init_args(fp, caches, argc, argv);

		// Read characters from the connection, then process
		uint8_t* query = NULL; 
		query = read_massage(newsockfd);
		
		// Complete arguments
		arguments->query = query;
		arguments->newsockfd = newsockfd;
		
		// Create the thread
		pthread_create(&tid, NULL, handle_query, arguments);
	}

    // Free memory
	free_cache_array(caches);
	freeaddrinfo(res);

	close(sockfd);
	close(newsockfd);
}


void modify_TTL(uint8_t* response, cache* current) {
	
	time_t current_time = time(NULL);
	int ttl = 0;
	int ttl_pos = 0;
	
	// Find ttl staring position in the raw
	// size + header + query(name + type + class) + answer(name + type +class)
	ttl_pos += BYTES_OF_SIZE + 12 + (strlen(current->name)+ 2 + 4) + 6;
	
	ttl = response[ttl_pos] << 24 | response[ttl_pos+1] << 16 |
		response[ttl_pos+2] << 8 | response[ttl_pos+3];

	ttl -= (int)difftime(current_time, current->record_time);
		
	response[ttl_pos] = ttl >> 24 & 0xFF; 
	response[ttl_pos+1] = ttl >> 16 & 0xFF;
	response[ttl_pos+2] = ttl >> 8 & 0xFF;
	response[ttl_pos+3] = ttl & 0xFF;
}


void replace_cache(int pos, uint8_t* raw, pack* packet, cache** caches) {
	free_cache(caches[pos]);
	caches[pos] = create_cache(raw, packet);
}
