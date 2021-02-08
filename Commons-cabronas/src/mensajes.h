/*
 * mensajes.h
 *
 *  Created on: 5 sep. 2020
 *      Author: utnso
 */

#ifndef MENSAJES_H_
#define MENSAJES_H_

#include <stdint.h>
#include<commons/collections/list.h>

typedef enum {
	HANDSHAKE,
	CONSULTAR_RESTAURANTES,
	SELECCIONAR_RESTAURANTE,
	OBTENER_RESTAURANTE,
	CONSULTAR_PLATOS,
	CREAR_PEDIDO,
	GUARDAR_PEDIDO,
	ANIADIR_PLATO,
	GUARDAR_PLATO,
	CONFIRMAR_PEDIDO,
	PLATO_LISTO,
	CONSULTAR_PEDIDO,
	OBTENER_PEDIDO,
	FINALIZAR_PEDIDO,
	TERMINAR_PEDIDO,
	OBTENER_RECETA,
	R_CONSULTAR_RESTAURANTES,
	R_SELECCIONAR_RESTAURANTE,
	R_OBTENER_RESTAURANTE,
	R_CONSULTAR_PLATOS,
	R_CREAR_PEDIDO,
	R_GUARDAR_PEDIDO,
	R_ANIADIR_PLATO,
	R_GUARDAR_PLATO,
	R_CONFIRMAR_PEDIDO,
	R_PLATO_LISTO,
	R_CONSULTAR_PEDIDO,
	R_OBTENER_PEDIDO,
	R_FINALIZAR_PEDIDO,
	R_TERMINAR_PEDIDO,
	R_OBTENER_RECETA,
	ERROR_PROCESO
} codigo_comando;

typedef enum{
	CLIENTE,
	APP,
	COMANDA,
	RESTAURANTE,
	SINDICATO
}t_modulo;

typedef enum {
	INEXISTENTE,
	PENDIENTE,
	CONFIRMADO,
	TERMINADO
}t_estado;

typedef struct {
	int pos_x;
	int pos_y;
} t_coordenadas;

typedef struct{
	char* id_cliente;
	t_coordenadas coordenadas_cliente;
}t_cliente;

typedef struct {
	t_modulo modulo;
	char* ip;
	char* puerto;
}t_contacto;

typedef struct {
	char* nombre_restaurante;
	t_cliente cliente;
} t_seleccionar_restaurante;

typedef struct {
	int lista_size;
	t_list* lista;
}t_lista;

typedef struct {
	char* nombre;
	uint32_t id_pedido;
}t_pedido;

typedef struct {
	t_pedido restaurante;
	char* comida;
	uint32_t cantidad;
}t_plato;

typedef struct {
	t_pedido restaurante;
	char* comida;
}t_plato_listo;

typedef struct{
	t_contacto contacto;
	bool respondo;
	void* informacion;
}t_handshake;

typedef struct {
	t_estado estado;
	t_list* platos;
} t_estado_pedido;

typedef struct {
	char* restaurante;
	t_estado_pedido estado_pedido;
} t_consultar_pedido;

typedef struct {
	char* comida;
	uint32_t total;
	uint32_t listo;
	t_list* pasos_receta;
} t_estado_comida;

typedef struct {
	t_list* afinidades;
	t_coordenadas coordenadas;
	t_list* recetas;
	uint32_t cantidad_hornos;
	uint32_t cantidad_cocineros;
	uint32_t cantidad_pedidos;
} t_info_restaurante;

typedef struct {
	char* nombre;
	uint32_t precio;
} t_receta;

typedef struct {
	char* nombre;
	uint32_t tiempo_requerido;
	uint32_t tiempo_pasado;
} t_paso;

#endif /* MENSAJES_H_ */
