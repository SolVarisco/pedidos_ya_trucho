/*
 * logger.c
 *
 *  Created on: 26 sep. 2020
 *      Author: utnso
 */

#include "logger.h"

pthread_mutex_t mutex_logger = PTHREAD_MUTEX_INITIALIZER;

void log_repartidor_cambio_de_cola_planificacion(uint32_t id_repartidor, char* razon, char* cola) {
	char* log_msg = "El repartidor con ID %d cambió a la cola %s porque %s";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, id_repartidor, cola, razon);
	pthread_mutex_unlock(&mutex_logger);
}

void log_pedido_finalizado(uint32_t id_repartidor) {
	char* log_msg = "El repartidor con ID %d quedo LIBRE tras FINALIZAR de repartir el pedido";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, id_repartidor);
	pthread_mutex_unlock(&mutex_logger);
}

void log_movimiento_repartidor(uint32_t id, uint32_t id_pedido, uint32_t coord_x, uint32_t coord_y) {
	char* log_msg = "El repartidor ID %d con pedido ID %d se movio a posición -> [%d,%d]";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, id, id_pedido, coord_x, coord_y);
	pthread_mutex_unlock(&mutex_logger);
}

void log_repartidor_elegido_para_pedido(uint32_t id, uint32_t id_pedido, char* ip, uint32_t coord_x, uint32_t coord_y) {
	char* log_msg = "Al repartidor con ID %d se le asigno el pedido %d del restaurante %s en la posicion [%d,%d]";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, id, id_pedido, ip, coord_x, coord_y);
	pthread_mutex_unlock(&mutex_logger);
}

void log_error_comunicacion_con_comanda() {
	char* log_msg = "Hubo un error de comunicación con Comanda";

	log_info(LOGGER,log_msg);
}
