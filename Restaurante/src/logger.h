/*
 * logger.h
 *
 *  Created on: 22 oct. 2020
 *      Author: utnso
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "globales_config.h"

void log_error_comunicacion_con_sindicato();
void log_error_comunicacion_con_app();
void log_nuevo_pcb(int pcb_id, int pcb_pedido_id, char* comida);
void log_asignacion_a_cocinero(int pcb_id, int cocinero_id, char* afinidad_cocinero);
void log_fin_de_quantum(int pcb_id, int tiempo_reposo);
void log_reposar_por(int pcb_id, int tiempo_reposo);
void log_hornear_por(int pcb_id, int tiempo_horneo);
void log_accion_status(int pcb_id, char* nombre, int tiempo_pasado);
void log_plato_terminado(int pcb_id);

#endif /* LOGGER_H_ */
