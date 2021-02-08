/*
 * comanda.h
 *
 *  Created on: 19 sep. 2020
 *      Author: utnso
 */

#ifndef COMANDA_H_
#define COMANDA_H_

#include<commons/string.h>
#include "memory.h"

//Utilities
t_config* CONFIG;
t_log* LOGGER;
t_contacto comanda;
t_list* SEGMENT_TABLES;

// Threads
pthread_t thread;

int APP_CONECTADA;

//Mutexes (se usan en otros archivos como "extern")

pthread_mutex_t mutex_segment_table = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_pages_in_memory = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_mem_bitmap = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_swap_bitmap = PTHREAD_MUTEX_INITIALIZER;

// Functions
t_config* init_config(void);
t_log* init_logger(void);
bool init_memory(void);
void* customer_suport(void* args);
void action(Paquete* package, int connection);

void gestionar_handshake(Paquete* package);
void guardar_pedido(Paquete* package, int connection);
void guardar_plato(Paquete* package, int connection);
void obtener_pedido(Paquete* package, int connection);
void confirmar_pedido(Paquete* package, int connection);
void plato_listo(Paquete* package, int connection);
void finalizar_pedido(Paquete* package, int connection);

void kill_mutexs(void);

#endif /* COMANDA_H_ */
