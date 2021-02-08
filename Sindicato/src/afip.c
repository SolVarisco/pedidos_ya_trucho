/*
 * afip.c
 *
 *  Created on: 26 nov. 2020
 *      Author: utnso
 */

#include "afip.h"

bool init_afip(void){
	if(!dir_exist(punto_montaje))
		return 0;

	read_metadata();

	char* bitmap_path = string_new();
	string_append_with_format(&bitmap_path, "%s/Metadata/Bitmap.bin", punto_montaje);

	if(access(bitmap_path, F_OK) != -1){
		open_bitmap();
	} else {
		create_fs(bitmap_path);
	}

	block_size -= 4;
	return 1;
}

bool dir_exist(char* path){
	DIR* dir = opendir(path);
	if(dir == NULL){
		return false;
	}
	closedir(dir);
	return true;
}

bool read_metadata(void){
	char* nombre = string_from_format("%s/Metadata/Metadata.AFIP", punto_montaje);

	t_config* metadata = config_create(nombre);
	blocks = atoi(config_get_string_value(metadata, "BLOCKS"));
	block_size = atoi(config_get_string_value(metadata, "BLOCK_SIZE"));
	return true;
}

bool open_bitmap(void){

	char* nombre_bitmap = string_from_format("%s/Metadata/Bitmap.bin", punto_montaje);

	int fd = open(nombre_bitmap, O_RDWR);
    if (fstat(fd, &size_bitmap) < 0) {
        printf("Error al establecer fstat\n");
        close(fd);
        return 0;
    }

    blocks_available=0;
    bmap = mmap(NULL, size_bitmap.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,	fd, 0);
	bitarray = bitarray_create_with_mode(bmap, blocks/8, MSB_FIRST);
	size_t len_bitarray = bitarray_get_max_bit(bitarray);
	for(int i=0 ; i < len_bitarray;i++){
		 if(bitarray_test_bit(bitarray, i) == 0){
			 blocks_available++;
		 }
	}
	log_info(LOGGER, "Cantidad de bloques disponibles: %d", blocks_available);
	free(nombre_bitmap);

	return 1;
}

void create_fs(char* bitmap_path) {

	char* dir_files = string_from_format("%s/Files", punto_montaje);
	mkdir(dir_files, 0777);
	char* dir_restaurants = string_from_format("%s/Restaurantes", dir_files);
	mkdir(dir_restaurants, 0777);
	char* dir_recipes = string_from_format("%s/Recetas", dir_files);
	mkdir(dir_recipes, 0777);
	char* dir_blocks = string_from_format("%s/Blocks", punto_montaje);
	mkdir(dir_blocks, 0777);

	create_bitmap(bitmap_path);
	//create_metadata_in_files_directory(dir_files);
	create_blocks(dir_blocks, blocks, block_size);

	free(bitmap_path);
	free(dir_files);
	free(dir_restaurants);
	free(dir_recipes);
	free(dir_blocks);

	printf("Creado AFIP correctamente con %d bloques, de tamaño %d \n", blocks, block_size);
}

void create_bitmap(char* bitmap_path){
	// Creo el archivo
	FILE* fpFile = fopen(bitmap_path,"wb");
	fclose(fpFile);

	//le doy el tamaño de mi cantidad de bloques (bits que contendra el array), expresada en bytes (1 byte = 8 bits)
	int bitarray_size = blocks/8;

	truncate(bitmap_path, bitarray_size);

	//Abro el archivo con libreria mmap para setear los valores iniciales en cero.
	int fd = open(bitmap_path, O_RDWR);
	if (fstat(fd, &size_bitmap) < 0) {
		printf("Error al establecer fstat\n");
		close(fd);
	}

	bmap = mmap(NULL, size_bitmap.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,	fd, 0);
	bitarray = bitarray_create_with_mode(bmap, bitarray_size, MSB_FIRST);


	for(int cont=0; cont < blocks; cont++){
		bitarray_clean_bit(bitarray, cont);
	}
	msync(bmap, sizeof(bitarray), MS_SYNC);
}

void create_metadata_in_files_directory(char* dir_files){
	char* path_metadata_file = string_from_format("%s/Metadata", dir_files);
	FILE* file = fopen(path_metadata_file, "wb+");
	fclose(file);

	char* is_directory = string_new();
	string_append(&is_directory, "Y");
	t_config* metadata = config_create(path_metadata_file);
	dictionary_put(metadata->properties,"DIRECTORY", is_directory);

	config_save(metadata);
	config_destroy(metadata);
	free(path_metadata_file);
}

void create_blocks(char* dir_blocks, int blocks_quantity, int block_size){
    for(int i=0; i<blocks_quantity; i++){
        char *block_file_path = string_from_format("%s/%d.AFIP", dir_blocks, i);
        FILE* fpFile = fopen(block_file_path,"wb+");
        fclose(fpFile);
        truncate(block_file_path, block_size);
        free(block_file_path);
    }
}

bool get_available_block(uint32_t* block){
	pthread_mutex_lock(&mutexBitmap);
	bool is_not_available=true;
	while(*block < (bitarray->size)*8) {
		is_not_available = bitarray_test_bit(bitarray, *block);
		if(!is_not_available) {
			bitarray_set_bit(bitarray, *block);
			blocks_available -= 1;
			msync(bmap, sizeof(bitarray), MS_SYNC);
			pthread_mutex_unlock(&mutexBitmap);
			return true;
		}
		*block += 1;

	}
	log_error(LOGGER, "No hay más bloques disponibles");
	pthread_mutex_unlock(&mutexBitmap);
	return false;

}

bool write_block(uint32_t free_block, char* total_string, int total_size, char* file_path){
	char * path = string_from_format("%s/Blocks/%d.AFIP", punto_montaje, free_block);
	FILE * block = fopen(path, "w+");
	free(path);

	pthread_mutex_lock(&mutex_file_system);

	if(total_size <= block_size){
		fwrite(total_string, 1, total_size, block);
		free(total_string);
		fclose(block);

	} else {
		fwrite(total_string, 1, block_size, block);
		uint32_t new_free_block = 0;

		if(!get_available_block(&new_free_block)){
			free(total_string);
			fclose(block);
			pthread_mutex_unlock(&mutex_file_system);
			return false;
		}

		log_block_asignation(file_path, new_free_block);

		fwrite(&new_free_block, sizeof(uint32_t), 1, block);
		pthread_mutex_unlock(&mutex_file_system);

		fclose(block);
		total_size = total_size - block_size;
		char* new_string = malloc(total_size);
		memcpy(new_string, total_string + block_size, total_size);
		free(total_string);

		if(!write_block(new_free_block, new_string, total_size, file_path)){
			return false;
		}
	}

	pthread_mutex_unlock(&mutex_file_system);
	return true;
}

bool search_restaurant(char* restaurant, char** path){

	string_append(path, punto_montaje);
	string_append(path, "/Files/Restaurantes/");
	string_append(path, restaurant);

	if(!dir_exist(*path)){
		log_error(LOGGER, "El restaurante %s no se encuentra registrado en el FS", restaurant);
		return false;
	}

	return true;
}

bool search_restaurant_info(char* restaurant, int* first_block, int* file_size){
	char* path = string_new();

	if(!search_restaurant(restaurant, &path))
		return false;

	string_append(&path, "/Info.AFIP");

	t_config* metadata = config_create(path);
	*first_block = config_get_int_value(metadata, "INITIAL_BLOCK");
	*file_size = config_get_int_value(metadata, "SIZE");
	config_destroy(metadata);

	return true;
}

t_list* get_food(char* restaurant){
	int first_block, file_size;
	if(!search_restaurant_info(restaurant, &first_block, &file_size))
		return NULL;

	return read_foods(first_block, file_size);

}

t_list* read_foods(uint32_t block, int file_size){
	char* foods_string;

	if(!read_blocks(&foods_string, 4, "]", block, file_size))
		return NULL;

	t_list* food_list = list_create();
	char** food = string_split(foods_string, ",");
	int i = 0;

	while(food[i] != NULL){
		if(i == 0){
			list_add(food_list, food[i] + 1);
		} else {
			list_add(food_list, food[i]);
		}
		i++;
	}

	return food_list;
}

bool read_blocks(char** final_info, int line, char* limit_char, uint32_t block, int file_size){
	char* block_path = string_from_format("%s/Blocks/%d.AFIP", punto_montaje, block);
	FILE* block_file = fopen(block_path, "r");
	free(block_path);
	uint32_t next_block = 0;
	bool exist_next = true;
	int i,max = 0;
	char* info;

	if(file_size <= block_size){
		max = file_size;
		info = malloc(file_size + 1);
		fread(info, 1, file_size, block_file);
		memcpy(info + file_size, "\0", 1);
		exist_next = false;
	} else {
		max=block_size;
		info = malloc(block_size + 1);
		fread(info, 1, block_size, block_file);
		memcpy(info + block_size, "\0", 1);
		fread(&next_block, sizeof(uint32_t), 1, block_file);
		file_size -= block_size;
	}
	fclose(block_file);

	for(i = 0; i<max; i++){
		if(info[i] == '='){
			line--;
			if(line == 0)
				break;
		}
	}

	if(line != 0 && exist_next){
		free(info);
		return read_blocks(final_info, line, limit_char, next_block, file_size);
	} else if(line != 0){
		free(info);
		return false;
	}

	char* search = malloc(strlen(info) - i + 1);
	memcpy(search, info + i, strlen(info)-i);
	memcpy(search + strlen(info)-i, "\0", 1);

	char* limit = strstr(search, limit_char);
	if(limit != NULL){
		int size = strlen(search) - 1 - strlen(limit);
		*final_info = malloc(size + 1);
		memcpy(*final_info, search + 1, size);
		memcpy(*final_info + size, "\0", 1);
		//free(limit);
	} else {
		*final_info = malloc(strlen(search));
		memcpy(*final_info, search + 1, (strlen(search)-1));
		memcpy(*final_info + (strlen(search)-1), "\0", 1);
		if (exist_next)
			check_next_block_limit(final_info, limit_char, next_block, file_size);
	}

	free(search);
	free(info);
	return true;

}

void check_next_block_limit(char** info, char* limit_char, uint32_t next_block, int file_size){
	char* block_path = string_from_format("%s/Blocks/%d.AFIP", punto_montaje, next_block);
	FILE* block_file = fopen(block_path, "r");
	free(block_path);
	int total;
	char * new_info;
	bool next_block_exist = false;


	if(file_size <= block_size){
		new_info = malloc(file_size + 1);
		fread(new_info, 1, file_size, block_file);
		memcpy(new_info + file_size, "\0", 1);
		total = file_size;
	} else {
		new_info = malloc(block_size + 1);
		fread(new_info, 1, block_size, block_file);
		memcpy(new_info + block_size, "\0", 1);
		fread(&next_block, sizeof(uint32_t), 1, block_file);
		file_size -= block_size;
		total = block_size;
		next_block_exist = true;
	}
	fclose(block_file);

	char* limit = strstr(new_info, limit_char);
	if(limit != NULL){
		char* useful_info = malloc(total - (strlen(limit)) + 1);
		memcpy(useful_info, new_info, total - (strlen(limit)));
		memcpy(useful_info + total - strlen(limit), "\0", 1);
		string_append(info, useful_info);
		free(new_info);
		return;
	}
	string_append(info, new_info);

	if(next_block_exist)
		check_next_block_limit(info, limit_char, next_block, file_size);
}

bool save_order(char* restaurant, uint32_t id){
	char* path = string_new();
	if(!search_restaurant(restaurant, &path))
		return false;
	string_append(&path, "/Pedido");
	string_append(&path, string_itoa(id));
	string_append(&path, ".AFIP");

	if(access(path, F_OK) != -1){
		log_error(LOGGER, "El pedido %d del restaurante %s ya se encuentra registrado en el FS", id, restaurant);
		return false;
	}

	char* total_string = string_new();
	string_append(&total_string, "ESTADO_PEDIDO=Pendiente");
	string_append(&total_string, "\nLISTA_PLATOS=[]");
	string_append(&total_string, "\nCANTIDAD_PLATOS=[]");
	string_append(&total_string, "\nCANTIDAD_LISTA=[]");
	string_append(&total_string, "\nPRECIO_TOTAL=");

	int file_size = strlen(total_string);

	uint32_t free_block = 0;

	if(!get_available_block(&free_block)){
		return false;
	}

	log_block_asignation(path, free_block);

	FILE* new_order = fopen(path, "w+");

	log_new_order(restaurant, id, path);

	t_config* metadata = config_create(path);
	dictionary_put(metadata->properties,"SIZE", string_itoa(file_size));
	dictionary_put(metadata->properties, "INITIAL_BLOCK", string_itoa(free_block));
	config_save(metadata);
	//config_destroy(metadata);
	fclose(new_order);

	write_block(free_block, total_string, file_size, path);

	free(path);

	return true;
}

bool add_food(char* restaurant, uint32_t id, char* food, int32_t total){
	int food_price = search_food_price(restaurant, food);

	if(food_price == -1)
		return false;

	food_price = food_price * total;

	char* path = string_new();
	int position = 0;
	bool exist = false;
	int size = 0;
	int old_size = 0;

	search_restaurant(restaurant, &path);
	string_append(&path, "/Pedido");
	string_append(&path, string_itoa(id));
	string_append(&path, ".AFIP");

	if(access(path, F_OK) == -1){
		log_error(LOGGER, "El pedido %d del restaurante %s no se encuentra registrado en el FS", id, restaurant);
		return false;
	}

	t_config* metadata = config_create(path);
	int first_block = config_get_int_value(metadata, "INITIAL_BLOCK");
	int file_size = config_get_int_value(metadata, "SIZE");
	config_destroy(metadata);

	char* status = string_new();

	pthread_mutex_lock(&mutex_file_system);
	read_blocks(&status, 1, "\n", first_block, file_size);

	if(strcmp(status, "Pendiente")){
		log_error(LOGGER, "El pedido %d del restaurante %s no se encuenta en estado \"Pendiente\"", id, restaurant);
		free(status);
		pthread_mutex_unlock(&mutex_file_system);
		return false;
	}
	free(status);

	char* info = string_new();

	read_blocks(&info, 2, ".", first_block, file_size);

	char** information = string_split(info, "]");

	if(strlen(information[0]) != 1){
		char** food_list = string_split(information[0], ",");
		while(food_list[position] != NULL){
			if(!strcmp(food_list[position], food) || (position == 0 && !strcmp(food_list[0]+1, food))){
				exist = true;
				break;
			}
			position++;
		}

		int j = 0;

		while(food_list[j] != NULL){
			free(food_list[j]);
			j++;
		}
		free(food_list);
	}


	if(exist){
		char** total_amount = string_split(information[1], ",");
		int new_total;

		if(position == 0){
			new_total = total + atoi(total_amount[position] + strlen("\nCANTIDAD_PLATOS=["));
			old_size = strlen(total_amount[position]);
			memcpy(total_amount[position] + strlen("\nCANTIDAD_PLATOS=["), string_itoa(new_total), strlen(string_itoa(new_total)));
		} else {
			new_total = total + atoi(total_amount[position]);
			old_size = strlen(total_amount[position]);
			strcpy(total_amount[position],  string_itoa(new_total));
		}

		size = strlen(total_amount[position]) - old_size;

		int i = 0;

		memset(information[1], 0, strlen(information[1]));

		while(total_amount[i + 1]!=NULL){
			string_append_with_format(&information[1], "%s,", total_amount[i]);
			free(total_amount[i]);
			i++;
		}
		string_append_with_format(&information[1], "%s", total_amount[i]);
		free(total_amount[i]);
		free(total_amount);

	} else {
		if(position == 0){
			string_append(&information[0], food);
			string_append(&information[1], string_itoa(total));
			string_append(&information[2], "0");
		} else {
			string_append_with_format(&information[0], ",%s", food);
			string_append_with_format(&information[1], ",%s", string_itoa(total));
			string_append(&information[2], ",0");
			size = strlen(",")*3;
		}

		size = strlen(string_itoa(food_price)) + strlen(food) + 1;
	}

	old_size = strlen(information[3]);
	int total_price = food_price + atoi(information[3]);
	memcpy(information[3] + strlen("\nPRECIO_TOTAL="), string_itoa(total_price), strlen(string_itoa(total_price)));
	memcpy(information[3] + strlen("\nPRECIO_TOTAL=") + strlen(string_itoa(total_price)), "\0", 1);
	size += strlen(information[3]) - old_size;

	free(info);

	char* string_updated = string_new();

	string_append_with_format(&string_updated, "%s]%s]%s]%s", information[0], information[1], information[2], information[3]);
	memcpy(string_updated + strlen(information[0]) + strlen(information[1]) + strlen(information[2]) + strlen(information[3]) + 3, "\0", 1);

	for(int i = 0; i<4; i++){
		free(information[i]);
	}
	free(information);

	int offset = strlen("ESTADO_PEDIDO=Pendiente\nLISTA_PLATOS=");

	if(size != 0){
		t_config* metadata = config_create(path);
		config_set_value(metadata, "SIZE", string_itoa(size + file_size));
		config_save(metadata);
		config_destroy(metadata);
	}

	return update_information(string_updated, first_block, file_size, size+file_size, offset, path);
}

int search_food_price(char* restaurant, char* food){
	int first_block, file_size;
	if(!search_restaurant_info(restaurant, &first_block, &file_size))
		return -1;

	t_list* foods = read_foods(first_block, file_size);
	int position = 0;
	t_link_element* element = foods->head;
	bool food_exist = false;

	while(element != NULL){
		if(!strcmp(food, (char*)element->data)){
			food_exist = true;
			break;
		}
		position++;
		element = element->next;
	}

	list_destroy(foods);

	if(!food_exist)
		return -1;

	char* prices_string;

	if(!read_blocks(&prices_string, 5, "]", first_block, file_size))
			return -1;

	char** price = string_split(prices_string, ",");

	if(price[position] == NULL){
		return -1;
	}
	if(position == 0)
		return atoi(price[position] + 1);

	return atoi(price[position]);
}

bool update_information(char* info, int block, int old_size, int new_size, int offset, char* file_path){
	char* block_path = string_from_format("%s/Blocks/%d.AFIP", punto_montaje, block);
	FILE* block_file = fopen(block_path, "r+");
	free(block_path);
	uint32_t next_block = 0;
	bool modicated = false;

	if(offset <= block_size){

		if(old_size <= block_size){

			fseek(block_file, offset, SEEK_SET);

			if(new_size > block_size){
				fwrite(info, 1, block_size - offset, block_file);

				if(!get_available_block(&next_block)){
					free(info);
					fclose(block_file);
					free(file_path);
					pthread_mutex_unlock(&mutex_file_system);
					return false;
				}

				log_block_asignation(file_path, next_block);

				fseek(block_file, block_size, SEEK_SET);
				fwrite(&next_block, sizeof(uint32_t), 1, block_file);
				modicated = true;

			} else {

				fwrite(info, 1, strlen(info), block_file);
				free(info);
				if(new_size < old_size)
					fwrite(" ", 1, 1, block_file);
				fclose(block_file);
				free(file_path);
				pthread_mutex_unlock(&mutex_file_system);
				return true;
			}

		} else {
			fseek(block_file, block_size, SEEK_SET);
			fread(&next_block, sizeof(uint32_t), 1, block_file);
			fseek(block_file, offset, SEEK_SET);
			fwrite(info, 1, block_size - offset, block_file);
			modicated = true;
		}

	} else {
		offset -= block_size;
		fseek(block_file, block_size, SEEK_SET);
		fread(&next_block, sizeof(uint32_t), 1, block_file);
	}

	fclose(block_file);

	new_size = new_size - block_size;
	old_size = old_size - block_size;

	if(modicated){
		char* new_string = malloc(new_size + 1);
		memcpy(new_string, info + block_size - offset, new_size + 1);
		free(info);
		return update_information(new_string, next_block, old_size, new_size, 0, file_path);
	}

	return update_information(info, next_block, old_size, new_size, offset, file_path);
}

t_estado_pedido get_order_information(char* restaurant, uint32_t id){
	char* path = string_new();
	t_estado_pedido order_info;
	order_info.platos = list_create();

	if(!search_restaurant(restaurant, &path)){
		free(path);
		order_info.estado = INEXISTENTE;
		return order_info;
	}
	string_append(&path, "/Pedido");
	string_append(&path, string_itoa(id));
	string_append(&path, ".AFIP");

	if(access(path, F_OK) == -1){
		log_error(LOGGER, "El pedido %d del restaurante %s no se encuentra registrado en el FS", id, restaurant);
		order_info.estado = INEXISTENTE;
		free(path);
		return order_info;
	}

	t_config* metadata = config_create(path);
	int first_block = config_get_int_value(metadata, "INITIAL_BLOCK");
	int file_size = config_get_int_value(metadata, "SIZE");
	config_destroy(metadata);
	free(path);

	char* state = string_new();
	read_blocks(&state, 1, "\n", first_block, file_size);

	if(!strcmp(state, "Pendiente")){
		order_info.estado = PENDIENTE;
	} else if (!strcmp(state, "Confirmado")) {
		order_info.estado = CONFIRMADO;
	} else {
		order_info.estado = TERMINADO;
	}

	free(state);

	char* foods = string_new();
	char* total = string_new();
	char* ready = string_new();

	read_blocks(&foods, 2, "]", first_block, file_size);
	read_blocks(&total, 3, "]", first_block, file_size);
	read_blocks(&ready, 4, "]", first_block, file_size);

	char** food_list = string_split(foods, ",");
	char** total_list = string_split(total, ",");
	char** ready_list = string_split(ready, ",");
	int i = 0;

	while(food_list[i] != NULL){
		t_estado_comida* food_info = malloc(sizeof(*food_info));
		if(i == 0){
			food_info->comida = malloc(strlen(food_list[i]));
			strcpy(food_info->comida, food_list[i] + 1);
			food_info->total = (uint32_t)atoi(total_list[i] +1);
			food_info->listo = (uint32_t)atoi(ready_list[i] +1);
		} else {
			food_info->comida = malloc(strlen(food_list[i])+1);
			strcpy(food_info->comida, food_list[i]);
			food_info->total = (uint32_t)atoi(total_list[i]);
			food_info->listo = (uint32_t)atoi(ready_list[i]);
		}
		food_info->pasos_receta = get_receta_information(food_info->comida);
		list_add(order_info.platos, (void*)food_info);

		free(food_list[i]);
		free(total_list[i]);
		free(ready_list[i]);
		i++;
	}

	free(food_list);
	free(total_list);
	free(ready_list);
	free(foods);
	free(total);
	free(ready);

	return order_info;

}

bool change_state(char* restaurant, uint32_t id, char* new_state){
	char* path = string_new();
	search_restaurant(restaurant, &path);
	string_append(&path, "/Pedido");
	string_append(&path, string_itoa(id));
	string_append(&path, ".AFIP");

	if(access(path, F_OK) == -1){
		log_error(LOGGER, "El pedido %d del restaurante %s no se encuentra registrado en el FS", id, restaurant);
		return false;
	}

	t_config* metadata = config_create(path);
	int first_block = config_get_int_value(metadata, "INITIAL_BLOCK");
	int file_size = config_get_int_value(metadata, "SIZE");
	config_destroy(metadata);

	char* info = string_new();
	int size = 0;
	read_blocks(&info, 1, ".", first_block, file_size);
	char** information = string_split(info, "\n");

	pthread_mutex_lock(&mutex_file_system);
	if(!strcmp(new_state, "Confirmado") && strcmp(information[0], "Pendiente")){
		log_error(LOGGER, "El pedido %d del restaurante %s no se encuenta en estado \"Pendiente\"", id, restaurant);
		for(int i = 0; i<5; i++){
			free(information[i]);
		}
		free(information);
		free(info);
		pthread_mutex_unlock(&mutex_file_system);
		return false;
	} else if (!strcmp(new_state, "Terminado") && strcmp(information[0], "Confirmado")){
		log_error(LOGGER, "El pedido %d del restaurante %s no se encuenta en estado \"Confirmado\"", id, restaurant);
		for(int i = 0; i<5; i++){
			free(information[i]);
		}
		free(information);
		free(info);
		pthread_mutex_unlock(&mutex_file_system);
		return false;
	}
	free(info);

	if(!strcmp(new_state, "Confirmado")){
		memset(information[0], 0, strlen("Pendiente"));
		information[0] = realloc(information[0], strlen("Confirmado"));
		strcpy(information[0], "Confirmado");

		size = strlen("Confirmado") - strlen("Pendiente");
	} else {
		memset(information[0], 0, strlen("Confirmado"));
		information[0] = realloc(information[0], strlen("Terminado"));
		strcpy(information[0], "Terminado");

		size = strlen("Terminado") - strlen("Confirmado");
	}

	char* string_updated = string_new();

	string_append_with_format(&string_updated, "%s\n%s\n%s\n%s\n%s", information[0], information[1], information[2], information[3], information[4]);
	memcpy(string_updated + strlen(information[0]) + strlen(information[1]) + strlen(information[2]) + strlen(information[3]) + strlen(information[4]) + 4, "\0", 1);

	for(int i = 0; i<5; i++){
		free(information[i]);
	}
	free(information);

	int offset = strlen("ESTADO_PEDIDO=");

	if(size != 0){
		t_config* metadata = config_create(path);
		config_set_value(metadata, "SIZE", string_itoa(size + file_size));
		config_save(metadata);
		config_destroy(metadata);
	}

	return update_information(string_updated, first_block, file_size, size+file_size, offset, path);
}

bool food_ready(char* restaurant, uint32_t id, char* food){
	char* path = string_new();
	search_restaurant(restaurant, &path);
	string_append(&path, "/Pedido");
	string_append(&path, string_itoa(id));
	string_append(&path, ".AFIP");

	if(access(path, F_OK) == -1){
		log_error(LOGGER, "El pedido %d del restaurante %s no se encuentra registrado en el FS", id, restaurant);
		return false;
	}

	t_config* metadata = config_create(path);
	int first_block = config_get_int_value(metadata, "INITIAL_BLOCK");
	int file_size = config_get_int_value(metadata, "SIZE");
	config_destroy(metadata);

	char* state = string_new();
	read_blocks(&state, 1, "\n", first_block, file_size);

	pthread_mutex_lock(&mutex_file_system);
	if(strcmp(state, "Confirmado")){
		log_error(LOGGER, "El pedido %d del restaurante %s no se encuentra en estado \"Pendiente\"", id, restaurant);
	}

	free(state);

	char* foods = string_new();
	read_blocks(&foods, 2, "]", first_block, file_size);
	char** food_list = string_split(foods, ",");
	int position = 0;
	int i = 0;
	bool exist = false;

	while(food_list[position] != NULL){
		if(!strcmp(food_list[position], food) || (position == 0 && !strcmp(food_list[position] + 1, food))){
			exist = true;
			break;
		}
		position += 1;
	}

	while(food_list[i] != NULL){
		free(food_list[i]);
		i += 1;
	}
	free(food_list);
	free(foods);

	if(!exist){
		log_error(LOGGER, "No pediste esto papa");
		pthread_mutex_unlock(&mutex_file_system);
		return false;
	}

	char* info = string_new();
	read_blocks(&info, 4, ".", first_block, file_size);
	char** information = string_split(info, "]");
	char** ready_list = string_split(information[0], ",");
	int food_ready;
	int offset = 0;

	if(position == 0){
		offset = 1;
	}

	food_ready = atoi(ready_list[position] + offset) + 1;
	int number_size = strlen(string_itoa(food_ready));

	int size = number_size - strlen(ready_list[position]) + offset;

	if(size != 0){
		ready_list[position] = realloc(ready_list[position], number_size + offset);
	}
	memcpy(ready_list[position] + offset, string_itoa(food_ready), number_size);

	i = 0;
	char* updated_information = string_new();
	while(ready_list[i] != NULL){
		if(i != 0)
			string_append(&updated_information, ",");
		string_append(&updated_information, ready_list[i]);
		free(ready_list[i]);
		i++;
	}
	free(ready_list);

	int string_lenght = strlen(updated_information);
	string_append_with_format(&updated_information, "]%s", information[1]);
	memcpy(updated_information + string_lenght + strlen(information[1]) + 1, "\0", 1);

	for(int i = 0; i<2; i++){
		free(information[i]);
	}
	free(information);

	offset = file_size - strlen(info);

	free(info);

	if(size != 0){
		t_config* metadata = config_create(path);
		config_set_value(metadata, "SIZE", string_itoa(size + file_size));
		config_save(metadata);
		config_destroy(metadata);
	}

	return update_information(updated_information, first_block, file_size, size+file_size, offset, path);
}

t_info_restaurante get_restaurante_information(char* restaurant) {
	t_info_restaurante restaurant_info;
	int restaurant_first_block, restaurant_file_size;
	restaurant_info.afinidades = list_create();
	restaurant_info.recetas = list_create();

	if(!search_restaurant_info(restaurant, &restaurant_first_block, &restaurant_file_size)){
		restaurant_info.cantidad_cocineros = 0;
		restaurant_info.cantidad_hornos = 0;
		restaurant_info.cantidad_pedidos = 0;
		restaurant_info.coordenadas.pos_x = 0;
		restaurant_info.coordenadas.pos_y = 0;
		return restaurant_info;
	}

	restaurant_info.cantidad_pedidos = number_of_orders(restaurant);

	t_list* platos = read_foods(restaurant_first_block, restaurant_file_size);

	char* nombre_plato;
	int precio_plato;

	char* info;
	read_blocks(&info, 1, ".", restaurant_first_block, restaurant_file_size);
	char** information = string_split(info, "\n");

	char** price = string_split(information[4], ",");
	int j = 0;
	int size;

	while(price[j + 1] != NULL) {
		nombre_plato = list_remove(platos, 0);
		if(j == 0){
			precio_plato = atoi(price[j] + strlen("PRECIO_PLATOS=["));
		} else {
			precio_plato = atoi(price[j]);
		}
		t_receta* receta= malloc(sizeof(t_receta));
		receta->nombre = nombre_plato;
		receta->precio = precio_plato;
		list_add(restaurant_info.recetas, receta);
		free(price[j]);
		j++;
	}

	char* ultimo_precio;
	size = strlen(price[j]);

	nombre_plato = list_remove(platos, 0);

	if(j == 0){
		size -= strlen("PRECIO_PLATOS=[");
		ultimo_precio = malloc(size);
		memcpy(ultimo_precio, price[j] + strlen("PRECIO_PLATOS=["), size - 1);
		memcpy(ultimo_precio + size -1, "\0", 1);
	}else{
		ultimo_precio = malloc(size);
		memcpy(ultimo_precio, price[j], size - 1);
		memcpy(ultimo_precio + size -1, "\0", 1);
	}

	precio_plato = atoi(ultimo_precio);
	t_receta* receta= malloc(sizeof(t_receta));
	receta->nombre = nombre_plato;
	receta->precio = precio_plato;
	list_add(restaurant_info.recetas, receta);
	free(price[j]);
	free(price);
	free(ultimo_precio);


	restaurant_info.afinidades = list_create();

	if(strcmp(information[2], "AFINIDAD_COCINEROS=[]")){

		char** afinidad = string_split(information[2], ",");
		int i = 0;

		while(afinidad[i + 1] != NULL){
			if(i == 0){
				list_add(restaurant_info.afinidades, afinidad[i] + strlen("AFINIDAD_COCINEROS=["));
			} else {
				list_add(restaurant_info.afinidades, afinidad[i]);
			}
			free(afinidad[i]);
			i++;
		}
		char* ultima_afinidad;
		size = strlen(afinidad[i]);
		if(i == 0){
			size -= strlen("AFINIDAD_COCINEROS=[");
			ultima_afinidad = malloc(size);
			memcpy(ultima_afinidad, afinidad[i] + strlen("AFINIDAD_COCINEROS=["), size - 1);
			memcpy(ultima_afinidad + size -1, "\0", 1);
		}else{
			ultima_afinidad = malloc(size);
			memcpy(ultima_afinidad, afinidad[i], size - 1);
			memcpy(ultima_afinidad + size -1, "\0", 1);
		}
		list_add(restaurant_info.afinidades, ultima_afinidad);

		free(afinidad[i]);
		free(afinidad);
	}

	char** posiciones = string_split(information[1], ",");

	restaurant_info.coordenadas.pos_x = atoi(posiciones[0] + strlen("POSICION=["));
	restaurant_info.coordenadas.pos_y = atoi(posiciones[1]);

	restaurant_info.cantidad_hornos = atoi(information[5] + strlen("CANTIDAD_HORNOS="));

	restaurant_info.cantidad_cocineros = atoi(information[0]);

	int i = 0;
	for(i = 0; i<7; i++)
		free(information[i]);
	free(information);

	for(i=0; i<2; i++)
		free(posiciones[i]);
	free(posiciones);

	free(info);

	return restaurant_info;
}

uint32_t number_of_orders(char* restaurant){
	uint32_t count = 0;
	struct dirent *de;
	char* path = string_new();

	search_restaurant(restaurant, &path);

	DIR *dr = opendir(path);

	while ((de = readdir(dr)) != NULL)
		count++;

	count -= 3;      // Info.AFIP   "."  ".."
	free(path);
	closedir(dr);
	return count;
}

t_list* get_receta_information(char* nombre_receta) {
	t_list* pasos_receta = list_create();
	char* path = string_new();
	int first_block, file_size;

	search_receta(nombre_receta, &path);

	t_config* metadata = config_create(path);
	first_block = config_get_int_value(metadata, "INITIAL_BLOCK");
	file_size = config_get_int_value(metadata, "SIZE");
	config_destroy(metadata);

	char* nombre_paso;
	int duracion_paso;

	char* info;
	read_blocks(&info, 1, ".", first_block, file_size);
	char** information = string_split(info, "\n");

	char** paso = string_split(information[0], ",");
	char** duracion = string_split(information[1], ",");
	int j = 0;
	int size_paso;
	int size_duracion;

	while(paso[j + 1] != NULL) {
		if(j == 0){
			nombre_paso = paso[j] + strlen("[");
			duracion_paso = atoi(duracion[j] + strlen("TIEMPO_PASOS=["));
		} else {
			nombre_paso = paso[j];
			duracion_paso = atoi(duracion[j]);
		}
		t_paso* receta = malloc(sizeof(t_receta));
		receta->nombre = nombre_paso;
		receta->tiempo_requerido = duracion_paso;
		receta->tiempo_pasado = 0;
		list_add(pasos_receta, receta);
		//free(paso[j]);
		free(duracion[j]);
		j++;
	}

	char* ultimo_paso;
	char* ultima_duracion;
	size_paso = strlen(paso[j]);
	size_duracion = strlen(duracion[j]);

	if(j == 0){
		size_paso -= strlen("[");
		ultimo_paso = malloc(size_paso);
		memcpy(ultimo_paso, paso[j] + strlen("["), size_paso - 1);
		memcpy(ultimo_paso + size_paso -1, "\0", 1);

		size_duracion -= strlen("TIEMPO_PASOS=[");
		ultima_duracion = malloc(size_duracion);
		memcpy(ultima_duracion, duracion[j] + strlen("TIEMPO_PASOS=["), size_duracion - 1);
		memcpy(ultima_duracion + size_duracion -1, "\0", 1);
	}else{
		ultimo_paso = malloc(size_paso);
		memcpy(ultimo_paso, paso[j], size_paso - 1);
		memcpy(ultimo_paso + size_paso -1, "\0", 1);

		ultima_duracion = malloc(size_duracion);
		memcpy(ultima_duracion, duracion[j], size_duracion - 1);
		memcpy(ultima_duracion + size_duracion -1, "\0", 1);
	}

	t_paso* receta= malloc(sizeof(t_receta));
	receta->nombre = ultimo_paso;
	receta->tiempo_requerido = atoi(ultima_duracion);
	receta->tiempo_pasado = 0;
	list_add(pasos_receta, receta);
	free(paso[j]);
	free(paso);
	free(duracion[j]);
	free(duracion);
	free(ultima_duracion);

	return pasos_receta;
}

bool search_receta(char* receta, char** path){

	string_append(path, punto_montaje);
	string_append(path, "/Files/Recetas/");
	string_append(path, receta);
	string_append(path, ".AFIP");

	/*
	if(!dir_exist(*path)){
		log_error(LOGGER, "La receta %s no se encuentra registrada en el FS", receta);
		return false;
	}*/

	return true;
}

/*
char* read_blocks_content(char* path){
	t_config* metadata = config_create(path);
	int content_size = config_get_int_value(metadata, "SIZE");
	char** blocks = config_get_int_value(metadata, "BLOCKS");

	int rest_to_read = content_size;
	int size_to_read = 0;
	int ind_blocks=0;
	char* buffer = string_new();
	char* blocks_content = string_new();

	if (content_size!=0){
		while(blocks[ind_blocks]!=NULL){
			log_trace(LOGGER, "READ_BLOCKS_CONTENT INICIO");
			free(buffer);
			if (rest_to_read-block_size > 0){
				size_to_read = block_size;
				rest_to_read = rest_to_read-block_size;
			}else{
				size_to_read = rest_to_read;
			}
			int block_num=atoi(blocks[ind_blocks]);

			char* dir_block = string_from_format("%s/Blocks/%i.bin", punto_montaje, block_num);
			FILE* file = fopen(dir_block, "rb+");
			buffer = malloc(size_to_read+1);

			fread(buffer, sizeof(char), size_to_read, file);
			buffer[size_to_read] = '\0';
			string_append(&blocks_content, buffer);
			fclose(file);
			free(dir_block);
			free(blocks[ind_blocks]);

			ind_blocks += 1;
			log_trace(LOGGER, "READ_BLOCKS_CONTENT FINAL");
		}
	}
	free(blocks);
	free(buffer);
	//log_info(LOGGER, "El contenido de los bloques en %s es %s",path, blocks_content);
	config_destroy(metadata);
	return blocks_content;
}
*/


