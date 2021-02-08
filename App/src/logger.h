/*
 * logger.h
 *
 *  Created on: 26 sep. 2020
 *      Author: utnso
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "globales_config.h"

void log_repartidor_cambio_de_cola_planificacion(uint32_t id_repartidor, char* razon, char* cola);
void log_pedido_finalizado(uint32_t id_repartidor);
void log_movimiento_repartidor(uint32_t id, uint32_t id_pedido, uint32_t coord_x, uint32_t coord_y);
void log_repartidor_elegido_para_pedido(uint32_t id, uint32_t id_pedido, char* ip, uint32_t coord_x, uint32_t coord_y);
void log_error_comunicacion_con_comanda();

#endif /* LOGGER_H_ */
