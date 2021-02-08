/*
 * lru.c
 *
 *  Created on: 22 oct. 2020
 *      Author: utnso
 */

#include "lru.h"

void init_lru(void){
	MEMORY_PAGES = list_create();
}

void new_page_in_memory_lru(t_page* new_memory_page){
	list_add(MEMORY_PAGES, new_memory_page);
}

t_page* choose_victim_lru(void){
	return (t_page*)list_remove(MEMORY_PAGES, 0);
}

void delete_page_lru(t_page* used_page){
	pthread_mutex_lock(&mutex_pages_in_memory);
	t_link_element* element = MEMORY_PAGES->head;
	t_page* page;
	int position = 0;
	while (element != NULL){
		page = (t_page*)element->data;
		if(page == used_page){
			list_remove(MEMORY_PAGES, position);
			break;
		}
		position ++;
		element = element->next;
	}
	pthread_mutex_unlock(&mutex_pages_in_memory);
}

void actualizate_page_lru(t_page* used_page){
	delete_page_lru(used_page);
	new_page_in_memory_lru(used_page);
}

