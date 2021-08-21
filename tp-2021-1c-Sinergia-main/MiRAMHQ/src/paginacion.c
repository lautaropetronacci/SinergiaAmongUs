/*
 * paginacion.c
 *
 *  Created on: 20 jun. 2021
 *      Author: utnso
 */


#include "paginacion.h"

pthread_mutex_t mutex_paginasEnMemoria  = PTHREAD_MUTEX_INITIALIZER;	//LRU
pthread_mutex_t mutex_tablaDePaginas = PTHREAD_MUTEX_INITIALIZER;		//para lista de TABLASdePAGINAS
pthread_mutex_t mutex_bitmapMEMORIA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_bitmapSWAP = PTHREAD_MUTEX_INITIALIZER;

/*	-------------------------------------------------------------------
						INICIALIZACION
	-------------------------------------------------------------------*/
bool paginacionInit(){

	MEMORIA_PARTICIONES = MEMORIA_SIZE/SIZE_PAGINA; /*tamaño de pagina = tamano de marco*/

	int fd;
	size_t filesize;

	fd = open(SWAP_PATH, O_RDWR | O_CREAT | O_EXCL, S_IRWXU); // permiso de read, write, exec para el propietario

	if(fd == -1)
		fd = open(SWAP_PATH, O_RDWR);

	filesize = SWAP_SIZE;
	ftruncate(fd, filesize);

/*Mapeo de SWAP*/
	SWAP = mmap(NULL, SWAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);  /*asigno lo tipos de permiso*/

	if(SWAP == MAP_FAILED){
		log_errorMapeoSwap();
		close(fd);
		return false;
	}

	close(fd);

	SWAP_PARTICIONES = SWAP_SIZE/SIZE_PAGINA;

	inicializarBitmaps();

	switch(ALGORITMO_REEM){
		case LRU:
			inicializarLRU();
			break;
		case CLOCK:
			inicializarClock(MEMORIA_PARTICIONES);
			break;
		default:
			log_algoritmoDeRemplazoInvalido();
			return false;
	}

	TABLAdePAGINAS = list_create();
	return true;
}


void inicializarBitmaps(void){
	int contador;

	MEMORIA_BITMAP = malloc(MEMORIA_PARTICIONES*sizeof(int));
	for(contador = 0; contador < MEMORIA_PARTICIONES; contador++)
			MEMORIA_BITMAP[contador] = 0;

	SWAP_BITMAP = malloc(SWAP_PARTICIONES*sizeof(int));
	for(contador = 0; contador<SWAP_PARTICIONES; contador++)
		SWAP_BITMAP[contador] = 0;

	FRAMES_MEMORIA = malloc(MEMORIA_PARTICIONES*sizeof(t_frame));
	for(contador = 0; contador<MEMORIA_PARTICIONES; contador++){
		t_frame frame;
		frame.idPatota = -1;
		frame.pagina = -1;
		frame.estado = 0;
		FRAMES_MEMORIA[contador] = frame;
	}
}


/*	-------------------------------------------------------------------
						FUNCIONES - CREAR PATOTA
	-------------------------------------------------------------------*/
bool espacioDisponible(double cantPaginas){
	uint32_t cantPaginasLibres = 0;

	pthread_mutex_lock(&mutex_bitmapSWAP);
	for(int i = 0; i < SWAP_PARTICIONES; i++){
		if(SWAP_BITMAP[i] == 0){
			cantPaginasLibres++;
		}
	}
	pthread_mutex_unlock(&mutex_bitmapSWAP);

	if(cantPaginasLibres >= cantPaginas){
		return 1;
	}
	return 0;
}


int encontrarPaginaLibreEnSwap(bool reservar){
	pthread_mutex_lock(&mutex_bitmapSWAP);
	for(int i = 0; i < SWAP_PARTICIONES; i++){
		if(SWAP_BITMAP[i] == 0){
			if(reservar)
				SWAP_BITMAP[i]=1;
			pthread_mutex_unlock(&mutex_bitmapSWAP);
			return i;
		}
	}
	pthread_mutex_unlock(&mutex_bitmapSWAP);
	return -1;
}


int encontrarPaginaLibreEnMemoria(bool reservar){
	pthread_mutex_lock(&mutex_bitmapMEMORIA);
	for(int i = 0; i < MEMORIA_PARTICIONES; i++){
		if(MEMORIA_BITMAP[i]==0){
			if(reservar)
				MEMORIA_BITMAP[i]=1;
			pthread_mutex_unlock(&mutex_bitmapMEMORIA);
			return i;
		}
	}
	pthread_mutex_unlock(&mutex_bitmapMEMORIA);
	return -1;
}

void guardarPCBenSWAP(uint32_t nroFrameEnSwap, t_pcb* pcb){
	memcpy(SWAP + nroFrameEnSwap * SIZE_PAGINA, &(pcb->pid), sizeof(uint32_t));
	memcpy(SWAP + nroFrameEnSwap * SIZE_PAGINA + sizeof(uint32_t), &(pcb->tareasPatota), sizeof(uint32_t));
	msync(SWAP + nroFrameEnSwap * SIZE_PAGINA, sizeof(*pcb), MS_SYNC |  MS_INVALIDATE);
}

void guardarTCBenSwap(uint32_t nroFrameEnSwap, t_tcb* tcb){
	memcpy(SWAP + nroFrameEnSwap * SIZE_PAGINA, &(tcb->tid), sizeof(uint32_t));
	memcpy(SWAP + nroFrameEnSwap * SIZE_PAGINA + sizeof(uint32_t), &(tcb->estado), sizeof(char));
	memcpy(SWAP + nroFrameEnSwap * SIZE_PAGINA + sizeof(uint32_t) + sizeof(char), &(tcb->posicionX), sizeof(uint32_t));
	memcpy(SWAP + nroFrameEnSwap * SIZE_PAGINA + (sizeof(uint32_t)*2) + sizeof(char), &(tcb->posicionY), sizeof(uint32_t));
	memcpy(SWAP + nroFrameEnSwap * SIZE_PAGINA + (sizeof(uint32_t)*3) + sizeof(char), &(tcb->proximaInstruccion), sizeof(uint32_t));
	memcpy(SWAP + nroFrameEnSwap * SIZE_PAGINA + (sizeof(uint32_t)*4) + sizeof(char), &(tcb->ptoPCB), sizeof(uint32_t));
	msync(SWAP + nroFrameEnSwap * SIZE_PAGINA, sizeof(*tcb), MS_SYNC |  MS_INVALIDATE);
}

uint32_t generarOFFSET(t_catalogoInfo* info, uint32_t offset, uint32_t contador){
	uint32_t valor = offset;
	if(info->inicioTCB == contador){
		return valor;
	}

	switch(info->inicioTCB){
		case tid:
			if(contador == estado){
				valor = offset + sizeof(uint32_t);
				return valor;
			}
			valor = offset + sizeof(char) + sizeof(uint32_t)*(contador - 2);
			break;
		case estado:
			valor = offset + sizeof(char) + sizeof(uint32_t)*(contador - 3);
			break;
		case posicionX:
			valor = offset + sizeof(uint32_t)*(contador - 3);
			break;
		case posicionY:
			valor = offset + sizeof(uint32_t)*(contador - 4);
			break;
		case proximaInstruccion:
			valor = offset + sizeof(uint32_t)*(contador - 5);
			break;
	}
	return valor;
}


void modificarFrameEnSwap(int nroFrameEnMemoria, int nroFrameEnSwap, t_pagina* pagina){
	t_link_element* elemento = pagina->listadoInfo->head;
	t_catalogoInfo* info = (t_catalogoInfo*) (elemento->data);
	int offset;

	while(elemento != NULL) {
		if(info->tipoInfo == PCB){
			t_pcb* pcb = malloc(sizeof(*pcb));
			memcpy(&(pcb->pid), MEMORIA + nroFrameEnMemoria * SIZE_PAGINA , sizeof(uint32_t));
			memcpy(&(pcb->tareasPatota), MEMORIA + nroFrameEnMemoria * SIZE_PAGINA + sizeof(uint32_t), sizeof(uint32_t));
			guardarPCBenSWAP(nroFrameEnSwap, pcb);

			free(pcb);
		}
		else if(info->tipoInfo == TAREAS){
			char vectorTareas[info->size];
			offset = nroFrameEnMemoria * SIZE_PAGINA + info->offset;
			memcpy(&(vectorTareas), MEMORIA + offset , info->size);

			offset = nroFrameEnSwap * SIZE_PAGINA + info->offset;
			memcpy(SWAP + offset, &(vectorTareas), info->size);
			msync(SWAP + offset, info->size, MS_SYNC |  MS_INVALIDATE);
			free(vectorTareas);
		}
		else if (info->tipoInfo == TCB){
			if(info->ultimo && info->inicioTCB == tid){
				t_tcb* tcb = malloc(sizeof(*tcb));
				uint32_t nuevoOffset = info->offset + nroFrameEnMemoria * SIZE_PAGINA;
				memcpy(&(tcb->tid), MEMORIA + nuevoOffset, sizeof(uint32_t));
				memcpy(&(tcb->estado), MEMORIA + nuevoOffset + sizeof(uint32_t), sizeof(char));
				memcpy(&(tcb->posicionX), MEMORIA + nuevoOffset + sizeof(uint32_t) + sizeof(char), sizeof(uint32_t));
				memcpy(&(tcb->posicionY), MEMORIA + nuevoOffset + (sizeof(uint32_t)*2) + sizeof(char), sizeof(uint32_t));
				memcpy(&(tcb->proximaInstruccion), MEMORIA + nuevoOffset + (sizeof(uint32_t)*3) + sizeof(char), sizeof(uint32_t));
				memcpy(&(tcb->ptoPCB), MEMORIA + nuevoOffset + (sizeof(uint32_t)*4) + sizeof(char), sizeof(uint32_t));
				guardarTCBenSwap(nroFrameEnSwap, tcb);
				free(tcb);
			}
			else {
				int contador = info->inicioTCB;
				int base = nroFrameEnMemoria * SIZE_PAGINA + info->offset;
				int espacioLibre = info->size;

				while(espacioLibre > 0){
					offset = generarOFFSET(info, base, contador);
					switch(contador){
						case estado:;

							char estado;
							memcpy(&(estado), MEMORIA + offset, sizeof(char));

							base = nroFrameEnSwap * SIZE_PAGINA + info->offset;
							offset = generarOFFSET(info, base, contador);
							memcpy(SWAP + offset, &(estado), sizeof(char));
							msync(SWAP + offset, sizeof(char), MS_SYNC |  MS_INVALIDATE);
							espacioLibre -= sizeof(char);
							break;

						case tid:
						case posicionX ... ptoPCB:;

							uint32_t valor;
							memcpy(&(valor), MEMORIA + offset, sizeof(uint32_t));

							base = nroFrameEnSwap * SIZE_PAGINA + info->offset;
							offset = generarOFFSET(info, base, contador);
							memcpy(SWAP + offset, &(valor), sizeof(uint32_t));
							msync(SWAP + offset, sizeof(uint32_t), MS_SYNC |  MS_INVALIDATE);
							espacioLibre -= sizeof(uint32_t);
							break;
					}
					contador++;
				}
			}
		}
		elemento = elemento->next;
		info = elemento == NULL ? NULL : elemento->data;
	}
}

void frameLibreEnMemoria(int nroFrameEnMemoria){
	memset(MEMORIA + nroFrameEnMemoria * SIZE_PAGINA, 0, SIZE_PAGINA);
}

void frameLibreEnSwap(int nroFrameEnSwap){
	memset(SWAP + nroFrameEnSwap * SIZE_PAGINA, 0, SIZE_PAGINA);
	msync(SWAP + nroFrameEnSwap * SIZE_PAGINA, SIZE_PAGINA, MS_SYNC |  MS_INVALIDATE);
}


void gestionDeVictima(t_pagina* victima){
	if(victima->modificado){
		//victima->nroFrameEnSwap = encontrarPaginaLibreEnSwap(true);
		frameLibreEnSwap(victima->nroFrameEnSwap);	//todo: frame libre en swap OK
		modificarFrameEnSwap(victima->nroFrameEnMemoria, victima->nroFrameEnSwap, victima);	//todo: modificar frame en swap
	}
	frameLibreEnMemoria(victima->nroFrameEnMemoria);	//todo: frame libre en memoria
	victima->nroFrameEnMemoria = -1;
	victima->modificado = false;

	t_tablaDePaginas* tablaDePaginas = get_tablaDePaginas(victima->idPatota);
	int indice = getIndicePagina(tablaDePaginas->tripulantes, victima);
	list_replace(tablaDePaginas->tripulantes, indice, (void*) victima);
}

void paginacion_asignarMemoria(t_pagina* nuevaPagina){
	log_transferenciaASwap();
	t_pagina* victima;

	switch(ALGORITMO_REEM){
		case LRU:
			pthread_mutex_lock(&mutex_paginasEnMemoria);
			victima = elegirVictimaLRU();
			nuevaPaginaEnMemoriaLRU(nuevaPagina);
			pthread_mutex_unlock(&mutex_paginasEnMemoria);
			break;

		case CLOCK:
			pthread_mutex_lock(&mutex_paginasEnMemoria);
			victima = elegirVictimaClock();
			nuevaPaginaEnMemoriaClock(nuevaPagina, victima->nroFrameEnMemoria);
			pthread_mutex_unlock(&mutex_paginasEnMemoria);
			break;
		default:
			log_errorDeAsignacionPorAlgtmRemplazo();
			break;
	}

	nuevaPagina->nroFrameEnMemoria = victima->nroFrameEnMemoria;
	log_paginaVictimaEnMemoria(victima->nroFrameEnMemoria);
	gestionDeVictima(victima);
}


void agregarPaginaEnMemoria(t_pagina* nuevaPagina){
	switch(ALGORITMO_REEM){
		case LRU:
			pthread_mutex_lock(&mutex_paginasEnMemoria);
			nuevaPaginaEnMemoriaLRU(nuevaPagina);
			pthread_mutex_unlock(&mutex_paginasEnMemoria);
			break;

		case CLOCK:
			pthread_mutex_lock(&mutex_paginasEnMemoria);
			nuevaPaginaEnMemoriaClock(nuevaPagina, nuevaPagina->nroFrameEnMemoria);
			pthread_mutex_unlock(&mutex_paginasEnMemoria);
			break;
		default:
			log_errorDeAsignacionPorAlgtmRemplazo();
			break;
	}
}


t_tablaDePaginas* get_tablaDePaginas(uint32_t idPatota){
	t_link_element* elemento = TABLAdePAGINAS->head;
	t_tablaDePaginas* tablaDePagina = (t_tablaDePaginas*) elemento->data;

	while(elemento != NULL) {
		if(tablaDePagina->idPatota == idPatota)
			return tablaDePagina;
		elemento = elemento->next;
		tablaDePagina = elemento == NULL ? NULL : elemento->data;
	}
	return NULL;
}

t_pagina* get_paginaPorCondicion(t_list* lista, uint32_t condicion){
	bool hayAlgunTCB(t_pagina* pagina){
		t_link_element* elemento = pagina->listadoInfo->head;
		t_catalogoInfo* info = (t_catalogoInfo*) elemento->data;

		while(elemento != NULL) {
			if(info->tipoInfo == TCB){
					return true;
			}
			elemento = elemento->next;
			info = elemento == NULL ? NULL : elemento->data;
		}
		return false;
	}

	t_pagina* pagina;
	int size_lista = list_size(lista);
	switch (condicion) {
		case ULTIMA:
			if(size_lista > 0){
				pagina = (t_pagina*)list_get(lista, size_lista - 1);
			}
			return pagina;
			break;
		case PCB:
			if(size_lista > 0){
				pagina = (t_pagina*)list_get(lista, 0);
			}
			return pagina;
			break;
		case TCB:;

			t_link_element* elemento = lista->head;
			pagina = (t_pagina*) elemento->data;

			while(elemento != NULL) {
				if(pagina->nroFrameEnMemoria == -1){
					traerPaginaAMemoria(pagina);
				}
				else
					actualizarPaginaXAlgoritmoReemplazo(pagina);

				if(hayAlgunTCB(pagina)){
					return pagina;
				}
				elemento = elemento->next;
				pagina = elemento == NULL ? NULL : elemento->data;
			}
			return NULL;
			break;
	}
	return NULL;
}

t_catalogoInfo* estaTCBenLista(t_list* lista, uint32_t idTripilante, int frame){
	t_link_element* elemento = lista->head;
	t_catalogoInfo* info = (t_catalogoInfo*) elemento->data;
	uint32_t tid;

	while(elemento != NULL) {
		if(info->tipoInfo == TCB){
			int offset = frame * SIZE_PAGINA + info->offset;
			memcpy(&(tid), MEMORIA + offset, sizeof(uint32_t));
			if(tid == idTripilante){
				return info;
			}
		}
		elemento = elemento->next;
		info = elemento == NULL ? NULL : elemento->data;
	}
	return NULL;
}


t_pagina* get_paginaPorTCByCondicion(t_list* lista, uint32_t idTripulante, int condicion){
	t_link_element* elemento = lista->head;
	t_pagina* pagina = (t_pagina*) elemento->data;
	t_catalogoInfo* info;// = malloc(sizeof(*info));

	while(elemento != NULL) {
		if(pagina->nroFrameEnMemoria == -1){
			traerPaginaAMemoria(pagina);
		}
		pagina->modificado = true;
		actualizarPaginaXAlgoritmoReemplazo(pagina);

		info = estaTCBenLista(pagina->listadoInfo, idTripulante, pagina->nroFrameEnMemoria);

		if(info == NULL){
			elemento = elemento->next;
			pagina = elemento == NULL ? NULL : elemento->data;
		}
		else{
			switch (condicion) {
				case tid:
					return pagina;
					break;
				case estado:
					if(info->offset + sizeof(uint32_t) > SIZE_PAGINA){
						elemento = elemento->next;
						pagina = elemento == NULL ? NULL : elemento->data;
						return pagina;
					}
					return pagina;
					break;
				case posicionX:
				case posicionY:
				case proximaInstruccion:
				case ptoPCB:
					if(info->offset + sizeof(char) + sizeof(uint32_t)*(condicion - 2) > SIZE_PAGINA){
						elemento = elemento->next;
						pagina = elemento == NULL ? NULL : elemento->data;
						return pagina;
					}
					return pagina;
					break;
			}
		}
	}
	return NULL;
}


int get_indiceInfo(t_list* lista, uint32_t idTripilante, int frame){
	t_link_element* elemento = lista->head;
	t_catalogoInfo* info = (t_catalogoInfo*) elemento->data;
	uint32_t tid, indice=0;

	while(elemento != NULL) {
		if(info->tipoInfo == TCB){
			int offset = frame * SIZE_PAGINA + info->offset;
			memcpy(&(tid), MEMORIA + offset, sizeof(uint32_t));
			if(tid == idTripilante){
				return indice;
			}
		}
		elemento = elemento->next;
		info = elemento == NULL ? NULL : elemento->data;
		indice++;
	}
	return -1;
}

int getIndicePagina(t_list* lista, t_pagina* pagina){
	t_link_element* elemento = lista->head;
	t_pagina* info = (t_pagina*) elemento->data;
	int indice = 0;

	while(elemento != NULL) {
		if(info == pagina){
			return indice;
		}
		elemento = elemento->next;
		info = elemento == NULL ? NULL : elemento->data;
		indice++;
	}
	return -1;
}

int get_indicetablaDePaginas(t_list* lista, int idPatota){
	t_link_element* elemento = lista->head;
	t_tablaDePaginas* info = (t_tablaDePaginas*) elemento->data;
	int indice = 0;

	while(elemento != NULL) {
		if(info->idPatota == idPatota){
			return indice;
		}
		elemento = elemento->next;
		info = elemento == NULL ? NULL : elemento->data;
		indice++;
	}
	return -1;
}

void actualizarPaginaXAlgoritmoReemplazo(t_pagina* pagina){
	switch(ALGORITMO_REEM){
		case LRU:
			actualizarPaginaLRU(pagina);
			break;
		case CLOCK:
			actualizarPaginaClock(pagina);
			break;
	}
}


void actualizarPagina(t_tablaDePaginas* tablaDePaginas, t_pagina* pagina, uint32_t condicion){
	switch (condicion) {
		case PCB:

			break;
		case ULTIMA:;

			int size_lista = list_size(tablaDePaginas->tripulantes);
			list_replace(tablaDePaginas->tripulantes, size_lista - 1, (void*)pagina);
			break;
		default:
			break;
	}
}


void traerPaginaAMemoria(t_pagina* pagina){
	pagina->nroFrameEnMemoria = encontrarPaginaLibreEnMemoria(1);

	if(pagina->nroFrameEnMemoria == -1)
		paginacion_asignarMemoria(pagina);

	t_link_element* elemento = pagina->listadoInfo->head;
	t_catalogoInfo* info = (t_catalogoInfo*) (elemento->data);
	int offset;

	while(elemento != NULL) {
		if(info->tipoInfo == PCB){
			t_pcb* pcb = malloc(sizeof(*pcb));
			memcpy(&(pcb->pid), SWAP + pagina->nroFrameEnSwap * SIZE_PAGINA , sizeof(uint32_t));
			memcpy(&(pcb->tareasPatota), SWAP + pagina->nroFrameEnSwap * SIZE_PAGINA + sizeof(uint32_t), sizeof(uint32_t));
			guardarPCBenMemoria(pcb, pagina->nroFrameEnMemoria);
			free(pcb);
		}
		else if(info->tipoInfo == TAREAS){
			char vectorTareas[info->size];
			offset = pagina->nroFrameEnSwap * SIZE_PAGINA + info->offset;
			memcpy(&(vectorTareas), SWAP + offset , info->size);

			offset = pagina->nroFrameEnMemoria * SIZE_PAGINA + info->offset;
			memcpy(MEMORIA + offset, &(vectorTareas), info->size);
			free(vectorTareas);
		}
		else if (info->tipoInfo == TCB){
			if(info->ultimo && info->inicioTCB == tid){
				t_tcb* tcb = malloc(sizeof(*tcb));
				uint32_t nuevoOffset = info->offset + pagina->nroFrameEnSwap * SIZE_PAGINA;
				memcpy(&(tcb->tid), SWAP + nuevoOffset, sizeof(uint32_t));
				memcpy(&(tcb->estado), SWAP + nuevoOffset + sizeof(uint32_t), sizeof(char));
				memcpy(&(tcb->posicionX), SWAP + nuevoOffset + sizeof(uint32_t) + sizeof(char), sizeof(uint32_t));
				memcpy(&(tcb->posicionY), SWAP + nuevoOffset + (sizeof(uint32_t)*2) + sizeof(char), sizeof(uint32_t));
				memcpy(&(tcb->proximaInstruccion), SWAP + nuevoOffset + (sizeof(uint32_t)*3) + sizeof(char), sizeof(uint32_t));
				memcpy(&(tcb->ptoPCB), SWAP + nuevoOffset + (sizeof(uint32_t)*4) + sizeof(char), sizeof(uint32_t));
				guardarTCBenMemoria_Paginacion(tcb, pagina->nroFrameEnMemoria, info->offset);
				free(tcb);
			}
			else {
				int contador = info->inicioTCB;
				int base = pagina->nroFrameEnSwap * SIZE_PAGINA + info->offset;
				int espacioLibre = info->offset;

				while(espacioLibre > 0){
					offset = generarOFFSET(info, base, contador);
					switch(contador){
						case estado:;

							char estado;
							memcpy(&(estado), SWAP + offset, sizeof(char));

							base = pagina->nroFrameEnMemoria * SIZE_PAGINA + info->offset;
							offset = generarOFFSET(info, base, contador);
							memcpy(MEMORIA + offset, &(estado), sizeof(char));
							espacioLibre -= sizeof(char);
							break;

						case tid:
						case posicionX ... ptoPCB:;

							uint32_t valor;
							memcpy(&(valor), SWAP + offset, sizeof(uint32_t));

							base = pagina->nroFrameEnMemoria * SIZE_PAGINA + info->offset;
							offset = generarOFFSET(info, base, contador);
							memcpy(MEMORIA + offset, &(valor), sizeof(uint32_t));
							espacioLibre -= sizeof(uint32_t);
							break;
					}
					contador++;
				}
			}
		}
		elemento = elemento->next;
		info = elemento == NULL ? NULL : elemento->data;
	}
	log_info(LOGGER, "Se paso la informacion del frame %d de swap al %d de memoria principal", pagina->nroFrameEnSwap, pagina->nroFrameEnMemoria);
}

/*	-------------------------------------------------------------------
						FUNCIONES para LRU
	-------------------------------------------------------------------*/

void inicializarLRU(void){
	PAGINASenMEMORIA = list_create();
}

void nuevaPaginaEnMemoriaLRU(t_pagina* nuevaPagina){
	list_add(PAGINASenMEMORIA, nuevaPagina);
}

t_pagina* elegirVictimaLRU(void){
	return (t_pagina*)list_remove(PAGINASenMEMORIA, 0);
}

void eliminarPaginaLRU(t_pagina* paginaUsada){
	pthread_mutex_lock(&mutex_paginasEnMemoria);
	t_link_element* elemento = PAGINASenMEMORIA->head;
	t_pagina* pagina;
	int position = 0;

	while (elemento != NULL){
		pagina = (t_pagina*)elemento->data;
		if(pagina->idPatota == paginaUsada->idPatota && pagina->idPagina == paginaUsada->idPagina){
			list_remove(PAGINASenMEMORIA, position);
			break;
		}
		position ++;
		elemento = elemento->next;
	}
	pthread_mutex_unlock(&mutex_paginasEnMemoria);
}

void actualizarPaginaLRU(t_pagina* paginaUsada){
	eliminarPaginaLRU(paginaUsada);
	nuevaPaginaEnMemoriaLRU(paginaUsada);
}


/*	-------------------------------------------------------------------
						FUNCIONES para CLOCK
	-------------------------------------------------------------------*/
void inicializarClock(int memoriaParticiones){
	indiceDelClock = 0;
	PUNTERO = malloc(sizeof(t_elemento)*memoriaParticiones);
}


void proximaPosicionClock(void){
	if(indiceDelClock < MEMORIA_PARTICIONES - 1)
		indiceDelClock++;
	else
		indiceDelClock = 0;
}

void nuevaPaginaEnMemoriaClock(t_pagina* nuevaPagina, int espacioLibre){
	t_elemento elemento;
	elemento.pagina = nuevaPagina;
	elemento.uso = 1;

	PUNTERO[espacioLibre] = elemento;
}

t_pagina* elegirVictimaClock(void){
	int contador;
	t_pagina* victima = NULL;

	for(int i = 1; i <= 2; i++){
		contador = 0;

		while(contador < MEMORIA_PARTICIONES){
			if(PUNTERO[indiceDelClock].uso == 0){
				victima = PUNTERO[indiceDelClock].pagina;
				break;
			}
			else
				PUNTERO[indiceDelClock].uso = 0;

			proximaPosicionClock();
			if(victima != NULL)
				return victima;

			contador++;
		}
	}
	return victima;
}


int buscarPaginaClock(t_pagina* paginaUsada){
	int posicion = 0;

	while (posicion < MEMORIA_PARTICIONES){
		if(PUNTERO[posicion].pagina == paginaUsada){
			return posicion;
		}
		posicion ++;
	}
	return -1;
}

void eliminarPaginaClock(t_pagina* paginaUsada){
	pthread_mutex_lock(&mutex_paginasEnMemoria);
	int position = buscarPaginaClock(paginaUsada);
	PUNTERO[position].pagina = NULL;
	PUNTERO[position].uso = 0;
	pthread_mutex_unlock(&mutex_paginasEnMemoria);
}

void actualizarPaginaClock(t_pagina* paginaUsada){
	pthread_mutex_lock(&mutex_paginasEnMemoria);
	int position = buscarPaginaClock(paginaUsada);
	PUNTERO[position].uso = 1;
	pthread_mutex_unlock(&mutex_paginasEnMemoria);
}
