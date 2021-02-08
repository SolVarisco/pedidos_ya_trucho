/*
 * cliente.h
 *
 *  Created on: 1 sep. 2020
 *      Author: utnso
 */

#ifndef CLIENTE_H_
#define CLIENTE_H_

#include<readline/readline.h>
#include<pthread.h>
#include<semaphore.h>

#include "logger.h"

#define CLIENTE_NOMBRE_LOG "cliente.log"
#define CLIENTE_NIVEL_LOG LOG_LEVEL_INFO

// Threads
pthread_t thread;
pthread_t hilo_receptor;
pthread_mutex_t mutex_id = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_id_lista = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_handshake = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_handshake = PTHREAD_COND_INITIALIZER;
sem_t sem_pedidos;

// Variables que utilizan todos los hilos
char* IP_CONEXION;
char* PUERTO_CONEXION;
t_modulo modulo_conectado;
t_contacto cliente;
t_list* id_pedidos;
uint32_t id_actual;
uint32_t id_a_remover;
char* plato;
char* restaurante;

void mostrar_comandos();
void asigno_valores_conexion(void);
codigo_comando string_a_codigo_comando(const char* comando_ingresado);
void armar_y_enviar_paquete(codigo_comando codigo_de_proceso, int conexion);
void* receptor_de_mensajes(void* args);
bool gestionar_handshake(int conexion, int servidor);
void gestionar_mensaje(int conexion);
void* accion(void* args);
void loggear_valores_lista(t_list* self, t_log* logger);
void gestionar_lista_platos(t_estado_pedido* estado_pedido);
void terminar_programa(void);

#endif /* CLIENTE_H_ */
