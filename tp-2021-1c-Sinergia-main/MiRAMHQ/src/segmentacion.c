/*
 * segmentacion.c
 *
 *  Created on: 20 jun. 2021
 *      Author: utnso
 */


#include "segmentacion.h"


pthread_mutex_t mutex_FREEParticiones = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_OCCUPIEDParticiones = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ALLParticiones = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_tablaDeSegmentos = PTHREAD_MUTEX_INITIALIZER;		//para lista de TABLASdePAGINAS



void segmentacionInit(){
	FREE_PARTITIONS = list_create();
	OCCUPIED_PARTITIONS = list_create();
	ALL_PARTITIONS = list_create();
	TABLAdeSEGMENTOS = list_create();
	backupListOCCUPIED = list_create();

	t_particion* particionInicial = malloc(sizeof(*particionInicial));
	particionInicial->id = 0;
	particionInicial->tipoSegmento = -1;
	particionInicial->nroSegmento = -1;
	particionInicial->free = 1;
	particionInicial->base = 0;
	particionInicial->size = MEMORIA_SIZE;

	list_add(FREE_PARTITIONS, (void*) particionInicial);
	list_add(ALL_PARTITIONS, (void*) particionInicial);

}


void* segmentacion_asignarMemoria(uint32_t size){
	t_particion* particion = encontrarParticionLibre(size);

	while (particion == NULL || particion->size < size){
		if (compactarMemoria()) {
			pthread_mutex_lock(&mutex_FREEParticiones);
			particion = (t_particion*)list_get(FREE_PARTITIONS, 0);
			pthread_mutex_unlock(&mutex_FREEParticiones);
		}
	}

	if (particion->size > size) {
		ajustarSizeDeParticion(particion, size);
	}

	particion->free = 0;

	pthread_mutex_lock(&mutex_OCCUPIEDParticiones);
	list_add(OCCUPIED_PARTITIONS, (void*) particion);
	pthread_mutex_unlock(&mutex_OCCUPIEDParticiones);

	return (void*)particion;
}


t_particion* encontrarParticionLibre(int size) {
	if (CRITERIO_REEM == FIRST_FIT) {
		return encontrarParticionLibre_FIRSTFIT(size);
	}
	else if (CRITERIO_REEM == BEST_FIT) {
		return encontrarParticionLibre_BESTFIT(size);
	}
	return NULL;
}


void ajustarSizeDeParticion(t_particion* particion, int size){
	t_particion* nuevaParticion = malloc(sizeof(*nuevaParticion));
	nuevaParticion->id = 0;
	nuevaParticion->tipoSegmento = -1;
	nuevaParticion->nroSegmento = -1;
	nuevaParticion->free = 1;
	nuevaParticion->size = particion->size - size;
	nuevaParticion->base = particion->base + size;

	particion->size = size;

	pthread_mutex_lock(&mutex_FREEParticiones);
	list_add(FREE_PARTITIONS, (void*) nuevaParticion);
	pthread_mutex_unlock(&mutex_FREEParticiones);

	pthread_mutex_lock(&mutex_ALLParticiones);
	int indiceDeParticion = get_segmentoPorCondicion(ALL_PARTITIONS, particion->base, BASE);
	list_add_in_index(ALL_PARTITIONS, indiceDeParticion + 1, (void*) nuevaParticion);
	pthread_mutex_unlock(&mutex_ALLParticiones);

}




//----------------------------------------------------------------------------------------------
void memoriaCompactar(int sig){
	if(ESQUEMA_MEM == SEGMENTACION){
		compactarMemoria();
		memoriaDump(1);
	}
	else
		log_warning(LOGGER, "Esquema de memoria actual PAGINACION -> no admite compactacion de memoria");
}


int compactarMemoria(){
	if(ESQUEMA_MEM == SEGMENTACION){
		pthread_mutex_lock(&mutex_OCCUPIEDParticiones);
		int ocupadosSize = list_size(OCCUPIED_PARTITIONS);
		pthread_mutex_unlock(&mutex_OCCUPIEDParticiones);

		if (ocupadosSize > 0){
			t_particion* particionLibreCompactada = compactarListaFREE();
			int baseAnteriorOccupied = 0;
			int sizeAnteriorOccupied = 0;

			compactarListaOCCUPIED(&baseAnteriorOccupied, &sizeAnteriorOccupied);
			particionLibreCompactada->base = baseAnteriorOccupied + sizeAnteriorOccupied;

			pthread_mutex_lock(&mutex_ALLParticiones);
			list_add(ALL_PARTITIONS, particionLibreCompactada);
			pthread_mutex_unlock(&mutex_ALLParticiones);

			ordenarMemoriaPorBase();
			ordenarListaALLporBase();
			//actualizarPtros();
			log_compactacion();
			memoriaDump(1);
			return 1;
		}
	}

	return 0;
}

t_particion* compactarListaFREE(){
	int size_particionCompactada = 0;
	t_particion* particionCompactada = malloc(sizeof(*particionCompactada));

	particionCompactada->id = 0;
	particionCompactada->tipoSegmento = -1;
	particionCompactada->nroSegmento = -1;
	particionCompactada->free = 1;

	int size_listaFree = list_size(FREE_PARTITIONS);

	for (int i = 0; i < size_listaFree; i++) {

		pthread_mutex_lock(&mutex_FREEParticiones);
		t_particion* particionLibre = (t_particion*)list_get(FREE_PARTITIONS, i);
		pthread_mutex_unlock(&mutex_FREEParticiones);

		size_particionCompactada += particionLibre->size;

		int indiceEnListaALL = get_segmentoPorCondicion(ALL_PARTITIONS, particionLibre->base, BASE);

		pthread_mutex_lock(&mutex_ALLParticiones);
		list_remove(ALL_PARTITIONS, indiceEnListaALL);
		pthread_mutex_unlock(&mutex_ALLParticiones);

		free(particionLibre);
	}

	particionCompactada->size = size_particionCompactada;

	pthread_mutex_lock(&mutex_FREEParticiones);
	list_clean(FREE_PARTITIONS);
	list_add(FREE_PARTITIONS, (void*) particionCompactada);
	pthread_mutex_unlock(&mutex_FREEParticiones);

	return particionCompactada;
}

int get_indiceParticion(t_particion* particion, t_list* lista){
	t_link_element* elemento = lista->head;
	t_particion* segmento = (t_particion*) elemento->data;
	int indice = 0;

	while(elemento != NULL) {
		if(segmento->nroSegmento == particion->nroSegmento)
			return indice;
		elemento = elemento->next;
		segmento = elemento == NULL ? NULL : elemento->data;
	}
	return -1;
}

void actualizarTablaDeSegmentos(t_particion* particion){
	/*consulta del nro segmento de la patota*/
	pthread_mutex_lock(&mutex_tablaDeSegmentos);
	t_tablaDeSegmentos* tablaDeSegmentos = get_tablaDeSegmentos(particion->id);
	pthread_mutex_unlock(&mutex_tablaDeSegmentos);

	int indice = get_indiceParticion(particion, tablaDeSegmentos->segmentos);
	list_replace(tablaDeSegmentos->segmentos, indice, (void*)particion);
}

void compactarListaOCCUPIED(int* baseAnteriorOccupied, int* sizeAnteriorOccupied){
	pthread_mutex_lock(&mutex_OCCUPIEDParticiones);
	list_clean(backupListOCCUPIED);
	backupListOCCUPIED = OCCUPIED_PARTITIONS;
	int size_listaOCCUPIED = list_size(OCCUPIED_PARTITIONS);
	pthread_mutex_unlock(&mutex_OCCUPIEDParticiones);

	if (size_listaOCCUPIED > 0) {
		pthread_mutex_lock(&mutex_OCCUPIEDParticiones);
		t_particion* particionOcupada = (t_particion*)list_get(OCCUPIED_PARTITIONS, 0);
		pthread_mutex_unlock(&mutex_OCCUPIEDParticiones);

		particionOcupada->base = 0;

		int baseAnterior = particionOcupada->base;
		int sizeAnterior = particionOcupada->size;

		int indiceOcupado = 1;
		actualizarTablaDeSegmentos(particionOcupada);	//TODO TESTEAR

		while(indiceOcupado < size_listaOCCUPIED) {
			pthread_mutex_lock(&mutex_OCCUPIEDParticiones);
			particionOcupada = (t_particion*)list_get(OCCUPIED_PARTITIONS, indiceOcupado);
			pthread_mutex_unlock(&mutex_OCCUPIEDParticiones);

			particionOcupada->base = baseAnterior + sizeAnterior;
			baseAnterior = particionOcupada->base;
			sizeAnterior = particionOcupada->size;
			indiceOcupado++;
			actualizarTablaDeSegmentos(particionOcupada);	//TODO
		}

		*baseAnteriorOccupied = baseAnterior;
		*sizeAnteriorOccupied = sizeAnterior;
	}
}


void ordenarMemoriaPorBase(){
	int backupSize = 0;
	void obtenerBackupSize(void* elemento) {
		t_particion* particion = (t_particion*) elemento;
		backupSize += particion->size;
	}
	pthread_mutex_lock(&mutex_OCCUPIEDParticiones);
	list_iterate(OCCUPIED_PARTITIONS, obtenerBackupSize);
	pthread_mutex_unlock(&mutex_OCCUPIEDParticiones);


	void* memoriaBackup = malloc(backupSize);

	void memcpyABackup(void* elemento) {
		t_particion* particionBackupOCCUPIED = (t_particion*) elemento;

		int indiceALL = get_segmentoPorCondicion(ALL_PARTITIONS, particionBackupOCCUPIED->base, BASE );
		t_particion* particionALL = (t_particion*)list_get(ALL_PARTITIONS, indiceALL);

		int indiceOCCUPIED = get_segmentoPorCondicion(backupListOCCUPIED, particionBackupOCCUPIED->base, BASE );
		t_particion* particionOCCUPIED = (t_particion*)list_get(OCCUPIED_PARTITIONS, indiceOCCUPIED);

		//TODO: revisar la copiada a memoria, hay 3 tipos de segmentos
		if(particionALL->tipoSegmento == PCB){
			memcpy(memoriaBackup + particionOCCUPIED->base, MEMORIA + particionALL->base, sizeof(uint32_t));
			memcpy(memoriaBackup + particionOCCUPIED->base + sizeof(uint32_t), MEMORIA + particionALL->base + sizeof(uint32_t), sizeof(uint32_t));
		}

		else if (particionALL->tipoSegmento == TAREAS){
			memcpy(memoriaBackup + particionOCCUPIED->base, MEMORIA + particionALL->base, particionALL->size);
		}

		else if (particionALL->tipoSegmento == TCB){
			memcpy(memoriaBackup + particionOCCUPIED->base, MEMORIA + particionALL->base, sizeof(uint32_t));
			memcpy(memoriaBackup + particionOCCUPIED->base + sizeof(uint32_t), MEMORIA + particionALL->base + sizeof(uint32_t), sizeof(char));
			memcpy(memoriaBackup + particionOCCUPIED->base + sizeof(uint32_t) + sizeof(char), MEMORIA + particionALL->base + sizeof(uint32_t) + sizeof(char), sizeof(uint32_t));
			memcpy(memoriaBackup + particionOCCUPIED->base + (sizeof(uint32_t)*2) + sizeof(char), MEMORIA + particionALL->base + (sizeof(uint32_t)*2) + sizeof(char), sizeof(uint32_t));
			memcpy(memoriaBackup + particionOCCUPIED->base + (sizeof(uint32_t)*3) + sizeof(char), MEMORIA + particionALL->base + (sizeof(uint32_t)*3) + sizeof(char), sizeof(uint32_t));
			memcpy(memoriaBackup + particionOCCUPIED->base + (sizeof(uint32_t)*4) + sizeof(char), MEMORIA + particionALL->base + (sizeof(uint32_t)*4) + sizeof(char), sizeof(uint32_t));

		}
	}

	void memcpyAMemoria(void* elemento) {
		t_particion* particionBackupOCCUPIED = (t_particion*) elemento;

		int indiceALL = get_segmentoPorCondicion(ALL_PARTITIONS, particionBackupOCCUPIED->base, BASE );
		t_particion* particionALL = (t_particion*)list_get(ALL_PARTITIONS, indiceALL);

		int indiceOCCUPIED = get_segmentoPorCondicion(backupListOCCUPIED, particionBackupOCCUPIED->base, BASE );
		t_particion* particionOCCUPIED = (t_particion*)list_get(OCCUPIED_PARTITIONS, indiceOCCUPIED);

		//TODO: revisar la copiada a memoria, hay 3 tipos de segmentos
		if(particionALL->tipoSegmento == PCB){
			memcpy(MEMORIA + particionOCCUPIED->base, memoriaBackup + particionOCCUPIED->base, sizeof(uint32_t));
			memcpy(MEMORIA + particionOCCUPIED->base + sizeof(uint32_t), memoriaBackup + particionOCCUPIED->base + sizeof(uint32_t), sizeof(uint32_t));
		}
		else if (particionALL->tipoSegmento == TAREAS){
			memcpy(MEMORIA + particionOCCUPIED->base, memoriaBackup + particionOCCUPIED->base, particionALL->size);
		}
		else if (particionALL->tipoSegmento == TAREAS){
			memcpy(MEMORIA + particionOCCUPIED->base, memoriaBackup + particionOCCUPIED->base, sizeof(uint32_t));
			memcpy(MEMORIA + particionOCCUPIED->base + sizeof(uint32_t), memoriaBackup + particionOCCUPIED->base + sizeof(uint32_t), sizeof(char));
			memcpy(MEMORIA + particionOCCUPIED->base + sizeof(uint32_t) + sizeof(char), memoriaBackup + particionOCCUPIED->base + sizeof(uint32_t) + sizeof(char), sizeof(uint32_t));
			memcpy(MEMORIA + particionOCCUPIED->base + (sizeof(uint32_t)*2) + sizeof(char), memoriaBackup + particionOCCUPIED->base + (sizeof(uint32_t)*2) + sizeof(char), sizeof(uint32_t));
			memcpy(MEMORIA + particionOCCUPIED->base + (sizeof(uint32_t)*3) + sizeof(char), memoriaBackup + particionOCCUPIED->base + (sizeof(uint32_t)*3) + sizeof(char), sizeof(uint32_t));
			memcpy(MEMORIA + particionOCCUPIED->base + (sizeof(uint32_t)*4) + sizeof(char), memoriaBackup + particionOCCUPIED->base + (sizeof(uint32_t)*4) + sizeof(char), sizeof(uint32_t));
		}

		particionALL->base = particionOCCUPIED->base;
		list_replace(ALL_PARTITIONS, indiceALL, (void*) particionALL);
	}


	list_iterate(backupListOCCUPIED, memcpyABackup);
	list_iterate(backupListOCCUPIED, memcpyAMemoria);

	free(memoriaBackup);
}


void ordenarListaALLporBase(){
	bool ordenarPorBase(void* particion1, void* particion2) {
		t_particion* particion = (t_particion*) particion1;
		t_particion* otraParticion = (t_particion*) particion2;
		return particion->base < otraParticion->base;
	}
	pthread_mutex_lock(&mutex_ALLParticiones);
	list_sort(ALL_PARTITIONS, ordenarPorBase);
	pthread_mutex_unlock(&mutex_ALLParticiones);

}


void actualizarPtros(){//TODO: REVISAR SI SE BORRA
	t_link_element *elemento = ALL_PARTITIONS->head;
	t_particion* particion = (t_particion*) (ALL_PARTITIONS->head->data);

	while(elemento != NULL) {
		if ( particion->tipoSegmento == PCB){
			int ptroTareas = get_segmentoPorCondicion(ALL_PARTITIONS, particion->id, TAREAS);
			memcpy(MEMORIA + particion->base + sizeof(uint32_t), &(ptroTareas), sizeof(uint32_t));
		}

		else if(particion->tipoSegmento == TCB){
			int ptroPCB = get_segmentoPorCondicion(ALL_PARTITIONS, particion->id, PCB);
			memcpy(MEMORIA + particion->base + (sizeof(uint32_t)*4) + sizeof(char), &(ptroPCB), sizeof(uint32_t));
		}

		elemento = elemento->next;
		particion = elemento == NULL ? NULL : elemento->data;
	}
}



//---------------------------------------------------------------------------------------------------------------------------------------

void consolidarParticionesLibres(t_particion* nuevaParticionLibre, uint32_t* indiceListaFREE){
	int indiceListaALL = get_segmentoPorCondicion(ALL_PARTITIONS, nuevaParticionLibre->base, BASE);

	int incideAnterior = indiceListaALL - 1;

	if(incideAnterior >= 0) {
		t_particion* particionAnterior = (t_particion*)list_get(ALL_PARTITIONS, incideAnterior);

		//consolidacion hacia arriba
		while (particionAnterior != NULL && particionAnterior->free) {

			nuevaParticionLibre->size += particionAnterior->size;
			nuevaParticionLibre->base = particionAnterior->base;

			list_remove(FREE_PARTITIONS, *indiceListaFREE);
			list_replace(FREE_PARTITIONS, get_segmentoPorCondicion(FREE_PARTITIONS, particionAnterior->base, BASE), (void*) nuevaParticionLibre);
			list_remove(ALL_PARTITIONS, indiceListaALL);
			list_replace(ALL_PARTITIONS, incideAnterior, (void*) nuevaParticionLibre);

			free(particionAnterior);

			indiceListaALL = incideAnterior;
			incideAnterior--;
			if(incideAnterior < 0)
				particionAnterior = NULL;
			else
				particionAnterior = (t_particion*)list_get(ALL_PARTITIONS, incideAnterior);
		}
	}

	int indiceSiguiente = indiceListaALL + 1;
	if(indiceSiguiente < list_size(ALL_PARTITIONS)){
		t_particion* proximaParticion = (t_particion*)list_get(ALL_PARTITIONS, indiceSiguiente);

		//CONSOLIDACION HACIA ABAJO
		while (proximaParticion != NULL && proximaParticion->free) {

			nuevaParticionLibre->size += proximaParticion->size;

			list_remove(FREE_PARTITIONS, get_segmentoPorCondicion(FREE_PARTITIONS, proximaParticion->base, BASE));
			list_remove(ALL_PARTITIONS, indiceSiguiente);

			free(proximaParticion);

			//indiceListaALL = indiceSiguiente;
			//indiceSiguiente++;
			if(indiceSiguiente >= list_size(ALL_PARTITIONS))
				proximaParticion = NULL;
			else
				proximaParticion = (t_particion*)list_get(ALL_PARTITIONS, indiceSiguiente);
			//proximaParticion = list_get(ALL_PARTITIONS, indiceSiguiente) == NULL? NULL: (t_particion*)list_get(ALL_PARTITIONS, indiceSiguiente);
		}
	}
	*indiceListaFREE = get_segmentoPorCondicion(FREE_PARTITIONS, nuevaParticionLibre->base, BASE);
}


int get_segmentoPorCondicion(t_list* particiones, uint32_t atributo, int condicion){
	if (particiones->head == NULL)
			return -1;

	t_link_element *elemento = particiones->head;
	t_particion* particion = (t_particion*) elemento->data;

	int indice = 0;
	while(elemento != NULL) {
		switch (condicion){
			case BASE:;
				if (particion->base == atributo){
					return indice;
				}
				break;
			case PCB:;
				if (particion->id == atributo && particion->tipoSegmento == PCB){
					return indice;
				}
				break;
			case TCB:;
				if (particion->tipoSegmento == TCB){
					int tcbABuscar;
					memcpy(&(tcbABuscar), MEMORIA + particion->base, sizeof(uint32_t));

					if( tcbABuscar == atributo){
						return indice;
					}
				}
				break;
			case TAREAS:;
				if(particion->id == atributo && particion->tipoSegmento == TAREAS){
					return indice;
				}
				break;
		}
		elemento = elemento->next;
		particion = elemento == NULL ? NULL : elemento->data;
		indice++;
	}
	return -1;
}


int get_segmentoTCBxPCByNroSegmento(t_list* particiones, uint32_t idPatota, uint32_t nroSegmento){
	if (particiones->head == NULL)
		return -1;

	t_link_element *elemento = particiones->head;
	t_particion* particion = (t_particion*) elemento->data;

	int indice = 0;
	while(elemento != NULL) {
		if (particion->id == idPatota && particion->tipoSegmento == nroSegmento)
			return indice;

		elemento = elemento->next;
		particion = elemento == NULL ? NULL : elemento->data;
		indice++;
	}
	return -1;

}


t_tablaDeSegmentos* get_tablaDeSegmentos(uint32_t idPatota){
	t_link_element* elemento = TABLAdeSEGMENTOS->head;
	t_tablaDeSegmentos* tablaDeSegmentos = (t_tablaDeSegmentos*) elemento->data;

	while(elemento != NULL) {
		if(tablaDeSegmentos->idPatota == idPatota)
			return tablaDeSegmentos;
		elemento = elemento->next;
		tablaDeSegmentos = elemento == NULL ? NULL : elemento->data;
	}
	return NULL;
}


int get_IndiceTablaDeSegmentos(int idPatota){
	t_link_element* elemento = TABLAdeSEGMENTOS->head;
	t_tablaDeSegmentos* info = (t_tablaDeSegmentos*) elemento->data;
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

/*	-------------------------------------------------------------------
						ALGORITMO DE REEEMPLAZO
	-------------------------------------------------------------------*/

t_particion* encontrarParticionLibre_FIRSTFIT(int size){
	t_link_element* elemento = FREE_PARTITIONS->head;

	if (elemento == NULL)
		return NULL;

	t_particion* particion = (t_particion*) elemento->data;

	int index = 0;
	while (elemento != NULL) {
		if (particion->size >= size)
			return (t_particion*) list_remove(FREE_PARTITIONS, index);

		elemento = elemento->next;
		particion = elemento == NULL ? NULL : elemento->data;
		index++;
	}

	return NULL;
}



t_particion* encontrarParticionLibre_BESTFIT(int size){
	t_link_element* elemento = FREE_PARTITIONS->head;

	if (elemento == NULL)
		return NULL;

	t_particion* particion = (t_particion*) elemento->data;
	t_particion* mejorOpcion = particion;

	int index = 0;
	int indice_MejorOpcion = 0;
	while (elemento != NULL) {
		if (particion->size == size)
			return (t_particion*) list_remove(FREE_PARTITIONS, index);

		if (particion->size > size && particion->size < mejorOpcion->size) {
			indice_MejorOpcion = index;
			mejorOpcion = particion;
		}

		elemento = elemento->next;
		particion = elemento == NULL ? NULL : elemento->data;
		index++;
	}

	if (mejorOpcion->size < size)
		return NULL;

	return (t_particion*) list_remove(FREE_PARTITIONS, indice_MejorOpcion);
}

