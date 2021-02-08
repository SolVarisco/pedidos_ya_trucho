/*
 * cliente.c
 *
 *  Created on: 1 sep. 2020
 *      Author: utnso
 */
#include "cliente.h"

int main(void) {

	int conexion;

	cliente.modulo = CLIENTE;

	char* leido;
	restaurante = NULL;
	plato = NULL;
	id_actual = -1;

	codigo_comando comando_ingresado;
	sem_init(&sem_pedidos, 1, 0);

	id_pedidos = list_create();
	list_clean(id_pedidos);

	//Iniciar config y loggin
	leer_config();

	if(!CONFIG)
		return -2;

	cliente.puerto = config_get_string_value(CONFIG, "PUERTO_ESCUCHA");
	cliente.ip = config_get_string_value(CONFIG, "IP_ESCUCHA");

	iniciar_logger();
	if(!LOGGER)
		return -3;

	// Defino los valores de conexion para cada modulo
	asigno_valores_conexion();


	// Inicio hilo para recibir mensajes
	pthread_create(&hilo_receptor, NULL, receptor_de_mensajes, NULL);

	// Verifico que haya un modulo escuchando
	pthread_mutex_lock(&mutex_handshake);
	pthread_cond_wait(&cond_handshake, &mutex_handshake);
	if(modulo_conectado == -1){
		terminar_programa();
		return -1;
	}
	pthread_mutex_unlock(&mutex_handshake);

	//leer que operacion quiere realizar el cliente
	mostrar_comandos();
	leido = readline(">");

	//enviar mensajes
	while(strcmp(leido, "\0")) {
		log_info(LOGGER, leido);
		comando_ingresado = string_a_codigo_comando(leido);

		if(comando_ingresado == ERROR_PROCESO){
			printf("El sistema no reconoce la operacion %s, por favor intente nuevamente\n", leido);
			leido = readline(">");
			continue;
		}

		conexion = crear_conexion(IP_CONEXION, PUERTO_CONEXION);
		if(!conexion)
			return -5;

		armar_y_enviar_paquete(comando_ingresado, conexion);
		pthread_create(&thread, NULL, accion, &conexion);
		pthread_detach(thread);

		leido = readline(">");
	}
	
	sem_post(&sem_pedidos);

	free(leido);
	free(restaurante);
	free(plato);

	terminar_programa();

	return 0;
}

//Tendriamos que ver realmente que operaciones puede ejecutar al principio.  ej: AGREGAR_PLATO??
void mostrar_comandos() {
	printf("Operaciones disponibles: \n");
	switch(modulo_conectado) {
		case APP:
			printf("\tCONSULTAR_RESTAURANTES\n");
			printf("\tSELECCIONAR_RESTAURANTE\n");
			if(restaurante != NULL){
				printf("\tCONSULTAR_PLATOS\n");
			}
			printf("\tCREAR_PEDIDO\n");
			if(id_actual != -1){
				printf("\tAÑADIR_PLATO\n");
				if(plato != NULL){
					printf("\tCONFIRMAR_PEDIDO\n");
				}
				printf("\tCONSULTAR_PEDIDO\n");
			}
			printf("\tPLATO_LISTO\n");
			break;
		case COMANDA:
			printf("\tGUARDAR_PEDIDO\n");
			printf("\tGUARDAR_PLATO\n");
			printf("\tCONFIRMAR_PEDIDO\n");
			printf("\tPLATO_LISTO\n");
			printf("\tOBTENER_PEDIDO\n");
			printf("\tFINALIZAR_PEDIDO\n");
			break;
		case RESTAURANTE:
			printf("\tCONSULTAR_PLATOS\n");
			printf("\tCREAR_PEDIDO\n");
			printf("\tANIADIR_PLATO\n");
			printf("\tCONFIRMAR_PEDIDO\n");
			printf("\tCONSULTAR_PEDIDO\n");
			break;
		case SINDICATO:
			printf("\tOBTENER_RESTAURANTE\n");
			printf("\tCONSULTAR_PLATOS\n");
			printf("\tGUARDAR_PEDIDO\n");
			printf("\tGUARDAR_PLATO\n");
			printf("\tCONFIRMAR_PEDIDO\n");
			printf("\tPLATO_LISTO\n");
			printf("\tOBTENER_PEDIDO\n");
			printf("\tTERMINAR_PEDIDO\n");
			printf("\tOBTENER_RECETA\n");
			break;
		default: break;
	}
}

void asigno_valores_conexion(){
	IP_CONEXION = config_get_string_value(CONFIG, "IP");
	PUERTO_CONEXION = config_get_string_value(CONFIG, "PUERTO");
}


codigo_comando string_a_codigo_comando(const char* comando_ingresado) {
	codigo_comando codigo_actual = -1;

	for(int i = 0; i < sizeof(conversion_codigo_comando) / sizeof(conversion_codigo_comando[0]); i++)
	{
		if(!strcmp(comando_ingresado, conversion_codigo_comando[i].str)){
			codigo_actual = conversion_codigo_comando[i].codigo_comando;
			break;
		}
	}
	if(codigo_actual == -1){
		return ERROR_PROCESO;
	}

	if (modulo_conectado == APP){
		switch(codigo_actual) {
			case CONSULTAR_RESTAURANTES:
			case SELECCIONAR_RESTAURANTE:
			case CONSULTAR_PLATOS:
			case CREAR_PEDIDO:
			case ANIADIR_PLATO:
			case CONFIRMAR_PEDIDO:
			case PLATO_LISTO:
			case CONSULTAR_PEDIDO:
				return codigo_actual;
			default: break;
		}
	} else if (modulo_conectado == COMANDA) {
		switch(codigo_actual){
			case GUARDAR_PEDIDO:
			case GUARDAR_PLATO:
			case CONFIRMAR_PEDIDO:
			case PLATO_LISTO:
			case OBTENER_PEDIDO:
			case FINALIZAR_PEDIDO:
				return codigo_actual;
			default: break;
		}
	} else if (modulo_conectado == RESTAURANTE) {
		switch(codigo_actual){
			case CONSULTAR_PLATOS:
			case CREAR_PEDIDO:
			case ANIADIR_PLATO:
			case CONFIRMAR_PEDIDO:
			case CONSULTAR_PEDIDO:
				return codigo_actual;
			default: break;
		}
	} else if (modulo_conectado == SINDICATO) {
		switch(codigo_actual){
			case OBTENER_RESTAURANTE:
			case CONSULTAR_PLATOS:
			case GUARDAR_PEDIDO:
			case GUARDAR_PLATO:
			case CONFIRMAR_PEDIDO:
			case PLATO_LISTO:
			case OBTENER_PEDIDO:
			case TERMINAR_PEDIDO:
			case OBTENER_RECETA:
				return codigo_actual;
			default: break;
		}
	}

	return ERROR_PROCESO;
}

void imprimir_int(int* valor){
	printf("%d\n", *valor);
}

bool es_igual(void* args){
	int* valor = (int*)args;
	return *valor == id_actual;
}

void armar_y_enviar_paquete(codigo_comando comando_ingresado, int conexion){
	/*Especificacion de como se debe enviar el paquete en cada caso. Si la funcion es propia del cliente las variables guardan memoria,
	 * En cambio, si pertenecen a otros modulos no lo hacen y siempre pide que se especifiquen sus datos */
char* leido;
	switch(comando_ingresado) {
		case CONSULTAR_RESTAURANTES:
			enviar_mensaje(conexion, comando_ingresado, NULL);
			log_codigo(comando_ingresado, 1);
			break;
		case SELECCIONAR_RESTAURANTE: ;
			t_seleccionar_restaurante seleccionar_restaurante;

			printf("Restaurante seleccionado: ");

			seleccionar_restaurante.nombre_restaurante = readline(">");
			restaurante = malloc(strlen(seleccionar_restaurante.nombre_restaurante)+1);
			strcpy(restaurante, seleccionar_restaurante.nombre_restaurante);
			seleccionar_restaurante.cliente.id_cliente = config_get_string_value(CONFIG, "ID_CLIENTE");
			seleccionar_restaurante.cliente.coordenadas_cliente.pos_x = config_get_int_value(CONFIG, "POSICION_X");
			seleccionar_restaurante.cliente.coordenadas_cliente.pos_y = config_get_int_value(CONFIG, "POSICION_Y");

			enviar_mensaje(conexion, comando_ingresado, &seleccionar_restaurante);
			log_seleccionar_restaurante(comando_ingresado, seleccionar_restaurante);
			break;

		case OBTENER_RESTAURANTE:
			printf("Restaurante seleccionado: ");
			leido = readline(">");
			enviar_mensaje(conexion, comando_ingresado, leido);
			log_restaurante(comando_ingresado, leido, 1);
			free(leido);
			break;

		case CREAR_PEDIDO:
		case CONSULTAR_PLATOS:
			if(modulo_conectado == APP){
				if(restaurante==NULL)
					break;
				printf("Restaurante: %s (en caso de querer realizar otra consulta, seleccione otro restaurante)\n", restaurante);
				enviar_mensaje(conexion, comando_ingresado, config_get_string_value(CONFIG, "ID_CLIENTE"));
				log_restaurante(comando_ingresado, restaurante, 1);

			} else if(modulo_conectado == RESTAURANTE && restaurante != NULL){
				enviar_mensaje(conexion, comando_ingresado, restaurante);
				log_restaurante(comando_ingresado, restaurante, 1);
			} else {
				printf("Restaurante seleccionado: ");
				leido = readline(">");
				if(modulo_conectado == RESTAURANTE){
					restaurante = malloc(strlen(leido)+1);
					strcpy(restaurante, leido);
				}
				enviar_mensaje(conexion, comando_ingresado, leido);
				log_restaurante(comando_ingresado, leido, 1);
				free(leido);
			}
			break;

		case GUARDAR_PEDIDO:
		case OBTENER_PEDIDO:
		case FINALIZAR_PEDIDO:
		case TERMINAR_PEDIDO: ;
			t_pedido pedido;
			printf("Restaurante seleccionado: ");
			pedido.nombre = readline(">");
			printf("ID del pedido seleccionado: ");
			pedido.id_pedido = (uint32_t)atoi(readline(">"));

			enviar_mensaje(conexion, comando_ingresado, &pedido);
			log_pedido(comando_ingresado, pedido, 1);
			break;

		case ANIADIR_PLATO: ;
			t_plato plato_seleccionado;
			printf("Plato seleccionado: ");
			plato_seleccionado.comida = readline(">");
			printf("Cantidad: ");
			plato_seleccionado.cantidad = (uint32_t) atoi(readline(">"));

			plato = malloc(strlen(plato_seleccionado.comida)+1);
			strcpy(plato, plato_seleccionado.comida);

			pthread_mutex_lock(&mutex_id);
			if(id_actual == -1){
				printf("No se ha creado ningun pedido\n");
				pthread_mutex_unlock(&mutex_id);
				break;
			}
			pthread_mutex_unlock(&mutex_id);


			plato_seleccionado.restaurante.id_pedido = 	id_actual;
			plato_seleccionado.restaurante.nombre = restaurante;

			enviar_mensaje(conexion, comando_ingresado, &plato_seleccionado);
			log_plato(comando_ingresado, plato_seleccionado);
			break;

		case GUARDAR_PLATO: ;
			t_plato plato_guardado;
			printf("Restaurante seleccionado: ");
			plato_guardado.restaurante.nombre = readline(">");

			printf("Id del pedido seleccionado: ");
			plato_guardado.restaurante.id_pedido = (uint32_t)atoi(readline(">"));

			printf("Plato seleccionado: ");
			plato_guardado.comida = readline(">");

			printf("Cantidad de %s: ", plato_guardado.comida);
			plato_guardado.cantidad = (uint32_t)atoi(readline(">"));

			enviar_mensaje(conexion, comando_ingresado, &plato_guardado);
			log_plato_guardado(comando_ingresado, plato_guardado);
			break;

		case CONFIRMAR_PEDIDO: ;
			t_pedido confirmar;
			uint32_t id_confirmado;

			if(modulo_conectado == RESTAURANTE){
				confirmar.nombre=malloc(strlen(restaurante +1));
				strcpy(confirmar.nombre, restaurante);
			} else {
				printf("Restaurante seleccionado: ");
				confirmar.nombre = readline(">");
			}

			if(modulo_conectado == APP || modulo_conectado == RESTAURANTE){

				printf("Los Ids de sus pedidos son:\n");

				pthread_mutex_lock(&mutex_id_lista);
				if(list_is_empty(id_pedidos)){
					pthread_mutex_unlock(&mutex_id_lista);
					printf("\tNo se encontro ningun pedido para consultar\n");
					break;
				}
				list_iterate(id_pedidos, (void*)imprimir_int);
				pthread_mutex_unlock(&mutex_id_lista);

				printf("Id del pedido a consultar: ");
				do{
					pthread_mutex_lock(&mutex_id);
					id_actual = (uint32_t)atoi(readline(">"));
					pthread_mutex_lock(&mutex_id_lista);
					if(list_find(id_pedidos, *es_igual)!= 0){
						pthread_mutex_unlock(&mutex_id_lista);
						confirmar.id_pedido = id_actual;
						id_confirmado = id_actual;
						id_actual = -1;
						pthread_mutex_unlock(&mutex_id);
						break;
					}
					pthread_mutex_unlock(&mutex_id_lista);
					printf("No existe nigun pedido con el id: %d\n", id_actual);
					pthread_mutex_unlock(&mutex_id);
					printf("Id del pedido a consultar: ");
				}while(1);

				sem_post(&sem_pedidos);
			} else {
				printf("ID del pedido seleccionado:");
				confirmar.id_pedido = (uint32_t)atoi(readline("ID: "));
			}

			enviar_mensaje(conexion, comando_ingresado, &confirmar);
            log_pedido(comando_ingresado, confirmar, 1);
			break;

		case PLATO_LISTO: ;
			t_plato_listo plato_listo;
			printf("Restaurante seleccionado: ");
			plato_listo.restaurante.nombre = readline(">");
			printf("ID del pedido seleccionado: ");
			plato_listo.restaurante.id_pedido = (uint32_t)atoi(readline(">"));
			printf("Plato seleccionado: ");
			plato_listo.comida = readline(">");

			enviar_mensaje(conexion, comando_ingresado, &plato_listo);
			log_plato_listo(comando_ingresado, plato_listo);
			break;

		case CONSULTAR_PEDIDO: ;
			t_pedido consultar;
			uint32_t id_consultado;

			if(modulo_conectado == RESTAURANTE){
				consultar.nombre=malloc(strlen(restaurante +1));
				strcpy(consultar.nombre, restaurante);
			} else {
				printf("Restaurante seleccionado: ");
				consultar.nombre = readline(">");
			}

			if(modulo_conectado == APP || modulo_conectado == RESTAURANTE){
				printf("Los Ids de sus pedidos son:\n");

				pthread_mutex_lock(&mutex_id_lista);
				if(list_is_empty(id_pedidos)){
					pthread_mutex_unlock(&mutex_id_lista);
					printf("\tNo se encontro ningun pedido para consultar\n");
					break;
				}
				list_iterate(id_pedidos, (void*)imprimir_int);
				pthread_mutex_unlock(&mutex_id_lista);

				printf("Id del pedido a consultar: ");
				do{
					pthread_mutex_lock(&mutex_id);
					id_actual = (uint32_t)atoi(readline(">"));
					pthread_mutex_lock(&mutex_id_lista);
					if(list_find(id_pedidos, *es_igual)!= 0){
						pthread_mutex_unlock(&mutex_id_lista);
						consultar.id_pedido = id_actual;
						id_consultado = id_actual;
						id_actual = -1;
						pthread_mutex_unlock(&mutex_id);
						break;
					}
					pthread_mutex_unlock(&mutex_id_lista);
					printf("No existe nigun pedido con el id: %d\n", id_actual);
					pthread_mutex_unlock(&mutex_id);
					printf("Id del pedido a consultar: ");
				}while(1);

			} else {
				printf("ID del pedido seleccionado: ");
				consultar.id_pedido = (uint32_t)readline(">");
			}

			enviar_mensaje(conexion, comando_ingresado, &consultar);
			log_id(comando_ingresado, id_consultado, 1);

			break;

		case OBTENER_RECETA:
			printf("Nombre del plato: ");
			leido = readline(">");

			enviar_mensaje(conexion, comando_ingresado, leido);
			log_receta(comando_ingresado, leido);
			free(leido);
			break;
		default :
			break;
	}

}

// Funcion hilo de recepcion de mensajes y envio de handshake

void* receptor_de_mensajes(void* args){
	int conexion = crear_conexion(IP_CONEXION, PUERTO_CONEXION);
	int servidor = iniciar_servidor(cliente.ip, cliente.puerto);
	pthread_mutex_lock(&mutex_handshake);
	if(!gestionar_handshake(conexion, servidor))
		return NULL;

	int lista_vacia;

	while(1){
		sem_wait(&sem_pedidos);
		pthread_mutex_lock(&mutex_id_lista);
		lista_vacia = list_is_empty(id_pedidos);
		pthread_mutex_unlock(&mutex_id_lista);
		if(lista_vacia == 0){
			conexion = esperar_cliente(servidor);
			gestionar_mensaje(conexion);
			liberar_conexion(conexion);
		} else {
			break;
		}
	}

	return NULL;

}


bool gestionar_handshake(int conexion, int servidor){
	t_cliente informacion_cliente;
	informacion_cliente.id_cliente = config_get_string_value(CONFIG, "ID_CLIENTE");
	informacion_cliente.coordenadas_cliente.pos_x = config_get_int_value(CONFIG, "POSICION_X");
	informacion_cliente.coordenadas_cliente.pos_y = config_get_int_value(CONFIG, "POSICION_Y");

	bool status = enviar_handshake(conexion, cliente, &informacion_cliente);
	bool retorno = true;

	if(status){
		conexion = esperar_cliente(servidor);
		Paquete* paquete_recibido = recibir_paquete(conexion, cliente);

		t_handshake* handshake_recibido = (t_handshake*)paquete_recibido->mensaje;
		modulo_conectado = handshake_recibido->contacto.modulo;

		switch(modulo_conectado){
			case APP:
				printf("Se logro una conexion exitosa con el modulo APP\n");
				break;
			case COMANDA:
				printf("Se logro una conexion exitosa con el modulo COMANDA\n");
				break;
			case RESTAURANTE:
				printf("Se logro una conexion exitosa con el modulo RESTAURANTE\n");
				break;
			case SINDICATO:
				printf("Se logro una conexion exitosa con el modulo SINDICATO\n");
				break;
			default:break;
		}

	} else {

		modulo_conectado = -1;
		printf("No se pudo concretar la conexion\n");
		liberar_conexion(conexion);
		retorno = false;
	}

	pthread_cond_signal(&cond_handshake);
	pthread_mutex_unlock(&mutex_handshake);

	return retorno;
}

// Funciones de los hilos

void* accion(void* args){
	int* conexion = (int*)args;
	gestionar_mensaje(*conexion);
	liberar_conexion(*conexion);
	if(modulo_conectado == APP)
		mostrar_comandos();

	return NULL;
}

void imprimir_plato(char* valor){
	printf("%s\n", valor);
	loggear_platos(valor);
	free(valor);
}

void imprimir_nombre_receta(t_receta* valor){
	printf("%s\t%d\n", valor->nombre, valor->precio);
	free(valor->nombre);
	free(valor);
}

void imprimir_nombre_paso(t_paso* valor){
	printf("%s\t%d\t%d\n", valor->nombre, valor->tiempo_requerido, valor->tiempo_pasado);
	log_info(LOGGER, "\t\tNombre: %s\tTiempo requerido: %d\tTiempo pasado: %d\n", valor->nombre, valor->tiempo_requerido, valor->tiempo_pasado);
	free(valor->nombre);
	free(valor);
}

bool igual_id(void* args){
	int* valor = (int*)args;
	return *valor == id_a_remover;
}

void gestionar_mensaje(int conexion){
	Paquete* paquete_recibido = recibir_paquete(conexion, cliente);
	switch(paquete_recibido->header.tipo_mensaje) {
		case FINALIZAR_PEDIDO: ;
			t_pedido* pedido_finalizado = (t_pedido*)paquete_recibido->mensaje;
			bool exito = false;
			printf("El pedido %d del restaurante %s ya fue finalizado", pedido_finalizado->id_pedido, pedido_finalizado->nombre);
			id_a_remover = pedido_finalizado->id_pedido;
			pthread_mutex_lock(&mutex_id_lista);
			if(list_find(id_pedidos, *igual_id)){
				free(list_remove_by_condition(id_pedidos, *igual_id));
				exito = true;
			}
			pthread_mutex_unlock(&mutex_id_lista);

			enviar_mensaje(conexion, R_FINALIZAR_PEDIDO,&exito);
			log_pedido(paquete_recibido->header.tipo_mensaje, *pedido_finalizado, 0);
			free(pedido_finalizado->nombre);
			free(pedido_finalizado);

			break;
		case R_CONSULTAR_RESTAURANTES: ;

			void imprimir_restaurante(char* valor){
				printf("%s\n", valor);
				loggear_restaurante(valor);
				free(valor);
			}

			printf("Restaurantes disponibles:\n");
			t_list* restaurantes = (t_list*)paquete_recibido->mensaje;
			log_codigo(paquete_recibido->header.tipo_mensaje, 0);

			list_iterate(restaurantes, (void*)imprimir_restaurante);
			list_destroy(restaurantes);


			break;

		case R_CONSULTAR_PLATOS:
			printf("Platos disponibles:\n");
			log_codigo(paquete_recibido->header.tipo_mensaje, 0);
			t_list* platos = (t_list*)paquete_recibido->mensaje;
			if(list_is_empty(platos)){
				printf("El restaurante seleccionado no posee platos");
				log_warning(LOGGER, "El restaurante seleccionado no posee platos");
			} else {
				list_iterate(platos, (void*)imprimir_plato);
			}

			list_destroy(platos);
			break;

		case R_SELECCIONAR_RESTAURANTE:
		case R_GUARDAR_PEDIDO:
		case R_ANIADIR_PLATO:
		case R_GUARDAR_PLATO:
		case R_CONFIRMAR_PEDIDO:
		case R_PLATO_LISTO:
		case R_FINALIZAR_PEDIDO:
		case R_TERMINAR_PEDIDO: ;
			bool* operacion_exitosa = (bool*)paquete_recibido->mensaje;
			log_bool(paquete_recibido->header.tipo_mensaje, *operacion_exitosa);
			free(operacion_exitosa);

			break;

		case R_CREAR_PEDIDO: ;
			uint32_t* id = (uint32_t*)paquete_recibido->mensaje;
			if(!*id){
				printf("Se produjo un error, no se pudo crear el pedido");
				log_error(LOGGER, "No se pudo crear correctamente el pedido");
				break;
			}
			printf("Se creo un pedido con el siguiente id: %d\n", *id);
			pthread_mutex_lock(&mutex_id);
			id_actual = *id;
			pthread_mutex_unlock(&mutex_id);
			pthread_mutex_lock(&mutex_id_lista);
			list_add(id_pedidos, id);
			pthread_mutex_unlock(&mutex_id_lista);
			log_id(paquete_recibido->header.tipo_mensaje, *id, 0);

			break;

		case R_CONSULTAR_PEDIDO: ;
			t_consultar_pedido* pedido_consultado = (t_consultar_pedido*)paquete_recibido->mensaje;
			printf("Restaurante del pedido consultado: %s\n", pedido_consultado->restaurante);
			log_restaurante(paquete_recibido->header.tipo_mensaje, pedido_consultado->restaurante, 0);
			gestionar_lista_platos(&(pedido_consultado->estado_pedido));

			free(pedido_consultado->restaurante);
			list_destroy(pedido_consultado->estado_pedido.platos);
			free(pedido_consultado);
			break;

		case R_OBTENER_PEDIDO: ;
			t_estado_pedido* estado_pedido = (t_estado_pedido*)paquete_recibido->mensaje;
			log_codigo(paquete_recibido->header.tipo_mensaje, 0);
			gestionar_lista_platos(estado_pedido);

			list_destroy(estado_pedido->platos);
			free(estado_pedido);
			break;

		case R_OBTENER_RESTAURANTE: ;
			t_info_restaurante* info_restaurante = (t_info_restaurante*)paquete_recibido->mensaje;

			log_codigo(paquete_recibido->header.tipo_mensaje, 0);
			log_obtener_restaurante(*info_restaurante);
			printf("Cantidad cocineros: %d \n",  info_restaurante->cantidad_cocineros);
			printf("Cantidad hornos: %d \n",  info_restaurante->cantidad_hornos);
			printf("Cantidad pedidos: %d \n",  info_restaurante->cantidad_pedidos);
			printf("Posicion x: %d \n",  info_restaurante->coordenadas.pos_x);
			printf("Posicion y: %d \n",  info_restaurante->coordenadas.pos_y);
			list_iterate(info_restaurante->afinidades, (void*)imprimir_plato);
			list_iterate(info_restaurante->recetas, (void*)imprimir_nombre_receta);

			list_destroy(info_restaurante->afinidades);
			list_destroy(info_restaurante->recetas);
			free(info_restaurante);
			break;

		case R_OBTENER_RECETA: ;
			t_list* pasos_receta = (t_list*)paquete_recibido->mensaje;
			log_codigo(paquete_recibido->header.tipo_mensaje, 0);
			log_info(LOGGER, "\tPasos:");
			list_iterate(pasos_receta, (void*)imprimir_nombre_paso);
			list_destroy(pasos_receta);
			break;
			
		default:
			printf("Error al recibir un mensaje\n");
			break;
	}
}

void gestionar_lista_platos(t_estado_pedido* estado_pedido){
	switch(estado_pedido->estado){
		case PENDIENTE:
			printf("Estado: Pendiente\n");
			log_estado("Pendiente");
			break;
		case CONFIRMADO:
			printf("Estado: Confirmado\n");
			log_estado("Confirmado");
			break;
		case TERMINADO:
			printf("Estado: Terminado\n");
			log_estado("Terminado");
			break;
		case INEXISTENTE:
			printf("Pedido no registrado\n");
			log_estado("Pedido no registrado");
			return;
	}

	void imprimir_estado_comida(void* args){
		t_estado_comida* comida = (t_estado_comida*)args;
		log_estado_comida(*comida);
		printf("Plato: %s\tTotal: %d\tListo: %d\n", comida->comida, comida->total, comida->listo);
		free(comida->comida);
		free(comida);
	}

	list_iterate(estado_pedido->platos, (void*)imprimir_estado_comida);
}


void terminar_programa() {
	pthread_join(hilo_receptor, NULL);
	log_destroy(LOGGER);
	config_destroy(CONFIG);
	pthread_mutex_destroy(&mutex_handshake);
	pthread_mutex_destroy(&mutex_id);
	pthread_mutex_destroy(&mutex_id_lista);
	pthread_cond_destroy(&cond_handshake);
}
