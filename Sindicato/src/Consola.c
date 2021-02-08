#include "Consola.h"
/*
 *Recetas:
 * CrearReceta AsadoJugoso [Cortar,Hornear,Servir] [3,3,2]
 * CrearReceta AsadoSeco [Cortar,Hornear,Servir] [3,10,2]
 * CrearReceta Choripan [Cortar,Hornear,Servir] [1,2,1]
 * CrearReceta WokDeVegetales [Cortar,Condimentar,Servir] [3,1,1]
 * CrearReceta TortillaDePapa [Cortar,Hervir,Hornear,Servir] [2,2,3,1]
 * CrearReceta MilaCompleta [Empanar,Hornear,Servir] [5,2,1]
 *
 *Restaurantes:
 * CrearRestaurante LaParri 2 [1,1] [AsadoJugoso] [AsadoJugoso,AsadoSeco,Choripan] [200,200,125] 1
 * CrearRestaurante GreenLife 1 [9,9] [] [WokDeVegetales,TortillaDePapa] [200,150] 1
 * CrearRestaurante MilangaPalace 1 [5,5] [] [MilaCompleta] [200] 1
*/

void consola(void) {
	char * linea;
	char ** parametros_restaurantes;
	char ** parametros_receta;
	bool info_correcta;
	printf("La consola ya se encuentra inicializada\n");

	while (1) {
		info_correcta = true;

		linea = readline(">");

		if (linea) {
			add_history(linea);
		}

		if (!strncmp(linea, "CrearRestaurante", 16)) {
			parametros_restaurantes = string_split(linea, " ");

			for(int i = 0; i<8; i++){
				if(parametros_restaurantes[i] == NULL){
					info_correcta = false;
					break;
				}
			}

			if (info_correcta && parametros_restaurantes[8] != NULL)
				info_correcta = false;

			if (!info_correcta){
				printf("La información recibida no fue la correcta para crear un restaurante\n");
				printf("FORMATO CORRECTO:\n\tCrearRestaurante NOMBRE CANTIDAD_COCINEROS [POSICION] [AFINIDAD_COCINEROS] [PLATOS] [PRECIOS] CANTIDAD_HORNOS\n");
			} else {
				CrearRestaurante(parametros_restaurantes);
			}

			string_iterate_lines(parametros_restaurantes, (void*)free);
			free(parametros_restaurantes);

		} else if (!strncmp(linea, "CrearReceta", 11)) {

			parametros_receta = string_split(linea, " ");

			for(int i = 0; i<4; i++){
				if(parametros_receta[i] == NULL){
					info_correcta = false;
					break;
				}
			}

			if (info_correcta && parametros_receta[4] != NULL)
				info_correcta = false;

			if (!info_correcta){
				printf("La información recibida no fue la correcta para crear una receta\n");
				printf("FORMATO CORRECTO:\n\tCrearReceta NOMBRE [PASOS] [TIEMPO_PASOS]\n");
			} else {

				CrearReceta(parametros_receta);
			}

			string_iterate_lines(parametros_receta, (void*)free);
			free(parametros_receta);

		} else if (!strncmp(linea, "Exit", 4)) {
			free(linea);
			return;
		} else {
			printf("No se reconoce el comando ingresado: %s\n", linea);
		}

		free(linea);
	}
}

int digits_of(int number){
	int counter = 0;
	do{
		counter++;
		number = number/10;

	}while(number > 0);

	return counter;
}

void CrearRestaurante(char ** parametros_restaurantes) {
	int pathlen = strlen(punto_montaje) + strlen("/Files/Restaurantes/");
	char * path = malloc(pathlen + strlen(parametros_restaurantes[1])+ strlen("/Info.AFIP")+1);

	strcpy(path, punto_montaje);
	strcpy(path + strlen(punto_montaje), "/Files/Restaurantes/");
	strcpy(path + pathlen, parametros_restaurantes[1]);
	if(dir_exist(path)){
		log_error(LOGGER, "El restaurante %s ya se encuentra registrado en el sistema", parametros_restaurantes[1]);
		return;
	}
	mkdir(path, 0777);
	log_info(LOGGER, "Se creo el directio %s (path: %s)", parametros_restaurantes[1], path);
	strcat(path, "/Info.AFIP");

	FILE * restaurante = fopen(path, "w+");

	log_new_restaurante(parametros_restaurantes[1], path);

	char* total_string = string_new();
	string_append(&total_string, "CANTIDAD_COCINEROS=");
	string_append(&total_string, parametros_restaurantes[2]);
	string_append(&total_string, "\nPOSICION=");
	string_append(&total_string, parametros_restaurantes[3]);
	string_append(&total_string, "\nAFINIDAD_COCINEROS=");
	string_append(&total_string, parametros_restaurantes[4]);
	string_append(&total_string, "\nPLATOS=");
	string_append(&total_string, parametros_restaurantes[5]);
	string_append(&total_string, "\nPRECIO_PLATOS=");
	string_append(&total_string, parametros_restaurantes[6]);
	string_append(&total_string, "\nCANTIDAD_HORNOS=");
	string_append(&total_string, parametros_restaurantes[7]);

	int file_size = strlen(total_string);

	uint32_t free_block = 0;

	if(!get_available_block(&free_block)){
		return;
	}
	log_block_asignation(path, free_block);

	t_config* metadata = config_create(path);
	dictionary_put(metadata->properties,"SIZE", string_itoa(file_size) );
	dictionary_put(metadata->properties, "INITIAL_BLOCK", string_itoa(free_block));
	config_save(metadata);
	config_destroy(metadata);
	fclose(restaurante);

	write_block(free_block, total_string, file_size, path);

	free(path);
}

void CrearReceta(char ** parametros_receta) {
	int pathlen = strlen(punto_montaje) + strlen("/Files/Recetas/");
	char * path = malloc(pathlen + strlen(parametros_receta[1]) + strlen(".AFIP")+1);

	strcpy(path, punto_montaje);
	strcpy(path + strlen(punto_montaje), "/Files/Recetas/");
	strcpy(path + pathlen, parametros_receta[1]);
	strcpy(path + pathlen + strlen(parametros_receta[1]), ".AFIP");

	FILE * receta = fopen(path ,"w+");

	log_new_recipe(parametros_receta[1], path);

	char* total_string = string_new();
	string_append(&total_string, "PASOS=");
	string_append(&total_string, parametros_receta[2]);
	string_append(&total_string, "\nTIEMPO_PASOS=");
	string_append(&total_string, parametros_receta[3]);

	int file_size = strlen(total_string);

	uint32_t free_block = 0;

	if(!get_available_block(&free_block)){
		return;
	}
	log_block_asignation(path, free_block);

	t_config* metadata = config_create(path);
	dictionary_put(metadata->properties,"SIZE", string_itoa(file_size) );
	dictionary_put(metadata->properties,"INITIAL_BLOCK", string_itoa(free_block));
	config_save(metadata);
	config_destroy(metadata);
	fclose(receta);

	write_block(free_block, total_string, file_size, path);

	free(path);
}

