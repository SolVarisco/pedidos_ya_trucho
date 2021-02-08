/*
 * planificacion.c
 *
 *  Created on: 26 sep. 2020
 *      Author: utnso
 */

#include "planificacion.h"

pthread_mutex_t mutex_lista_nuevos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_lista_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_lista_bloqueado_esperando_pedido = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_lista_bloqueado_descanso = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_cantidad_cambios_contexto = PTHREAD_MUTEX_INITIALIZER;


pthread_mutex_t mutex_descanzos_activos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_repartidores_activos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_contador_pedidos_repartibles = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_espera_default_activos = PTHREAD_MUTEX_INITIALIZER;

t_repartidor* repartidor_mas_cercano(t_pcb_ped* pcb_pedido) {
	t_repartidor* repartidor_temporal;
	t_repartidor* repartidor_mas_cercano_sin_pedido;

	int distancia_temporal;
	int menor_distancia = 1000;

	pthread_mutex_lock(&mutex_lista_nuevos);
	t_list* repartidores_new = list_duplicate(lista_estado_new);
	pthread_mutex_unlock(&mutex_lista_nuevos);

	if(!list_is_empty(repartidores_new)){
		repartidor_mas_cercano_sin_pedido = list_get(repartidores_new, 0);
		menor_distancia = distancia_entre(repartidor_mas_cercano_sin_pedido->coordenadas, pcb_pedido->coordenadas_restaurante);

		for(int i=0; i < repartidores_new->elements_count; i++){

			if(menor_distancia == 0){
				break;
			}

			repartidor_temporal = list_get(repartidores_new, i);
			distancia_temporal = distancia_entre(repartidor_temporal->coordenadas, pcb_pedido->coordenadas_restaurante);

			if(distancia_temporal < menor_distancia){
				repartidor_mas_cercano_sin_pedido = repartidor_temporal;
				menor_distancia = distancia_temporal;
			}
		}
	}

	list_destroy(repartidores_new);

	pthread_mutex_lock(&mutex_lista_nuevos);
	sacar_repartidor_de_lista(repartidor_mas_cercano_sin_pedido, lista_estado_new);
	pthread_mutex_unlock(&mutex_lista_nuevos);

	log_repartidor_elegido_para_pedido(repartidor_mas_cercano_sin_pedido->id_repartidor, pcb_pedido->id, pcb_pedido->nombre_restaurante, pcb_pedido->coordenadas_restaurante->pos_x, pcb_pedido->coordenadas_restaurante->pos_y);

	pthread_mutex_lock(&mutex_contador_pedidos_repartibles);
	CONTADOR_PEDIDOS_REPARTIBLES += 1;
	printf("repartidor mas cercano: %d", CONTADOR_PEDIDOS_REPARTIBLES);
	if(CONTADOR_PEDIDOS_REPARTIBLES - 1 ==0){
		sem_post(&sem_coordinar_listo);
	}
	pthread_mutex_unlock(&mutex_contador_pedidos_repartibles);

	return repartidor_mas_cercano_sin_pedido;
}

void poner_repartidor_en_ready(t_repartidor* repartidor, t_pcb_ped* pedido, char* razon) {
	repartidor->estado = READY;

	pthread_mutex_lock(&mutex_lista_ready);
	log_repartidor_cambio_de_cola_planificacion(repartidor->id_repartidor, razon, "READY");
	list_add(lista_estado_ready, repartidor);
	pthread_mutex_unlock(&mutex_lista_ready);

	repartidor->pedido_asignado = pedido;
}

void poner_repartidor_en_new(t_repartidor* repartidor) {
	t_cliente_registrado* cliente_registrado;

	repartidor->estado = NEW;

	bool encontrar_cliente_registrado_por_id_cliente(void* elemento) {
		cliente_registrado = (t_cliente_registrado*) elemento;
		return !strcmp(cliente_registrado->id_cliente, repartidor->pedido_asignado->id_cliente);
	}

	pthread_mutex_lock(&mutex_clientes_registrados);
	cliente_registrado = list_find(clientes_registrados, encontrar_cliente_registrado_por_id_cliente);
	pthread_mutex_unlock(&mutex_clientes_registrados);

	t_pedido* pedido = malloc(sizeof(t_pedido));
	pedido->id_pedido = repartidor->pedido_asignado->id;
	pedido->nombre = repartidor->pedido_asignado->nombre_restaurante;

	int conexion_cliente = crear_conexion(cliente_registrado->ip, cliente_registrado->puerto);

	enviar_mensaje(conexion_cliente, FINALIZAR_PEDIDO, pedido);

	recibir_paquete(conexion_cliente, contacto_app);

	liberar_conexion(conexion_cliente);

	if(COMANDA_CONECTADA) {
		int conexion_comanda = crear_conexion(IP_COMANDA, PUERTO_COMANDA);

		enviar_mensaje(conexion_comanda, FINALIZAR_PEDIDO, pedido);

		recibir_paquete(conexion_comanda, contacto_app);

		liberar_conexion(conexion_comanda);
	}

	free(pedido);
	free(repartidor->pedido_asignado);

	pthread_mutex_lock(&mutex_lista_nuevos);
	log_pedido_finalizado(repartidor->id_repartidor);
	list_add(lista_estado_new, repartidor);
	pthread_mutex_unlock(&mutex_lista_nuevos);

	repartidor->pedido_asignado = NULL;
	repartidor->camino_a_cliente = 0;
	repartidor->estimacion_inicial = ESTIMACION_INICIAL;
	repartidor->rafaga_anterior_real = 0;

	pthread_mutex_lock(&mutex_contador_pedidos_repartibles);
	CONTADOR_PEDIDOS_REPARTIBLES -= 1;
	pthread_mutex_unlock(&mutex_contador_pedidos_repartibles);

	sem_post(&sem_buscar_repartidor_mas_cercano);
}

void poner_repartidor_en_block_espera_pedido(t_repartidor* repartidor) {
	repartidor->estado = BLOCKED;

	pthread_mutex_lock(&mutex_lista_bloqueado_esperando_pedido);
	log_repartidor_cambio_de_cola_planificacion(repartidor->id_repartidor, "el repartidor esta esperando el pedido", "BLOQUEDO ESPERANDO PEDIDO");
	list_add(lista_estado_bloqueado_esperando_pedido, repartidor);
	pthread_mutex_unlock(&mutex_lista_bloqueado_esperando_pedido);

	repartidor->camino_a_cliente = 1;

	if(!strcmp(repartidor->pedido_asignado->nombre_restaurante, "DEFAULT")) {
		sem_t sem_defaults_avansen;

		sem_init(&sem_defaults_avansen, 0, 0);

		pthread_mutex_lock(&mutex_espera_default_activos);
		list_add(lista_espera_a_default, &sem_defaults_avansen);
		pthread_mutex_unlock(&mutex_espera_default_activos);

		sem_wait(&sem_defaults_avansen);

		sem_destroy(&sem_defaults_avansen);

		pedido_listo(repartidor->pedido_asignado->id);
		/*pthread_t thread_default;
		pthread_create(&thread_default,NULL,(void*)pedido_listo, 0);
		pthread_detach(thread_default);*/
	} else {
		pthread_mutex_lock(&mutex_contador_pedidos_repartibles);
		CONTADOR_PEDIDOS_REPARTIBLES -= 1;
		pthread_mutex_unlock(&mutex_contador_pedidos_repartibles);

		sem_wait(&repartidor->pedido_asignado->sem_esperando_pedido);

		pthread_mutex_lock(&mutex_contador_pedidos_repartibles);
		CONTADOR_PEDIDOS_REPARTIBLES += 1;
		if(CONTADOR_PEDIDOS_REPARTIBLES - 1 ==0){
			sem_post(&sem_coordinar_listo);
		}
		pthread_mutex_unlock(&mutex_contador_pedidos_repartibles);

		pedido_listo(repartidor->pedido_asignado->id);
	}
}

void poner_repartidor_en_block_descanso(t_repartidor* repartidor) {
	repartidor->estado = BLOCKED;

	pthread_mutex_lock(&mutex_lista_bloqueado_descanso);
	log_repartidor_cambio_de_cola_planificacion(repartidor->id_repartidor, "el repartidor esta descansando", "BLOQUEDO DESCANSO");
	list_add(lista_estado_bloqueado_descanso, repartidor);
	pthread_mutex_unlock(&mutex_lista_bloqueado_descanso);

	descansar(repartidor);
}

void pedido_listo(uint32_t id_pedido) {

	bool estaEnLista(void* elemento) {
		t_repartidor* repartidor = (t_repartidor*) elemento;
		return repartidor->pedido_asignado->id == id_pedido;
	}

	pthread_mutex_lock(&mutex_lista_bloqueado_esperando_pedido);
	t_repartidor* repartidor = list_remove_by_condition(lista_estado_bloqueado_esperando_pedido, estaEnLista);
	pthread_mutex_unlock(&mutex_lista_bloqueado_esperando_pedido);

	poner_repartidor_en_ready(repartidor, repartidor->pedido_asignado, "el pedido esta listo para llevar");
	planificar_segun();
}

void descansar(t_repartidor* repartidor){
	sleep(repartidor->tiempo_de_descanzo);

	int ya_descanzado = 0;

	while(ya_descanzado < repartidor->tiempo_de_descanzo) {

		pthread_mutex_lock(&mutex_descanzos_activos);
		DESCANZOS_ACTIVOS += 1;
		pthread_mutex_unlock(&mutex_descanzos_activos);

		sem_wait(&sem_repartidor_descanse);
		sleep(1);

		pthread_mutex_lock(&mutex_descanzos_activos);
		DESCANZOS_ACTIVOS -= 1;
		pthread_mutex_unlock(&mutex_descanzos_activos);
		sem_post(&sem_repartidor_ya_descanso);

		ya_descanzado += 1;
	}

	repartidor->ciclos_hasta_descanso = 0;

	poner_repartidor_en_ready(repartidor, repartidor->pedido_asignado, "termino de descansar");
	planificar_segun();
}

int distancia_entre(t_coordenadas* desde, t_coordenadas* hasta) {

	if (desde == NULL || hasta == NULL) {
		return -1;
	}

	int distanciaX = abs(desde->pos_x - hasta->pos_x);
	int distanciaY = abs(desde->pos_y - hasta->pos_y);

	return distanciaX + distanciaY;
}

void sacar_repartidor_de_lista(t_repartidor* repartidor, t_list* lista) {
	int a = list_size(lista);
	for(int i=0; i<a ; i++){
		t_repartidor* repartidor_de_lista = list_get(lista, i);
		if(repartidor->id_repartidor == repartidor_de_lista->id_repartidor){
			list_remove(lista, i);
			break;
		}
	}
}

void planificar_segun() {

	switch (string_a_codigo_algoritmo(ALGORITMO_DE_PLANIFICACION)) {

		case FIFO:
			planificar_segun_fifo();

			break;
		case HRRN:
			planificar_segun_hrrn();

			break;
		case SJF:
			planificar_segun_sjf();

			break;
		case ERROR_CODIGO_ALGORITMO:

			break;
		default:

			break;

	}

}

algoritmo_code string_a_codigo_algoritmo(const char* string) {
	for (int i = 0; i < sizeof(conversion_algoritmo) / sizeof(conversion_algoritmo[0]); i++) {
		if (!strcmp(string, conversion_algoritmo[i].str))
			return conversion_algoritmo[i].codigo_algoritmo;
	}
	return ERROR_CODIGO_ALGORITMO;
}

void planificar_segun_fifo() {
	int distancia;

	sem_wait(&sem_planificar);

	pthread_mutex_lock(&mutex_lista_ready);
	t_repartidor* repartidor = (t_repartidor*) list_remove(lista_estado_ready, 0);
	pthread_mutex_unlock(&mutex_lista_ready);

	repartidor->estado = EXEC;

	pthread_mutex_lock(&mutex_cantidad_cambios_contexto);
	cantidad_cambios_contexto+=1;
	pthread_mutex_unlock(&mutex_cantidad_cambios_contexto);

	log_repartidor_cambio_de_cola_planificacion(repartidor->id_repartidor, "fue seleccionado para ejecutar", "EXEC");

	sem_t* semaforo_del_repartidor = (sem_t*) list_get(sem_repartidores_ejecutar, repartidor->id_repartidor);
	sem_t* semaforo_del_repartidor_moviendose = (sem_t*) list_get(sem_repartidores_ejecutar_moviendose, repartidor->id_repartidor);
	sem_post(semaforo_del_repartidor);
	sem_wait(semaforo_del_repartidor_moviendose);
	distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado != NULL ? repartidor->pedido_asignado->coordenadas_restaurante : 0);

	while (distancia != 0 && distancia != -1) {
		sem_post(semaforo_del_repartidor);
		sem_wait(semaforo_del_repartidor_moviendose);

		if(!repartidor->camino_a_cliente)
			distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado->coordenadas_restaurante != NULL ? repartidor->pedido_asignado->coordenadas_restaurante : NULL);
		else
			distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado->coordenadas_cliente != NULL ? repartidor->pedido_asignado->coordenadas_cliente : NULL);

		if(repartidor->ciclos_hasta_descanso == repartidor->frecuencia_de_descanzo)
			break;
		else
			repartidor->ciclos_hasta_descanso++;
	}

	sem_post(&sem_planificar);

	if(repartidor->ciclos_hasta_descanso == repartidor->frecuencia_de_descanzo)
		poner_repartidor_en_block_descanso(repartidor);
	else if(!repartidor->camino_a_cliente)
		poner_repartidor_en_block_espera_pedido(repartidor);
	else
		poner_repartidor_en_new(repartidor);
}

void planificar_segun_sjf(){

	int distancia;

	sem_wait(&sem_planificar);

	pthread_mutex_lock(&mutex_lista_ready);
	ordenar_lista_por_estimacion_sjf(lista_estado_ready);
	t_repartidor* repartidor = (t_repartidor*) list_remove(lista_estado_ready, 0);
	pthread_mutex_unlock(&mutex_lista_ready);

	repartidor->estimacion_inicial = ALPHA * (repartidor->rafaga_anterior_real) + (1-ALPHA)*(repartidor->estimacion_inicial);

	repartidor->estado = EXEC;

	pthread_mutex_lock(&mutex_cantidad_cambios_contexto);
	cantidad_cambios_contexto+=1;
	pthread_mutex_unlock(&mutex_cantidad_cambios_contexto);

	log_repartidor_cambio_de_cola_planificacion(repartidor->id_repartidor, "fue seleccionado para ejecutar", "EXEC");

	sem_t* semaforo_del_repartidor = (sem_t*) list_get(sem_repartidores_ejecutar, repartidor->id_repartidor);
	sem_t* semaforo_del_repartidor_moviendose = (sem_t*) list_get(sem_repartidores_ejecutar_moviendose, repartidor->id_repartidor);
	sem_post(semaforo_del_repartidor);
	sem_wait(semaforo_del_repartidor_moviendose);

	if(!repartidor->camino_a_cliente)
		distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado->coordenadas_restaurante != NULL ? repartidor->pedido_asignado->coordenadas_restaurante : NULL);
	else
		distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado->coordenadas_cliente != NULL ? repartidor->pedido_asignado->coordenadas_cliente : NULL);

	if(distancia < repartidor->frecuencia_de_descanzo)
		repartidor->rafaga_anterior_real = distancia;
	else
		repartidor->rafaga_anterior_real =	repartidor->frecuencia_de_descanzo;

	while (distancia != 0 && distancia != -1) {
		sem_post(semaforo_del_repartidor);
		sem_wait(semaforo_del_repartidor_moviendose);

		if(!repartidor->camino_a_cliente)
			distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado->coordenadas_restaurante != NULL ? repartidor->pedido_asignado->coordenadas_restaurante : NULL);
		else
			distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado->coordenadas_cliente != NULL ? repartidor->pedido_asignado->coordenadas_cliente : NULL);

		if(repartidor->ciclos_hasta_descanso == repartidor->frecuencia_de_descanzo)
			break;
		else
			repartidor->ciclos_hasta_descanso++;
	}

	sem_post(&sem_planificar);

	if(repartidor->ciclos_hasta_descanso == repartidor->frecuencia_de_descanzo)
		poner_repartidor_en_block_descanso(repartidor);
	else if(!repartidor->camino_a_cliente)
		poner_repartidor_en_block_espera_pedido(repartidor);
	else
		poner_repartidor_en_new(repartidor);
}

void planificar_segun_hrrn(){

	int distancia;

	sem_wait(&sem_planificar);

	pthread_mutex_lock(&mutex_lista_ready);
	ordenar_lista_por_estimacion_hrrn(lista_estado_ready);
	t_repartidor* repartidor = (t_repartidor*) list_remove(lista_estado_ready, 0);
	incrementar_wait_time(lista_estado_ready);
	pthread_mutex_unlock(&mutex_lista_ready);

	repartidor->wait_time = 0;

	repartidor->estimacion_inicial = ALPHA * (repartidor->rafaga_anterior_real) + (1-ALPHA)*(repartidor->estimacion_inicial);

	repartidor->estado = EXEC;

	pthread_mutex_lock(&mutex_cantidad_cambios_contexto);
	cantidad_cambios_contexto+=1;
	pthread_mutex_unlock(&mutex_cantidad_cambios_contexto);

	log_repartidor_cambio_de_cola_planificacion(repartidor->id_repartidor, "fue seleccionado para ejecutar", "EXEC");

	sem_t* semaforo_del_repartidor = (sem_t*) list_get(sem_repartidores_ejecutar, repartidor->id_repartidor);
	sem_t* semaforo_del_repartidor_moviendose = (sem_t*) list_get(sem_repartidores_ejecutar_moviendose, repartidor->id_repartidor);
	sem_post(semaforo_del_repartidor);
	sem_wait(semaforo_del_repartidor_moviendose);

	if(!repartidor->camino_a_cliente)
		distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado->coordenadas_restaurante != NULL ? repartidor->pedido_asignado->coordenadas_restaurante : NULL);
	else
		distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado->coordenadas_cliente != NULL ? repartidor->pedido_asignado->coordenadas_cliente : NULL);

	if(distancia < repartidor->frecuencia_de_descanzo)
		repartidor->rafaga_anterior_real = distancia;
	else
		repartidor->rafaga_anterior_real =	repartidor->frecuencia_de_descanzo;

	while (distancia != 0 && distancia != -1) {
		sem_post(semaforo_del_repartidor);
		sem_wait(semaforo_del_repartidor_moviendose);

		if(!repartidor->camino_a_cliente)
			distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado->coordenadas_restaurante != NULL ? repartidor->pedido_asignado->coordenadas_restaurante : NULL);
		else
			distancia = distancia_entre(repartidor->coordenadas, repartidor->pedido_asignado->coordenadas_cliente != NULL ? repartidor->pedido_asignado->coordenadas_cliente : NULL);

		if(repartidor->ciclos_hasta_descanso == repartidor->frecuencia_de_descanzo)
			break;
		else
			repartidor->ciclos_hasta_descanso++;
	}

	sem_post(&sem_planificar);

	if(repartidor->ciclos_hasta_descanso == repartidor->frecuencia_de_descanzo)
		poner_repartidor_en_block_descanso(repartidor);
	else if(!repartidor->camino_a_cliente)
		poner_repartidor_en_block_espera_pedido(repartidor);
	else
		poner_repartidor_en_new(repartidor);
}

void ordenar_lista_por_estimacion_sjf(t_list* list) {

	bool ordenar_sjf(void* elemento1, void* elemento2){
		t_repartidor* repartidor1 = (t_repartidor*) elemento1;
		t_repartidor* repartidor2 = (t_repartidor*) elemento2;

		double estimado_prox_rafaga1 = ALPHA * (repartidor1->rafaga_anterior_real) + (1-ALPHA)*(repartidor1->estimacion_inicial);
		double estimado_prox_rafaga2 = ALPHA * (repartidor2->rafaga_anterior_real) + (1-ALPHA)*(repartidor2->estimacion_inicial);

		return estimado_prox_rafaga1 <= estimado_prox_rafaga2;
	}

	list_sort(list, ordenar_sjf);
}

void ordenar_lista_por_estimacion_hrrn(t_list* list) {

	bool ordenar_hrrn(void* elemento1, void* elemento2){
		t_repartidor* repartidor1 = (t_repartidor*) elemento1;
		t_repartidor* repartidor2 = (t_repartidor*) elemento2;

		double estimado_prox_rafaga1 = ALPHA * (repartidor1->rafaga_anterior_real) + (1-ALPHA)*(repartidor1->estimacion_inicial);
		double estimado_prox_rafaga2 = ALPHA * (repartidor2->rafaga_anterior_real) + (1-ALPHA)*(repartidor2->estimacion_inicial);

		return (repartidor1->wait_time / estimado_prox_rafaga1) <= (repartidor2->wait_time / estimado_prox_rafaga2);
	}

	list_sort(list, ordenar_hrrn);
}

void incrementar_wait_time(t_list* list) {
	t_repartidor* repartidor;
	for(int i = 0; i < list_size(list); i++) {
		repartidor = list_get(list, i);
		repartidor->wait_time += 1;
	}
}

void planificador_por_turnos() {
	REPARTIDORES_ACTIVOS = 0;
	DESCANZOS_ACTIVOS = 0;

	int i;

	int repartidores_a_esperar;
	int descanzos_a_esperar;
	int contador_activos;

	while(1) {

		sem_wait(&sem_coordinar_listo);

		while(1) {

			pthread_mutex_lock(&mutex_repartidores_activos);
			repartidores_a_esperar = REPARTIDORES_ACTIVOS;
			pthread_mutex_unlock(&mutex_repartidores_activos);

			for(i = 0; i < repartidores_a_esperar; i++) {
				sem_post(&sem_repartidores_mov);
			}

			for(i = 0; i < repartidores_a_esperar; i++) {
				sem_wait(&sem_repartidor_ya_mov);
			}

			pthread_mutex_lock(&mutex_descanzos_activos);
			descanzos_a_esperar = DESCANZOS_ACTIVOS;
			pthread_mutex_unlock(&mutex_descanzos_activos);

			for(i = 0; i < descanzos_a_esperar; i++) {
				sem_post(&sem_repartidor_descanse);
			}

			for(i = 0; i < descanzos_a_esperar; i++) {
				sem_wait(&sem_repartidor_ya_descanso);
			}

			pthread_mutex_lock(&mutex_espera_default_activos);
			while(list_size(lista_espera_a_default)) {
				sem_t* sem_default = list_remove(lista_espera_a_default, 0);
				sem_post(sem_default);
			}
			pthread_mutex_unlock(&mutex_espera_default_activos);

			pthread_mutex_lock(&mutex_contador_pedidos_repartibles);
			contador_activos = CONTADOR_PEDIDOS_REPARTIBLES;
			pthread_mutex_unlock(&mutex_contador_pedidos_repartibles);
			if(contador_activos==0)
				break;

		}
	}
}
