/*
 * estructurasDeMemoria.h
 *
 *  Created on: 16 may. 2021
 *      Author: utnso
 */

#ifndef ESTRUCTURASDEMEMORIA_H_
#define ESTRUCTURASDEMEMORIA_H_

#include <commonsSinergicas.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <nivel-gui/nivel-gui.h>
#include <nivel-gui/tad_nivel.h>
#include <math.h>

#include "logger.h"
#include "gui.h"
#include "segmentacion.h"
#include "paginacion.h"
//#include "Memoria.h"


/*	-------------------------------------------------------------------
						ESTRUCTURAS
	-------------------------------------------------------------------*/
typedef enum{
	LRU,
	CLOCK,
	PAGINACION,
	SEGMENTACION,
	BEST_FIT,
	FIRST_FIT
}comandoInterno;

const static struct {
	comandoInterno codigo;
	const char* str;
} conversionComandoInterno[] = {
		{LRU, "LRU"},
		{CLOCK, "CLOCK"},
		{PAGINACION, "PAGINACION"},
		{SEGMENTACION, "SEGMENTACION"},
		{BEST_FIT, "BEST_FIT"},
		{FIRST_FIT, "FIRST_FIT"}
};

typedef enum{
	BASE,
	PCB,
	TCB,
	TAREAS,
	TCBxPCB,
	ULTIMA
}condicion;


/*	-------------------------------------------------------------------
						LISTAS
	-------------------------------------------------------------------*/
	//SEGMENTACION
t_list* FREE_PARTITIONS;
t_list* OCCUPIED_PARTITIONS;
t_list* ALL_PARTITIONS;
t_list* CANT_SEGMENTOS;

	//PAGINACION
t_list* TABLAdePAGINAS;
t_list* PAGINASenMEMORIA;
t_list* TABLAdeSEGMENTOS;

//MAPA
t_list* tripulantesEnMapa;
t_list* listaSockets;
bool ON;

/*	-------------------------------------------------------------------
						VARIABLES GLOBALES
	-------------------------------------------------------------------*/
void* MEMORIA;
int MEMORIA_PARTICIONES;
int* MEMORIA_BITMAP;
int SWAP_PARTICIONES;
int* SWAP_BITMAP;
int SWAP_SIZE;
void* SWAP;
int MEMORIA_SIZE;
char* DUMP_PATH;
char* SWAP_PATH;

int ALGORITMO_REEM;
int ESQUEMA_MEM;
int CRITERIO_REEM;
int SIZE_PAGINA;


#endif /* ESTRUCTURASDEMEMORIA_H_ */
