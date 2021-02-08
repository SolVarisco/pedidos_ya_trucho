/*
 * clock.c
 *
 *  Created on: 22 oct. 2020
 *      Author: utnso
 */

#include "clock.h"

void init_clock(int memory_partitions){
	clock_index = 0;
	POINTER = malloc(sizeof(t_element)*memory_partitions);
}

void next_clock_position(void){
	if(clock_index < MEMORY_PARTITIONS - 1){
		clock_index++;
	} else {
		clock_index = 0;
	}
}

void new_page_in_memory_clock(t_page* new_memory_page, int free_space){
	t_element element;
	element.page = new_memory_page;
	element.uso = 1;

	POINTER[free_space] = element;

}

t_page* choose_victim_clock(void){
	int counter;
	t_page* victim = NULL;

	for(int i = 0; i < 4; i++){
		counter = 0;
		while(counter < MEMORY_PARTITIONS){
			switch(i){
				case 1:
				case 3:
					if(POINTER[clock_index].page->modified == 0 && POINTER[clock_index].uso == 0)
						victim = POINTER[clock_index].page;

					break;

				case 2:
				case 4:
					if(POINTER[clock_index].page->modified == 1 && POINTER[clock_index].uso == 0){
						victim = POINTER[clock_index].page;
					} else {
						POINTER[clock_index].uso = 0;
					}
					break;

			}
			next_clock_position();
			if(victim != NULL)
				return victim;

			counter++;
		}
	}
	return victim;
}

int search_page_clock(t_page* used_page){
	int position = 0;
	while (position < MEMORY_PARTITIONS){
		if(POINTER[position].page == used_page){
			return position;
		}
		position ++;
	}
	return -1;
}

void delete_page_clock(t_page* used_page){
	pthread_mutex_lock(&mutex_pages_in_memory);
	int position = search_page_clock(used_page);
	POINTER[position].page = NULL;
	POINTER[position].uso = 0;
	pthread_mutex_unlock(&mutex_pages_in_memory);
}

void actualizate_page_clock(t_page* used_page){
	pthread_mutex_lock(&mutex_pages_in_memory);
	int position = search_page_clock(used_page);
	POINTER[position].uso = 1;
	pthread_mutex_unlock(&mutex_pages_in_memory);
}
