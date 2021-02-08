/*
 * app.c
 *
 *  Created on: 14 sep. 2020
 *      Author: utnso
 */

#include "app.h"

pthread_mutex_t mutex_id_pedidos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_pedidos_pendientes = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_restaurantes = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cliente_x_restaurante = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_planificacion = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_pedidos_en_planificacion = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_clientes_registrados = PTHREAD_MUTEX_INITIALIZER;

int main(void) {
	init_config();
	init_valores_config();
	init_listas();
	init_logger();
	init_sem_planificador();
	init_listas_estados();

	activar_coordinador();

	poner_repartidores_en_lista();
	crear_hilos_repartidores();

	init_server();

}

void init_config() {
	CONFIG = config_create(APP_CONFIG);
}

void init_valores_config() {
	IP_COMANDA = config_get_string_value(CONFIG, "IP_COMANDA");
	PUERTO_COMANDA = config_get_string_value(CONFIG, "PUERTO_COMANDA");
	IP_ESCUCHA = config_get_string_value(CONFIG, "IP_ESCUCHA");
	PUERTO_ESCUCHA = config_get_string_value(CONFIG, "PUERTO_ESCUCHA");
	RETARDO_CICLO_CPU = config_get_int_value(CONFIG, "RETARDO_CICLO_CPU");
	GRADO_DE_MULTIPROCESAMIENTO = config_get_int_value(CONFIG, "GRADO_DE_MULTIPROCESAMIENTO");
	ALGORITMO_DE_PLANIFICACION = config_get_string_value(CONFIG, "ALGORITMO_DE_PLANIFICACION");
	ALPHA = config_get_double_value(CONFIG, "ALPHA");
	ARCHIVO_LOG = config_get_string_value(CONFIG, "ARCHIVO_LOG");
	PLATOS_DEFAULT = config_get_array_value(CONFIG, "PLATOS_DEFAULT");
	POSICION_REST_DEFAULT_X = config_get_int_value(CONFIG, "POSICION_REST_DEFAULT_X");
	POSICION_REST_DEFAULT_Y = config_get_int_value(CONFIG, "POSICION_REST_DEFAULT_Y");
	ESTIMACION_INICIAL = config_get_int_value(CONFIG, "ESTIMACION_INICIAL");

	contacto_app.ip = IP_ESCUCHA;
	contacto_app.puerto = PUERTO_ESCUCHA;
	contacto_app.modulo = APP;
}

void init_listas() {
	t_restaurante* restaurante_default = malloc(sizeof(t_restaurante));
	restaurante_default->coordenadas = malloc(sizeof(t_coordenadas));
	restaurante_default->nombre = "DEFAULT";
	restaurante_default->coordenadas->pos_x = POSICION_REST_DEFAULT_X;
	restaurante_default->coordenadas->pos_y = POSICION_REST_DEFAULT_Y;
	restaurantes = list_create();
	list_add(restaurantes, restaurante_default);
	lista_cliente_x_restaurante = list_create();
	pedidos_pendientes = list_create();
	clientes_registrados = list_create();
	pedidos_en_planificacion = list_create();
	lista_espera_a_default = list_create();

	CONTADOR_PEDIDOS_REPARTIBLES = 0;
}

void init_logger() {
	LOGGER = log_create(ARCHIVO_LOG, "app_logger", true, LOG_LEVEL_INFO);
}

void init_sem_planificador() {
	sem_init(&sem_planificar, 0, GRADO_DE_MULTIPROCESAMIENTO);
}

void init_listas_estados() {
	lista_estado_new = list_create();
	lista_estado_ready = list_create();
	lista_estado_bloqueado_descanso = list_create();
	lista_estado_bloqueado_esperando_pedido = list_create();
	lista_estado_finished = list_create();
}

void init_server() {
	int socket_servidor = iniciar_servidor(IP_ESCUCHA, PUERTO_ESCUCHA);
	enviar_handshake_a_comanda(socket_servidor);
	while(1) {
		int socket_potencial = esperar_cliente(socket_servidor);
		if(socket_potencial > 0) {
			int* socket_cliente = (int*) malloc(sizeof(int));
			*socket_cliente = socket_potencial;
			pthread_create(&thread_atencion,NULL,(void*)serve_client,socket_cliente);
			pthread_detach(thread_atencion);
		}
	}
}

void enviar_handshake_a_comanda(int socket_servidor) {
	int conexion = crear_conexion(IP_COMANDA, PUERTO_COMANDA);

	bool status = enviar_handshake(conexion, contacto_app, NULL);

	if(status){
		conexion = esperar_cliente(socket_servidor);

		Paquete* paquete_recibido = recibir_paquete(conexion, contacto_app);
		t_handshake* handshake_recibido = (t_handshake*)paquete_recibido->mensaje;

		COMANDA_CONECTADA = handshake_recibido->contacto.modulo == COMANDA;
	} else {
		log_error_comunicacion_con_comanda();
	}

	liberar_conexion(conexion);
}

void serve_client(int* socket_cliente) {
	Paquete* paquete_recibido = recibir_paquete(*socket_cliente, contacto_app);

	process_request(paquete_recibido, *socket_cliente);

	close(*socket_cliente);
}

void process_request(Paquete* paquete_recibido, int socket_cliente) {
	switch(paquete_recibido->header.tipo_mensaje) {
	    case HANDSHAKE:
	    	agregar_contacto(paquete_recibido, socket_cliente);
	    	break;
		case CONSULTAR_RESTAURANTES:
			listar_restaurantes(socket_cliente);
			break;
		case SELECCIONAR_RESTAURANTE:
			seleccionar_restaurante(paquete_recibido, socket_cliente);
			break;
		case CONSULTAR_PLATOS:
			listar_platos(paquete_recibido, socket_cliente);
			break;
		case CREAR_PEDIDO:
			crear_pedido(paquete_recibido, socket_cliente);
			break;
		case ANIADIR_PLATO:
			aniadir_plato(paquete_recibido, socket_cliente);
			break;
		case PLATO_LISTO:
			plato_listo(paquete_recibido, socket_cliente);
			break;
		case CONFIRMAR_PEDIDO: ;
			confirmar_pedido(paquete_recibido, socket_cliente);
			break;
		case CONSULTAR_PEDIDO:
			consultar_pedido(paquete_recibido, socket_cliente);
			break;
		default: break;
	}
}

void dar_a_repartidor_mas_cercano(t_pcb_ped* pcb_pedido) {
	t_repartidor* repartidor = repartidor_mas_cercano(pcb_pedido);
	poner_repartidor_en_ready(repartidor, pcb_pedido, "es el mas cercano al restaurante del pedido");
}

void agregar_contacto(Paquete* paquete_recibido, int socket_cliente) {
	t_handshake* handshake = (t_handshake*) paquete_recibido->mensaje;

	if(handshake->contacto.modulo == RESTAURANTE) {
		t_seleccionar_restaurante* informacion_restaurante = (t_seleccionar_restaurante*) handshake->informacion;

		t_restaurante* restaurante = malloc(sizeof(t_restaurante));
		restaurante->ip = handshake->contacto.ip;
		restaurante->puerto = handshake->contacto.puerto;
		restaurante->nombre = informacion_restaurante->nombre_restaurante;
		restaurante->coordenadas = malloc(sizeof(t_coordenadas));
		restaurante->coordenadas->pos_x = informacion_restaurante->cliente.coordenadas_cliente.pos_x;
		restaurante->coordenadas->pos_y = informacion_restaurante->cliente.coordenadas_cliente.pos_y;

		pthread_mutex_lock(&mutex_restaurantes);
		list_add(restaurantes, restaurante);
		pthread_mutex_unlock(&mutex_restaurantes);
	} else if(handshake->contacto.modulo == CLIENTE) {
		t_cliente* cliente = (t_cliente*) handshake->informacion;

		t_cliente_registrado* cliente_registrado = malloc(sizeof(t_cliente_registrado));
		cliente_registrado->ip = handshake->contacto.ip;
		cliente_registrado->puerto = handshake->contacto.puerto;
		cliente_registrado->id_cliente = cliente->id_cliente;

		pthread_mutex_lock(&mutex_clientes_registrados);
		list_add(clientes_registrados, cliente_registrado);
		pthread_mutex_unlock(&mutex_clientes_registrados);
	}
}

void listar_restaurantes(int socket_cliente) {
	t_restaurante* restaurante;

	void* nombre_restaurante(void* elemento) {
		restaurante = (t_restaurante*) elemento;
		return restaurante->nombre;
	}

	pthread_mutex_lock(&mutex_restaurantes);
	t_list* restaurantes_a_mostrar = list_duplicate(restaurantes);
	pthread_mutex_unlock(&mutex_restaurantes);

	enviar_mensaje(socket_cliente, R_CONSULTAR_RESTAURANTES, list_map(restaurantes_a_mostrar, nombre_restaurante));

	list_destroy(restaurantes_a_mostrar);
}

void seleccionar_restaurante(Paquete* paquete_recibido, int socket_cliente) {
	t_seleccionar_restaurante * seleccionar_restaurante = (t_seleccionar_restaurante *)paquete_recibido->mensaje;
	t_restaurante* restaurante;
	t_cliente_x_restaurante* cliente_x_restaurante;

	bool encontrar_restaurante_por_nombre(void* elemento) {
		restaurante = (t_restaurante*) elemento;
		return !strcmp(restaurante->nombre, seleccionar_restaurante->nombre_restaurante);
	}

	bool encontrar_cliente_x_restaurante_por_id_cliente(void* elemento) {
		cliente_x_restaurante = (t_cliente_x_restaurante*) elemento;
		return !strcmp(cliente_x_restaurante->id_cliente, seleccionar_restaurante->cliente.id_cliente);
	}

	pthread_mutex_lock(&mutex_restaurantes);
	restaurante = list_find(restaurantes, encontrar_restaurante_por_nombre);
	pthread_mutex_unlock(&mutex_restaurantes);

	bool operacion_exitosa = restaurante != NULL;

	enviar_mensaje(socket_cliente, R_SELECCIONAR_RESTAURANTE, &operacion_exitosa);

	if(!operacion_exitosa)
		return;

	pthread_mutex_lock(&mutex_cliente_x_restaurante);
	cliente_x_restaurante = list_find(lista_cliente_x_restaurante, encontrar_cliente_x_restaurante_por_id_cliente);
	pthread_mutex_unlock(&mutex_cliente_x_restaurante);

	if(!cliente_x_restaurante) {
		cliente_x_restaurante = crear_cliente_x_restaurante(socket_cliente, seleccionar_restaurante, restaurante);

		pthread_mutex_lock(&mutex_cliente_x_restaurante);
		list_add(lista_cliente_x_restaurante, cliente_x_restaurante);
		pthread_mutex_unlock(&mutex_cliente_x_restaurante);
	} else {
		cliente_x_restaurante->nombre_restaurante = restaurante->nombre;
		cliente_x_restaurante->ip_restaurante = restaurante->ip;
		cliente_x_restaurante->puerto_restaurante = restaurante->puerto;
		cliente_x_restaurante->socket_restaurante = restaurante->socket;
		cliente_x_restaurante->coordenadas_restaurante->pos_x = restaurante->coordenadas->pos_x;
		cliente_x_restaurante->coordenadas_restaurante->pos_x = restaurante->coordenadas->pos_y;
	}
}

t_cliente_x_restaurante* crear_cliente_x_restaurante(int socket_cliente, t_seleccionar_restaurante * seleccionar_restaurante, t_restaurante* restaurante) {
	t_cliente_x_restaurante* cliente_x_restaurante = malloc(sizeof(t_cliente_x_restaurante));

	cliente_x_restaurante->id_cliente = seleccionar_restaurante->cliente.id_cliente;
	cliente_x_restaurante->nombre_restaurante = restaurante->nombre;
	cliente_x_restaurante->socket_restaurante = restaurante->socket;
	cliente_x_restaurante->ip_restaurante = restaurante->ip;
	cliente_x_restaurante->puerto_restaurante = restaurante->puerto;
	cliente_x_restaurante->coordenadas_restaurante = malloc(sizeof(t_coordenadas));
	cliente_x_restaurante->coordenadas_cliente = malloc(sizeof(t_coordenadas));
	cliente_x_restaurante->coordenadas_restaurante->pos_x = restaurante->coordenadas->pos_x;
	cliente_x_restaurante->coordenadas_restaurante->pos_y = restaurante->coordenadas->pos_y;
	cliente_x_restaurante->coordenadas_cliente->pos_x = seleccionar_restaurante->cliente.coordenadas_cliente.pos_x;
	cliente_x_restaurante->coordenadas_cliente->pos_y = seleccionar_restaurante->cliente.coordenadas_cliente.pos_y;

	return cliente_x_restaurante;
}

void listar_platos(Paquete* paquete_recibido, int socket_cliente) {
	char* id_cliente = (char*) paquete_recibido->mensaje;
	t_cliente_x_restaurante* cliente_x_restaurante;
	t_cliente_x_restaurante* cliente_x_restaurante_aux;

	pthread_mutex_lock(&mutex_cliente_x_restaurante);
	t_list* lista_copia_cliente_x_restaurante = list_duplicate(lista_cliente_x_restaurante);
	pthread_mutex_unlock(&mutex_cliente_x_restaurante);

	bool encontrar_cliente_x_restaurante_por_id_cliente(void* elemento) {
		cliente_x_restaurante_aux = (t_cliente_x_restaurante*) elemento;
		return !strcmp(cliente_x_restaurante_aux->id_cliente, id_cliente);
	}

	cliente_x_restaurante = list_find(lista_copia_cliente_x_restaurante, encontrar_cliente_x_restaurante_por_id_cliente);

	list_destroy(lista_copia_cliente_x_restaurante);

    if(!cliente_x_restaurante) {
    	t_list* error = list_create();
        enviar_mensaje(socket_cliente, R_CONSULTAR_PLATOS, &error);
		return;
	}

	t_list* lista_platos = list_create();

	if(!strcmp(cliente_x_restaurante->nombre_restaurante, "DEFAULT")) {
		for(int i = 0; PLATOS_DEFAULT[i] != NULL; i++)
			list_add(lista_platos, PLATOS_DEFAULT[i]);
	} else {
		int conexion_restaurante = crear_conexion(cliente_x_restaurante->ip_restaurante, cliente_x_restaurante->puerto_restaurante);

		enviar_mensaje(conexion_restaurante, CONSULTAR_PLATOS, id_cliente);

		Paquete* paquete_recibido = recibir_paquete(conexion_restaurante, contacto_app);

		lista_platos = (t_list*) paquete_recibido->mensaje;

		liberar_conexion(conexion_restaurante);
	}

	enviar_mensaje(socket_cliente, R_CONSULTAR_PLATOS, lista_platos);

	list_destroy(lista_platos);
}

void crear_pedido(Paquete* paquete_recibido, int socket_cliente) {
	char* id_cliente = (char*) paquete_recibido->mensaje;
	t_cliente_x_restaurante* cliente_x_restaurante;
	t_cliente_x_restaurante* cliente_x_restaurante_aux;
	uint32_t id_pedido;

	bool encontrar_cliente_x_restaurante_por_id_cliente(void* elemento) {
		cliente_x_restaurante_aux = (t_cliente_x_restaurante*) elemento;
		return !strcmp(cliente_x_restaurante_aux->id_cliente, id_cliente);
	}

	pthread_mutex_lock(&mutex_cliente_x_restaurante);
	cliente_x_restaurante = list_find(lista_cliente_x_restaurante, encontrar_cliente_x_restaurante_por_id_cliente);
	pthread_mutex_unlock(&mutex_cliente_x_restaurante);

        if(!cliente_x_restaurante) {
        	uint32_t error = 0;
        	enviar_mensaje(socket_cliente, R_CREAR_PEDIDO, &error);
            return;
        }

	if(!strcmp(cliente_x_restaurante->nombre_restaurante, "DEFAULT"))
		id_pedido = generar_pedido_id();
	else {
		int conexion_restaurante = crear_conexion(cliente_x_restaurante->ip_restaurante, cliente_x_restaurante->puerto_restaurante);

		enviar_mensaje(conexion_restaurante, CREAR_PEDIDO, id_cliente);

		Paquete* paquete_recibido = recibir_paquete(conexion_restaurante, contacto_app);

		uint32_t* id = (uint32_t*)paquete_recibido->mensaje;

		id_pedido = *id;

		liberar_conexion(conexion_restaurante);
	}

	t_pcb_ped* pedido = crear_pcb(id_pedido, cliente_x_restaurante);

	pthread_mutex_lock(&mutex_pedidos_pendientes);
	list_add(pedidos_pendientes, pedido);
	pthread_mutex_unlock(&mutex_pedidos_pendientes);

	//TODO version inicial
	if(COMANDA_CONECTADA) {
		t_pedido pedido_comanda;

		pedido_comanda.id_pedido = id_pedido;
		pedido_comanda.nombre = cliente_x_restaurante->nombre_restaurante;

		int conexion_comanda = crear_conexion(IP_COMANDA, PUERTO_COMANDA);

		enviar_mensaje(conexion_comanda, GUARDAR_PEDIDO, &pedido_comanda);

		recibir_paquete(conexion_comanda, contacto_app);

		liberar_conexion(conexion_comanda);
	}

	enviar_mensaje(socket_cliente, R_CREAR_PEDIDO, &pedido->id);
}

uint32_t generar_pedido_id() {
	pthread_mutex_lock(&mutex_id_pedidos);
	uint32_t id_generado = ++ID_PEDIDOS;
	pthread_mutex_unlock(&mutex_id_pedidos);

	return id_generado;
}

t_pcb_ped* crear_pcb(int id_pedido, t_cliente_x_restaurante* cliente_x_restaurante) {
	t_pcb_ped* pcb_pedido = malloc(sizeof(t_pcb_ped));

	pcb_pedido->id = id_pedido;
	pcb_pedido->id_cliente = cliente_x_restaurante->id_cliente;
	pcb_pedido->nombre_restaurante = cliente_x_restaurante->nombre_restaurante;
	pcb_pedido->socket_restaurante = cliente_x_restaurante->socket_restaurante;
	pcb_pedido->ip_restaurante = cliente_x_restaurante->ip_restaurante;
	pcb_pedido->puerto_restaurante = cliente_x_restaurante->puerto_restaurante;
	pcb_pedido->coordenadas_restaurante = malloc(sizeof(t_coordenadas));
	pcb_pedido->coordenadas_cliente = malloc(sizeof(t_coordenadas));
	pcb_pedido->coordenadas_restaurante->pos_x = cliente_x_restaurante->coordenadas_restaurante->pos_x;
	pcb_pedido->coordenadas_restaurante->pos_y = cliente_x_restaurante->coordenadas_restaurante->pos_y;
	pcb_pedido->coordenadas_cliente->pos_x = cliente_x_restaurante->coordenadas_cliente->pos_x;
	pcb_pedido->coordenadas_cliente->pos_y = cliente_x_restaurante->coordenadas_cliente->pos_y;

	return pcb_pedido;
}

void aniadir_plato(Paquete* paquete_recibido, int socket_cliente) {
	t_plato* plato = (t_plato*) paquete_recibido->mensaje;
	t_pcb_ped* pedido_pendiente;
	t_pcb_ped* pedido_pendiente_aux;
	Paquete* paquete_intermedio;
	bool* operacion_exitosa = malloc(sizeof(bool));
        *operacion_exitosa = true;

	bool encontrar_pedido_por_id_pedido(void* elemento) {
		pedido_pendiente_aux = (t_pcb_ped*) elemento;
		return pedido_pendiente_aux->id == plato->restaurante.id_pedido && !strcmp(pedido_pendiente_aux->nombre_restaurante, plato->restaurante.nombre);
	}

	pthread_mutex_lock(&mutex_pedidos_pendientes);
	pedido_pendiente = list_find(pedidos_pendientes, encontrar_pedido_por_id_pedido);
	pthread_mutex_unlock(&mutex_pedidos_pendientes);

      	if(pedido_pendiente) {
		if(strcmp(pedido_pendiente->nombre_restaurante, "DEFAULT")) {
			int conexion_restaurante = crear_conexion(pedido_pendiente->ip_restaurante, pedido_pendiente->puerto_restaurante);

			enviar_mensaje(conexion_restaurante, ANIADIR_PLATO, plato);

			paquete_intermedio = recibir_paquete(conexion_restaurante, contacto_app);

			operacion_exitosa = (bool*)paquete_intermedio->mensaje;

			liberar_conexion(conexion_restaurante);

			free(paquete_intermedio);
		}

		//TODO version inicial
		if(COMANDA_CONECTADA && operacion_exitosa) {
			int conexion = crear_conexion(IP_COMANDA, PUERTO_COMANDA);

			enviar_mensaje(conexion, GUARDAR_PLATO, plato);

			paquete_intermedio = recibir_paquete(conexion, contacto_app);

			operacion_exitosa = (bool*)paquete_intermedio->mensaje;

			liberar_conexion(conexion);

			free(paquete_intermedio);
		}
	} else {
        	*operacion_exitosa = false;
	}

	enviar_mensaje(socket_cliente, R_GUARDAR_PEDIDO, operacion_exitosa);

	free(operacion_exitosa);
}

void confirmar_pedido(Paquete* paquete_recibido, int socket_cliente) {
	t_pedido* pedido_solicitado = (t_pedido*)paquete_recibido->mensaje;
	Paquete* paquete_intermedio;
	bool* operacion_exitosa = malloc(sizeof(bool));
	*operacion_exitosa = false;
	t_pcb_ped* pedido;
        t_pcb_ped* pedido_aux;

	bool encontrar_pedido_por_id_pedido(void* elemento) {
		pedido_aux = (t_pcb_ped*) elemento;
		return pedido_aux->id == pedido_solicitado->id_pedido && !strcmp(pedido_aux->nombre_restaurante, pedido_solicitado->nombre);
	}

	if(COMANDA_CONECTADA) {
		int conexion_comanda = crear_conexion(IP_COMANDA, PUERTO_COMANDA);

		enviar_mensaje(conexion_comanda, CONFIRMAR_PEDIDO, pedido_solicitado);

		paquete_intermedio = recibir_paquete(conexion_comanda, contacto_app);

		operacion_exitosa = (bool*)paquete_intermedio->mensaje;

		if(*operacion_exitosa) {
			pthread_mutex_lock(&mutex_pedidos_pendientes);
			pedido = list_remove_by_condition(pedidos_pendientes, encontrar_pedido_por_id_pedido);
			pthread_mutex_unlock(&mutex_pedidos_pendientes);

                        if(pedido) {
				if(strcmp(pedido_solicitado->nombre, "DEFAULT")) {
					int conexion_restaurante = crear_conexion(pedido->ip_restaurante, pedido->puerto_restaurante);

					enviar_mensaje(conexion_restaurante, CONFIRMAR_PEDIDO, pedido_solicitado);

					paquete_intermedio = recibir_paquete(conexion_restaurante, contacto_app);

					operacion_exitosa = (bool*)paquete_intermedio->mensaje;

					liberar_conexion(conexion_restaurante);
				}

				sem_init(&pedido->sem_esperando_pedido, 0, 0);

            			pthread_mutex_lock(&mutex_pedidos_en_planificacion);
				list_add(pedidos_en_planificacion, pedido);
				pthread_mutex_unlock(&mutex_pedidos_en_planificacion);

				pthread_mutex_lock(&mutex_planificacion);
				pthread_create(&thread_planificacion,NULL,(void*)planificar_a_largo_plazo, pedido);
				pthread_detach(thread_planificacion);
				pthread_mutex_unlock(&mutex_planificacion);

			}
		}

		liberar_conexion(conexion_comanda);
	}

	enviar_mensaje(socket_cliente, R_CONFIRMAR_PEDIDO, operacion_exitosa);

	free(paquete_intermedio);
	free(operacion_exitosa);
}

void planificar_a_largo_plazo(t_pcb_ped* pedido) {
	sem_wait(&sem_buscar_repartidor_mas_cercano);
	dar_a_repartidor_mas_cercano(pedido);
	planificar_segun();
}

void consultar_pedido(Paquete* paquete_recibido, int socket_cliente) {
	t_pedido* pedido_solicitado = (t_pedido*)paquete_recibido->mensaje;
	//uint32_t* id_consultado = (uint32_t*) paquete_recibido->mensaje;
	Paquete* paquete_intermedio = malloc(sizeof(Paquete));
	t_consultar_pedido* consulta_pedido = malloc(sizeof(t_consultar_pedido));
	t_estado_pedido* estado_pedido = malloc(sizeof(t_estado_pedido));

	if(COMANDA_CONECTADA) {
		int conexion = crear_conexion(IP_COMANDA, PUERTO_COMANDA);

		enviar_mensaje(conexion, OBTENER_PEDIDO, pedido_solicitado);

		paquete_intermedio = recibir_paquete(conexion, contacto_app);

		estado_pedido = (t_estado_pedido*) paquete_intermedio->mensaje;

		consulta_pedido->estado_pedido.estado = estado_pedido->estado;
		consulta_pedido->estado_pedido.platos = estado_pedido->platos;
		consulta_pedido->restaurante = pedido_solicitado->nombre;

		enviar_mensaje(socket_cliente, R_CONSULTAR_PEDIDO, consulta_pedido);

		liberar_conexion(conexion);
	}

	free(paquete_intermedio);
	free(estado_pedido);
	free(consulta_pedido);
}

void plato_listo(Paquete* paquete_recibido, int socket_cliente) {
	t_plato_listo* plato = (t_plato_listo*)paquete_recibido->mensaje;
	Paquete* paquete_intermedio_obtener_pedido;
	bool* operacion_exitosa = malloc(sizeof(bool));
	*operacion_exitosa = true;
	t_pcb_ped* pedido;
	t_pcb_ped* pedido_aux;

	t_estado_pedido* estado = malloc(sizeof(t_estado_pedido));

	bool encontrar_pedido_por_id_pedido(void* elemento) {
		pedido_aux = (t_pcb_ped*) elemento;
		return pedido_aux->id == plato->restaurante.id_pedido && !strcmp(pedido_aux->nombre_restaurante, plato->restaurante.nombre);
	}

	if(COMANDA_CONECTADA){
		int conexion_comanda = crear_conexion(IP_COMANDA, PUERTO_COMANDA);

		enviar_mensaje(conexion_comanda, PLATO_LISTO, plato);

		paquete_intermedio_obtener_pedido = recibir_paquete(conexion_comanda, contacto_app);

		estado = (t_estado_pedido*) paquete_intermedio_obtener_pedido->mensaje;

		liberar_conexion(conexion_comanda);

		////////////////////////////////////////////////////////////

		if(estado->estado == TERMINADO) {
			pthread_mutex_lock(&mutex_pedidos_en_planificacion);
			pedido = list_find(pedidos_en_planificacion, encontrar_pedido_por_id_pedido);
			pthread_mutex_unlock(&mutex_pedidos_en_planificacion);

			if(pedido)
				sem_post(&pedido->sem_esperando_pedido);
                        else
				*operacion_exitosa = false;
		}
	}

	enviar_mensaje(socket_cliente, R_PLATO_LISTO, operacion_exitosa);

	free(paquete_intermedio_obtener_pedido);
	//TODO: destroy listas
	free(estado);
	free(operacion_exitosa);
}

void activar_coordinador() {
	sem_init(&sem_coordinar_listo, 0, 0);
	sem_init(&sem_repartidores_mov, 0, 0);
	sem_init(&sem_repartidor_ya_mov, 0, 0);
	sem_init(&sem_repartidor_descanse, 0, 0);
	sem_init(&sem_repartidor_ya_descanso, 0, 0);


	pthread_create(&thread_atencion,NULL,(void*)planificador_por_turnos,NULL);
	pthread_detach(thread_atencion);
}
