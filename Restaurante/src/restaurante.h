/*
 * restaurante.h
 *
 *  Created on: 22 oct. 2020
 *      Author: utnso
 */

#ifndef RESTAURANTE_H_
#define RESTAURANTE_H_

#include "planificacion.h"


#define RESTAURANTE_CONFIG "/home/utnso/tp-2020-2c-CabreadOS/Restaurante/Debug/restaurante.config"

//Mutexs
pthread_mutex_t mutex_metadata = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_genera_pedidos = PTHREAD_MUTEX_INITIALIZER;

//threads
pthread_t thread_atencion;
pthread_t thread_genera_pedidos;

int cont_pedidos;		//para crear los ID_PEDIDOS

uint32_t ID_PLATOS;


// PROTOTIPOS
void init_config();
void read_config();
void init_logger();
void init_listas();

void enviar_handshake_a_sindicato(int socket_servidor);
void get_metadata();
void get_metadata_del_sindicato();
int conexion_sindicato();
void check_conexion(int conexion);
void generar_metadata_local();
void set_metadata(Paquete* paquete_recibido);
void activar_coordinador();

void enviar_handshake_a_app(int socket_servidor);
void registrarse_en_app();

void init_server();
void serve_client(int* socket_cliente);
void gestionar_mensaje(Paquete* paquete_recibido, int socket_cliente);

void listar_platos(Paquete* paquete_recibido, int socket_cliente);
void crear_pedido(Paquete* paquete_recibido, int socket_cliente);
uint32_t generar_pedido_id();
void aniadir_plato(Paquete* paquete_recibido, int socket_cliente);
void confirmar_pedido(Paquete* paquete_recibido, int socket_cliente);
void agregar_platos_demo(t_list* platos_a_planificar, uint32_t* id_pedido);
void mandar_a_planificar(t_list* platos_a_planificar, uint32_t* id_pedido);
t_list* duplicar_receta(t_list* self);
uint32_t generar_plato_id();
void consultar_pedido(Paquete* paquete_recibido, int socket_cliente);

void imprimir_nombre_paso(t_paso* valor);
void imprimir_estado_comida(void* args);

void terminar_programa();

////////////////////////

t_paso* generar_troceo_demo();
t_paso* generar_empanar_demo();
t_paso* generar_reposar_demo();
t_paso* generar_hornear_demo(int tiempo);
t_paso* generar_preparar_demo(int tiempo);
t_paso* generar_servir_demo(int tiempo);
t_paso* generar_cortar_demo(int tiempo);
t_paso* generar_hervir_demo(int tiempo);

#endif /* RESTAURANTE_H_ */
