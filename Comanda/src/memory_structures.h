/*
 * memory_structures.h
 *
 *  Created on: 22 oct. 2020
 *      Author: utnso
 */

#ifndef SRC_MEMORY_STRUCTURES_H_
#define SRC_MEMORY_STRUCTURES_H_

#include<commons/string.h>
#include<commons/collections/list.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include<pthread.h>

// Estructuras

typedef struct {
	char* food;
	int memory_frame_number;
	int swap_frame_number;
	bool ready;
	bool modified;
}t_page;

typedef struct {
	char food[24];
	uint32_t total;
	uint32_t ready;
}t_frame;


void* MEMORY;
int MEMORY_PARTITIONS;
int* MEMORY_BITMAP;

int SWAP_PARTITIONS;
int* SWAP_BITMAP;
int SWAP_SIZE;
void* SWAP;

// Mutex

extern pthread_mutex_t mutex_pages_in_memory;

#endif /* SRC_MEMORY_STRUCTURES_H_ */
