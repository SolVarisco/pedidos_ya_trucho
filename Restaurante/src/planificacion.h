/*
 * planificacion.h
 *
 *  Created on: 23 oct. 2020
 *      Author: utnso
 */

#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include "cocina.h"

typedef enum{
	FIFO = 1,
	RR = 2,
	ERROR_CODIGO_ALGORITMO = 9

}algoritmo_code;

const static struct {
	algoritmo_code codigo_algoritmo;
	const char* str;
} conversion_algoritmo[] = {
		{FIFO, "FIFO"},
		{RR, "RR"}
};

void dar_a_cocinero_por_afinidad(t_pcb_pla* pcb_pla);
void dar_a_cocinero_por_id(int id_cocinero, t_pcb_pla* pcb_pla);
algoritmo_code string_a_codigo_algoritmo(const char* string);
void planificar_segun(int id_cocinero);
void planificar_segun_fifo(int id_cocinero);
void planificar_segun_rr(int id_cocinero);
void notificar(t_pcb_pla* plato);
void reposar(int id_cocinero, t_pcb_pla* plato);
void hornear(int id_cocinero, t_pcb_pla* plato);
void planificador_por_turnos();

#endif /* PLANIFICACION_H_ */
