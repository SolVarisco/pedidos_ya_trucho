/*
 * logger.c
 *
 *  Created on: 24 oct. 2020
 *      Author: utnso
 */

#include "logger.h"

void leer_config(void) {
	CONFIG = config_create("/home/utnso/tp-2020-2c-CabreadOS/Cliente/Debug/cliente.config");
}

void iniciar_logger(void) {
	LOGGER = log_create(
			config_get_string_value(CONFIG, "ARCHIVO_LOG"),
			CLIENTE_NOMBRE_LOG,
			0,
			CLIENTE_NIVEL_LOG);
}

char* codigo_comando_a_string(codigo_comando comando_ingresado) {
	char* codigo_actual;
	for(int i = 0; i < 30; i++)
	{
		if(comando_ingresado == conversion_codigo_comando[i].codigo_comando){
			codigo_actual = malloc(strlen(conversion_codigo_comando[i].str)+1);
			strcpy(codigo_actual, conversion_codigo_comando[i].str);
			return codigo_actual;
		}
	}
	codigo_actual = malloc(sizeof("ERROR_PROCESO")+1);
	return codigo_actual;
}

void log_codigo(codigo_comando comando_ingresado, bool enviado){
	if(enviado){
		log_info(LOGGER, "Mensaje enviado");
	} else {
		log_info(LOGGER, "Mensaje recibido");
	}

	log_info(LOGGER, "%s", codigo_comando_a_string(comando_ingresado));
}

void log_seleccionar_restaurante(codigo_comando comando_ingresado, t_seleccionar_restaurante seleccionar_restaurante){
	log_codigo(comando_ingresado, 1);
	log_info(LOGGER, "\tRestaurante: %s\t ID Cliente: %s", seleccionar_restaurante.nombre_restaurante, seleccionar_restaurante.cliente.id_cliente);
}

void loggear_restaurante(char* restaurante){
	log_info(LOGGER, "\tRestaurante: %s", restaurante);
}


void loggear_platos(char* plato){
	log_info(LOGGER, "\tPlato: %s", plato);
}

void log_restaurante(codigo_comando comando_ingresado, char* restaurante, bool enviado){
	log_codigo(comando_ingresado, enviado);
	loggear_restaurante(restaurante);
}

void log_pedido(codigo_comando comando_ingresado, t_pedido pedido, bool enviado){
	log_codigo(comando_ingresado, enviado);
	log_info(LOGGER, "\tRestaurante: %s\t ID Pedido: %d", pedido.nombre, pedido.id_pedido);
}

void log_plato(codigo_comando comando_ingresado, t_plato plato_seleccionado){
	log_codigo(comando_ingresado, 1);
	log_info(LOGGER, "\tPlato: %s\t ID Pedido: %d", plato_seleccionado.comida, plato_seleccionado.restaurante.id_pedido);
}

void log_plato_guardado(codigo_comando comando_ingresado, t_plato plato_guardado){
	log_codigo(comando_ingresado, 1);
	log_info(LOGGER, "\tRestaurante: %s\ţ ID Pedido: %d\t Plato: %s\t Cantidad: %d", plato_guardado.restaurante.nombre, plato_guardado.restaurante.id_pedido, plato_guardado.comida, plato_guardado.cantidad);
}

void log_plato_listo(codigo_comando comando_ingresado, t_plato_listo plato_listo){
	log_codigo(comando_ingresado, 1);
	log_info(LOGGER, "\tRestaurante: %s\t ID Pedido: %d\t Plato: %s", plato_listo.restaurante.nombre, plato_listo.restaurante.id_pedido, plato_listo.comida);
}

void log_id(codigo_comando comando_ingresado, uint32_t id, bool eviado){
	log_codigo(comando_ingresado, eviado);
	log_info(LOGGER, "\tID: %d", id);
}

void log_receta(codigo_comando comando_ingresado, char* receta){
	log_codigo(comando_ingresado, 1);
	log_info(LOGGER, "\tReceta: %s", receta);
}

void log_bool(codigo_comando comando_ingresado, bool operacion_exitosa){
	log_codigo(comando_ingresado, 0);
	char* valor_a_loggear;

	if (operacion_exitosa){
		valor_a_loggear = "Operacion exitosa";
		log_info(LOGGER, "\t%s", valor_a_loggear);
	} else {
		valor_a_loggear = "No se pudo realizar la accion correctamente";
		log_warning(LOGGER, "\t%s", valor_a_loggear);
	}

	printf("%s\n", valor_a_loggear);

}

void log_estado(char* estado){
	log_info(LOGGER, "\tEstado: %s", estado);
}

void log_estado_comida(t_estado_comida comida){
	log_info(LOGGER, "\tPlato: %s\tTotal: %d\tListo: %d\n", comida.comida, comida.total, comida.listo);
}

void log_obtener_restaurante(t_info_restaurante info_restaurante){
	log_info(LOGGER, "t\Coordenadas -> (%d, %d)", info_restaurante.coordenadas.pos_x, info_restaurante.coordenadas.pos_y);
	log_info(LOGGER, "t\Cantidad hornos: %d\n\tCantidad cocineros: %d\n\tCantidad pedidos: %d", info_restaurante.cantidad_hornos, info_restaurante.cantidad_cocineros, info_restaurante.cantidad_pedidos);
	log_info(LOGGER, "t\Recetas:");

	void log_receta(void* args){
		t_receta* receta = (t_receta*) args;
		log_info(LOGGER, "\t\tNombre: %s\tPrecio: %d", receta->nombre, receta->precio);
	}

	list_iterate(info_restaurante.recetas, (void*)log_receta);
	log_info(LOGGER, "t\Afinidades:");

}
