/*
 * Memoria.h
 *
 *  Created on: 15 may. 2021
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_


#include <commons/config.h>
#include <time.h>
#include <math.h>

#include "estructurasDeMemoria.h"
#include "logger.h"


/*	-------------------------------------------------------------------
						MUTEX
	-------------------------------------------------------------------*/
extern pthread_mutex_t Memoria;
extern pthread_mutex_t sizeMemoria;
extern pthread_mutex_t mutex_ALLParticiones;
extern pthread_mutex_t mutex_OCCUPIEDParticiones;
extern pthread_mutex_t mutex_FREEParticiones;
extern pthread_mutex_t mutex_tablaDeSegmentos;


extern pthread_mutex_t mutex_tablaDePaginas;
extern pthread_mutex_t mutex_bitmapSWAP;
extern pthread_mutex_t mutex_bitmapMEMORIA;




/*	-------------------------------------------------------------------
						VARIABLES GLOBALES
	-------------------------------------------------------------------*/




/*	-------------------------------------------------------------------
						FUNCIONES PRINCIPALES
	-------------------------------------------------------------------*/
bool cargarMemoria (int tamanioMemoria, char* esquemaMemoria, int tamanioPagina, int tamanioSwap, char* pathSwap, char* algoritmoReemplazo, char* criterioReemplazo, char* dumpPath);
bool crearPatota(int idPatota, char* tareas);
bool crearTripulante(int idPatota, int idTripulante, int poscX, int poscY);
bool actualizarUbicacionTripulante(int idPatota, int idTripulante, int poscX, int poscY);
bool actualizarEstadoTripulante(int idPatota, int idTripulante, int status);
t_RSolicitarProximaTarea proximaTareaTripulante(int idPatota, int idTripulante);
bool eliminarTripulante(int idPatota, int idTripulante);


/*	-------------------------------------------------------------------
						FUNCIONES AUXILIARES
	-------------------------------------------------------------------*/
int stringAcomandoInterno(char* comandoRecibido);
void* asignarMemoria(void* parametro);
void guardarPCBenMemoria(t_pcb* nuevoPCB, uint32_t baseONroFrame);
void guardarTCBenMemoria_Paginacion(t_tcb* nuevoTCB, uint32_t nroFrame, int offset);
void guardarTCBenMemoria_Segmentacion(t_tcb* nuevoTCB, uint32_t baseONroFrame);
int crearParticion(uint32_t patota, uint32_t segmento, void* data, uint32_t sizeData);

void imprimirLista(t_list* lista);
void memoriaDump(int sig);

#endif /* MEMORIA_H_ */
