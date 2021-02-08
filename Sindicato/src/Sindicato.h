#ifndef SINDICATO_H_
#define SINDICATO_H_

#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <conexiones.h>

//#include "serializacion.h"
#include "Consola.h"

t_config* config;
t_contacto sindicato;

pthread_t thread;
pthread_t console;

pthread_mutex_t mutexBitmap = PTHREAD_MUTEX_INITIALIZER;;
pthread_mutex_t mutex_file_system = PTHREAD_MUTEX_INITIALIZER;

void read_config(void);
t_log* init_logger(void);
void* console_accion(void* args);
void* customer_suport(void* args);
void accion(Paquete* package, int connetion);

void gestionar_handshake(Paquete* package);
void consultar_platos(Paquete* package, int connetion);
void guardar_pedido(Paquete* package, int connetion);
void guardar_plato(Paquete* package, int connetion);
void confirmar_pedido(Paquete* package, int connetion);
void obtener_pedido(Paquete* package, int connetion);
void obtener_restaurante(Paquete* package, int connetion);
void plato_listo(Paquete* package, int connetion);
void obtener_receta(Paquete* package, int connetion);
void terminar_pedido(Paquete* package, int connetion);

void kill_mutexs(void);

void imprimir_nombre_paso(t_paso* valor);
void imprimir_estado_comida(void* args);

char* server_ip;
int puerto;

t_list* hilos;




int tiempo_retardo_operacion;

void crearLogger(char* logPath, char* logMemoNombreArch, bool consolaActiva);
void leerArchivoDeConfiguracion(char* configPath);
void leerConfig(char* configPath);
void setearValores(t_config * archivoConfig);
void crearServidor();



// FS
t_list* file_blocks(int blocks_needed, char* metadata_path, int size_array_positions);
char* read_blocks_content(char* path);
int size_of_content_to_write(int index_next_block, int quantity_total_blocks, int total_content_size);
void write_positions_on_block(char* block, char* data);
void remove_block_from_bitmap(char* block);
t_list* write_blocks_and_metadata(int size_array_positions, char* array_positions, char* metadata_path);
char* remove_last_block_from_array(char* blocks_as_array);
char* add_block_to_array(char* blocks_as_array, char* block_to_add);
char** metadata_blocks_to_actual_blocks(char* metadata_blocks);
int my_ceil(int a, int b);
t_list* file_blocks(int blocks_needed, char* metadata_path, int size_array_positions);
void read_config();



;
pthread_mutex_t mutexMetadataPath = PTHREAD_MUTEX_INITIALIZER;;
struct stat mystat;



#endif /* SINDICATO_H_ */
