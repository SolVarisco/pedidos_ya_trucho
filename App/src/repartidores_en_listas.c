/*
 * utils_planificacion.c
 *
 *  Created on: 15 sep. 2020
 *      Author: utnso
 */

#include "repartidores_en_listas.h"

pthread_mutex_t mutex_id_repartidores = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_repartidores = PTHREAD_MUTEX_INITIALIZER;

void poner_repartidores_en_lista() {

	repartidores = list_create();

	char** coordenadas_repartidores = config_get_array_value(CONFIG, "REPARTIDORES");

	char** frecuencia_descanso_repartidores = config_get_array_value(CONFIG, "FRECUENCIA_DE_DESCANSO");

	char** tiempo_descanso_repartidores = config_get_array_value(CONFIG, "TIEMPO_DE_DESCANSO");


	for(int i = 0; coordenadas_repartidores[i] != NULL; i++) {

		t_coordenadas* coordenadas = malloc(sizeof(t_coordenadas));

		int frecuencia_descanso_repartidor = atoi(&frecuencia_descanso_repartidores[i][0]);
		int tiempo_descanso_repartidor = atoi(&tiempo_descanso_repartidores[i][0]);

		coordenadas->pos_x = atoi(&coordenadas_repartidores[i][0]);
		coordenadas->pos_y = atoi(&coordenadas_repartidores[i][2]);

		uint32_t id_repartidor = generar_id();

		t_repartidor* repartidor = crear_repartidor(id_repartidor, coordenadas, frecuencia_descanso_repartidor, tiempo_descanso_repartidor, NEW);

		pthread_mutex_lock(&mutex_repartidores);
		list_add(repartidores, repartidor);
		pthread_mutex_unlock(&mutex_repartidores);

		pthread_mutex_lock(&mutex_lista_nuevos);
		list_add(lista_estado_new, repartidor);
		pthread_mutex_unlock(&mutex_lista_nuevos);

	}

	pthread_mutex_lock(&mutex_repartidores);
	int cant_repartidores = list_size(repartidores);
	sem_init(&sem_buscar_repartidor_mas_cercano, 0, cant_repartidores);
	pthread_mutex_unlock(&mutex_repartidores);

	free_array(coordenadas_repartidores);
	free_array(frecuencia_descanso_repartidores);
	free_array(tiempo_descanso_repartidores);
}

uint32_t generar_id() {
	pthread_mutex_lock(&mutex_id_repartidores);
	uint32_t id_generado = ID_REPARTIDORES++;
	pthread_mutex_unlock(&mutex_id_repartidores);

	return id_generado;
}

t_repartidor* crear_repartidor(uint32_t id_repartidor, t_coordenadas* coordenadas, int frecuencia_descanso_repartidor, int tiempo_descanso_repartidor, status_code estado) {
	t_repartidor* repartidor = malloc(sizeof(t_repartidor));

	repartidor->id_repartidor = id_repartidor;
	repartidor->coordenadas = coordenadas;
	repartidor->estado = estado;
	repartidor->camino_a_cliente = 0;
	repartidor->mis_ciclos_CPU = 0;
	repartidor->frecuencia_de_descanzo = frecuencia_descanso_repartidor;
	repartidor->tiempo_de_descanzo = tiempo_descanso_repartidor;
	repartidor->rafaga_anterior_real = 0;
	repartidor->ciclos_hasta_descanso = 0;
	repartidor->estimacion_inicial = ESTIMACION_INICIAL;
	repartidor->rafaga_anterior_real = 0;
	repartidor->wait_time = 0;

	return repartidor;
}

void free_array(char** array) {
	for(int i = 0; i < tamano_array(array); i++)
		free(array[i]);
	free(array);
}

int tamano_array(char** array) {
	int i = 0;
	while(array[i]) {
		i++;
	}
	return i;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// EJECUCION ////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void crear_hilos_repartidores() {
	//TODO: creo que al final no estoy usando esta lista
	hilos_repartidores = list_create();
	sem_repartidores_ejecutar = list_create();
	sem_repartidores_ejecutar_moviendose = list_create();

	pthread_mutex_lock(&mutex_repartidores);
	int cantidad_repartidores = list_size(repartidores);
	pthread_t pthread_id[cantidad_repartidores];

	for (int i = 0; i < cantidad_repartidores; i++) {

		t_repartidor* repartidor = (t_repartidor*) list_get(repartidores, i);

		sem_t* semaforo_del_repartidor = malloc(sizeof(sem_t));

		sem_init(semaforo_del_repartidor, 0, 0);

		list_add(sem_repartidores_ejecutar, (void*) semaforo_del_repartidor);

		sem_t* semaforo_del_repartidor_moviendose = malloc(sizeof(sem_t));

		sem_init(semaforo_del_repartidor_moviendose, 0, 0);

		list_add(sem_repartidores_ejecutar_moviendose, (void*) semaforo_del_repartidor_moviendose);

		pthread_create(&pthread_id[i], NULL, (void*) ejecutar_repartidor, repartidor);

		pthread_detach(pthread_id[i]);

		list_add(hilos_repartidores, &pthread_id[i]);
	}
	pthread_mutex_unlock(&mutex_repartidores);

}

void ejecutar_repartidor(t_repartidor* repartidor) {
	sem_t* semaforo_del_repartidor = (sem_t*) list_get(sem_repartidores_ejecutar, repartidor->id_repartidor);
	sem_t* semaforo_del_repartidor_moviendose = (sem_t*) list_get(sem_repartidores_ejecutar_moviendose, repartidor->id_repartidor);
	while(1) {

		sem_wait(semaforo_del_repartidor);

		pthread_mutex_lock(&mutex_repartidores_activos);
		REPARTIDORES_ACTIVOS += 1;
		pthread_mutex_unlock(&mutex_repartidores_activos);

		sem_wait(&sem_repartidores_mov);
		mover_al_repartidor_hasta_destino(repartidor->id_repartidor);

		pthread_mutex_lock(&mutex_repartidores_activos);
		REPARTIDORES_ACTIVOS -= 1;
		pthread_mutex_unlock(&mutex_repartidores_activos);

		sem_post(&sem_repartidor_ya_mov);

		sem_post(semaforo_del_repartidor_moviendose);
	}
}

void mover_al_repartidor_hasta_destino(uint32_t id_repartidor) {

	sleep(RETARDO_CICLO_CPU);

	t_repartidor* repartidor = list_get(repartidores, id_repartidor);

	uint32_t posicion_x_repartidor = repartidor->coordenadas->pos_x;
	uint32_t posicion_y_repartidor = repartidor->coordenadas->pos_y;

	uint32_t posicion_x_destino;
	uint32_t posicion_y_destino;

	if(repartidor->camino_a_cliente) {
		posicion_x_destino = repartidor->pedido_asignado->coordenadas_cliente->pos_x;
		posicion_y_destino = repartidor->pedido_asignado->coordenadas_cliente->pos_y;
	} else {
		posicion_x_destino = repartidor->pedido_asignado->coordenadas_restaurante->pos_x;
		posicion_y_destino = repartidor->pedido_asignado->coordenadas_restaurante->pos_y;
	}

	if (posicion_x_repartidor != posicion_x_destino) {

		int diferencia_en_x = posicion_x_destino - posicion_x_repartidor;
		if (diferencia_en_x > 0) {
			repartidor->coordenadas->pos_x = posicion_x_repartidor + 1;
		} else if (diferencia_en_x < 0) {
			repartidor->coordenadas->pos_x = posicion_x_repartidor - 1;
		}

	} else if (posicion_y_repartidor != posicion_y_destino) {

		int diferencia_en_y = posicion_y_destino - posicion_y_repartidor;
		if (diferencia_en_y > 0) {
			repartidor->coordenadas->pos_y = posicion_y_repartidor + 1;
		} else if (diferencia_en_y < 0) {
			repartidor->coordenadas->pos_y = posicion_y_repartidor - 1;
		}

	}

	log_movimiento_repartidor(id_repartidor, repartidor->pedido_asignado->id, repartidor->coordenadas->pos_x, repartidor->coordenadas->pos_y);
	repartidor->mis_ciclos_CPU++;

}

int llego_al_destino(t_repartidor* repartidor) {

	uint32_t posicion_x_repartidor = repartidor->coordenadas->pos_x;
	uint32_t posicion_y_repartidor = repartidor->coordenadas->pos_y;

	uint32_t posicion_x_destino;
	uint32_t posicion_y_destino;

	if(repartidor->camino_a_cliente) {
		posicion_x_destino = repartidor->pedido_asignado->coordenadas_cliente->pos_x;
		posicion_y_destino = repartidor->pedido_asignado->coordenadas_cliente->pos_y;
	} else {
		posicion_x_destino = repartidor->pedido_asignado->coordenadas_restaurante->pos_x;
		posicion_y_destino = repartidor->pedido_asignado->coordenadas_restaurante->pos_y;
	}

	return (posicion_x_repartidor == posicion_x_destino) && (posicion_y_repartidor == posicion_y_destino);
}

