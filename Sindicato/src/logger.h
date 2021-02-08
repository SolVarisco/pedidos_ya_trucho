/*
 * logger.h
 *
 *  Created on: 8 dic. 2020
 *      Author: utnso
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<conexiones.h>

const static struct {
	codigo_comando codigo_comando;
	const char* str;
} conversion_codigo_comando[] = {
		{OBTENER_RESTAURANTE, "OBTENER_RESTAURANTE"},
		{CONSULTAR_PLATOS, "CONSULTAR_PLATOS"},
		{CREAR_PEDIDO, "CREAR_PEDIDO"},
		{CONFIRMAR_PEDIDO, "CONFIRMAR_PEDIDO"},
		{PLATO_LISTO, "PLATO_LISTO"},
		{OBTENER_PEDIDO, "OBTENER_PEDIDO"},
		{TERMINAR_PEDIDO, "TERMINAR_PEDIDO"},
		{OBTENER_RECETA, "OBTENER_RECETA"},
		{R_OBTENER_RESTAURANTE, "R_OBTENER_RESTAURANTE"},
		{R_CONSULTAR_PLATOS, "R_CONSULTAR_PLATOS"},
		{R_CREAR_PEDIDO, "R_CREAR_PEDIDO"},
		{R_CONFIRMAR_PEDIDO, "R_CONFIRMAR_PEDIDO"},
		{R_PLATO_LISTO, "R_PLATO_LISTO"},
		{R_OBTENER_PEDIDO, "R_OBTENER_PEDIDO"},
		{R_TERMINAR_PEDIDO, "R_TERMINAR_PEDIDO"},
		{R_OBTENER_RECETA, "R_OBTENER_RECETA"},
		{GUARDAR_PEDIDO, "GUARDAR_PEDIDO"},
		{GUARDAR_PLATO, "GUARDAR_PLATO"},
		{R_GUARDAR_PEDIDO, "R_GUARDAR_PEDIDO"},
		{R_GUARDAR_PLATO, "R_GUARDAR_PLATO"},
};

t_log* LOGGER;

// Recepcion y envio de mensajes
char* string_a_codigo_comando(codigo_comando comando_ingresado);
void log_mensaje(codigo_comando codigo_recibido, bool recibido);
void log_conexion(t_modulo modulo, char* contacto);
void log_restaurante(char* restaurant);
void log_r_consultar_platos(t_list* platos);
void log_pedido(t_pedido* pedido);
void log_bool(codigo_comando mensaje, bool exito);
void log_guardar_plato(t_plato* plato);
char* estado_a_char(t_estado estado);
void log_r_obtener_pedido(t_estado_pedido estado_pedido);
void log_r_obtener_restaurante(t_info_restaurante info_restaurante);
void log_plato_listo(t_plato_listo* plato_listo);
void log_receta(char* receta);
void log_r_obtener_receta(t_list* pasos_receta);
void listar_pasos(void* args);

// Creacion de archivos

void log_new_restaurant(char* file, char* path);
void log_new_recipe(char* recipe, char* path);
void log_new_order(char* restaurant, uint32_t id, char* path);

// Block

void log_block_asignation(char* file, int block);

#endif /* LOGGER_H_ */
