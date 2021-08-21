/*
 * logger.h
 *
 *  Created on: 13 may. 2021
 *      Author: utnso
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "commonsSinergicas.h"

#include <sys/types.h>
#include <sys/stat.h>

// Info para el Logger y el Config File

#define DIR_CONFIG "/home/utnso/tp-2021-1c-Sinergia/iMongoStore/iMongoStore.config"
#define DIR_LOG "/home/utnso/tp-2021-1c-Sinergia/iMongoStore/iMongoStore.log"

t_log* logger;
t_config* config;
t_contacto contactoIMongoStore;

char* PUNTO_MONTAJE;
int TIEMPO_SINCRONIZACION;
char** POSICIONES_SABOTAJE;
char*IP_ESCUCHA;
char*PUERTO_ESCUCHA;
char*IP_DISCORDIADOR;
char*PUERTO_DISCORDIADOR;
int TAMANIO_BLOQUES;
int CANTIDAD_BLOQUES;
// TODO: Chequear el tipo de la variable global POSICIONES_SABOTAJE
// char POSICIONES_SABOTAJE[];
char* ADDRBLOCKS;
char* ADDRSUPERBLOCKS;


int fdBlocks;
struct stat statbufBlocks;
char* BLOCKS;



//Sabotajes
int sabotajesRecibidos;


void logConexion(uint32_t);
void logIMongoStore();
void logCaracteresAgregados(int);
void logBorrarCaracteres();

#endif /* LOGGER_H_ */
