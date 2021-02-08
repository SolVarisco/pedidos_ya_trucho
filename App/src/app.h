/*
 * app.h
 *
 *  Created on: 14 sep. 2020
 *      Author: utnso
 */

#ifndef APP_H_
#define APP_H_

#include "repartidores_en_listas.h"
#include "globales_config.h"
#include "planificacion.h"

pthread_t thread_atencion;
pthread_t thread_planificacion;

t_list* restaurantes;
t_list* pedidos_pendientes;
t_list* lista_cliente_x_restaurante;
t_list* pedidos_en_planificacion;

uint32_t ID_PEDIDOS;

void init_sem_planificador();
void init_listas();
void init_config();
void init_valores_config();
void init_logger();
void init_listas_estados();
void init_server();

void enviar_handshake_a_comanda(int socket_servidor);

void serve_client(int* socket_cliente);
void process_request(Paquete* paquete_recibido, int socket_cliente);

void agregar_contacto(Paquete* paquete_recibido, int socket_cliente);
void listar_restaurantes(int socket_cliente);
void seleccionar_restaurante(Paquete* paquete_recibido, int socket_cliente);
uint32_t generar_pedido_id();
t_cliente_x_restaurante* crear_cliente_x_restaurante(int socket_cliente, t_seleccionar_restaurante * seleccionar_restaurante, t_restaurante* restaurante);
t_pcb_ped* crear_pcb(int id_pedido, t_cliente_x_restaurante* cliente_x_restaurante);
void listar_platos(Paquete* paquete_recibido, int socket_cliente);
void crear_pedido(Paquete* paquete_recibido, int socket_cliente);
void aniadir_plato(Paquete* paquete_recibido, int socket_cliente);
void confirmar_pedido(Paquete* paquete_recibido, int socket_cliente);
void consultar_pedido(Paquete* paquete_recibido, int socket_cliente);
void plato_listo(Paquete* paquete_recibido, int socket_cliente);

void dar_a_repartidor_mas_cercano(t_pcb_ped* pcb_pedido);
void activar_coordinador();
void planificar_a_largo_plazo(t_pcb_ped* pedido);


#endif /* APP_H_ */
