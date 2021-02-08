 #include "Sindicato.h"

int main(void){
	int connetion;
	read_config();
	LOGGER = init_logger();
	if(!init_afip()){
		return -1;
	}

	pthread_create(&console, NULL, console_accion, NULL);

	sindicato.modulo = SINDICATO;
	sindicato.ip = config_get_string_value(config, "SERVER_IP");
	sindicato.puerto = config_get_string_value(config, "PUERTO_ESCUCHA");

	int server = iniciar_servidor(sindicato.ip, sindicato.puerto);
	log_info(LOGGER, "El modulo sindicato ya esta listo para recibir mensajes");

	Paquete* new_package;
	for(int i = 0; i<5; i++){
		connetion = esperar_cliente(server);
		new_package = recibir_paquete(connetion, sindicato);
		accion(new_package, connetion);
		liberar_conexion(connetion);
	}


	while(1){
		connetion = esperar_cliente(server);
		if(connetion == -1){
			liberar_conexion(connetion);
			continue;
		}
		pthread_create(&thread, NULL, customer_suport, &connetion);
		pthread_detach(thread);
	}

	kill_mutexs();
	return 0;
}

void read_config(void) {
	config = config_create("/home/utnso/tp-2020-2c-CabreadOS/Sindicato/sindicato.config");
	punto_montaje = config_get_string_value(config, "PUNTO_MONTAJE");
}

t_log* init_logger(void) {
	return log_create(
			"sindicato.log",
			"Sindicato",
			true,
			LOG_LEVEL_INFO);
}

void* console_accion(void* args){
	consola();
	return NULL;
}

void* customer_suport(void* args){
	int* connetion = (int*) args;
	Paquete* new_package = recibir_paquete(*connetion, sindicato);

	log_mensaje(new_package->header.tipo_mensaje, 1);
	accion(new_package, *connetion);
	liberar_conexion(*connetion);

	return NULL;
}

void accion(Paquete* package, int connetion){
	switch(package->header.tipo_mensaje){
		case HANDSHAKE:
			gestionar_handshake(package);
			break;
		case CONSULTAR_PLATOS:
			consultar_platos(package, connetion);
			break;
		case GUARDAR_PEDIDO:
			guardar_pedido(package, connetion);
			break;
		case GUARDAR_PLATO:
			guardar_plato(package, connetion);
			break;
		case CONFIRMAR_PEDIDO:
			confirmar_pedido(package, connetion);
			break;
		case OBTENER_PEDIDO:
			obtener_pedido(package, connetion);
			break;
		case OBTENER_RESTAURANTE:
			obtener_restaurante(package, connetion);
			break;
		case PLATO_LISTO:
			plato_listo(package, connetion);
			break;
		case OBTENER_RECETA:
			obtener_receta(package, connetion);
			break;
		case TERMINAR_PEDIDO:
			terminar_pedido(package, connetion);
			break;
		default:
			printf("Operacion no valida\n");
			break;
	}
}

void gestionar_handshake(Paquete* package){
	t_handshake* handshke_recibido = (t_handshake*)package->mensaje;

	switch(handshke_recibido->contacto.modulo){
		case CLIENTE: ;
			t_cliente* cliente = (t_cliente*)handshke_recibido->informacion;
			log_conexion(CLIENTE, cliente->id_cliente);
			free(cliente->id_cliente);
			free(cliente);
			break;
		case RESTAURANTE: ;
			t_seleccionar_restaurante* restaurante = (t_seleccionar_restaurante*)handshke_recibido->informacion;
			log_conexion(RESTAURANTE, restaurante->nombre_restaurante);
			free(restaurante->nombre_restaurante);
			free(restaurante->cliente.id_cliente);
			free(restaurante);
			break;
		default:
			break;
	}
}

void consultar_platos(Paquete* package, int connetion){
	char* restaurant = (char*)package->mensaje;
	log_restaurante(restaurant);
	t_list* foods = get_food(restaurant);
	log_r_consultar_platos(foods);
	enviar_mensaje(connetion, R_CONSULTAR_PLATOS, foods);
	free(restaurant);
}

void guardar_pedido(Paquete* package, int connetion){
	t_pedido* pedido = (t_pedido*)package->mensaje;
	log_pedido(pedido);
	bool success = save_order(pedido->nombre, pedido->id_pedido);
	log_bool(R_GUARDAR_PEDIDO, success);
	enviar_mensaje(connetion, R_GUARDAR_PEDIDO, &success);
	free(pedido->nombre);
	free(pedido);
}

void guardar_plato(Paquete* package, int connetion){
	t_plato* plato = (t_plato*)package->mensaje;
	log_guardar_plato(plato);
	bool success = add_food(plato->restaurante.nombre, plato->restaurante.id_pedido, plato->comida, plato->cantidad);
	log_bool(R_GUARDAR_PLATO, success);
	//TODO: Decia: enviar_mensaje(connetion, R_GUARDAR_PEDIDO, &success);
	enviar_mensaje(connetion, R_GUARDAR_PLATO, &success);
	free(plato->comida);
	free(plato->restaurante.nombre);
	free(plato);
}

void confirmar_pedido(Paquete* package, int connetion){
	t_pedido* pedido = (t_pedido*)package->mensaje;
	log_pedido(pedido);
	change_state(pedido->nombre, pedido->id_pedido, "Confirmado");
	t_estado_pedido estado_pedido = get_order_information(pedido->nombre, pedido->id_pedido);
	log_r_obtener_pedido(estado_pedido);
	enviar_mensaje(connetion, R_OBTENER_PEDIDO, &estado_pedido);
	free(pedido->nombre);
	free(pedido);
}

void obtener_pedido(Paquete* package, int connetion){
	t_pedido* pedido = (t_pedido*)package->mensaje;
	log_pedido(pedido);
	t_estado_pedido estado_pedido = get_order_information(pedido->nombre, pedido->id_pedido);
	log_r_obtener_pedido(estado_pedido);
	enviar_mensaje(connetion, R_OBTENER_PEDIDO, &estado_pedido);
	free(pedido->nombre);
	free(pedido);
}

void obtener_restaurante(Paquete* package, int connetion){
	char* restaurant = (char*)package->mensaje;
	log_restaurante(restaurant);
	t_info_restaurante info_restaurante = get_restaurante_information(restaurant);
	log_r_obtener_restaurante(info_restaurante);
	enviar_mensaje(connetion, R_OBTENER_RESTAURANTE, &info_restaurante);
	free(restaurant);
}

void plato_listo(Paquete* package, int connetion){
	t_plato_listo* plato_listo = (t_plato_listo*)package->mensaje;
	log_plato_listo(plato_listo);
	bool success = food_ready(plato_listo->restaurante.nombre, plato_listo->restaurante.id_pedido, plato_listo->comida);
	log_bool(R_PLATO_LISTO, success);
	enviar_mensaje(connetion, R_PLATO_LISTO, &success);
	free(plato_listo->comida);
	free(plato_listo->restaurante.nombre);
	free(plato_listo);
}

void obtener_receta(Paquete* package, int connetion){
	char* nombre_receta = (char*)package->mensaje;
	log_receta(nombre_receta);
	t_list* pasos_receta = get_receta_information(nombre_receta);
	log_r_obtener_receta(pasos_receta);
	enviar_mensaje(connetion, R_OBTENER_RECETA, pasos_receta);
	free(nombre_receta);
}

void terminar_pedido(Paquete* package, int connetion){
	t_pedido* pedido = (t_pedido*)package->mensaje;
	log_pedido(pedido);
	bool success = change_state(pedido->nombre, pedido->id_pedido, "Terminado");
	log_bool(R_TERMINAR_PEDIDO, success);
	enviar_mensaje(connetion, R_TERMINAR_PEDIDO, &success);
	free(pedido->nombre);
	free(pedido);
}



/*
void sigchld_handler(int s){
     while(wait(NULL) > 0);
 }

void accion(void* socket){
	int socket_actual = *(int*) socket;
	Paquete paquete;
	while (RecibirPaqueteServidor(socket_actual, SINDICATO, &paquete) > 0) {
		if (paquete.header.quienEnvia == CLIENTE) {
			switch(paquete.header.tipoMensaje){
				case t_HANDSHAKE:{
					printf("Sindicato: Recibi un Handshake %s\n","Del Cliente");
					log_info(LOGGER,"Se agrego un nuevo Cliente");
					break;
				}
				case t_OBTENER_RESTAURANTE: {
					printf("Sindicato: Recibi un Obtener Restaurante %s\n","Del Cliente");

					// Deserializamos la estructura recibida (persona)
					//t_persona* persona = deserializar_persona(paquete.mensaje);

					// printf("Sindicato: Recibi el dni %d\n", persona->dni);
					// printf("Sindicato: Recibi la edad %d\n", persona->edad);
					// printf("Sindicato: Recibi el pasaporte %d\n", persona->pasaporte);
					// printf("Sindicato: Recibi el nombre %s\n", persona->nombre);

					// free(persona);

					break;
				}
				}
		}else{
			log_error(LOGGER,"No es ningún proceso Cliente.");
		}
		if (paquete.mensaje != NULL){
			free(paquete.mensaje);
			log_info(LOGGER,"Se libero la memoria del paquete.");
		}
	}
}


void crearServidor() {
	int sockfd,socket_sin; // Escuchar sobre sock_fd, nuevas conexiones sobre new_fd
	struct sockaddr_in my_addr;    // información sobre mi dirección
	struct sockaddr_in their_addr; // información sobre la dirección del cliente
	int sin_size;
	struct sigaction sa;
	int yes = 1;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log_error(LOGGER,"Socket: %s",strerror(errno));
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		log_error(LOGGER,"Setsockopt: %s",strerror(errno));
	}

	my_addr.sin_family = AF_INET;         // Ordenación de bytes de la máquina
	my_addr.sin_port = htons(puerto);    // short, Ordenación de bytes de la red
	my_addr.sin_addr.s_addr = inet_addr(server_ip); // Rellenar con mi dirección IP
	memset(&(my_addr.sin_zero), '\0', 8); // Poner a cero el resto de la estructura

	if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
		log_error(LOGGER,"Bind: %s",strerror(errno));
	}
	log_info(LOGGER,"El Servidor esta levantado esperando conexiones.");
	if (listen(sockfd, 10) == -1) {//revisar esto que solo acepta 10 conexiones
		log_error(LOGGER,"Listen: %s",strerror(errno));
	}

	sa.sa_handler = sigchld_handler; // Eliminar procesos muertos
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		log_error(LOGGER,"Sigaction: %s",strerror(errno));
	}

	while (true) {
		sin_size = sizeof(struct sockaddr_in);
		if ((socket_sin = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
			log_error(LOGGER,"Accept: %s",strerror(errno));
			continue;
		}
		log_info(LOGGER,"Se recibio una conexion de: %s",inet_ntoa(their_addr.sin_addr));

		t_hilo* itemNuevo = malloc(sizeof(t_hilo));
		itemNuevo->socket = socket_sin;
		pthread_create(&(itemNuevo->hilo), NULL, (void*)accion, &socket_sin);
	}

	close(socket_sin);
	log_info(LOGGER,"Se cerró el socket %d.",socket_sin);
}*

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

int size_of_content_to_write(int index_next_block, int quantity_total_blocks, int total_content_size){
	int size_of_content = block_size;

	if(index_next_block == quantity_total_blocks){
		size_of_content = total_content_size - ((quantity_total_blocks-1) * block_size);
	}
	return size_of_content;
}

void write_positions_on_block(char* block, char* data){

	char* path_block = string_from_format("%s/Blocks/%s.bin", punto_montaje, block);
	FILE* file = fopen(path_block, "wb+");

	if(file == NULL){
		log_error(LOGGER,"error al abrir el archivo %s",path_block);
	} else {
		size_t data_length = string_length(data);
		size_t bit_to_write = sizeof(*data);

		fwrite(data, bit_to_write, data_length, file);
		log_info(LOGGER, "Se escribe el bloque %s en el File System", block);
	}
	sleep(tiempo_retardo_operacion);
	fclose(file);
	free(path_block);
}

void remove_block_from_bitmap(char* block){
	pthread_mutex_lock(&mutexBitmap);
	int block_to_remove = atoi(block);
	bitarray_clean_bit(bitarray, block_to_remove);
	blocks_available++;

	log_info(LOGGER, "Cantidad de bloques disponibles: %d", blocks_available);
	log_info(LOGGER, "Se desocupa el bloque %d", block_to_remove);
	msync(bmap, sizeof(bitarray), MS_SYNC);
	pthread_mutex_unlock(&mutexBitmap);
}

t_list* write_blocks_and_metadata(int size_array_positions, char* array_positions, char* metadata_path){
	int quantity_of_blocks = my_ceil(size_array_positions, block_size);
	log_info(LOGGER, "Cantidad de bloques a ocupar: [%d] para %s", quantity_of_blocks, metadata_path);
	t_list* blocks = file_blocks(quantity_of_blocks, metadata_path, size_array_positions);

	for(int i=0; i < quantity_of_blocks; i++){
		int first_byte = i*block_size;
		int size_positions_to_write = size_of_content_to_write(i+1, quantity_of_blocks, size_array_positions);

		char* data = string_substring(array_positions, first_byte, size_positions_to_write);
		string_append(&data,"\0");
		char* block = list_get(blocks, i);

		log_info(LOGGER, "Se escriben [%d] bytes de registros, en el bloque: [%s] para %s", size_positions_to_write, block, metadata_path);

		write_positions_on_block(block, data);
		//log_info(LOGGER, "Bloque escrito satisfactoriamente");
		free(data);
	}
	return blocks;
}

char* remove_last_block_from_array(char* blocks_as_array){
	//Ej: blocks_as_array = [1,2,4] len=6
	//si queda vacio blocks_as_array = [1] len=3
	int length = string_length(blocks_as_array);
	blocks_as_array[length-1] = '\0';
	blocks_as_array[length-2] = '\0';
	blocks_as_array[length-3] = '\0';
	if(length == 3){
		//TODO:remueve el ultimo bloque y elimina el directorio
		int available_block = get_available_block();
		char* next_available_block = string_from_format("[%d]", available_block);
		string_append(&blocks_as_array,next_available_block);
	} else {
		char* close_block = "]\0";
		string_append(&blocks_as_array,close_block);
	}
	return blocks_as_array;
}

char* add_block_to_array(char* blocks_as_array, char* block_to_add){
	if(string_length(blocks_as_array)){
		blocks_as_array[(string_length(blocks_as_array))-1] = ',';
		string_append(&blocks_as_array,block_to_add);
		char* close_block = "]\0";
		string_append(&blocks_as_array,close_block);
	} else {
		blocks_as_array = string_from_format("[%s]\0",block_to_add);
	}
	return blocks_as_array;
}

char** metadata_blocks_to_actual_blocks(char* metadata_blocks){
	int blocks_len = string_length(metadata_blocks);
	int blocks_quantity = (blocks_len-1)/2;
	char** blocks = malloc(blocks_quantity);
	blocks[0] = string_from_format("%c", metadata_blocks[1]);
	if(blocks_quantity > 1){
		for(int i = 1; i<=(blocks_quantity*2);i+=2) {
			blocks[i] = string_from_format("%c", metadata_blocks[i+2]);
		}
	}

	return blocks;
}

int my_ceil(int a, int b){
	int resto = a % b;
	int retorno = a/b;

	return resto==0 ? retorno : (retorno+1);

	//devuelve la cantidad de blockes que necesitas en base a tu tamaño
}

t_list* file_blocks(int blocks_needed, char* metadata_path, int size_array_positions){
	pthread_mutex_lock(&mutexMetadataPath);
	t_config* metadata = config_create(metadata_path);
	char** actual_blocks = config_get_array_value(metadata, "BLOCKS");
	char* blocks_as_array_of_char = string_new();
	int quantity_actual_blocks = 0;
	t_list* blocks_as_char =  list_create();

	while(actual_blocks[quantity_actual_blocks]!=NULL){
		log_trace(LOGGER, "FILE_BLOCKS1 INICIO");
		list_add(blocks_as_char, actual_blocks[quantity_actual_blocks]);
		blocks_as_array_of_char = add_block_to_array(blocks_as_array_of_char, actual_blocks[quantity_actual_blocks]);
		quantity_actual_blocks++;
		log_trace(LOGGER, "FILE_BLOCKS1 FINAL");
	}

	int blocks_are_enough = blocks_needed - quantity_actual_blocks;
	while(blocks_are_enough > 0){
		log_trace(LOGGER, "FILE_BLOCKS2 INICIO");
		int new_block = get_available_block();
		blocks_are_enough -= 1;
		blocks_as_array_of_char = add_block_to_array(blocks_as_array_of_char, string_from_format("%d", new_block));
		quantity_actual_blocks++;

		list_add(blocks_as_char, string_from_format("%d",new_block));
		log_trace(LOGGER, "FILE_BLOCKS2 FINAL");
	}

	while(blocks_are_enough < 0){
		log_trace(LOGGER, "FILE_BLOCKS3 INICIO");
		remove_block_from_bitmap(actual_blocks[quantity_actual_blocks-1]);
		blocks_are_enough++;
		actual_blocks[quantity_actual_blocks-1] = '\0';
		blocks_as_array_of_char = remove_last_block_from_array(blocks_as_array_of_char);
		list_remove_and_destroy_element(blocks_as_char,quantity_actual_blocks-1, free);
		quantity_actual_blocks--;
		log_trace(LOGGER, "FILE_BLOCKS3 FINAL");
	}

	log_info(LOGGER,"Bloques a guardar para el archivo %s:%s",metadata_path, blocks_as_array_of_char);
	dictionary_put(metadata->properties, "BLOCKS", blocks_as_array_of_char);
	char* size_array = string_new();
	string_append_with_format(&size_array,"%d", size_array_positions);
	config_set_value(metadata,"SIZE", size_array);
	//TODO:
	*for(int i = 0;actual_blocks[i]!=NULL;i++){
		free(actual_blocks[i]);
	}*
	free(actual_blocks);
	config_save(metadata);
	config_destroy(metadata);
	pthread_mutex_unlock(&mutexMetadataPath);
	//TODO:free(blocks_as_array_of_char);
	free(size_array);

	return blocks_as_char;
}*/



void kill_mutexs(void){

}
