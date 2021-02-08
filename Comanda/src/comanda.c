/*
 * comanda.c
 *
 *  Created on: 19 sep. 2020
 *      Author: utnso
 */

#include "comanda.h"

int main(void){
	int connetion;
	CONFIG = init_config();
	LOGGER = init_logger();
	if(!init_memory())
		return -1;

	comanda.modulo = COMANDA;
	comanda.ip = config_get_string_value(CONFIG, "IP_ESCUCHA");
	comanda.puerto = config_get_string_value(CONFIG, "PUERTO_ESCUCHA");

	int server = iniciar_servidor(comanda.ip, comanda.puerto);
	log_info(LOGGER, "El modulo comanda ya esta listo para recibir mensajes");

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

t_config* init_config(void) {
	return config_create("/home/utnso/tp-2020-2c-CabreadOS/Comanda/Debug/comanda.config");
	APP_CONECTADA = 0;
}

t_log* init_logger(void) {
	return log_create(
			config_get_string_value(CONFIG, "ARCHIVO_LOG"),
			"Comanda",
			true,
			LOG_LEVEL_INFO);
}

bool init_memory(void){
	int memory_size = config_get_int_value(CONFIG, "TAMANIO_MEMORIA");
	int swap_size = config_get_int_value(CONFIG, "TAMANIO_SWAP");
	char* algorithim = config_get_string_value(CONFIG, "ALGORITMO_REEMPLAZO");
	bool success = create_memory(memory_size, swap_size, algorithim, LOGGER);

	free(algorithim);

	return success;
}

void* customer_suport(void* args){
	int* connetion = (int*) args;
	Paquete* new_package = recibir_paquete(*connetion, comanda);

	action(new_package, *connetion);
	liberar_conexion(*connetion);

	return NULL;
}

void action(Paquete* package, int connetion){
	switch(package->header.tipo_mensaje){
		case HANDSHAKE:
			gestionar_handshake(package);
			break;
		case GUARDAR_PEDIDO:
			guardar_pedido(package, connetion);
			break;
		case GUARDAR_PLATO:
			guardar_plato(package, connetion);
			break;
		case OBTENER_PEDIDO:
			obtener_pedido(package, connetion);
			break;
		case CONFIRMAR_PEDIDO:
			confirmar_pedido(package, connetion);
			break;
		case PLATO_LISTO:
			plato_listo(package, connetion);
			break;
		case FINALIZAR_PEDIDO:
			finalizar_pedido(package, connetion);
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
			log_info(LOGGER, "Se recibio un Handshake del cliente: %s\n", cliente->id_cliente);
			break;
		case APP:
			log_info(LOGGER, "Se recibio un Handshake del modulo APP");
			APP_CONECTADA = 1;
			break;
		default:
			break;
	}
}

void guardar_pedido(Paquete* package, int connection){
	t_pedido* pedido = (t_pedido*)package->mensaje;
	bool success = create_order(pedido->nombre, pedido->id_pedido, LOGGER);
	enviar_mensaje(connection, R_GUARDAR_PEDIDO, &success);
	free(pedido);
}

void guardar_plato(Paquete* package, int connection){
	t_plato* plato = (t_plato*)package->mensaje;
	bool success = add_food(plato->restaurante.nombre, plato->restaurante.id_pedido, plato->comida, plato->cantidad, LOGGER);
	enviar_mensaje(connection, R_GUARDAR_PLATO, &success);
	free(plato);
}


void obtener_pedido(Paquete* package, int connection){
	t_pedido* pedido = (t_pedido*)package->mensaje;
	t_estado_pedido estado_pedido = read_order_pages(pedido->nombre, pedido->id_pedido, LOGGER);
	enviar_mensaje(connection, R_OBTENER_PEDIDO, &estado_pedido);
	free(pedido);
}

void confirmar_pedido(Paquete* package, int connection){
	t_pedido* pedido = (t_pedido*)package->mensaje;
	bool success = confirm_order(pedido->nombre, pedido->id_pedido, LOGGER);
	enviar_mensaje(connection, R_CONFIRMAR_PEDIDO, &success);
	free(pedido);
}

void plato_listo(Paquete* package, int connection){
	t_plato_listo* plato = (t_plato_listo*)package->mensaje;
	bool success = food_ready(plato->restaurante.nombre, plato->restaurante.id_pedido, plato->comida, LOGGER);
	if(APP_CONECTADA == 1) {
		t_estado_pedido estado_pedido = read_order_pages(plato->restaurante.nombre, plato->restaurante.id_pedido, LOGGER);
		enviar_mensaje(connection, R_OBTENER_PEDIDO, &estado_pedido);
	} else {
		enviar_mensaje(connection, R_PLATO_LISTO, &success);
	}
	free(plato);
}

void finalizar_pedido(Paquete* package, int connection){
	t_pedido* pedido = (t_pedido*)package->mensaje;
	bool success = finish_order(pedido->nombre, pedido->id_pedido, LOGGER);
	enviar_mensaje(connection, R_FINALIZAR_PEDIDO, &success);
	free(pedido);
}

void kill_mutexs(void){
	pthread_mutex_destroy(&mutex_segment_table);
	pthread_mutex_destroy(&mutex_pages_in_memory);
	pthread_mutex_destroy(&mutex_swap_bitmap);
	pthread_mutex_destroy(&mutex_mem_bitmap);
}
