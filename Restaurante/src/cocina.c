/*
 * cocina.c
 *
 *  Created on: 23 oct. 2020
 *      Author: utnso
 */

#include"cocina.h"


void preparar_cocina() {
	char* afinidad;

	void crear_listas_readys_por_afinidad_de_cocineros(void* elemento) {
		afinidad = (char*) elemento;

		t_cocinero* cocinero = malloc(sizeof(t_cocinero));
		cocinero->afinidad = afinidad;
		cocinero->lista_ready = list_create();
		pthread_mutex_init(&cocinero->mutex_ready, NULL);

		list_add(cocineros, cocinero);
	}

	/*t_cocinero* normal = malloc(sizeof(t_cocinero));
	normal->afinidad = "normal";
	normal->lista_ready = list_create();*/
	//list_add(cocineros, normal);

	list_iterate(mi_restaurante->afinidades, crear_listas_readys_por_afinidad_de_cocineros);

	cocineros_listos();
}

void cocineros_listos() {
	sem_cocineros_ejecutar = list_create();
	sem_cocineros_planificar = list_create();
	sem_cocineros_cocinar = list_create();

	int cantidad_cocineros = list_size(cocineros);
	pthread_t pthread_id[cantidad_cocineros];

	for (int i = 0; i < cantidad_cocineros; i++) {

		t_cocinero* cocinero = (t_cocinero*) list_get(cocineros, i);

		cocinero->id_cocinero = i;
		cocinero->quantum = QUANTUM;

		sem_t* semaforo_del_cocinero = malloc(sizeof(sem_t));

		sem_init(semaforo_del_cocinero, 0, 1);

		list_add(sem_cocineros_ejecutar, (void*) semaforo_del_cocinero);

		sem_t* semaforo_de_planificacion = malloc(sizeof(sem_t));

		sem_init(semaforo_de_planificacion, 0, 0);

		list_add(sem_cocineros_planificar, (void*) semaforo_de_planificacion);

		sem_t* semaforo_cocinar = malloc(sizeof(sem_t));

		sem_init(semaforo_cocinar, 0, 0);

		list_add(sem_cocineros_cocinar, (void*) semaforo_cocinar);

		//pthread_create(&pthread_id[i], NULL, (void*) ejecutar_cocinero, cocinero);

		//pthread_detach(pthread_id[i]);
	}

	prender_hornos();
}

void prender_hornos() {
	sem_init(&semaforo_hornos_disponibles, 0, mi_restaurante->cantidad_hornos);
}

void ejecutar_cocinero(t_cocinero* cocinero) {
	sem_t* semaforo_planificacion = (sem_t*) list_get(sem_cocineros_planificar, cocinero->id_cocinero);
	sem_t* semaforo_cocinar = (sem_t*) list_get(sem_cocineros_cocinar, cocinero->id_cocinero);

	while(1) {
		sem_wait(semaforo_cocinar);



		sem_post(semaforo_planificacion);
	}
}
