/*
 * conexiones.h
 *
 *  Created on: 1 sep. 2020
 *      Author: utnso
 */

#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "mensajes.h"

//////////////////////////////////////////
//           Comunicacion Base          //
//////////////////////////////////////////

typedef struct {
	codigo_comando tipo_mensaje;
	int tamanio_mensaje;
}__attribute__((packed)) Header;

typedef struct {
	Header header;
	void* mensaje;
}__attribute__((packed)) Paquete;

//////////////////////////////////////////
//          Estructuras Utiles          //
//////////////////////////////////////////

typedef struct {
	pthread_t hilo;
	int socket;
} t_hilo;

//////////////////////////////////////////
//              Funciones               //
//////////////////////////////////////////

int crear_conexion(char *ip, char* puerto);

int iniciar_servidor(char *ip, char* puerto);

int esperar_cliente(int socket_servidor);

void liberar_conexion(int socket);

bool enviar_paquete(int socketCliente, Paquete* paquete);

bool enviar_mensaje(int socketFD, codigo_comando tipo_mensaje, void* estructura);

bool enviar_handshake(int socketFD, t_contacto modulo, void* mensaje);

void* serializar_mensaje(void* estructura, codigo_comando comando_ingresado, int* tamanio);

void serializar_char(void* a_enviar, char* mensaje, uint32_t mesaje_size, int* offset);

void serializar_lista_platos(void* a_enviar, t_estado_pedido* estado_pedido, int lista_size, int* offset);

void serializar_restaurante(void* a_enviar, t_info_restaurante* info_restaurante, int lista_afinidades_size, int lista_recetas_size, int* offset);

void serializar_variable(void* a_enviar, void* a_serializar, int tamanio, int *offset);

int recibir_datos(void* paquete, int socketFD, uint32_t cant_a_recibir);

Paquete* recibir_paquete(int socketFD, t_contacto contacto);

void deserializar_mensaje(void* mensaje_recibido, Paquete* paquete_recibido, t_contacto contacto);

void deserializar_char(void* stream, int* offset, char** receptor);

void deserializar_lista_platos(t_estado_pedido* estado_pedido, void* stream, int* offset);

void deserializar_info_restaurante(t_info_restaurante* info_restaurante, void* stream, int* offset);

void copiar_variable(void* variable, void* stream, int* offset, int size);


void imprimir_nombre_paso(t_paso* valor);
void deserializando_Receta(t_paso* valor);
#endif /* CONEXIONES_H_ */
