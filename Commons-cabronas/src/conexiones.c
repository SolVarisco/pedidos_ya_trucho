/*
 * conexiones.c
 *
 *  Created on: 1 sep. 2020
 *      Author: utnso
 */
#include "conexiones.h"

// Creación de conexiones

int crear_conexion(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		socket_cliente = -1;

	freeaddrinfo(server_info);

	return socket_cliente;
}

int iniciar_servidor(char *ip, char* puerto) {
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int esperar_cliente(int socket_servidor) {
	struct sockaddr_in dir_cliente;

	socklen_t tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	return socket_cliente;
}

void liberar_conexion(int socket) {
	close(socket);
}

// ENVIAR DATOS

bool enviar_paquete(int socket_cliente, Paquete* paquete) {
	int cant_a_enviar = sizeof(Header) + paquete->header.tamanio_mensaje;
	void* datos = malloc(cant_a_enviar);
	int offset = 0;
	serializar_variable(datos, &(paquete->header), sizeof(Header), &offset);
	if(paquete->header.tamanio_mensaje > 0){
		serializar_variable(datos, (paquete->mensaje), paquete->header.tamanio_mensaje, &offset);
	}
	int enviado = 0; //bytes enviados
	int total_enviado = 0;
	bool valor_retorno=true;
	do {
		enviado = send(socket_cliente, datos + total_enviado, cant_a_enviar - total_enviado, 0);
		total_enviado += enviado;
		if(enviado==-1){
			valor_retorno=false;
			break;
		}
	} while (total_enviado != cant_a_enviar);
	free(datos);
	return valor_retorno;
}

bool enviar_mensaje(int socketFD, codigo_comando tipo_mensaje, void* estructura){
	Paquete* paquete = malloc(sizeof(Paquete));
	paquete->header.tipo_mensaje = tipo_mensaje;
	bool valor_retorno;
	int tam_datos = 0;
	void* datos = serializar_mensaje(estructura, tipo_mensaje, &tam_datos);

	paquete->header.tamanio_mensaje = tam_datos;
	paquete->mensaje=datos;

	valor_retorno=enviar_paquete(socketFD, paquete);

	free(paquete);

	return valor_retorno;
}

bool enviar_handshake(int socketFD, t_contacto modulo, void* mensaje){
	t_handshake informacion_handshake;
	informacion_handshake.contacto = modulo;
	informacion_handshake.respondo = true;
	informacion_handshake.informacion = mensaje;
	bool valor_retorno = enviar_mensaje(socketFD, HANDSHAKE, &informacion_handshake);
	return valor_retorno;
}

void* serializar_mensaje(void* estructura, codigo_comando comando_ingresado, int* tamanio){
	void* a_enviar;
	int offset = 0;
	uint32_t tamanio_1, tamanio_2, tamanio_3;
	uint32_t lista_size;
	int desplazamiento;

	switch(comando_ingresado) {
		case HANDSHAKE:;
			t_handshake* handshake = estructura;
			tamanio_1 = strlen(handshake->contacto.ip)+1;
			tamanio_2 = strlen(handshake->contacto.puerto)+1;

			*tamanio += sizeof(handshake->respondo)
					    + sizeof(handshake->contacto.modulo)
						+ sizeof(uint32_t)*2
						+ tamanio_1
						+ tamanio_2;


			if(handshake->respondo && handshake->contacto.modulo == CLIENTE){
				t_cliente* cliente = handshake->informacion;
				tamanio_3 = strlen(cliente->id_cliente)+1;

				*tamanio += sizeof(int)*2
							+ sizeof(uint32_t)
							+ tamanio_3;

				a_enviar = malloc(*tamanio);

				serializar_variable(a_enviar, &(handshake->contacto.modulo), sizeof(handshake->contacto.modulo), &offset);
				serializar_char(a_enviar, handshake->contacto.ip, tamanio_1, &offset);
				serializar_char(a_enviar, handshake->contacto.puerto, tamanio_2, &offset);
				serializar_variable(a_enviar, &(handshake->respondo), sizeof(handshake->respondo), &offset);
				serializar_char(a_enviar, cliente->id_cliente, tamanio_3, &offset);
				serializar_variable(a_enviar, &(cliente->coordenadas_cliente.pos_x), sizeof(cliente->coordenadas_cliente.pos_x), &offset);
				serializar_variable(a_enviar, &(cliente->coordenadas_cliente.pos_y), sizeof(cliente->coordenadas_cliente.pos_y), &offset);
			} else if(handshake->respondo && handshake->contacto.modulo == RESTAURANTE) {
				//TODO: agregado caso de handshake contra restaurante
				t_seleccionar_restaurante* restaurante_seleccionado = handshake->informacion;

				tamanio_3 = strlen(restaurante_seleccionado->nombre_restaurante)+1;

				*tamanio += tamanio_3
						+ 1
						+ sizeof(tamanio_3)
						+ sizeof(1)
						+ sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_x)
						+ sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_y);

				a_enviar = malloc(*tamanio);

				serializar_variable(a_enviar, &(handshake->contacto.modulo), sizeof(handshake->contacto.modulo), &offset);
				serializar_char(a_enviar, handshake->contacto.ip, tamanio_1, &offset);
				serializar_char(a_enviar, handshake->contacto.puerto, tamanio_2, &offset);
				serializar_variable(a_enviar, &(handshake->respondo), sizeof(handshake->respondo), &offset);
				serializar_char(a_enviar, restaurante_seleccionado->nombre_restaurante, tamanio_3, &offset);
				serializar_char(a_enviar, "", 1, &offset);
				serializar_variable(a_enviar, &(restaurante_seleccionado->cliente.coordenadas_cliente.pos_x), sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_x), &offset);
				serializar_variable(a_enviar, &(restaurante_seleccionado->cliente.coordenadas_cliente.pos_y), sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_y), &offset);
			} else {
				a_enviar = malloc(*tamanio);

				serializar_variable(a_enviar, &(handshake->contacto.modulo), sizeof(handshake->contacto.modulo), &offset);
				serializar_char(a_enviar, handshake->contacto.ip, tamanio_1, &offset);
				serializar_char(a_enviar, handshake->contacto.puerto, tamanio_2, &offset);
				serializar_variable(a_enviar, &(handshake->respondo), sizeof(handshake->respondo), &offset);
			}

			break;

		case CONSULTAR_RESTAURANTES:
			//TODO: movi crear pedido porque requiere el id del cliente
			*tamanio = 0;
			a_enviar = NULL;

			break;

		case SELECCIONAR_RESTAURANTE:;
			t_seleccionar_restaurante* restaurante_seleccionado = estructura;

			tamanio_1 = strlen(restaurante_seleccionado->nombre_restaurante)+1;
			tamanio_2 = strlen(restaurante_seleccionado->cliente.id_cliente)+1;

			*tamanio += tamanio_1
					+ tamanio_2
					+ sizeof(tamanio_1)
					+ sizeof(tamanio_2)
					+ sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_x)
					+ sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_y);

			a_enviar = malloc(*tamanio);

			serializar_char(a_enviar, restaurante_seleccionado->nombre_restaurante, tamanio_1, &offset);
			serializar_char(a_enviar, restaurante_seleccionado->cliente.id_cliente, tamanio_2, &offset);
			serializar_variable(a_enviar, &(restaurante_seleccionado->cliente.coordenadas_cliente.pos_x), sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_x), &offset);
			serializar_variable(a_enviar, &(restaurante_seleccionado->cliente.coordenadas_cliente.pos_y), sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_y), &offset);

			break;

		case OBTENER_RESTAURANTE:
		case CONSULTAR_PLATOS:
		case CREAR_PEDIDO:
		case OBTENER_RECETA: ;
			//TODO: se agrego crear pedido porque necesita el id del cliente
			char* mensaje = estructura;
			tamanio_1 = strlen(mensaje)+1;

			*tamanio += tamanio_1 + sizeof(tamanio_1);

			a_enviar = malloc(*tamanio);

			serializar_char(a_enviar, mensaje, tamanio_1, &offset);

			break;

		case CONSULTAR_PEDIDO:
		case GUARDAR_PEDIDO:
		case OBTENER_PEDIDO:
		case CONFIRMAR_PEDIDO:
		case FINALIZAR_PEDIDO:
		case TERMINAR_PEDIDO: ;
			t_pedido* informacion_pedido = estructura;
			tamanio_1 = strlen(informacion_pedido->nombre)+1;

			*tamanio += sizeof(uint32_t)*2 + tamanio_1;

			a_enviar = malloc(*tamanio);

			serializar_char(a_enviar, informacion_pedido->nombre, tamanio_1, &offset);
			serializar_variable(a_enviar, &(informacion_pedido->id_pedido), sizeof(informacion_pedido->id_pedido), &offset);

			break;

		case ANIADIR_PLATO:
		case GUARDAR_PLATO: ;
			t_plato* informacion_plato = estructura;
			tamanio_1 = strlen(informacion_plato->restaurante.nombre)+1;
			tamanio_2 = strlen(informacion_plato->comida)+1;

			*tamanio += tamanio_1
					 + tamanio_2
					 + sizeof(uint32_t)*4;		// informacion_plato->restaurante.id_pedido y informacion_plato->cantidad

			a_enviar = malloc(*tamanio);

			serializar_char(a_enviar,  informacion_plato->restaurante.nombre, tamanio_1, &offset);
			serializar_variable(a_enviar, &(informacion_plato->restaurante.id_pedido), sizeof(informacion_plato->restaurante.id_pedido), &offset);
			serializar_char(a_enviar,  informacion_plato->comida, tamanio_2, &offset);
			serializar_variable(a_enviar, &(informacion_plato->cantidad), sizeof(informacion_plato->cantidad), &offset);
			break;

		case PLATO_LISTO: ;
			t_plato_listo* plato_listo = estructura;

			tamanio_1 = strlen(plato_listo->restaurante.nombre)+1;
			tamanio_2 = strlen(plato_listo->comida)+1;

			*tamanio += tamanio_1
					 + tamanio_2
					 + sizeof(uint32_t)*3;		// informacion_plato->restaurante.id_pedido

			a_enviar = malloc(*tamanio);

			serializar_char(a_enviar,  plato_listo->restaurante.nombre, tamanio_1, &offset);
			serializar_variable(a_enviar, &(plato_listo->restaurante.id_pedido), sizeof(plato_listo->restaurante.id_pedido), &offset);
			serializar_char(a_enviar,  plato_listo->comida, tamanio_2, &offset);
			break;

		case R_CONSULTAR_RESTAURANTES:
		case R_CONSULTAR_PLATOS: ;
			t_list* lista_restaurantes = estructura;

			lista_size = (uint32_t)list_size(lista_restaurantes);
			desplazamiento = 0;
			char* elemento;
			uint32_t tamanio_elemento;

			*tamanio += sizeof(int);

			while(desplazamiento < lista_size){
				elemento = (char*)list_get(lista_restaurantes, desplazamiento);
				tamanio_elemento = strlen(elemento)+1;
				*tamanio += sizeof(tamanio_elemento) + tamanio_elemento;
				desplazamiento ++;
			}

			desplazamiento = 0;

			a_enviar = malloc(*tamanio);

			serializar_variable(a_enviar, &(lista_size), sizeof(lista_size), &offset);

			while(lista_size > 0){
				elemento = list_remove(lista_restaurantes, 0);
				tamanio_elemento = strlen(elemento)+1;
				serializar_variable(a_enviar, &tamanio_elemento, sizeof(tamanio_elemento), &offset);
				serializar_variable(a_enviar, elemento, tamanio_elemento, &offset);
				lista_size--;
			}

			break;

		case R_SELECCIONAR_RESTAURANTE:
		case R_GUARDAR_PEDIDO:
		case R_GUARDAR_PLATO:
		case R_CONFIRMAR_PEDIDO:
		case R_PLATO_LISTO:
		case R_TERMINAR_PEDIDO:
		case R_FINALIZAR_PEDIDO:;
			bool* operacion_exitosa = estructura;

			*tamanio += sizeof(operacion_exitosa);

			a_enviar = malloc(*tamanio);

			serializar_variable(a_enviar, operacion_exitosa, sizeof(operacion_exitosa), &offset);

			break;

		case R_CREAR_PEDIDO: ;
			uint32_t* id = estructura;

			*tamanio = sizeof(id);

			a_enviar = malloc(*tamanio);

			serializar_variable(a_enviar, id, sizeof(id), &offset);
			break;

		case R_CONSULTAR_PEDIDO: ;
			t_consultar_pedido* pedido_consultado = estructura;
			tamanio_1 = strlen(pedido_consultado->restaurante)+1;
			lista_size = (uint32_t)list_size(pedido_consultado->estado_pedido.platos);
			desplazamiento = 0;

			*tamanio += sizeof(tamanio_1) + tamanio_1
					  + sizeof(pedido_consultado->estado_pedido.estado)
					  + sizeof(lista_size);

			t_estado_comida* nodo = malloc(sizeof(*nodo));

			while(desplazamiento < lista_size){
				nodo = (t_estado_comida*)list_get(pedido_consultado->estado_pedido.platos, desplazamiento);
				tamanio_2 = strlen(nodo->comida)+1;
				*tamanio += sizeof(uint32_t)*3 // tamanio_2, nodo->listo, nodo->total
							+ tamanio_2;
				//TODO:Esto es nuevo
				uint32_t lista_size_2 = (uint32_t)list_size(nodo->pasos_receta);
				int desplazamiento_2 = 0;
				t_paso* paso_receta;

				*tamanio += sizeof(lista_size_2);

				while(desplazamiento_2 < lista_size_2){
					paso_receta = (t_paso*)list_get(nodo->pasos_receta, desplazamiento_2);
					tamanio_3 = strlen(paso_receta->nombre)+1;
					*tamanio += sizeof(uint32_t)*3 // tamanio_3, receta->tiempo_requerido, receta->tiempo_pasado
								+ tamanio_3;
					desplazamiento_2 ++;
				}
				//
				desplazamiento ++;
			}

			a_enviar = malloc(*tamanio);

			serializar_char(a_enviar, pedido_consultado->restaurante, tamanio_1, &offset);

			serializar_lista_platos(a_enviar, &(pedido_consultado->estado_pedido), lista_size, &offset);

			break;

		case R_OBTENER_PEDIDO: ;
			t_estado_pedido* estado_pedido = estructura;
			*tamanio += sizeof(estado_pedido->estado);
			lista_size = (uint32_t)list_size(estado_pedido->platos);
			desplazamiento = 0;
			uint32_t tamanio_1;

			*tamanio += sizeof(lista_size);

			while(desplazamiento < lista_size){
				nodo = (t_estado_comida*)list_get(estado_pedido->platos, desplazamiento);
				tamanio_1 = strlen(nodo->comida)+1;
				*tamanio += sizeof(uint32_t)*3 // tamanio_2, nodo->listo, nodo->total
							+ tamanio_1;

				//TODO:Esto es nuevo
				uint32_t lista_size_2 = list_size(nodo->pasos_receta);
				int desplazamiento_2 = 0;
				t_paso* paso_receta;

				*tamanio += sizeof(lista_size_2);

				while(desplazamiento_2 < lista_size_2){
					paso_receta = (t_paso*)list_get(nodo->pasos_receta, desplazamiento_2);
					tamanio_3 = strlen(paso_receta->nombre)+1;
					*tamanio += sizeof(uint32_t)*3 // tamanio_3, receta->tiempo_requerido, receta->tiempo_pasado
								+ tamanio_3;
					desplazamiento_2 ++;
				}
				//

				desplazamiento ++;
			}

			a_enviar = malloc(*tamanio);

			serializar_lista_platos(a_enviar, estado_pedido, lista_size, &offset);

			break;

		case R_OBTENER_RESTAURANTE:;
			t_info_restaurante* restaurante = estructura;

			lista_size = (uint32_t)list_size(restaurante->afinidades);
			desplazamiento = 0;
			char* afinidad;
			t_receta* receta;

			*tamanio += sizeof(lista_size);

			while(desplazamiento < lista_size){
				afinidad = (char*)list_get(restaurante->afinidades, desplazamiento);
				tamanio_1 = strlen(afinidad)+1;
				*tamanio += sizeof(tamanio_1) + tamanio_1;
				desplazamiento ++;
			}

			desplazamiento = 0;

			*tamanio += sizeof(int) * 2 // coordenadas
					+ sizeof(uint32_t) * 3; //restaurante->cantidad_cocineros, restaurante->cantidad_hornos, restaurante->cantidad_pedidos

			uint32_t lista_size_2 = (uint32_t)list_size(restaurante->recetas);

			*tamanio += sizeof(lista_size_2);

			while(desplazamiento < lista_size_2){
				receta = (t_receta*)list_get(restaurante->recetas, desplazamiento);
				tamanio_1 = strlen(receta->nombre)+1;
				*tamanio += sizeof(uint32_t)*2 // tamanio_1, receta->precio
							+ tamanio_1;
				desplazamiento ++;
			}

			desplazamiento = 0;

			a_enviar = malloc(*tamanio);

			serializar_restaurante(a_enviar, restaurante, lista_size, lista_size_2, &offset);

			break;

		case R_OBTENER_RECETA:;
			t_list* pasos_receta = estructura;

			lista_size = (uint32_t)list_size(pasos_receta);
			desplazamiento = 0;
			t_paso* paso_receta;

			*tamanio += sizeof(lista_size);

			while(desplazamiento < lista_size){
				paso_receta = (t_paso*)list_get(pasos_receta, desplazamiento);
				tamanio_1 = strlen(paso_receta->nombre)+1;
				*tamanio += sizeof(uint32_t)*3 // tamanio_1, receta->tiempo_requerido, receta->tiempo_pasado
							+ tamanio_1;
				desplazamiento ++;
			}

			a_enviar = malloc(*tamanio);

			int tamanio_nodo;

			serializar_variable(a_enviar, &(lista_size), sizeof(lista_size), &offset);
			while(lista_size > 0){
				paso_receta = (t_paso*)list_remove(pasos_receta, 0);
				tamanio_nodo = strlen(paso_receta->nombre)+1;
				serializar_char(a_enviar, paso_receta->nombre, tamanio_nodo, &offset);
				serializar_variable(a_enviar, &(paso_receta->tiempo_pasado), sizeof(uint32_t), &offset);
				serializar_variable(a_enviar, &(paso_receta->tiempo_requerido), sizeof(uint32_t), &offset);
				lista_size--;
			}

			break;

		default: printf("\n[!] Error en el codigo de operacion al serializar paquete.\n"); break;
	}

	return a_enviar;
}

void imprimir_nombre_paso(t_paso* valor){
	printf("Serializar receta: %s\t%d\t%d\n", valor->nombre, valor->tiempo_requerido, valor->tiempo_pasado);
}

void serializar_char(void* a_enviar, char* mensaje, uint32_t mensaje_size, int* offset){
	serializar_variable(a_enviar, &(mensaje_size), sizeof(mensaje_size), offset);
	serializar_variable(a_enviar, mensaje, mensaje_size, offset);
}

void serializar_lista_platos(void* a_enviar, t_estado_pedido* estado_pedido, int lista_size, int* offset){
	t_estado_comida* nodo = malloc(sizeof(*nodo));
	t_paso* paso_receta;
	uint32_t tamanio_nodo;

	serializar_variable(a_enviar, &(estado_pedido->estado), sizeof(estado_pedido->estado), offset);
	serializar_variable(a_enviar, &(lista_size), sizeof(lista_size), offset);

	while(lista_size > 0){
		nodo = (t_estado_comida*)list_remove(estado_pedido->platos, 0);
		tamanio_nodo = strlen(nodo->comida)+1;
		serializar_char(a_enviar, nodo->comida, tamanio_nodo, offset);
		serializar_variable(a_enviar, &(nodo->total), sizeof(uint32_t), offset);
		serializar_variable(a_enviar, &(nodo->listo), sizeof(uint32_t), offset);
		lista_size--;
		//TODO: Esto es nuevo
		uint32_t tamanio_nodo_2;
		uint32_t lista_size_2 = (uint32_t)list_size(nodo->pasos_receta);

		serializar_variable(a_enviar, &(lista_size_2), sizeof(lista_size_2), offset);
		while(lista_size_2 > 0){
			paso_receta = (t_paso*)list_remove(nodo->pasos_receta, 0);
			tamanio_nodo_2 = strlen(paso_receta->nombre)+1;
			serializar_char(a_enviar, paso_receta->nombre, tamanio_nodo_2, offset);
			serializar_variable(a_enviar, &(paso_receta->tiempo_pasado), sizeof(uint32_t), offset);
			serializar_variable(a_enviar, &(paso_receta->tiempo_requerido), sizeof(uint32_t), offset);
			lista_size_2--;
		}
		//
	}
}

void serializar_restaurante(void* a_enviar, t_info_restaurante* info_restaurante, int lista_afinidades_size, int lista_recetas_size, int* offset){
	t_receta* nodo_receta = malloc(sizeof(*nodo_receta));
	uint32_t tamanio_nodo;
	char* elemento;
	uint32_t tamanio_elemento;

	serializar_variable(a_enviar, &(lista_afinidades_size), sizeof(lista_afinidades_size), offset);
	while(lista_afinidades_size > 0){
		elemento = list_remove(info_restaurante->afinidades, 0);
		tamanio_elemento = strlen(elemento)+1;
		serializar_variable(a_enviar, &tamanio_elemento, sizeof(tamanio_elemento), offset);
		serializar_variable(a_enviar, elemento, tamanio_elemento, offset);
		lista_afinidades_size--;
	}

	serializar_variable(a_enviar, &(info_restaurante->coordenadas.pos_x), sizeof(info_restaurante->coordenadas.pos_x), offset);
	serializar_variable(a_enviar, &(info_restaurante->coordenadas.pos_y), sizeof(info_restaurante->coordenadas.pos_y), offset);

	serializar_variable(a_enviar, &(lista_recetas_size), sizeof(lista_recetas_size), offset);
	while(lista_recetas_size > 0){
		nodo_receta = (t_receta*)list_remove(info_restaurante->recetas, 0);
		tamanio_nodo = strlen(nodo_receta->nombre)+1;
		serializar_char(a_enviar, nodo_receta->nombre, tamanio_nodo, offset);
		serializar_variable(a_enviar, &(nodo_receta->precio), sizeof(uint32_t), offset);
		lista_recetas_size--;
	}

	serializar_variable(a_enviar, &(info_restaurante->cantidad_hornos), sizeof(info_restaurante->cantidad_hornos), offset);
	serializar_variable(a_enviar, &(info_restaurante->cantidad_cocineros), sizeof(info_restaurante->cantidad_cocineros), offset);
	serializar_variable(a_enviar, &(info_restaurante->cantidad_pedidos), sizeof(info_restaurante->cantidad_pedidos), offset);
}

void serializar_variable(void* a_enviar, void* a_serializar, int tamanio, int *offset) {
	memcpy(a_enviar + *offset, a_serializar, tamanio);
	*offset += tamanio;
}

//RECIBIR DATOS
int recibir_datos(void* paquete, int socketFD, uint32_t cant_a_recibir) {
	void* datos = malloc(cant_a_recibir);
	int recibido = 0;
	int totalRecibido = 0;

	do {
		recibido = recv(socketFD, datos + totalRecibido, cant_a_recibir - totalRecibido, 0);
		totalRecibido += recibido;
	} while (totalRecibido != cant_a_recibir && recibido > 0);
	memcpy(paquete, datos, cant_a_recibir);
	free(datos);
	if (recibido < 0) {
		printf("Cliente Desconectado\n");
		close(socketFD); // ¡Hasta luego!
	} else if (recibido == 0) {
		printf("Fin de Conexion en socket %d\n", socketFD);
		close(socketFD); // ¡Hasta luego!
	}
	return recibido;
}

Paquete* recibir_paquete(int socketFD, t_contacto contacto) {
	Paquete* paquete = malloc(sizeof(Paquete));
	void* mensaje_recibido;
	recibir_datos(&(paquete->header), socketFD, sizeof(Header));
	if (paquete->header.tamanio_mensaje == 0){
		paquete->mensaje = NULL;
		return paquete;
	}
	mensaje_recibido = malloc(paquete->header.tamanio_mensaje);
	recibir_datos(mensaje_recibido, socketFD, paquete->header.tamanio_mensaje);
	deserializar_mensaje(mensaje_recibido, paquete, contacto);
	return paquete;
}

void deserializar_mensaje(void* mensaje_recibido, Paquete* paquete_recibido, t_contacto contacto){
	int offset = 0;
	switch(paquete_recibido->header.tipo_mensaje){
		case HANDSHAKE: ;
			t_handshake* handshake_recibido = malloc(sizeof(*handshake_recibido));

			copiar_variable(&(handshake_recibido->contacto.modulo), mensaje_recibido, &offset, sizeof(handshake_recibido->contacto.modulo));
			deserializar_char(mensaje_recibido, &offset, &(handshake_recibido->contacto.ip));
			deserializar_char(mensaje_recibido, &offset, &(handshake_recibido->contacto.puerto));
			copiar_variable(&(handshake_recibido->respondo), mensaje_recibido, &offset, sizeof(handshake_recibido->respondo));
			handshake_recibido->informacion = NULL;

			if (handshake_recibido->respondo){
				t_handshake respuesta;
				respuesta.contacto = contacto;
				respuesta.respondo = false;
				respuesta.informacion = NULL;
				int cliente = crear_conexion(handshake_recibido->contacto.ip, handshake_recibido->contacto.puerto);
				enviar_mensaje(cliente, HANDSHAKE, &respuesta);

				switch(handshake_recibido->contacto.modulo){
					case CLIENTE: ;
						t_cliente* informacion_cliente = malloc(sizeof(*informacion_cliente));

						deserializar_char(mensaje_recibido, &offset, &(informacion_cliente->id_cliente));
						copiar_variable(&(informacion_cliente->coordenadas_cliente.pos_x), mensaje_recibido, &offset, sizeof(informacion_cliente->coordenadas_cliente.pos_x));
						copiar_variable(&(informacion_cliente->coordenadas_cliente.pos_y), mensaje_recibido, &offset, sizeof(informacion_cliente->coordenadas_cliente.pos_y));

						handshake_recibido->informacion = informacion_cliente;
						break;
					case RESTAURANTE: ;
						//TODO: caso del restaurante
						t_seleccionar_restaurante* restaurante_seleccionado = malloc(sizeof(*restaurante_seleccionado));

						deserializar_char(mensaje_recibido, &offset, &(restaurante_seleccionado->nombre_restaurante));

						deserializar_char(mensaje_recibido, &offset, &(restaurante_seleccionado->cliente.id_cliente));
						copiar_variable(&(restaurante_seleccionado->cliente.coordenadas_cliente.pos_x), mensaje_recibido, &offset, sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_x));
						copiar_variable(&(restaurante_seleccionado->cliente.coordenadas_cliente.pos_y), mensaje_recibido, &offset, sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_y));

						handshake_recibido->informacion = restaurante_seleccionado;
						break;
					default : break;
				}

				liberar_conexion(cliente);
			} else {
				printf("Se establecio una conexion\n");
			}

			paquete_recibido->mensaje = handshake_recibido;

			break;

		case SELECCIONAR_RESTAURANTE: ;
			t_seleccionar_restaurante* restaurante_seleccionado = malloc(sizeof(*restaurante_seleccionado));

			deserializar_char(mensaje_recibido, &offset, &(restaurante_seleccionado->nombre_restaurante));

			deserializar_char(mensaje_recibido, &offset, &(restaurante_seleccionado->cliente.id_cliente));
			copiar_variable(&(restaurante_seleccionado->cliente.coordenadas_cliente.pos_x), mensaje_recibido, &offset, sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_x));
			copiar_variable(&(restaurante_seleccionado->cliente.coordenadas_cliente.pos_y), mensaje_recibido, &offset, sizeof(restaurante_seleccionado->cliente.coordenadas_cliente.pos_y));

			paquete_recibido->mensaje = restaurante_seleccionado;

			break;

		case OBTENER_RESTAURANTE:
		case CONSULTAR_PLATOS:
		case CREAR_PEDIDO:
		case OBTENER_RECETA: ;
			char* restaurante;

			deserializar_char(mensaje_recibido, &offset, &restaurante);

			paquete_recibido->mensaje = restaurante;

			break;


		case CONSULTAR_PEDIDO:
		case GUARDAR_PEDIDO:
		case OBTENER_PEDIDO:
		case CONFIRMAR_PEDIDO:
		case FINALIZAR_PEDIDO: 
		case TERMINAR_PEDIDO: ;
			t_pedido* pedido_a_guardar = malloc(sizeof(*pedido_a_guardar));

			deserializar_char(mensaje_recibido, &offset, &(pedido_a_guardar->nombre));
			copiar_variable(&(pedido_a_guardar->id_pedido), mensaje_recibido, &offset, sizeof(pedido_a_guardar->id_pedido));

			paquete_recibido->mensaje = pedido_a_guardar;
			break;

		case ANIADIR_PLATO:
		case GUARDAR_PLATO: ;
			t_plato* plato_a_guardar = malloc(sizeof(*plato_a_guardar));

			deserializar_char(mensaje_recibido, &offset, &(plato_a_guardar->restaurante.nombre));
			copiar_variable(&(plato_a_guardar->restaurante.id_pedido), mensaje_recibido, &offset, sizeof(plato_a_guardar->restaurante.id_pedido));
			deserializar_char(mensaje_recibido, &offset, &(plato_a_guardar->comida));
			copiar_variable(&(plato_a_guardar->cantidad), mensaje_recibido, &offset, sizeof(plato_a_guardar->cantidad));

			paquete_recibido->mensaje = plato_a_guardar;
			break;

		case PLATO_LISTO: ;
			t_plato_listo* plato_listo = malloc(sizeof(*plato_listo));

			deserializar_char(mensaje_recibido, &offset, &(plato_listo->restaurante.nombre));
			copiar_variable(&(plato_listo->restaurante.id_pedido), mensaje_recibido, &offset, sizeof(plato_listo->restaurante.id_pedido));
			deserializar_char(mensaje_recibido, &offset, &(plato_listo->comida));

			paquete_recibido->mensaje = plato_listo;
			break;

		case R_CONSULTAR_RESTAURANTES:
		case R_CONSULTAR_PLATOS: ;
			t_list* lista_restaurantes = list_create();
			int lista_size;

			copiar_variable(&(lista_size), mensaje_recibido, &offset, sizeof(lista_size));

			uint32_t tamanio_elemento;
			while (lista_size > 0){
				copiar_variable(&(tamanio_elemento), mensaje_recibido, &offset, sizeof(tamanio_elemento));
				char* elemento = malloc(tamanio_elemento);
				copiar_variable(elemento, mensaje_recibido, &offset, tamanio_elemento);
				list_add(lista_restaurantes, (void*)elemento);
				lista_size--;
			}
			paquete_recibido->mensaje = lista_restaurantes;
			break;

		case R_SELECCIONAR_RESTAURANTE:
		case R_GUARDAR_PEDIDO:
		case R_GUARDAR_PLATO:
		case R_CONFIRMAR_PEDIDO:
		case R_PLATO_LISTO:
		case R_FINALIZAR_PEDIDO:
		case R_TERMINAR_PEDIDO: ;
			bool* operacion_exitosa = malloc(sizeof(_Bool));

			copiar_variable(operacion_exitosa, mensaje_recibido, &offset, sizeof(operacion_exitosa));

			paquete_recibido->mensaje = operacion_exitosa;
			break;

		case R_CREAR_PEDIDO: ;
			uint32_t* id = malloc(sizeof(*id));

			copiar_variable(id, mensaje_recibido, &offset, sizeof(id));

			paquete_recibido->mensaje = id;
			break;

		case R_CONSULTAR_PEDIDO:;
			t_consultar_pedido* pedido_consultado = malloc(sizeof(*pedido_consultado));

			deserializar_char(mensaje_recibido, &offset, &(pedido_consultado->restaurante));

			deserializar_lista_platos(&(pedido_consultado->estado_pedido), mensaje_recibido, &offset);

			paquete_recibido->mensaje = pedido_consultado;
			break;

		case R_OBTENER_PEDIDO: ;
			t_estado_pedido* estado_pedido = malloc(sizeof(*estado_pedido));

			deserializar_lista_platos(estado_pedido, mensaje_recibido, &offset);

			paquete_recibido->mensaje = estado_pedido;
			break;

		case R_OBTENER_RESTAURANTE: ;
			t_info_restaurante* info_restaurante = malloc(sizeof(t_info_restaurante));

			deserializar_info_restaurante(info_restaurante, mensaje_recibido, &offset);

			paquete_recibido->mensaje = info_restaurante;
			break;

		case R_OBTENER_RECETA: ;
			t_list* pasos_receta = list_create();
			int lista_recetas_size;

			copiar_variable(&(lista_recetas_size), mensaje_recibido, &offset, sizeof(lista_recetas_size));
			while (lista_recetas_size > 0){
				t_paso* receta = malloc(sizeof(*receta));
				deserializar_char(mensaje_recibido, &offset, &(receta->nombre));
				copiar_variable(&(receta->tiempo_pasado), mensaje_recibido, &offset, sizeof(receta->tiempo_pasado));
				copiar_variable(&(receta->tiempo_requerido), mensaje_recibido, &offset, sizeof(receta->tiempo_requerido));
				list_add(pasos_receta, (void*)receta);
				lista_recetas_size--;
			}

			list_iterate(pasos_receta, (void*)deserializando_Receta);

			paquete_recibido->mensaje = pasos_receta;
			break;
			
		default: printf("\n[!] Error en el codigo de operacion al deserializar paquete.\n"); break;
	}
}

void deserializando_Receta(t_paso* valor){
	printf("Deserializar receta: %s\t%d\t%d\n", valor->nombre, valor->tiempo_requerido, valor->tiempo_pasado);
}

void deserializar_char(void* stream, int* offset, char** receptor){
	uint32_t tamanio;
	copiar_variable(&(tamanio), stream, offset, sizeof(tamanio));
	*receptor = malloc(tamanio);
	copiar_variable(*receptor, stream, offset, tamanio);
}

void deserializar_lista_platos(t_estado_pedido* estado_pedido, void* stream, int* offset){
	copiar_variable(&(estado_pedido->estado), stream, offset, sizeof(estado_pedido->estado));
	estado_pedido->platos = list_create();
	int lista_size;


	copiar_variable(&(lista_size), stream, offset, sizeof(lista_size));

	while (lista_size > 0){
		t_estado_comida* elemento = malloc(sizeof(*elemento));
		deserializar_char(stream, offset, &(elemento->comida));
		copiar_variable(&(elemento->total), stream, offset, sizeof(elemento->total));
		copiar_variable(&(elemento->listo), stream, offset, sizeof(elemento->listo));
		list_add(estado_pedido->platos, (void*)elemento);
		lista_size--;

		//TODO:Esto es nuevo
		elemento->pasos_receta = list_create();

		int lista_recetas_size;

		copiar_variable(&(lista_recetas_size), stream, offset, sizeof(lista_recetas_size));
		while (lista_recetas_size > 0){
			t_paso* receta = malloc(sizeof(*receta));
			deserializar_char(stream, offset, &(receta->nombre));
			copiar_variable(&(receta->tiempo_pasado), stream, offset, sizeof(receta->tiempo_pasado));
			copiar_variable(&(receta->tiempo_requerido), stream, offset, sizeof(receta->tiempo_requerido));
			list_add(elemento->pasos_receta, receta);
			lista_recetas_size--;
		}
		//
	}
}

void deserializar_info_restaurante(t_info_restaurante* info_restaurante, void* stream, int* offset){
	info_restaurante->afinidades = list_create();
	info_restaurante->recetas = list_create();
	int lista_afinidades_size;
	int lista_recetas_size;

	copiar_variable(&(lista_afinidades_size), stream, offset, sizeof(lista_afinidades_size));
	uint32_t tamanio_elemento;

	while (lista_afinidades_size > 0){
		copiar_variable(&(tamanio_elemento), stream, offset, sizeof(tamanio_elemento));
		char* elemento = malloc(tamanio_elemento);
		copiar_variable(elemento, stream, offset, tamanio_elemento);
		list_add(info_restaurante->afinidades, (void*)elemento);
		lista_afinidades_size--;
	}

	copiar_variable(&(info_restaurante->coordenadas.pos_x), stream, offset, sizeof(info_restaurante->coordenadas.pos_x));
	copiar_variable(&(info_restaurante->coordenadas.pos_y), stream, offset, sizeof(info_restaurante->coordenadas.pos_y));

	copiar_variable(&(lista_recetas_size), stream, offset, sizeof(lista_recetas_size));
	while (lista_recetas_size > 0){
		t_receta* receta = malloc(sizeof(*receta));
		deserializar_char(stream, offset, &(receta->nombre));
		copiar_variable(&(receta->precio), stream, offset, sizeof(receta->precio));
		list_add(info_restaurante->recetas, (void*)receta);
		lista_recetas_size--;
	}

	copiar_variable(&(info_restaurante->cantidad_hornos), stream, offset, sizeof(info_restaurante->cantidad_hornos));
	copiar_variable(&(info_restaurante->cantidad_cocineros), stream, offset, sizeof(info_restaurante->cantidad_cocineros));
	copiar_variable(&(info_restaurante->cantidad_pedidos), stream, offset, sizeof(info_restaurante->cantidad_pedidos));
}

void copiar_variable(void* variable, void* stream, int* offset, int size) {
	memcpy(variable, stream + *offset, size);
	*offset += size;
}
