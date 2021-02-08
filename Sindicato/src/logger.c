/*
 * logger.c
 *
 *  Created on: 8 dic. 2020
 *      Author: utnso
 */

#include "logger.h"

// Recepción y envio de mensajes

char* string_a_codigo_comando(codigo_comando comando_ingresado) {
	char* codigo_actual;
	for(int i = 0; i < sizeof(conversion_codigo_comando) / sizeof(conversion_codigo_comando[0]); i++){
		if(comando_ingresado == conversion_codigo_comando[i].codigo_comando){
			codigo_actual = malloc(strlen(conversion_codigo_comando[i].str)+1);
			strcpy(codigo_actual, conversion_codigo_comando[i].str);
			break;
		}
	}
	return codigo_actual;
}

void log_mensaje(codigo_comando codigo_recibido, bool recibido){
	char* codigo_actual = string_a_codigo_comando(codigo_recibido);
	if (recibido){
		log_info(LOGGER, "Se recibio el mensaje %s", codigo_actual);
	} else {
		log_info(LOGGER, "Se envio el mensaje %s", codigo_actual);
	}

	free(codigo_actual);
}


void log_conexion(t_modulo modulo, char* contacto){
	switch(modulo){
		case CLIENTE:
			log_info(LOGGER, "Se establecio una conexion con el cliente %s", contacto);
			break;
		case RESTAURANTE:
			log_info(LOGGER, "Se establecio una conexion con el restaurante %s", contacto);
			break;
		default: break;
	}
}

void log_restaurante(char* restaurant){
	log_info(LOGGER, "\tRestaurante: %s", restaurant);
}

void log_r_consultar_platos(t_list* platos){
	log_mensaje(R_CONSULTAR_PLATOS, 0);

	void listar_platos(char* plato){
		log_info(LOGGER, "\t%s", plato);
	}

	list_iterate(platos, (void*)listar_platos);
}

void log_pedido(t_pedido* pedido){
	log_info(LOGGER, "\tRestaurante: %s\tPedido %d", pedido->nombre, pedido->id_pedido);
}

void log_bool(codigo_comando mensaje, bool exito){
	log_mensaje(mensaje, 0);
	if(exito){
		log_info(LOGGER, "\tOperación exitosa");
	} else {
		log_info(LOGGER, "\tNo se pudo concretar la operación");
	}
}

void log_guardar_plato(t_plato* plato){
	log_info(LOGGER, "\tRestaurante: %s\tPedido: %d\tPlato: %s\tCantidad: %d", plato->restaurante.nombre, plato->restaurante.id_pedido, plato->comida, plato->cantidad);
}

char* estado_a_char(t_estado estado){
	char* string;
	switch(estado){
		case PENDIENTE:
			string = malloc(strlen("Pendiente")+1);
			strcpy(string, "Pendiente");
			break;
		case CONFIRMADO:
			string = malloc(strlen("Confirmado")+1);
			strcpy(string, "Confirmado");
			break;
		case TERMINADO:
			string = malloc(strlen("Terminado")+1);
			strcpy(string, "Terminado");
			break;
		case INEXISTENTE:
			string = malloc(strlen("Inexistente")+1);
			strcpy(string, "Inexistente");
			break;
	}
	return string;
}

void log_r_obtener_pedido(t_estado_pedido estado_pedido){
	log_mensaje(R_OBTENER_PEDIDO, 0);
	char* estado = estado_a_char(estado_pedido.estado);
	log_info(LOGGER, "\tEstado: %s", estado);
	log_info(LOGGER, "\tLista platos:");

	void listar_estado_platos(void* args){
		t_estado_comida* plato = (t_estado_comida*)args;
		log_info(LOGGER, "\t\tPlato: %s\tTotal: %d\tListo: %d\n", plato->comida, plato->total, plato->listo);
	}

	list_iterate(estado_pedido.platos, (void*)listar_estado_platos);
	free(estado);
}

void log_r_obtener_restaurante(t_info_restaurante info_restaurante){

	void listar_afinidades(char* afinidad){
		log_info(LOGGER, "\t\t%s", afinidad);
	}

	void listar_recetas(void* arg){
		t_receta* receta = (t_receta*)arg;
		log_info(LOGGER, "\t\tPlato: %s\tPrecio: %d", receta->nombre, receta->precio);
	}

	log_mensaje(R_OBTENER_RESTAURANTE, 0);

	log_info(LOGGER, "\tCantidad de cocineros: %d\tCantidad de hornos: %d\tCantidad de pedidos: %d", info_restaurante.cantidad_cocineros, info_restaurante.cantidad_hornos, info_restaurante.cantidad_pedidos);
	log_info(LOGGER, "\tCoordenadas -> (%d, %d)", info_restaurante.coordenadas.pos_x, info_restaurante.coordenadas.pos_y);
	log_info(LOGGER, "\tAfinidades:");
	list_iterate(info_restaurante.afinidades, (void*)listar_afinidades);
	log_info(LOGGER, "\tRecetas:");
	list_iterate(info_restaurante.recetas, (void*)listar_recetas);
}

void log_plato_listo(t_plato_listo* plato_listo){
	log_info(LOGGER, "\tRestaurante: %s\tPedido: %d\tPlato: %s", plato_listo->restaurante.nombre, plato_listo->restaurante.id_pedido, plato_listo->comida);
}

void log_receta(char* receta){
	log_info(LOGGER, "\tReceta: %s", receta);
}

void log_r_obtener_receta(t_list* pasos_receta){
	void listar_pasos(void* args){
		t_paso* paso = (t_paso*) args;
		log_info(LOGGER, "\t\tPaso: %s\tTiempo: %d", paso->nombre, paso->tiempo_requerido);
	}

	log_info(LOGGER, "Pasos a seguir:");
	list_iterate(pasos_receta, (void*)listar_pasos);
}

// Nuevos archivos

void log_new_restaurante(char* restaurant, char* path){
	log_info(LOGGER,"Se creo el archivo \"Info.AFIP\" del restaurante %s (path: %s).", restaurant, path);
}

void log_new_recipe(char* recipe, char* path){
	log_info(LOGGER,"Se creo el archivo \"%s.AFIP\" (path: %s).", recipe, path);
}

void log_new_order(char* restaurant, uint32_t id, char* path){
	log_info(LOGGER,"Se creo el archivo \"Pedido%d.AFIP\" del restaurante %s (path: %s).", id, restaurant, path);
}

// Bloques

void log_block_asignation(char* file, int block_number){
	log_info(LOGGER, "Asignacipn del bloque %d al archivo %s", block_number, file);
}
