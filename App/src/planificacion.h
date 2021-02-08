/*
 * planificacion.h
 *
 *  Created on: 26 sep. 2020
 *      Author: utnso
 */

#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include "logger.h"

int cantidad_cambios_contexto;

t_list* repartidores;
t_list* lista_estado_new;
t_list* lista_estado_ready;
t_list* lista_estado_bloqueado_descanso;
t_list* lista_estado_bloqueado_esperando_pedido;
t_list* lista_estado_finished;

t_list* lista_espera_a_default;

extern pthread_mutex_t mutex_lista_nuevos;
extern pthread_mutex_t mutex_lista_ready;
extern pthread_mutex_t mutex_lista_bloqueado_esperando_pedido;
extern pthread_mutex_t mutex_lista_bloqueado_descanso;
extern pthread_mutex_t mutex_clientes_registrados;

t_list* sem_repartidores_ejecutar;
t_list* sem_repartidores_ejecutar_moviendose;
t_list* clientes_registrados;
sem_t sem_planificar;
//sem_t sem_repartidor_moviendose;
sem_t sem_buscar_repartidor_mas_cercano;

extern pthread_mutex_t mutex_cantidad_cambios_contexto;

extern pthread_mutex_t mutex_repartidores_activos;


typedef struct {
	char* nombre;
	uint32_t socket;
	t_coordenadas* coordenadas;
	char* ip;
	char* puerto;
} t_restaurante;

typedef struct {
	char* id_cliente;
	char* ip;
	char* puerto;
} t_cliente_registrado;

typedef struct {
	char* id_cliente;
	char* nombre_restaurante;
	uint32_t socket_restaurante;
	char* ip_restaurante;
	char* puerto_restaurante;
	t_coordenadas* coordenadas_cliente;
	t_coordenadas* coordenadas_restaurante;
} t_cliente_x_restaurante;

typedef struct {
	uint32_t id;
	char* id_cliente;
	char* nombre_restaurante;
	uint32_t socket_restaurante;
	char* ip_restaurante;
	char* puerto_restaurante;
	t_coordenadas* coordenadas_cliente;
	t_coordenadas* coordenadas_restaurante;
	sem_t sem_esperando_pedido;
} t_pcb_ped;

typedef enum {
	NEW = 1,
	READY = 2,
	BLOCKED = 3,
	EXEC = 4,
	FINISHED = 5
} status_code;

typedef struct {
	uint32_t id_repartidor;
	t_coordenadas* coordenadas;
	status_code estado;
	uint32_t camino_a_cliente;
	uint32_t mis_ciclos_CPU;
	uint32_t frecuencia_de_descanzo;
	uint32_t tiempo_de_descanzo;
	double rafaga_anterior_real;
	double estimacion_inicial;
	t_pcb_ped* pedido_asignado;
	uint32_t ciclos_hasta_descanso;
	uint32_t wait_time;
} t_repartidor;

typedef enum{
	FIFO = 1,
	HRRN = 2,
	SJF = 3,
	ERROR_CODIGO_ALGORITMO = 9

}algoritmo_code;

const static struct {
	algoritmo_code codigo_algoritmo;
	const char* str;
} conversion_algoritmo[] = {
		{FIFO, "FIFO"},
		{HRRN, "HRRN"},
		{SJF, "SJF"}
};

t_repartidor* repartidor_mas_cercano(t_pcb_ped* pcb_pedido);
int distancia_entre(t_coordenadas* desde, t_coordenadas* hasta);
void sacar_repartidor_de_lista(t_repartidor* repartidor, t_list* lista);
void pedido_listo(uint32_t id_pedido);
void descansar(t_repartidor* repartidor);

void poner_repartidor_en_ready(t_repartidor* repartidor, t_pcb_ped* pedido, char* razon);
void poner_repartidor_en_new(t_repartidor* repartidor);
void poner_repartidor_en_block_espera_pedido(t_repartidor* repartidor);

void planificar_segun();
algoritmo_code string_a_codigo_algoritmo(const char* string);
void planificar_segun_fifo();
void planificar_segun_hrrn();
void planificar_segun_sjf();

void ordenar_lista_por_estimacion_hrrn(t_list* list);
void ordenar_lista_por_estimacion_sjf(t_list* list);
void incrementar_wait_time(t_list* list);

void planificador_por_turnos();

#endif /* PLANIFICACION_H_ */
