/*
 * paginacion.h
 *
 *  Created on: 20 jun. 2021
 *      Author: utnso
 */

#ifndef PAGINACION_H_
#define PAGINACION_H_

#include "estructurasDeMemoria.h"
#include <sys/mman.h>
#include <fcntl.h>


/*	-------------------------------------------------------------------
						ESTRUCTURAS
	-------------------------------------------------------------------*/
typedef struct {
	int idPatota;
	int idPagina;
	int nroFrameEnMemoria;
	int nroFrameEnSwap;
	bool modificado;
	t_list* listadoInfo;
	int espacioLibre;
}t_pagina;

typedef struct {
	int idPatota;
	int pagina;
	bool estado;
}t_frame;

typedef struct{
	int idPatota;
	t_list* tripulantes;
	pthread_mutex_t mutex_tripulantes;
}t_tablaDePaginas;


typedef struct{
	int tipoInfo;
	int offset;
	int size;
	bool ultimo;
	int inicioTCB;
}t_catalogoInfo;

typedef enum{
	inicio,
	tid,
	estado,
	posicionX,
	posicionY,
	proximaInstruccion,
	ptoPCB
}tcb;


t_frame* FRAMES_MEMORIA;

/*	-------------------------------------------------------------------
						FUNCIONES
	-------------------------------------------------------------------*/
bool paginacionInit();
void inicializarBitmaps(void);

bool espacioDisponible(double cantPaginas);
int encontrarPaginaLibreEnSwap(bool reservar);
int encontrarPaginaLibreEnMemoria(bool reservar);
void frameLibreEnMemoria(int nroFrameEnMemoria);
void frameLibreEnSwap(int nroFrameEnSwap);
void paginacion_asignarMemoria(t_pagina* nuevaPagina);
void agregarPaginaEnMemoria(t_pagina* nuevaPagina);

t_tablaDePaginas* get_tablaDePaginas(uint32_t idPatota);
t_pagina* get_paginaPorCondicion(t_list* lista, uint32_t condicion);
t_catalogoInfo* estaTCBenLista(t_list* lista, uint32_t idTripilante, int frame);
t_pagina* get_paginaPorTCByCondicion(t_list* lista, uint32_t idTripulante, int condicion);
int get_indiceInfo(t_list* lista, uint32_t idTripilante, int frame);
int get_indicetablaDePaginas(t_list* lista, int idPatota);
int getIndicePagina(t_list* lista, t_pagina* pagina);
void actualizarPaginaXAlgoritmoReemplazo(t_pagina* pagina);
void actualizarPagina(t_tablaDePaginas* tablaDePaginas, t_pagina* pagina, uint32_t condicion);
void traerPaginaAMemoria(t_pagina* pagina);
uint32_t generarOFFSET(t_catalogoInfo* info, uint32_t offset, uint32_t contador);


/*	-------------------------------------------------------------------
						LRU
	-------------------------------------------------------------------*/
void inicializarLRU(void);
void nuevaPaginaEnMemoriaLRU(t_pagina*);
t_pagina* elegirVictimaLRU(void);
void eliminarPaginaLRU(t_pagina*);
void actualizarPaginaLRU(t_pagina*);


/*	-------------------------------------------------------------------
						CLOCK
	-------------------------------------------------------------------*/
typedef struct {
	t_pagina* pagina;
	int uso;
} t_elemento;

t_elemento* PUNTERO;
int indiceDelClock;



	/* FUNCIONES */
void inicializarClock(int);
void nuevaPaginaEnMemoriaClock(t_pagina*, int );
t_pagina* elegirVictimaClock(void);
int buscarPaginaClock(t_pagina*);
void eliminarPaginaClock(t_pagina*);
void actualizarPaginaClock(t_pagina*);

#endif /* PAGINACION_H_ */
