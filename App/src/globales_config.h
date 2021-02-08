/*
 * globales_config.h
 *
 *  Created on: 15 sep. 2020
 *      Author: utnso
 */

#include<commons/config.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<mensajes.h>
#include<stdlib.h>
#include<semaphore.h>
#include<pthread.h>
#include<string.h>
#include<unistd.h>
#include<conexiones.h>

#define APP_CONFIG "/home/utnso/tp-2020-2c-CabreadOS/App/Debug/app.config"

t_contacto contacto_app;

//COORDENACION
int CONTADOR_PEDIDOS_REPARTIBLES;
int REPARTIDORES_ACTIVOS;
int DESCANZOS_ACTIVOS;

sem_t sem_coordinar_listo;
sem_t sem_repartidores_mov;
sem_t sem_repartidor_ya_mov;
sem_t sem_repartidor_descanse;
sem_t sem_repartidor_ya_descanso;
//COORDINACION

t_config* CONFIG;
t_log* LOGGER;
char* IP_COMANDA;
char* PUERTO_COMANDA;
char* IP_ESCUCHA;
char* PUERTO_ESCUCHA;
int RETARDO_CICLO_CPU;
int GRADO_DE_MULTIPROCESAMIENTO;
char* ALGORITMO_DE_PLANIFICACION;
double ALPHA;
int ESTIMACION_INICIAL;
char* ARCHIVO_LOG;
char** PLATOS_DEFAULT;
int POSICION_REST_DEFAULT_X;
int POSICION_REST_DEFAULT_Y;
int ESTIMACION_INICIAL;

bool COMANDA_CONECTADA;
