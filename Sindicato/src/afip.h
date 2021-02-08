/*
 * afip.h
 *
 *  Created on: 26 nov. 2020
 *      Author: utnso
 */

#ifndef AFIP_H_
#define AFIP_H_

#include<commons/string.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include<pthread.h>
#include<commons/bitarray.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<dirent.h>
#include<errno.h>
#include<math.h>
#include "logger.h"

char* punto_montaje;
char* bmap;
struct stat size_bitmap;
t_bitarray* bitarray;
int block_size, blocks, blocks_available;

extern pthread_mutex_t mutexBitmap;
extern pthread_mutex_t mutex_file_system;


bool init_afip(void);
bool punto_montaje_exist(void);
bool dir_exist(char* path);
bool read_metadata(void);
bool open_bitmap(void);


void create_fs(char* bitmap_path);
void create_bitmap(char* path_metadata);
void create_metadata_in_files_directory(char* dir_files);
void create_blocks(char* dir_blocks, int blocks_quantity, int block_size);
bool get_available_block(uint32_t* block);
bool write_block(uint32_t free_block, char* total_string, int total_size, char* file_path);

bool search_restaurant(char* restaurant, char** path);
bool search_restaurant_info(char* restaurant, int* first_block, int* file_size);
t_list* get_food(char* restaurant);
t_list* read_foods(uint32_t block, int file_size);
bool read_blocks(char** final_info, int line, char* limit_char, uint32_t block, int file_size);
void check_next_block_limit(char** info, char* limit_char, uint32_t next_block, int file_size);
bool save_order(char* restaurant, uint32_t id);
bool add_food(char* restaurant, uint32_t id, char* food, int32_t total);
int search_food_price(char* restaurant, char* food);
bool update_information(char* info, int block, int old_size, int new_size, int offset, char* file_path);
t_estado_pedido get_order_information(char* restaurant, uint32_t id);
bool change_state(char* restaurant, uint32_t id, char* new_state);
bool food_ready(char* restaurant, uint32_t id, char* food);
t_info_restaurante get_restaurante_information(char* restaurant);
uint32_t number_of_orders(char* restaurant);
t_list* get_receta_information(char* nombre_receta);
bool search_receta(char* receta, char** path);

#endif /* AFIP_H_ */
