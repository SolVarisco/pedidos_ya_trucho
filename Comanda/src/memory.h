/*
 * memory.h
 *
 *  Created on: 21 sep. 2020
 *      Author: utnso
 */

#ifndef SRC_MEMORY_H_
#define SRC_MEMORY_H_


#include<conexiones.h>
#include<commons/config.h>
#include<sys/mman.h>
#include<fcntl.h>
#include "lru.h"
#include "clock.h"

// Estructuras

typedef struct {
	char* restaurant;
	t_list* food_orders;
	pthread_mutex_t mutex_foods;
}t_segment_table;

typedef struct {
	uint32_t order_id;
	t_estado estado;
	t_list* foods_pages;
	pthread_mutex_t mutex_segment;
}t_segment;

// Variables

typedef enum{
	LRU,
	B_CLOCK
}reeplace_alg;

reeplace_alg ALGORITHM;

// Mutex

extern pthread_mutex_t mutex_mem_bitmap;
extern pthread_mutex_t mutex_swap_bitmap;
extern pthread_mutex_t mutex_segment_table;
t_list* SEGMENT_TABLES;

// Functions

bool create_memory(int memory_size, int swap_size, char* alg, t_log* logger);
void init_bitmaps(void);
void init_pages_mutexs(int memory_pages);

t_segment_table* create_new_restaurant(char* restaurant);
t_segment_table* search_restaurant(char* restaurant, bool create, t_log* logger);
bool create_order(char* restaurant, uint32_t order_id, t_log* logger);
t_segment* search_order(char* restaurant, uint32_t order_id, t_log* logger, int* position, bool mensaje);
t_page* search_food(t_segment* segment, char* food);
bool add_food(char* restaurant, uint32_t order_id, char* food, uint32_t total, t_log* logger);
t_estado_pedido read_order_pages(char* restaurant, uint32_t order_id, t_log* logger);
bool confirm_order(char* restaurant, uint32_t order_id, t_log* logger);
bool food_ready(char* restaurant, uint32_t order_id, char* food, t_log* logger);
bool finish_order(char* restaurant, uint32_t order_id, t_log* logger);

bool free_space(void);
int find_free_memory_page(bool reserve);
int find_free_swap_page(bool reserve);
void save_in_memory(int frame_number, t_frame frame);
void save_in_swap(int frame_number, t_frame frame);
void modify_swap(int memory_frame_number, int swap_frame_number);
void free_memory_page(int memory_frame_number);
void free_swap_page(int swap_frame_number);
void victim_service(t_page* victim);
void generate_a_new_free_frame(t_log* logger, t_page* new_page);
void bring_page_to_memory(t_page* page, t_log* logger);
void add_page_in_memory(t_page* page);
void actualizate_page(t_page* page);


#endif /* SRC_MEMORY_H_ */
