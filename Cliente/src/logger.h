/*
 * logger.h
 *
 *  Created on: 24 oct. 2020
 *      Author: utnso
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include<commons/string.h>
#include<commons/config.h>
#include<conexiones.h>

#define CLIENTE_NOMBRE_LOG "cliente.log"
#define CLIENTE_NIVEL_LOG LOG_LEVEL_INFO

t_config* CONFIG;
t_log* LOGGER;

const static struct {
	codigo_comando codigo_comando;
	const char* str;
} conversion_codigo_comando[] = {
		{CONSULTAR_RESTAURANTES, "CONSULTAR_RESTAURANTES"},
		{SELECCIONAR_RESTAURANTE, "SELECCIONAR_RESTAURANTE"},
		{OBTENER_RESTAURANTE, "OBTENER_RESTAURANTE"},
		{CONSULTAR_PLATOS, "CONSULTAR_PLATOS"},
		{CREAR_PEDIDO, "CREAR_PEDIDO"},
		{GUARDAR_PEDIDO, "GUARDAR_PEDIDO"},
		{ANIADIR_PLATO, "AÑADIR_PLATO"},
		{GUARDAR_PLATO, "GUARDAR_PLATO"},
		{CONFIRMAR_PEDIDO, "CONFIRMAR_PEDIDO"},
		{PLATO_LISTO, "PLATO_LISTO"},
		{CONSULTAR_PEDIDO, "CONSULTAR_PEDIDO"},
		{OBTENER_PEDIDO, "OBTENER_PEDIDO"},
		{TERMINAR_PEDIDO, "TERMINAR_PEDIDO"},
		{OBTENER_RECETA, "OBTENER_RECETA"},
		{FINALIZAR_PEDIDO, "FINALIZAR_PEDIDO"},
		{R_CONSULTAR_RESTAURANTES, "R_CONSULTAR_RESTAURANTES"},
		{R_SELECCIONAR_RESTAURANTE, "R_SELECCIONAR_RESTAURANTE"},
		{R_OBTENER_RESTAURANTE, "R_OBTENER_RESTAURANTE"},
		{R_CONSULTAR_PLATOS, "R_CONSULTAR_PLATOS"},
		{R_CREAR_PEDIDO, "R_CREAR_PEDIDO"},
		{R_GUARDAR_PEDIDO, "R_GUARDAR_PEDIDO"},
		{R_ANIADIR_PLATO, "R_AÑADIR_PLATO"},
		{R_GUARDAR_PLATO, "R_GUARDAR_PLATO"},
		{R_CONFIRMAR_PEDIDO, "R_CONFIRMAR_PEDIDO"},
		{R_PLATO_LISTO, "R_PLATO_LISTO"},
		{R_CONSULTAR_PEDIDO, "R_CONSULTAR_PEDIDO"},
		{R_OBTENER_PEDIDO, "R_OBTENER_PEDIDO"},
		{R_FINALIZAR_PEDIDO, "R_FINALIZAR_PEDIDO"},
		{R_TERMINAR_PEDIDO, "R_TERMINAR_PEDIDO"},
		{R_OBTENER_RECETA, "R_OBTENER_RECETA"},

};

void leer_config(void);
void iniciar_logger(void);
char* codigo_comando_a_string(codigo_comando comando_ingresado);

void log_codigo(codigo_comando comando_ingresado, bool enviado);
void log_seleccionar_restaurante(codigo_comando comando_ingresado, t_seleccionar_restaurante seleccionar_restaurante);
void loggear_restaurante(char* restaurante);
void loggear_platos(char* plato);
void log_restaurante(codigo_comando comando_ingresado, char* restaurante, bool enviado);
void log_pedido(codigo_comando comando_ingresado, t_pedido pedido, bool enviado);
void log_plato(codigo_comando comando_ingresado, t_plato plato_seleccionado);
void log_plato_guardado(codigo_comando comando_ingresado, t_plato plato_guardado);
void log_plato_listo(codigo_comando comando_ingresado, t_plato_listo plato_listo);
void log_id(codigo_comando comando_ingresado, uint32_t id, bool eviado);
void log_receta(codigo_comando comando_ingresado, char* receta);
void log_bool(codigo_comando comando_ingresado, bool operacion_exitosa);
void log_estado(char* estado);
void log_estado_comida(t_estado_comida comida);
void log_obtener_restaurante(t_info_restaurante info_restaurante);

#endif /* LOGGER_H_ */
