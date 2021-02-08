/*
 * lru.h
 *
 *  Created on: 22 oct. 2020
 *      Author: utnso
 */

#ifndef SRC_LRU_H_
#define SRC_LRU_H_

#include "memory_structures.h"

t_list*  MEMORY_PAGES;

// Functions

void init_lru(void);
void new_page_in_memory_lru(t_page* new_memory_page);
t_page* choose_victim_lru(void);
void delete_page_lru(t_page* used_page);
void actualizate_page_lru(t_page* used_page);

#endif /* SRC_LRU_H_ */
