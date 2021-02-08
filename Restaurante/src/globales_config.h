/*
 * globales_config.h
 *
 *  Created on: 22 oct. 2020
 *      Author: utnso
 */

#ifndef GLOBALES_CONFIG_H_
#define GLOBALES_CONFIG_H_

#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
#include <conexiones.h>
#include <mensajes.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdint.h>

t_log* LOGGER;
t_config* CONFIG;
char* IP_ESCUCHA;
char* PUERTO_ESCUCHA;
char* IP_SINDICATO;
char* PUERTO_SINDICATO;
char* IP_APP;
char* PUERTO_APP;
int QUANTUM;
char* ARCHIVO_LOG;
char* ALGORITMO_PLANIFICACION;
char* NOMBRE_RESTAURANTE;
int RETARDO_CICLO_CPU;

bool SINDICATO_CONECTADO;
bool APP_CONECTADA;

int CONEXION_APP;

int HORNOS_ACTIVOS;
int COCINEROS_ACTIVOS;
int REPOSOS_ACTIVOS;
int CONTADOR_PLATOS;

sem_t sem_cocinero_accion;
extern pthread_mutex_t mutex_cocineros;

sem_t semaforo_hornos_disponibles;

//COORDENACION
sem_t sem_cocineros_cocinen;
sem_t sem_cocinero_ya_cocino;
sem_t sem_hornos_horneen;
sem_t sem_horno_ya_horneo;
sem_t sem_reposos_reposen;
sem_t sem_reposo_ya_reposo;
sem_t sem_coordinar_listo;
//COORDINACION

//Contacto Restaurante
t_contacto contacto_restaurante;

typedef struct {
	t_list* afinidad_cocineros;
	int posicion_X;
	int	posicion_Y;
	t_list* platos;
	int cant_hornos;
	int cant_pedidos;
} t_restaurante_data;

typedef struct {
	t_list* lista_ready;
	char* afinidad;
	int id_cocinero;
	pthread_mutex_t mutex_ready;
	int quantum;
} t_cocinero;

typedef struct {
	t_list* receta;
	char* afinidad;
	int id_plato;
	uint32_t id_pedido;
} t_pcb_pla;

t_info_restaurante* mi_restaurante;
t_list* cocineros;
t_list* sem_cocineros_ejecutar;
t_list* sem_cocineros_planificar;
t_list* sem_cocineros_cocinar;
t_list* sem_hornos_io;
t_list* platos_demo;



#endif /* GLOBALES_CONFIG_H_ */
