/*
 * memory.c
 *
 *  Created on: 21 sep. 2020
 *      Author: utnso
 */

#include "memory.h"

bool create_memory(int memory_size, int swap_size, char* algorithm, t_log* logger){
	MEMORY = malloc(memory_size);
	MEMORY_PARTITIONS = memory_size/32;

	int fd;
	size_t filesize;

	fd = open("swap.bin", O_RDWR | O_CREAT | O_EXCL, S_IRWXU);

	if(fd == -1){
		fd = open("swap.bin", O_RDWR);
	}

	filesize = swap_size;

	ftruncate(fd, filesize);

	SWAP = mmap(NULL, swap_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);

	if(SWAP == MAP_FAILED){
		log_error(logger, "Error en el mapeo de la memoria swap");
		return false;
	}

	close(fd);

	SWAP_SIZE = swap_size;
	SWAP_PARTITIONS = SWAP_SIZE/32;
	init_bitmaps();

	if(!strcmp(algorithm, "LRU")){
		ALGORITHM = LRU;
		init_lru();
	} else if (!strcmp(algorithm, "CLOCK_M")){
		ALGORITHM = B_CLOCK;
		init_clock(MEMORY_PARTITIONS);
	} else {
		printf("Algoritmo de reemplazo no valido");
		return false;
	}

	SEGMENT_TABLES = list_create();

	return true;
}

void init_bitmaps(void){
	int counter;

	MEMORY_BITMAP = malloc(MEMORY_PARTITIONS*sizeof(int));
	for(counter = 0; counter<MEMORY_PARTITIONS; counter++)
			MEMORY_BITMAP[counter] = 0;

	SWAP_BITMAP = malloc(SWAP_PARTITIONS*sizeof(int));
	for(counter = 0; counter<SWAP_PARTITIONS; counter++)
		SWAP_BITMAP[counter] = 0;
}

t_segment_table* create_new_restaurant(char* restaurant){
	t_segment_table* new_segment = malloc(sizeof(*new_segment));

	new_segment->restaurant = malloc(strlen(restaurant)+1);
	strcpy(new_segment->restaurant, restaurant);
	new_segment->food_orders = list_create();
	pthread_mutex_init(&new_segment->mutex_foods, NULL);

	pthread_mutex_lock(&mutex_segment_table);
	list_add(SEGMENT_TABLES, new_segment);
	pthread_mutex_unlock(&mutex_segment_table);

	return new_segment;
}

t_segment_table* search_restaurant(char* restaurant, bool create, t_log* logger){

	pthread_mutex_lock(&mutex_segment_table);
	t_link_element* element = SEGMENT_TABLES->head;
	t_segment_table* segment;

	while(element != NULL){
		segment = (t_segment_table*)element->data;
		if(strcmp(segment->restaurant, restaurant)==0){
			pthread_mutex_unlock(&mutex_segment_table);
			return segment;
		}
		element = element->next;
	}
	pthread_mutex_unlock(&mutex_segment_table);

	if(create){
		segment = create_new_restaurant(restaurant);
		log_info(logger, "Se creó una nueva tabla de segmentos para el restaurante %s", restaurant);
		return segment;
	} else {
		segment = NULL;
		log_warning(logger, "El restaurante %s no se encuentra registrado en el sistema\n", restaurant);
	}
	return NULL;
}

bool create_order(char* restaurant, uint32_t order_id, t_log* logger){
	if(!free_space()){
		log_warning(logger, "No hay más espacio disponible en momoria");
		return 0;
	}
	t_segment_table* restaurant_segment = search_restaurant(restaurant, 1, logger);
	if(restaurant_segment == NULL){
		return 0;
	}
	int position;
	if(search_order(restaurant, order_id, logger, &position, 0)!=NULL){
		log_warning(logger, "El pedido %d del restaurante %s ya se encuentra registrado en el sistema\n", order_id, restaurant);
		return 0;
	}
	t_segment* order = malloc(sizeof(*order));

	order->order_id = order_id;
	order->foods_pages = list_create();
	order->estado = PENDIENTE;
	pthread_mutex_init(&(order->mutex_segment), NULL);

	pthread_mutex_lock(&(restaurant_segment->mutex_foods));
	list_add(restaurant_segment->food_orders, (void*)order);
	pthread_mutex_unlock(&(restaurant_segment->mutex_foods));

	log_info(logger, "Se creó una nueva tabla de paginas para el pedido %d del restaurante %s", order_id, restaurant);
	return 1;
}

t_segment* search_order(char* restaurant, uint32_t order_id, t_log* logger, int* position, bool exist){
	t_segment_table* restaurant_segment = search_restaurant(restaurant, 0, logger);
	t_segment* segment;

	if(restaurant_segment != NULL){

		pthread_mutex_lock(&(restaurant_segment->mutex_foods));
		t_link_element* element = restaurant_segment->food_orders->head;
		*position = 0;
		while(element != NULL){
			segment = (t_segment*)element->data;
			if(segment->order_id == order_id){
				pthread_mutex_unlock(&(restaurant_segment->mutex_foods));
				return segment;
			}
		*position+=1;
		element = element->next;
		}
		pthread_mutex_unlock(&(restaurant_segment->mutex_foods));

		if(exist)
			log_warning(logger, "El pedido %d del restaurante %s no se encuentra registrado en el sistema\n", order_id, restaurant);

	}
	return NULL;
}

t_page* search_food(t_segment* segment, char* food){
	t_link_element* element = segment->foods_pages->head;
	t_page* page;

	while(element != NULL){
		page = (t_page*)element->data;
		if(strcmp(page->food, food) == 0){
			return page;
		}
		element = element->next;
	}
	return NULL;
}

bool add_food(char* restaurant, uint32_t order_id, char* food, uint32_t total, t_log* logger){
	if(!free_space()){
		log_warning(logger, "No hay más espacio disponible en memoria");
		return 0;
	}
	int position;
	t_segment* segment = search_order(restaurant, order_id, logger, &position, 1);
	if(segment == NULL){
		return 0;
	}

	pthread_mutex_lock(&(segment->mutex_segment));
	t_page* page = search_food(segment, food);
	pthread_mutex_unlock(&(segment->mutex_segment));

	if(page == NULL){
		page = malloc(sizeof(*page));
		page->swap_frame_number = find_free_swap_page(true);

		page->memory_frame_number = find_free_memory_page(true);
		if(page->memory_frame_number == -1){
			generate_a_new_free_frame(logger, page);
		} else {
			add_page_in_memory(page);
		}

		page->ready = false;
		page->modified = false;
		page->food = malloc(strlen(food)+1);
		strcpy(page->food, food);


		pthread_mutex_lock(&(segment->mutex_segment));
		list_add(segment->foods_pages, (void*)page);
		pthread_mutex_unlock(&(segment->mutex_segment));

		t_frame frame;
		frame.total = total;
		frame.ready = 0;
		strcpy(frame.food, food);

		save_in_memory(page->memory_frame_number, frame);
		save_in_swap(page->swap_frame_number, frame);

		log_info(logger, "Se almaceno la información del plato %s del pedido %d del restaurante %s en memoria", food, order_id, restaurant);
		log_info(logger, "Frame utilizado en memoria: %d \tBase: %d (real: %d)",page->memory_frame_number, page->memory_frame_number * 32, MEMORY + page->memory_frame_number * 32);
		log_info(logger, "Base de la pagina utilizada en swap: %d", page->swap_frame_number * 32);


	} else {
		pthread_mutex_lock(&(segment->mutex_segment));
		uint32_t total_actual;
		if(page->memory_frame_number == -1){
			bring_page_to_memory(page, logger);
		} else {
			actualizate_page(page);
		}
		memcpy(&total_actual, MEMORY + page->memory_frame_number * 32 + 24, sizeof(uint32_t));
		total_actual += total;
		memcpy(MEMORY + page->memory_frame_number * 32 + 24, &total_actual, sizeof(uint32_t));
		page->modified = true;

		pthread_mutex_unlock(&(segment->mutex_segment));
		log_info(logger, "Se actualizo la información del plato %s del pedido %d del restaurante %s en memoria", food, order_id, restaurant);
	}

	return true;
}

t_estado_pedido read_order_pages(char* restaurant, uint32_t order_id, t_log* logger){
	t_estado_pedido order;
	int position;
	t_segment* segment = search_order(restaurant, order_id, logger, &position, 1);
	order.platos = list_create();

	if(segment == NULL){
		order.estado = INEXISTENTE;
		list_clean(order.platos);
	} else {
		t_page* page;
		char* food_name = malloc(24);

		pthread_mutex_lock(&(segment->mutex_segment));
		order.estado = segment->estado;
		int size = list_size(segment->foods_pages);
		int food_lenght;

		for(int i = 0; i < size; i++){
			page = (t_page*)list_get(segment->foods_pages, i);

			if(page->memory_frame_number == -1){
				bring_page_to_memory(page, logger);
			} else {
				actualizate_page(page);
			}
			t_estado_comida* food = malloc(sizeof(*food));
			food->pasos_receta = list_create();

			memcpy(food_name, MEMORY + page->memory_frame_number * 32, 24);
			food_lenght = strlen(food_name)+1;
			food->comida = malloc(food_lenght);
			strncpy(food->comida, food_name, food_lenght);
			memcpy(&(food->total), MEMORY + page->memory_frame_number * 32 + 24, sizeof(uint32_t));
			memcpy(&(food->listo), MEMORY + page->memory_frame_number * 32 + 24 + sizeof(uint32_t), sizeof(uint32_t));

			list_add(order.platos, (void*)food);
		}
		pthread_mutex_unlock(&(segment->mutex_segment));

		free(food_name);
	}

	return order;
}

bool confirm_order(char* restaurant, uint32_t order_id, t_log* logger){
	int position;
	t_segment* segment = search_order(restaurant, order_id, logger, &position, 1);
	if(segment == NULL){
		return 0;
	}

	pthread_mutex_lock(&(segment->mutex_segment));
	if(segment->estado != PENDIENTE){
		pthread_mutex_unlock(&(segment->mutex_segment));
		log_warning(logger, "El pedido no se encuentra en estado PENDIENTE, por lo cual no puede ser confirmado");
		return 0;
	}
	segment->estado = CONFIRMADO;
	pthread_mutex_unlock(&(segment->mutex_segment));

	return 1;
}

bool food_ready(char* restaurant, uint32_t order_id, char* food, t_log* logger){
	int position;
	t_segment* segment = search_order(restaurant, order_id, logger, &position, 1);
	if(segment == NULL){
		return 0;
	}
	t_page* page = search_food(segment, food);
	if(page == NULL){
		return 0;
	}

	pthread_mutex_lock(&(segment->mutex_segment));
	if(segment->estado != CONFIRMADO){
		pthread_mutex_unlock(&(segment->mutex_segment));
		log_warning(logger, "El pedido no se encuentra confirmado");
		return 0;
	}
	if(page->ready){
		pthread_mutex_unlock(&(segment->mutex_segment));
		log_warning(logger, "Ya se terminaron de cocinar todos los platos pedidos de %s", food);
		return 0;
	}
	if(page->memory_frame_number == -1){
		bring_page_to_memory(page, logger);
	} else {
		actualizate_page(page);
	}
	t_frame frame;

	memcpy(&(frame.total), MEMORY + page->memory_frame_number * 32 + 24, sizeof(uint32_t));
	memcpy(&(frame.ready), MEMORY + page->memory_frame_number * 32 + 24 + sizeof(uint32_t), sizeof(uint32_t));
	frame.ready += 1;
	memcpy(MEMORY + page->memory_frame_number * 32 + 24 + sizeof(uint32_t), &(frame.ready), sizeof(uint32_t));
	page->modified = true;
	log_info(logger, "Se actualizo la información del plato %s del pedido %d del restaurante %s en memoria", food, order_id, restaurant);

	if(frame.ready == frame.total){
		int size = list_size(segment->foods_pages);
		bool ready = true;

		page->ready = true;
		log_info(logger, "Todos los/as %ss estan listos", food);
		for(int i = 0; i < size; i++){
			page = (t_page*)list_get(segment->foods_pages, i);

			if(!page->ready){
				ready = false;
				break;
			}
		}
		if(ready){
			segment->estado = TERMINADO;
			log_info(logger, "El pedido %i del restaurante %s fue terminado", order_id, restaurant);
		}
	}
	pthread_mutex_unlock(&(segment->mutex_segment));

	return 1;
}

bool finish_order(char* restaurant, uint32_t order_id, t_log* logger){
	int position;
	t_segment* segment = search_order(restaurant, order_id, logger, &position, 1);
	if(segment == NULL){
		return 0;
	}

	pthread_mutex_lock(&(segment->mutex_segment));
	int size = list_size(segment->foods_pages);
	t_page* page;

	for(int i = 0; i < size; i++){
		page = (t_page*)list_remove(segment->foods_pages, 0);

		free_memory_page(page->memory_frame_number);
		free_swap_page(page->swap_frame_number);
		log_info(logger, "Informacion del plato %s eliminada de memoria", page->food);

		if(page->memory_frame_number != -1){
			log_info(logger, "Frame eliminado: %d\tBase: %d (real: %d)",page->memory_frame_number, page->memory_frame_number * 32, MEMORY + page->memory_frame_number * 32);
			switch (ALGORITHM) {
				case LRU: delete_page_lru(page); break;
				case B_CLOCK: delete_page_clock(page);break;
			}
			MEMORY_BITMAP[page->memory_frame_number] = 0;
		}


		log_info(logger, "Base de la particion eliminada en swap: %d", page->swap_frame_number * 32);

		SWAP_BITMAP[page->swap_frame_number] = 0;

		//memset(page, 0, strlen(page->food)+1+ sizeof(int)*2+sizeof(bool)*2);
		free(page->food);
		free(page);
		page = NULL;
	}
	segment->foods_pages = NULL;
	pthread_mutex_unlock(&(segment->mutex_segment));
	pthread_mutex_destroy(&(segment->mutex_segment));

	t_segment_table* restaurant_table = search_restaurant(restaurant, 0, logger);

	pthread_mutex_lock(&(restaurant_table->mutex_foods));
	free(list_remove(restaurant_table->food_orders, position));
	pthread_mutex_unlock(&(restaurant_table->mutex_foods));

	memset(segment, 0, sizeof(uint32_t) + sizeof(t_estado));
	segment = NULL;
	log_info(logger, "Segmento del pedido %d del restaurante %s eliminado", order_id, restaurant);
	return 1;
}

bool free_space(void){
	if(find_free_swap_page(0)>-1){
		return 1;
	}
	return 0;
}

int find_free_memory_page(bool reserve){
	pthread_mutex_lock(&mutex_mem_bitmap);
	for(int i = 0; i < MEMORY_PARTITIONS; i++){
		if(MEMORY_BITMAP[i]==0){
			if(reserve)
				MEMORY_BITMAP[i]=1;
			pthread_mutex_unlock(&mutex_mem_bitmap);
			return i;
		}
	}
	pthread_mutex_unlock(&mutex_mem_bitmap);
	return -1;
}

int find_free_swap_page(bool reserve){
	pthread_mutex_lock(&mutex_swap_bitmap);
	for(int i = 0; i < SWAP_PARTITIONS; i++){
		if(SWAP_BITMAP[i]==0){
			if(reserve)
				SWAP_BITMAP[i]=1;
			pthread_mutex_unlock(&mutex_swap_bitmap);
			return i;
		}
	}
	pthread_mutex_unlock(&mutex_swap_bitmap);
	return -1;
}

void save_in_memory(int frame_number, t_frame frame){
	memcpy(MEMORY + frame_number * 32, frame.food, 24);
	memcpy(MEMORY + frame_number * 32 + 24, &(frame.total), sizeof(uint32_t));
	memcpy(MEMORY + frame_number * 32 + 24 + sizeof(uint32_t), &(frame.ready), sizeof(uint32_t));
}

void save_in_swap(int frame_number, t_frame frame){
	memcpy(SWAP + frame_number * 32, frame.food, 24);
	memcpy(SWAP + frame_number * 32 + 24, &(frame.total), sizeof(uint32_t));
	memcpy(SWAP + frame_number * 32 + 24 + sizeof(uint32_t), &(frame.ready), sizeof(uint32_t));
	msync(SWAP+ frame_number * 32, 32, MS_SYNC |  MS_INVALIDATE);
}

void modify_swap(int memory_frame_number, int swap_frame_number){
	t_frame frame;

	memcpy(frame.food, MEMORY + memory_frame_number * 32, 24);
	memcpy(&(frame.total), MEMORY + memory_frame_number * 32 + 24, sizeof(uint32_t));
	memcpy(&(frame.ready), MEMORY + memory_frame_number * 32 + 24 + sizeof(uint32_t), sizeof(uint32_t));
	save_in_swap(swap_frame_number, frame);
}

void free_memory_page(int memory_frame_number){
	memset(MEMORY + memory_frame_number * 32, 0, 32);
}

void free_swap_page(int swap_frame_number){
	memset(SWAP + swap_frame_number * 32, 0, 32);
	msync(SWAP+ swap_frame_number * 32, 32, MS_SYNC |  MS_INVALIDATE);
}

void victim_service(t_page* victim){
	if(victim->modified){
		free_swap_page(victim->swap_frame_number);
		modify_swap(victim->memory_frame_number, victim->swap_frame_number);
	}
	free_memory_page(victim->memory_frame_number);
	victim->memory_frame_number = -1;
}

void generate_a_new_free_frame(t_log* logger, t_page* new_page){
	log_info(logger, "No hay más espacio en memoria real, comienza el proceso de swap");
	t_page* victim;

	switch(ALGORITHM){
		case LRU:
			pthread_mutex_lock(&mutex_pages_in_memory);
			victim = choose_victim_lru();
			new_page_in_memory_lru(new_page);
			pthread_mutex_unlock(&mutex_pages_in_memory);
			break;

		case B_CLOCK:
			pthread_mutex_lock(&mutex_pages_in_memory);
			victim = choose_victim_clock();
			new_page_in_memory_clock(new_page, victim->memory_frame_number);
			pthread_mutex_unlock(&mutex_pages_in_memory);
			break;
	}
	new_page->memory_frame_number = victim->memory_frame_number;

	log_info(logger, "Se selecciono a la pagina que ocupa el frame %d en memoria como victima", victim->memory_frame_number);
	victim_service(victim);
}

void bring_page_to_memory(t_page* page, t_log* logger){
	page->memory_frame_number = find_free_memory_page(1);

	if(page->memory_frame_number == -1)
		generate_a_new_free_frame(logger, page);

	memcpy(MEMORY + page->memory_frame_number * 32, SWAP + page->swap_frame_number * 32, 24);
	memcpy(MEMORY + page->memory_frame_number * 32 + 24, SWAP + page->swap_frame_number * 32 + 24, sizeof(uint32_t));
	memcpy(MEMORY + page->memory_frame_number * 32 + 24 + sizeof(uint32_t), SWAP + page->swap_frame_number * 32 + 24 + sizeof(uint32_t), sizeof(uint32_t));

	log_info(logger, "Se paso la informacion del frame %d de swap al %d de memoria principal", page->swap_frame_number, page->memory_frame_number);
}

void add_page_in_memory(t_page* page){
	switch(ALGORITHM){
		case LRU:
			pthread_mutex_lock(&mutex_pages_in_memory);
			new_page_in_memory_lru(page);
			pthread_mutex_unlock(&mutex_pages_in_memory);
			break;
		case B_CLOCK:
			pthread_mutex_lock(&mutex_pages_in_memory);
			new_page_in_memory_clock(page, page->memory_frame_number);
			pthread_mutex_unlock(&mutex_pages_in_memory);
			break;
	}
}

void actualizate_page(t_page* page){
	switch(ALGORITHM){
		case LRU:
			actualizate_page_lru(page);
			break;
		case B_CLOCK:
			actualizate_page_clock(page);
			break;
	}
}

