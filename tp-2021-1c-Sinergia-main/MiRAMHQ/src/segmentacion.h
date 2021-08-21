/*
 * segmentacion.h
 *
 *  Created on: 20 jun. 2021
 *      Author: utnso
 */

#ifndef SEGMENTACION_H_
#define SEGMENTACION_H_

#include "estructurasDeMemoria.h"
#include "Memoria.h"



/*	-------------------------------------------------------------------
						ESTRUCTURAS Y VARIABLES
	-------------------------------------------------------------------*/
typedef struct {
	//int ghost;
	int id;
	int tipoSegmento;
	int nroSegmento;
	int base;
	int free;
	int size;
} t_particion;

typedef struct{
	int idPatota;
	t_list* segmentos;
	pthread_mutex_t mutex_segmentos;
}t_tablaDeSegmentos;

t_list* backupListOCCUPIED;



/*	-------------------------------------------------------------------
						FUNCIONES
	-------------------------------------------------------------------*/
void segmentacionInit();
void* segmentacion_asignarMemoria(uint32_t size);
t_particion* encontrarParticionLibre(int size);
void ajustarSizeDeParticion(t_particion* particion, int size);

void memoriaCompactar(int sig);
int compactarMemoria();
t_particion* compactarListaFREE();
void compactarListaOCCUPIED(int* baseAnteriorOccupied, int* sizeAnteriorOccupied);
void ordenarMemoriaPorBase();
void ordenarListaALLporBase();
void actualizarPtros();

void consolidarParticionesLibres(t_particion* nuevaParticionLibre, uint32_t* indiceListaFREE);
int get_segmentoPorCondicion(t_list* particiones, uint32_t atributo, int condicion);
int get_segmentoTCBxPCByNroSegmento(t_list* particiones, uint32_t idPatota, uint32_t nroSegmento);
t_tablaDeSegmentos* get_tablaDeSegmentos(uint32_t idPatota);
int get_IndiceTablaDeSegmentos(int idPatota);


/*	-------------------------------------------------------------------
						ALGORITMO DE REEMPLAZO
	-------------------------------------------------------------------*/
//	PRIMER AJUSTE
t_particion* encontrarParticionLibre_FIRSTFIT(int size);


//	MEJOR AJUSTE
t_particion* encontrarParticionLibre_BESTFIT(int size);



#endif /* SEGMENTACION_H_ */
