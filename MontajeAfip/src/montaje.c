/*
 * montaje.c
 *
 *  Created on: 10 dic. 2020
 *      Author: utnso
 */
#include "montaje.h"

int main(){
	read_config();

	char **afip_path_splited = string_split(PUNTO_MONTAJE, "/");
	char* base_path = string_new();
	int i = 0;

	while(afip_path_splited[i] != NULL){
		string_append_with_format(&base_path, "/%s", afip_path_splited[i]);
		if(!dir_exist(base_path)){
			mkdir(base_path, 0777);
		}
		i++;
	}
	free(base_path);
	i = 0;
	while(afip_path_splited[i]!=NULL){
		free(afip_path_splited[i]);
		i = i +1;
	}
	free(afip_path_splited);

	char* dir_metadata = string_from_format("%s/Metadata", PUNTO_MONTAJE);
	mkdir(dir_metadata, 0777);

	char* path_metadata_file = string_from_format("%s/Metadata.AFIP", dir_metadata);
	FILE* file = fopen(path_metadata_file, "wb+");
	fclose(file);

	t_config* metadata = config_create(path_metadata_file);
	dictionary_put(metadata->properties, "BLOCK_SIZE", string_itoa(BLOCK_SIZE));
	dictionary_put(metadata->properties, "BLOCKS", string_itoa(BLOCKS));
	dictionary_put(metadata->properties, "MAGIC_NUMBER", "AFIP");
	config_save(metadata);
	free(dir_metadata);
	free(path_metadata_file);
	free(PUNTO_MONTAJE);
	config_destroy(CONFIG);
	return 0;
}

void read_config(void) {
	CONFIG = config_create("/home/utnso/tp-2020-2c-CabreadOS/MontajeAfip/montaje.config");
	PUNTO_MONTAJE = config_get_string_value(CONFIG, "PUNTO_MONTAJE");
	BLOCKS = config_get_int_value(CONFIG, "BLOCKS");
	BLOCK_SIZE = config_get_int_value(CONFIG, "BLOCK_SIZE");
}

bool dir_exist(char* path){
	DIR* dir = opendir(path);
	if(dir == NULL){
		return false;
	}
	closedir(dir);
	return true;
}



