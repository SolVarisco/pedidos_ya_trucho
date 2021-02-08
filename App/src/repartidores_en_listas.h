/*
 * utils_planificacion.h
 *
 *  Created on: 15 sep. 2020
 *      Author: utnso
 */

#ifndef REPARTIDORES_EN_LISTAS_H_
#define REPARTIDORES_EN_LISTAS_H_

#include "planificacion.h"

t_list* hilos_repartidores;

uint32_t ID_REPARTIDORES;

void poner_repartidores_en_lista();
uint32_t generar_id();
t_repartidor* crear_repartidor(uint32_t id_repartidor, t_coordenadas* coordenadas, int frecuencia_descanso_repartidor, int tiempo_descanso_repartidor, status_code estado);
void free_array(char** array);
int tamano_array(char** array);

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// EJECUCION ////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void crear_hilos_repartidores();
void ejecutar_repartidor(t_repartidor* repartidor);
void mover_al_repartidor_hasta_destino(uint32_t id_repartidor);
int llego_al_destino(t_repartidor* repartidor);

#endif /* REPARTIDORES_EN_LISTAS_H_ */
