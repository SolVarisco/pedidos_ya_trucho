/*
 * montaje.h
 *
 *  Created on: 10 dic. 2020
 *      Author: utnso
 */

#ifndef MONTAJE_H_
#define MONTAJE_H_

#include<commons/string.h>
#include<commons/config.h>
#include<dirent.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<sys/stat.h>

char* PUNTO_MONTAJE;
int BLOCKS;
int BLOCK_SIZE;
t_config* CONFIG;

int main(void);
void read_config(void);
bool dir_exist(char* path);

#endif /* MONTAJE_H_ */
