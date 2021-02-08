/*
 * restaurante.c
 *
 *  Created on: 22 oct. 2020
 *      Author: utnso
 */
#include "restaurante.h"

pthread_mutex_t mutex_id_pedidos = PTHREAD_MUTEX_INITIALIZER;

int main (void) {
	init_config();
	read_config();
	init_logger();
	init_listas();
	activar_coordinador();

	int socket_servidor = iniciar_servidor(IP_ESCUCHA, PUERTO_ESCUCHA);

	enviar_handshake_a_sindicato(socket_servidor);
	get_metadata();

	preparar_cocina();

	enviar_handshake_a_app(socket_servidor);

	init_server(socket_servidor);
}


// FUNCIONES
void init_config() {
	CONFIG = config_create(RESTAURANTE_CONFIG);
}

void read_config(){
	IP_ESCUCHA = config_get_string_value(CONFIG, "IP_ESCUCHA");
	PUERTO_ESCUCHA = config_get_string_value(CONFIG, "PUERTO_ESCUCHA");
	IP_SINDICATO = config_get_string_value(CONFIG, "IP_SINDICATO");
	PUERTO_SINDICATO = config_get_string_value(CONFIG, "PUERTO_SINDICATO");
	IP_APP = config_get_string_value(CONFIG, "IP_APP");
	PUERTO_APP = config_get_string_value(CONFIG, "PUERTO_APP");
	QUANTUM = config_get_int_value(CONFIG, "QUANTUM");
	ALGORITMO_PLANIFICACION = config_get_string_value(CONFIG, "ALGORITMO_PLANIFICACION");
	NOMBRE_RESTAURANTE = config_get_string_value(CONFIG, "NOMBRE_RESTAURANTE");
	RETARDO_CICLO_CPU = config_get_int_value(CONFIG, "RETARDO_CICLO_CPU");

	contacto_restaurante.ip = IP_ESCUCHA;
	contacto_restaurante.puerto = PUERTO_ESCUCHA;
	contacto_restaurante.modulo = RESTAURANTE;
}

void init_logger() {
	LOGGER = log_create(ARCHIVO_LOG, "restaurante", true, LOG_LEVEL_INFO);
}

void init_listas() {
	CONTADOR_PLATOS = 0;
	cocineros = list_create();
	platos_demo = list_create();
}

void activar_coordinador() {
	sem_init(&sem_cocineros_cocinen, 0, 0);
	sem_init(&sem_cocinero_ya_cocino, 0, 0);
	sem_init(&sem_hornos_horneen, 0, 0);
	sem_init(&sem_horno_ya_horneo, 0, 0);
	sem_init(&sem_reposos_reposen, 0, 0);
	sem_init(&sem_reposo_ya_reposo, 0, 0);
	sem_init(&sem_coordinar_listo, 0, 0);

	pthread_create(&thread_atencion,NULL,(void*)planificador_por_turnos,NULL);
	pthread_detach(thread_atencion);
}

//conexion para obtener metadata del restaurante
void enviar_handshake_a_sindicato(int socket_servidor) {
	int conexion = crear_conexion(IP_SINDICATO, PUERTO_SINDICATO);

	//TODO revisar bien posicion
	t_seleccionar_restaurante informacion_restaurante;
	informacion_restaurante.cliente.id_cliente = "";
	informacion_restaurante.cliente.coordenadas_cliente.pos_x = 0;
	informacion_restaurante.cliente.coordenadas_cliente.pos_y = 0;
	informacion_restaurante.nombre_restaurante = NOMBRE_RESTAURANTE;

	bool status = enviar_handshake(conexion, contacto_restaurante, &informacion_restaurante);

	if(status){
		conexion = esperar_cliente(socket_servidor);

		Paquete* paquete_recibido = recibir_paquete(conexion, contacto_restaurante);
		t_handshake* handshake_recibido = (t_handshake*)paquete_recibido->mensaje;

		SINDICATO_CONECTADO = handshake_recibido->contacto.modulo == SINDICATO;
	} else {
		log_error_comunicacion_con_sindicato();
	}

	liberar_conexion(conexion);
}

void get_metadata(){
	if(SINDICATO_CONECTADO)
		get_metadata_del_sindicato();
	else
		generar_metadata_local();
}

void get_metadata_del_sindicato() {
	int socket_sindicato = conexion_sindicato();
	int status = enviar_mensaje(socket_sindicato, OBTENER_RESTAURANTE, NOMBRE_RESTAURANTE);

	if(status > 0)
		serve_client(&socket_sindicato);

	liberar_conexion(socket_sindicato);
}

int conexion_sindicato(){
	int socket_sindicato = crear_conexion(IP_SINDICATO, PUERTO_SINDICATO);

	//check_conexion(socket_sindicato);

	while (socket_sindicato <= 0)
		socket_sindicato = crear_conexion(IP_SINDICATO, PUERTO_SINDICATO);

	check_conexion(socket_sindicato);

	return socket_sindicato;
}

void check_conexion(int conexion){
	if (conexion)
		log_info(LOGGER, "Conexion Exitosa");
	else
		perror("Conexion");
}

void generar_metadata_local() {
	mi_restaurante->afinidades = list_create();
	list_add(mi_restaurante->afinidades, "asado jugoso");
	list_add(mi_restaurante->afinidades, "normal");
	mi_restaurante->coordenadas.pos_x = 5;
	mi_restaurante->coordenadas.pos_y = 5;
	mi_restaurante->recetas = list_create();
	list_add(mi_restaurante->recetas, "asado jugoso");
	list_add(mi_restaurante->recetas, "tortilla de papa");
	list_add(mi_restaurante->recetas, "asado completo");
	list_add(mi_restaurante->recetas, "choripan");
	mi_restaurante->cantidad_hornos = 1;
	mi_restaurante->cantidad_cocineros = 3;
	//mi_restaurante.cantidad_pedidos = 0;
}

void init_server(int socket_servidor) {
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

void serve_client(int* socket_cliente) {
	Paquete* paquete_recibido = recibir_paquete(*socket_cliente, contacto_restaurante);

	gestionar_mensaje(paquete_recibido, *socket_cliente);

	close(*socket_cliente);
}

void set_metadata(Paquete* paquete_recibido){
	pthread_mutex_lock(&mutex_metadata);
	mi_restaurante = (t_info_restaurante*)paquete_recibido->mensaje;
	for(int i = 0; i < mi_restaurante->cantidad_cocineros - list_size(mi_restaurante->afinidades); i++)
		list_add(mi_restaurante->afinidades, "normal");

	pthread_mutex_unlock(&mutex_metadata);
}

void enviar_handshake_a_app(int socket_servidor) {
	int conexion = crear_conexion(IP_APP, PUERTO_APP);

	t_seleccionar_restaurante informacion_restaurante;
	informacion_restaurante.cliente.id_cliente = "";
	informacion_restaurante.cliente.coordenadas_cliente.pos_x = mi_restaurante->coordenadas.pos_x;
	informacion_restaurante.cliente.coordenadas_cliente.pos_y = mi_restaurante->coordenadas.pos_y;
	informacion_restaurante.nombre_restaurante = NOMBRE_RESTAURANTE;

	bool status = enviar_handshake(conexion, contacto_restaurante, &informacion_restaurante);

	if(status){
		conexion = esperar_cliente(socket_servidor);

		Paquete* paquete_recibido = recibir_paquete(conexion, contacto_restaurante);
		t_handshake* handshake_recibido = (t_handshake*)paquete_recibido->mensaje;

		APP_CONECTADA = handshake_recibido->contacto.modulo == APP;
	} else {
		log_error_comunicacion_con_app();
	}

	liberar_conexion(conexion);
}

void gestionar_mensaje (Paquete* paquete_recibido, int socket_cliente){
	switch(paquete_recibido->header.tipo_mensaje) {
		case R_OBTENER_RESTAURANTE:
			set_metadata(paquete_recibido);
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
		case CONFIRMAR_PEDIDO:
			confirmar_pedido(paquete_recibido, socket_cliente);
			break;
		case CONSULTAR_PEDIDO:
			consultar_pedido(paquete_recibido, socket_cliente);
			break;
		default:
			break;
	}
}

void listar_platos(Paquete* paquete_recibido, int socket_cliente) {
	if(SINDICATO_CONECTADO) {
		t_list* lista_platos = list_create();

		int conexion_sindicato = crear_conexion(IP_SINDICATO, PUERTO_SINDICATO);

		enviar_mensaje(conexion_sindicato, CONSULTAR_PLATOS, NOMBRE_RESTAURANTE);

		Paquete* paquete_recibido = recibir_paquete(conexion_sindicato, contacto_restaurante);

		lista_platos = (t_list*) paquete_recibido->mensaje;

		liberar_conexion(conexion_sindicato);

		enviar_mensaje(socket_cliente, R_CONSULTAR_RESTAURANTES, lista_platos);

		list_destroy(lista_platos);
	} else {
		enviar_mensaje(socket_cliente, R_CONSULTAR_RESTAURANTES, mi_restaurante->afinidades);
	}
}

void crear_pedido(Paquete* paquete_recibido, int socket_cliente) {
	uint32_t pedido_id = generar_pedido_id();
	Paquete* paquete_intermedio = malloc(sizeof(paquete_intermedio));

	if(SINDICATO_CONECTADO) {
		int conexion_sindicato = crear_conexion(IP_SINDICATO, PUERTO_SINDICATO);
		t_pedido pedido;
		pedido.id_pedido = pedido_id;
		pedido.nombre = NOMBRE_RESTAURANTE;
		enviar_mensaje(conexion_sindicato, GUARDAR_PEDIDO, &pedido);

		paquete_intermedio = recibir_paquete(conexion_sindicato, contacto_restaurante);

		bool* operacion_exitosa = (bool*)paquete_intermedio->mensaje;

		liberar_conexion(conexion_sindicato);

		free(operacion_exitosa);
	}

	enviar_mensaje(socket_cliente, R_CREAR_PEDIDO, &pedido_id);

	free(paquete_intermedio);
}

uint32_t generar_pedido_id() {
	pthread_mutex_lock(&mutex_id_pedidos);
	uint32_t id_generado = ++mi_restaurante->cantidad_pedidos;
	pthread_mutex_unlock(&mutex_id_pedidos);

	return id_generado;
}

void aniadir_plato(Paquete* paquete_recibido, int socket_cliente) {
	t_plato* plato = (t_plato*) paquete_recibido->mensaje;
	bool* operacion_exitosa;

	if(SINDICATO_CONECTADO) {
		int conexion_sindicato = crear_conexion(IP_SINDICATO, PUERTO_SINDICATO);

		enviar_mensaje(conexion_sindicato, GUARDAR_PLATO, plato);

		Paquete* paquete_intermedio = recibir_paquete(conexion_sindicato, contacto_restaurante);

		operacion_exitosa = (bool*)paquete_intermedio->mensaje;

		liberar_conexion(conexion_sindicato);

		free(paquete_intermedio);
	} else {
		list_add(platos_demo, plato);
	}

	enviar_mensaje(socket_cliente, R_GUARDAR_PLATO, operacion_exitosa);

	free(operacion_exitosa);
}

void confirmar_pedido(Paquete* paquete_recibido, int socket_cliente) {
	t_pedido* pedido_solicitado = (t_pedido*) paquete_recibido->mensaje;
	bool operacion_exitosa = true;
	t_list* pedido_a_planificar = list_create();
	Paquete* paquete_intermedio;
	t_estado_pedido* estado_pedido;

	if(SINDICATO_CONECTADO) {
		int conexion_sindicato = crear_conexion(IP_SINDICATO, PUERTO_SINDICATO);

		enviar_mensaje(conexion_sindicato, CONFIRMAR_PEDIDO, pedido_solicitado);

		paquete_intermedio = recibir_paquete(conexion_sindicato, contacto_restaurante);

		estado_pedido = (t_estado_pedido*)paquete_intermedio->mensaje;

		pedido_a_planificar = estado_pedido->platos;

		liberar_conexion(conexion_sindicato);
	} /*else {
		//TODO quedo desactualizado
		agregar_platos_demo(pedido_a_planificar, &pedido_solicitado->id_pedido);
	}*/

	mandar_a_planificar(pedido_a_planificar, &pedido_solicitado->id_pedido);

	enviar_mensaje(socket_cliente, R_CONFIRMAR_PEDIDO, &operacion_exitosa);
}

void agregar_platos_demo(t_list* pedido_a_planificar, uint32_t* id_pedido) {
	void agregar_platos_por_pedido_id(void* elemento) {
		t_plato* plato_a_planificar = (t_plato*) elemento;
		if(plato_a_planificar->restaurante.id_pedido == *id_pedido)
			list_add(pedido_a_planificar, plato_a_planificar);
	}

	list_iterate(platos_demo, agregar_platos_por_pedido_id);
}

void mandar_a_planificar(t_list* pedido_a_planificar, uint32_t*  id_pedido) {
	void pcb_y_cola(void* elemento) {
		t_estado_comida* plato_a_planificar = (t_estado_comida*) elemento;
		int platos_ya_mandados_a_preparar = 0;

		while(platos_ya_mandados_a_preparar < plato_a_planificar->total) {
			platos_ya_mandados_a_preparar += 1;
			t_pcb_pla* pcb_pla = malloc(sizeof(t_pcb_pla));

			//pcb_pla->afinidad = plato_a_planificar->comida;
			pcb_pla->afinidad = malloc(strlen(plato_a_planificar->comida) + 1);
			strcpy(pcb_pla->afinidad, plato_a_planificar->comida);

			//pcb_pla->id_pedido = malloc(sizeof(uint32_t));
			pcb_pla->id_pedido = *id_pedido;

			pcb_pla->receta = duplicar_receta(plato_a_planificar->pasos_receta);

			pcb_pla->id_plato = generar_plato_id();

			log_nuevo_pcb(pcb_pla->id_plato, pcb_pla->id_pedido, pcb_pla->afinidad);

			pthread_create(&thread_genera_pedidos,NULL,(void*)dar_a_cocinero_por_afinidad,pcb_pla);
			pthread_detach(thread_genera_pedidos);
		}
	}

	pthread_mutex_lock(&mutex_genera_pedidos);
	list_iterate(pedido_a_planificar, pcb_y_cola);
	pthread_mutex_unlock(&mutex_genera_pedidos);

	list_destroy(pedido_a_planificar);
}

t_list* duplicar_receta(t_list* self) {
	t_list* duplicated = list_create();
	t_paso* paso;

	int tam_lista = list_size(self);

	for(int i = 0; i < tam_lista; i++) {
		paso = list_get(self, i);
		t_paso* paso_copia = malloc(sizeof(t_paso));

		paso_copia->tiempo_pasado = paso->tiempo_pasado;
		paso_copia->tiempo_requerido = paso->tiempo_requerido;
		paso_copia->nombre = malloc(strlen(paso->nombre) + 1);
		strcpy(paso_copia->nombre, paso->nombre);

		list_add(duplicated, paso_copia);
	}

	return duplicated;
}


void consultar_pedido(Paquete* paquete_recibido, int socket_cliente) {
	t_pedido* pedido_solicitado = (t_pedido*)paquete_recibido->mensaje;
	Paquete* paquete_intermedio = malloc(sizeof(Paquete));
	t_consultar_pedido* consulta_pedido = malloc(sizeof(t_consultar_pedido));
	t_estado_pedido* estado_pedido = malloc(sizeof(t_estado_pedido));

	if(SINDICATO_CONECTADO) {
		int conexion_sindicato = crear_conexion(IP_SINDICATO, PUERTO_SINDICATO);

		enviar_mensaje(conexion_sindicato, OBTENER_PEDIDO, pedido_solicitado);

		paquete_intermedio = recibir_paquete(conexion_sindicato, contacto_restaurante);

		estado_pedido = (t_estado_pedido*) paquete_intermedio->mensaje;

		consulta_pedido->estado_pedido.estado = estado_pedido->estado;
		consulta_pedido->estado_pedido.platos = estado_pedido->platos;
		consulta_pedido->restaurante = pedido_solicitado->nombre;

		enviar_mensaje(socket_cliente, R_CONSULTAR_PEDIDO, consulta_pedido);

		liberar_conexion(conexion_sindicato);
	}

	free(paquete_intermedio);
	free(estado_pedido);
	free(consulta_pedido);
}

uint32_t generar_plato_id() {
	pthread_mutex_lock(&mutex_id_pedidos);
	uint32_t id_generado = ID_PLATOS++;
	pthread_mutex_unlock(&mutex_id_pedidos);

	return id_generado;
}

void terminar_programa() {
	log_destroy(LOGGER);
	config_destroy(CONFIG);
}

////////////////////////////////////////////////////////
t_paso* generar_troceo_demo() {
	t_paso* troceo = malloc(sizeof(t_paso));

	troceo->nombre = "troceo";
	troceo->tiempo_requerido = 4;
	troceo->tiempo_pasado = 0;

	return troceo;
}

t_paso* generar_empanar_demo() {
	t_paso* empanar = malloc(sizeof(t_paso));

	empanar->nombre = "empanar";
	empanar->tiempo_requerido = 5;
	empanar->tiempo_pasado = 0;

	return empanar;
}

t_paso* generar_reposar_demo(int tiempo) {
	t_paso* reposar = malloc(sizeof(t_paso));

	reposar->nombre = "reposar";
	reposar->tiempo_requerido = tiempo;
	reposar->tiempo_pasado = 0;

	return reposar;
}

t_paso* generar_hornear_demo(int tiempo) {
	t_paso* hornear = malloc(sizeof(t_paso));

	hornear->nombre = "hornear";
	hornear->tiempo_requerido = tiempo;
	hornear->tiempo_pasado = 0;

	return hornear;
}

t_paso* generar_preparar_demo(int tiempo) {
	t_paso* preparar = malloc(sizeof(t_paso));

	preparar->nombre = "preparar";
	preparar->tiempo_requerido = tiempo;
	preparar->tiempo_pasado = 0;

	return preparar;
}

t_paso* generar_servir_demo(int tiempo) {
	t_paso* servir = malloc(sizeof(t_paso));

	servir->nombre = "servir";
	servir->tiempo_requerido = tiempo;
	servir->tiempo_pasado = 0;

	return servir;
}

t_paso* generar_cortar_demo(int tiempo) {
	t_paso* cortar = malloc(sizeof(t_paso));

	cortar->nombre = "cortar";
	cortar->tiempo_requerido = tiempo;
	cortar->tiempo_pasado = 0;

	return cortar;
}

t_paso* generar_hervir_demo(int tiempo) {
	t_paso* hervir = malloc(sizeof(t_paso));

	hervir->nombre = "hervir";
	hervir->tiempo_requerido = tiempo;
	hervir->tiempo_pasado = 0;

	return hervir;
}
