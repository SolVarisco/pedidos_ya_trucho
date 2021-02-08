/*
 * cocina.h
 *
 *  Created on: 23 oct. 2020
 *      Author: utnso
 */

#ifndef COCINA_H_
#define COCINA_H_

#include "logger.h"

void preparar_cocina();
void cocineros_listos();
void ejecutar_cocinero(t_cocinero* cocinero);
void accion(int id_cocinero);
void prender_hornos();

#endif /* COCINA_H_ */
