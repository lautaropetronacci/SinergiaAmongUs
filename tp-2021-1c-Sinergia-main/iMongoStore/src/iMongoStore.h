/*
 * iMongoStore.h
 *
 *  Created on: 2 may. 2021
 *      Author: utnso
 */

#ifndef IMONGOSTORE_H_
#define IMONGOSTORE_H_


#include "commonsSinergicas.h"
#include "logger.h"
#include "movimientoBloques.h"
#include "archivos.h"
#include "sabotajes.h"

// probando asdasdad

#include <sys/socket.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// includes de FCNTL_H
#include <sys/mman.h>
//#include <sys/stat.h>       /*    logger.h    */
#include <fcntl.h>
//#include <sys/types.h>      /*    logger.h    */
#include <unistd.h>


typedef struct {
	uint32_t Block_size;
	uint32_t Blocks;
	char* Bitmap;
}superBloque;

typedef struct{
	int SIZE;
	int BLOCK_COUNT;
	char** BLOCKS;
	char CARACTER_LLENADO;
	char* MD5_ARCHIVO;
}metadata;

// Config


// Funciones

void inicializacion();
void leerConfiguracion();
void iniciarConfiguracion();
void iniciarLogger();
void realizarEstructuras();
void gestionarMensaje(Paquete*, int);
void gestionarHandshake(Paquete*);
void atenderDiscordiador(int*);
void realizarConexiones();
void atenderCliente(int*);

// threads

pthread_t servidor;
pthread_t AtencionDiscordiador;
pthread_t actualizarBloques;


#endif /* IMONGOSTORE_H_ */
