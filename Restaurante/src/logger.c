/*
 * logger.c
 *
 *  Created on: 22 oct. 2020
 *      Author: utnso
 */
#include "logger.h"

pthread_mutex_t mutex_logger = PTHREAD_MUTEX_INITIALIZER;

void log_error_comunicacion_con_sindicato() {
	char* log_msg = "Hubo un error de comunicación con Sindicato";

	log_info(LOGGER,log_msg);
}

void log_error_comunicacion_con_app() {
	char* log_msg = "Hubo un error de comunicación con App";

	log_info(LOGGER,log_msg);
}

void log_nuevo_pcb(int pcb_id, int pcb_pedido_id, char* comida) {
	char* log_msg = "Se creo el nuevo pcb con id %d del pedido %d, para la comida %s";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, pcb_id, pcb_pedido_id, comida);
	pthread_mutex_unlock(&mutex_logger);
}

void log_asignacion_a_cocinero(int pcb_id, int cocinero_id, char* afinidad_cocinero) {
	char* log_msg = "Se asigno el pcb con id %d, al cocinero %d, de afinidad %s";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, pcb_id, cocinero_id, afinidad_cocinero);
	pthread_mutex_unlock(&mutex_logger);
}

void log_fin_de_quantum(int pcb_id, int tiempo_reposo) {
	char* log_msg = "El plato con id %d, se quedo sin tiempo de quantum faltando %d ciclos para terminar el paso actual";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, pcb_id, tiempo_reposo);
	pthread_mutex_unlock(&mutex_logger);
}

void log_reposar_por(int pcb_id, int tiempo_reposo) {
	char* log_msg = "El plato con id %d, se mando a reposar por %d";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, pcb_id, tiempo_reposo);
	pthread_mutex_unlock(&mutex_logger);
}

void log_hornear_por(int pcb_id, int tiempo_horneo) {
	char* log_msg = "El plato con id %d, se mando a hornear por %d";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, pcb_id, tiempo_horneo);
	pthread_mutex_unlock(&mutex_logger);
}

void log_accion_status(int pcb_id, char* nombre, int tiempo_pasado) {
	char* log_msg = "Plato id: %d | Paso: %s | tiempo: %d";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, pcb_id, nombre, tiempo_pasado);
	pthread_mutex_unlock(&mutex_logger);
}

void log_plato_terminado(int pcb_id) {
	char* log_msg = "Finalizado el plato con id %d";

	pthread_mutex_lock(&mutex_logger);
	log_info(LOGGER,log_msg, pcb_id);
	pthread_mutex_unlock(&mutex_logger);
}
