/*
 * planificacion.c
 *
 *  Created on: 23 oct. 2020
 *      Author: utnso
 */

#include "planificacion.h"

pthread_mutex_t mutex_cocineros = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cocineros_activos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_hornos_activos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_reposos_activos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_contador_platos = PTHREAD_MUTEX_INITIALIZER;

void dar_a_cocinero_por_afinidad(t_pcb_pla* pcb_pla) {
	bool cocineros_afines(void* elemento) {
		t_cocinero* cocinero = (t_cocinero*) elemento;

		return !strcmp(cocinero->afinidad, pcb_pla->afinidad);
	}

	bool cocineros_normales(void* elemento) {
		t_cocinero* cocinero = (t_cocinero*) elemento;

		return !strcmp(cocinero->afinidad, "normal");
	}

	bool ordenar_por_carga(void* elemento_1, void* elemento_2) {
		t_cocinero* cocinero_con_menor_carga = (t_cocinero*) elemento_1;
		t_cocinero* cocinero_con_mayor_carga = (t_cocinero*) elemento_2;

		return list_size(cocinero_con_menor_carga->lista_ready) <= list_size(cocinero_con_mayor_carga->lista_ready);
	}

	pthread_mutex_lock(&mutex_cocineros);
	t_list* cocineros_copia = list_duplicate(cocineros);

	t_list* filtrada = list_create();

	filtrada = list_filter(cocineros_copia, cocineros_afines);

	if(!list_size(filtrada)) {
		filtrada = list_filter(cocineros_copia, cocineros_normales);
	}

	list_sort(filtrada, ordenar_por_carga);

	t_cocinero* cocinero_copia_mas_apropiado = list_get(filtrada, 0);

	dar_a_cocinero_por_id(cocinero_copia_mas_apropiado->id_cocinero, pcb_pla);
	pthread_mutex_unlock(&mutex_cocineros);

	list_destroy(cocineros_copia);
	list_destroy(filtrada);

	pthread_mutex_lock(&mutex_contador_platos);
	CONTADOR_PLATOS += 1;
	if(CONTADOR_PLATOS - 1 ==0){
		sem_post(&sem_coordinar_listo);
	}
	pthread_mutex_unlock(&mutex_contador_platos);

	planificar_segun(cocinero_copia_mas_apropiado->id_cocinero);
}

void dar_a_cocinero_por_id(int id_cocinero, t_pcb_pla* pcb_pla) {
	t_cocinero* cocinero = list_get(cocineros, id_cocinero);

	pthread_mutex_lock(&cocinero->mutex_ready);
	list_add(cocinero->lista_ready, pcb_pla);
	pthread_mutex_unlock(&cocinero->mutex_ready);

	log_asignacion_a_cocinero(pcb_pla->id_plato, cocinero->id_cocinero, cocinero->afinidad);
}

void planificar_segun(int id_cocinero) {

	switch (string_a_codigo_algoritmo(ALGORITMO_PLANIFICACION)) {

		case FIFO:
			planificar_segun_fifo(id_cocinero);

			break;
		case RR:
			planificar_segun_rr(id_cocinero);

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

void planificar_segun_fifo(int id_cocinero) {

	sem_t* semaforo_del_cocinero = list_get(sem_cocineros_ejecutar, id_cocinero);
	sem_wait(semaforo_del_cocinero);

	pthread_mutex_lock(&mutex_cocineros);
	t_cocinero* cocinero = (t_cocinero*) list_get(cocineros, id_cocinero);
	pthread_mutex_unlock(&mutex_cocineros);

	pthread_mutex_lock(&cocinero->mutex_ready);
	t_pcb_pla* plato = (t_pcb_pla*) list_remove(cocinero->lista_ready, 0);
	pthread_mutex_unlock(&cocinero->mutex_ready);

	//log_repartidor_cambio_de_cola_planificacion(repartidor->id_repartidor, "fue seleccionado para ejecutar", "EXEC");

	//sem_t* semaforo_planificacion = (sem_t*) list_get(sem_cocineros_planificar, cocinero->id_cocinero);
	//sem_t* semaforo_cocinar = (sem_t*) list_get(sem_cocineros_cocinar, cocinero->id_cocinero);

	t_paso* paso_actual = list_get(plato->receta, 0);

	while (paso_actual) {
		//sem_post(semaforo_cocinar);
		//sem_wait(semaforo_planificacion);
		if(!strcmp(paso_actual->nombre, "Reposar")) {
			log_reposar_por(plato->id_plato, paso_actual->tiempo_requerido);
			break;
		}

		if(!strcmp(paso_actual->nombre, "Hornear")) {
			log_hornear_por(plato->id_plato, paso_actual->tiempo_requerido);
			break;
		}

		pthread_mutex_lock(&mutex_cocineros_activos);
		COCINEROS_ACTIVOS += 1;
		pthread_mutex_unlock(&mutex_cocineros_activos);

		sem_wait(&sem_cocineros_cocinen);
		sleep(RETARDO_CICLO_CPU);

		pthread_mutex_lock(&mutex_cocineros_activos);
		COCINEROS_ACTIVOS -= 1;
		pthread_mutex_unlock(&mutex_cocineros_activos);

		sem_post(&sem_cocinero_ya_cocino);

		paso_actual->tiempo_pasado += 1;
		log_accion_status(plato->id_plato, paso_actual->nombre, paso_actual->tiempo_pasado);

		if(paso_actual->tiempo_pasado >= paso_actual->tiempo_requerido) {
			free(list_remove(plato->receta, 0));
			if(list_is_empty(plato->receta)) {
				paso_actual = NULL;
			}
			else {
				paso_actual = (t_paso*) list_get(plato->receta, 0);
			}

		}
	}

	sem_post(semaforo_del_cocinero);

	if(!paso_actual) {
		//plato_listo
		notificar(plato);
	} else if(!strcmp(paso_actual->nombre, "Reposar")) {
		//bloquear a reposar
		reposar(cocinero->id_cocinero, plato);
	} else if(!strcmp(paso_actual->nombre, "Hornear")) {
		//bloquear a hornear
		hornear(cocinero->id_cocinero, plato);
	}
}

void planificar_segun_rr(int id_cocinero) {

	sem_t* semaforo_del_cocinero = list_get(sem_cocineros_ejecutar, id_cocinero);
	sem_wait(semaforo_del_cocinero);

	pthread_mutex_lock(&mutex_cocineros);
	t_cocinero* cocinero = (t_cocinero*) list_get(cocineros, id_cocinero);
	pthread_mutex_unlock(&mutex_cocineros);

	pthread_mutex_lock(&cocinero->mutex_ready);
	t_pcb_pla* plato = (t_pcb_pla*) list_remove(cocinero->lista_ready, 0);
	pthread_mutex_unlock(&cocinero->mutex_ready);

	//log_repartidor_cambio_de_cola_planificacion(repartidor->id_repartidor, "fue seleccionado para ejecutar", "EXEC");

	//sem_t* semaforo_planificacion = (sem_t*) list_get(sem_cocineros_planificar, cocinero->id_cocinero);
	//sem_t* semaforo_cocinar = (sem_t*) list_get(sem_cocineros_cocinar, cocinero->id_cocinero);

	t_paso* paso_actual = list_get(plato->receta, 0);

	while (paso_actual) {
		//sem_post(semaforo_cocinar);
		//sem_wait(semaforo_planificacion);
		if(cocinero->quantum == 0) {
			log_fin_de_quantum(plato->id_plato, paso_actual->tiempo_requerido - paso_actual->tiempo_pasado);
			break;
		}
		if(!strcmp(paso_actual->nombre, "Reposar")) {
			log_reposar_por(plato->id_plato, paso_actual->tiempo_requerido);
			break;
		}

		if(!strcmp(paso_actual->nombre, "Hornear")) {
			log_hornear_por(plato->id_plato, paso_actual->tiempo_requerido);
			break;
		}

		pthread_mutex_lock(&mutex_cocineros_activos);
		COCINEROS_ACTIVOS += 1;
		pthread_mutex_unlock(&mutex_cocineros_activos);

		sem_wait(&sem_cocineros_cocinen);
		sleep(RETARDO_CICLO_CPU);

		pthread_mutex_lock(&mutex_cocineros_activos);
		COCINEROS_ACTIVOS -= 1;
		pthread_mutex_unlock(&mutex_cocineros_activos);

		sem_post(&sem_cocinero_ya_cocino);

		paso_actual->tiempo_pasado += 1;
		log_accion_status(plato->id_plato, paso_actual->nombre, paso_actual->tiempo_pasado);
		cocinero->quantum -= 1;

		if(paso_actual->tiempo_pasado >= paso_actual->tiempo_requerido) {
			free(list_remove(plato->receta, 0));
			if(list_is_empty(plato->receta)) {
				paso_actual = NULL;
			}
			else {
				paso_actual = (t_paso*) list_get(plato->receta, 0);
			}
		}
	}

	int quantum_restante = cocinero->quantum;
	cocinero->quantum = QUANTUM;

	sem_post(semaforo_del_cocinero);

	if(!paso_actual) {
		//plato_listo
		notificar(plato);
	} else if(quantum_restante == 0) {
		dar_a_cocinero_por_id(cocinero->id_cocinero, plato);
		planificar_segun(cocinero->id_cocinero);
	} else if(!strcmp(paso_actual->nombre, "Reposar")) {
		//bloquear a reposar
		reposar(cocinero->id_cocinero, plato);
	} else if(!strcmp(paso_actual->nombre, "Hornear")) {
		//bloquear a hornear
		hornear(cocinero->id_cocinero, plato);
	}
}

void notificar(t_pcb_pla* plato) {
	t_plato_listo* plato_listo;
	plato_listo = malloc(sizeof(t_plato_listo));
	Paquete* paquete_intermedio;
	/*bool* operacion_exitosa = malloc(sizeof(bool));
	*operacion_exitosa = true;*/

	plato_listo->comida = plato->afinidad;
	plato_listo->restaurante.nombre = NOMBRE_RESTAURANTE;
	plato_listo->restaurante.id_pedido = plato->id_pedido;

	if(SINDICATO_CONECTADO) {
		int conexion_sindicato = crear_conexion(IP_SINDICATO, PUERTO_SINDICATO);

		enviar_mensaje(conexion_sindicato, PLATO_LISTO, plato_listo);

		paquete_intermedio = recibir_paquete(conexion_sindicato, contacto_restaurante);

		//operacion_exitosa = (bool*) paquete_intermedio->mensaje;

		liberar_conexion(conexion_sindicato);
	}

	int conexion_app = crear_conexion(IP_APP, PUERTO_APP);

    enviar_mensaje(conexion_app, PLATO_LISTO, plato_listo);

	liberar_conexion(conexion_app);

	free(plato_listo);
	//free(operacion_exitosa);

	log_plato_terminado(plato->id_plato);

	pthread_mutex_lock(&mutex_contador_platos);
	CONTADOR_PLATOS -= 1;
	pthread_mutex_unlock(&mutex_contador_platos);
}

//TODO dar a cocinero por id
void reposar(int cocinero_id, t_pcb_pla* plato) {
	t_paso* reposo = (t_paso*) list_remove(plato->receta, 0);

	int ya_reposado = 0;

	while(ya_reposado < reposo->tiempo_requerido) {

		pthread_mutex_lock(&mutex_reposos_activos);
		REPOSOS_ACTIVOS += 1;
		pthread_mutex_unlock(&mutex_reposos_activos);

		sem_wait(&sem_reposos_reposen);
		sleep(RETARDO_CICLO_CPU);

		pthread_mutex_lock(&mutex_reposos_activos);
		REPOSOS_ACTIVOS -= 1;
		pthread_mutex_unlock(&mutex_reposos_activos);
		sem_post(&sem_reposo_ya_reposo);

		ya_reposado += 1;
		//log_accion_status(plato->id_plato, "Reposar", ya_reposado);
	}

	free(reposo);

	dar_a_cocinero_por_id(cocinero_id, plato);
	planificar_segun(cocinero_id);
}

void hornear(int cocinero_id, t_pcb_pla* plato) {
	t_paso* horneo = (t_paso*) list_remove(plato->receta, 0);

	sem_wait(&semaforo_hornos_disponibles);
/*
	pthread_mutex_lock(&mutex_hornos_activos);
	HORNOS_ACTIVOS += 1;
	pthread_mutex_unlock(&mutex_hornos_activos);
*/
	int ya_horneado = 0;

	while(ya_horneado < horneo->tiempo_requerido) {

		//if(ya_horneado > 0)
			//sem_post(&sem_horno_ya_horneo);

		pthread_mutex_lock(&mutex_hornos_activos);
		HORNOS_ACTIVOS += 1;
		pthread_mutex_unlock(&mutex_hornos_activos);

	    sem_wait(&sem_hornos_horneen);
		sleep(RETARDO_CICLO_CPU);

		pthread_mutex_lock(&mutex_hornos_activos);
		HORNOS_ACTIVOS -= 1;
		pthread_mutex_unlock(&mutex_hornos_activos);
		sem_post(&sem_horno_ya_horneo);

		ya_horneado += 1;
		log_accion_status(plato->id_plato, "Hornear", ya_horneado);
	}
/*
	pthread_mutex_lock(&mutex_hornos_activos);
	HORNOS_ACTIVOS -= 1;
	pthread_mutex_unlock(&mutex_hornos_activos);
	sem_post(&sem_horno_ya_horneo);
*/
	sem_post(&semaforo_hornos_disponibles);

	free(horneo);

	dar_a_cocinero_por_id(cocinero_id, plato);
	planificar_segun(cocinero_id);
}

void planificador_por_turnos() {
	COCINEROS_ACTIVOS = 0;
	HORNOS_ACTIVOS = 0;
	REPOSOS_ACTIVOS = 0;

	int i;

	int cocineros_a_esperar;
	int hornos_a_esperar;
	int reposos_a_esperar;
	int contador_activos;

	while(1) {

		sem_wait(&sem_coordinar_listo);

		while(1) {

			pthread_mutex_lock(&mutex_cocineros_activos);
			cocineros_a_esperar = COCINEROS_ACTIVOS;
			pthread_mutex_unlock(&mutex_cocineros_activos);

			for(i = 0; i < cocineros_a_esperar; i++) {
				sem_post(&sem_cocineros_cocinen);
			}

			for(i = 0; i < cocineros_a_esperar; i++) {
				sem_wait(&sem_cocinero_ya_cocino);
			}

			pthread_mutex_lock(&mutex_hornos_activos);
			hornos_a_esperar = HORNOS_ACTIVOS;
			pthread_mutex_unlock(&mutex_hornos_activos);

			for(i = 0; i < hornos_a_esperar; i++) {
				sem_post(&sem_hornos_horneen);
			}

			for(i = 0; i < hornos_a_esperar; i++) {
				sem_wait(&sem_horno_ya_horneo);
			}

			pthread_mutex_lock(&mutex_reposos_activos);
			reposos_a_esperar = REPOSOS_ACTIVOS;
			pthread_mutex_unlock(&mutex_reposos_activos);

			for(i = 0; i < reposos_a_esperar; i++) {
				sem_post(&sem_reposos_reposen);
			}

			for(i = 0; i < reposos_a_esperar; i++) {
				sem_wait(&sem_reposo_ya_reposo);
			}

			pthread_mutex_lock(&mutex_contador_platos);
			contador_activos = CONTADOR_PLATOS;
			pthread_mutex_unlock(&mutex_contador_platos);
			if(contador_activos==0)
				break;

		}
	}
}
