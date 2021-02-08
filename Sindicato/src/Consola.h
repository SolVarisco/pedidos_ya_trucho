#ifndef CONSOLA_H_
#define CONSOLA_H_

#include<readline/readline.h>
#include<readline/history.h>
#include<stdio.h>
#include<stddef.h>

#include "afip.h"

void consola(void);
int digits_of(int number);
void CrearRestaurante(char ** parametros_restaurantes);
void CrearReceta(char ** parametros_receta);


#endif /* CONSOLA_H_ */
