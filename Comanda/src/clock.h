/*
 * clock.h
 *
 *  Created on: 22 oct. 2020
 *      Author: utnso
 */

#ifndef SRC_CLOCK_H_
#define SRC_CLOCK_H_

#include "memory_structures.h"

typedef struct {
	t_page* page;
	int uso;
} t_element;

t_element* POINTER;
int clock_index;

void init_clock(int memory_partitions);
void new_page_in_memory_clock(t_page* new_memory_page, int free_space);
t_page* choose_victim_clock(void);
void delete_page_clock(t_page* used_page);
void actualizate_page_clock(t_page* used_page);

#endif /* SRC_CLOCK_H_ */
