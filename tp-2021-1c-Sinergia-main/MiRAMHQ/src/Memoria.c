/*
 * Memoria.c
 *
 *  Created on: 15 may. 2021
 *      Author: utnso
 */
#include "Memoria.h"


pthread_mutex_t mutex_Memoria = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_sizeMemoria = PTHREAD_MUTEX_INITIALIZER;


/*	-------------------------------------------------------------------
					INICIALIZACION DE MEMORIA
	-------------------------------------------------------------------*/
bool cargarMemoria (int tamanioMemoria, char* esquemaMemoria, int tamanioPagina, int tamanioSwap, char* pathSwap, char* algoritmoReemplazo, char* criterioReemplazo, char* dumpPath){
	MEMORIA = malloc(tamanioMemoria);	/*guarda la DATA posta*/
	MEMORIA_SIZE = tamanioMemoria;		/*este valor se va actualizando en segmentacion*/
	ESQUEMA_MEM = stringAcomandoInterno(esquemaMemoria);
	DUMP_PATH = dumpPath;

	switch (ESQUEMA_MEM){
		case PAGINACION:;

			SIZE_PAGINA = tamanioPagina;
			SWAP_SIZE = tamanioSwap;
			SWAP_PATH = pathSwap;
			ALGORITMO_REEM = stringAcomandoInterno(algoritmoReemplazo);
			return paginacionInit();
			break;

		case SEGMENTACION:;

			CRITERIO_REEM = stringAcomandoInterno(criterioReemplazo);
			segmentacionInit();
			return true;
			break;

		default:
			log_errorEsquemaMemoria();
			return false;
	}
}


/*	-------------------------------------------------------------------
					ACCIONES A LOS MENSAJES RECIBIDOS
	-------------------------------------------------------------------*/
bool crearPatota(int idPatota, char* tareas){
	t_pcb* nuevoPCB;
	uint32_t memoriaNecesaria_Size;

	switch (ESQUEMA_MEM) {
		case PAGINACION:;

			void crearPaginaNueva(t_pagina* paginaNueva, int i){
				paginaNueva->idPatota = idPatota;
				paginaNueva->idPagina = i;
				paginaNueva->modificado = true;
				paginaNueva->nroFrameEnSwap = encontrarPaginaLibreEnSwap(true);
				paginaNueva->nroFrameEnMemoria = encontrarPaginaLibreEnMemoria(true);
				paginaNueva->listadoInfo = list_create();

				if(paginaNueva->nroFrameEnMemoria == -1)
					paginacion_asignarMemoria(paginaNueva);
				else
					agregarPaginaEnMemoria(paginaNueva);
			}

			uint32_t paginasNecesarias;
			nuevoPCB = malloc(sizeof(*nuevoPCB));

			memoriaNecesaria_Size = sizeof(*nuevoPCB) + strlen(tareas) + 1;
			div_t resultadoDeLaDivision = div(memoriaNecesaria_Size, SIZE_PAGINA);
			paginasNecesarias = resultadoDeLaDivision.rem == 0? resultadoDeLaDivision.quot: resultadoDeLaDivision.quot + 1;

			if(!espacioDisponible(paginasNecesarias)){
				log_memoriaInsuficiente();
				free(nuevoPCB);
				return false;
			}

		/*CREACION DE TABLA DE PAGINAS DE LA PATOTA*/
			t_tablaDePaginas* nuevaTPporPatota = malloc(sizeof(*nuevaTPporPatota));
			nuevaTPporPatota->idPatota = idPatota;
			nuevaTPporPatota->tripulantes = list_create();
			pthread_mutex_init(&nuevaTPporPatota->mutex_tripulantes, NULL);

			pthread_mutex_lock(&mutex_tablaDePaginas);
			list_add(TABLAdePAGINAS, (void*)nuevaTPporPatota);
			pthread_mutex_unlock(&mutex_tablaDePaginas);

			Log_nuevaTPcreada(idPatota);


		/*CREACION DE PAGINAS PCB Y TAREAS PATOTA*/
			void actualizarVariasblesAuxiliares(t_pagina* paginaNueva, int i, int* espacioLibreEnPagina, int* size_tareasAux, int* contadorString, char** tareasAux){
				*espacioLibreEnPagina -= *size_tareasAux;
				*contadorString += *size_tareasAux;

				if(*contadorString < strlen(tareas) + 1){
					*tareasAux =  string_substring_from(tareas, *contadorString);
					*size_tareasAux = i + 1 == paginasNecesarias - 1? strlen(*tareasAux) + 1: strlen(*tareasAux);
				}

				paginaNueva->espacioLibre = *espacioLibreEnPagina;

				pthread_mutex_lock(&(nuevaTPporPatota->mutex_tripulantes));
				list_add(nuevaTPporPatota->tripulantes, (void*)paginaNueva);
				pthread_mutex_unlock(&(nuevaTPporPatota->mutex_tripulantes));
			}

			int espacioLibreEnPagina = SIZE_PAGINA;
			int size_tareasAux = 0;
			int contadorString = 0;
			char* tareasAux = string_new();

			for(int i = 0; i < paginasNecesarias; i++){
				if(i == 0){
					t_pagina* paginaNueva = malloc(sizeof(*paginaNueva));
					crearPaginaNueva(paginaNueva, i);

				/*PCB - armado de info*/
					nuevoPCB->pid = idPatota;
					nuevoPCB->tareasPatota = sizeof(uint32_t) * 2;
					guardarPCBenMemoria(nuevoPCB, paginaNueva->nroFrameEnMemoria);
					espacioLibreEnPagina -= sizeof(*nuevoPCB);

					t_catalogoInfo* info = malloc(sizeof(*info));
					info->tipoInfo = PCB;
					info->offset = 0;
					info->size = sizeof(*nuevoPCB);
					info->ultimo = 1;
					info->inicioTCB = -1;
					list_add(paginaNueva->listadoInfo, (void*)info);

				/*TAREAS - inicio de armado de info*/
					size_tareasAux = espacioLibreEnPagina > strlen(tareas) + 1? strlen(tareas) + 1 : espacioLibreEnPagina;
					tareasAux = string_substring(tareas, 0, size_tareasAux);

					char vectorTareas[size_tareasAux];
					strcpy(vectorTareas, tareasAux);

					memcpy(MEMORIA + paginaNueva->nroFrameEnMemoria * SIZE_PAGINA + sizeof(*nuevoPCB), &(vectorTareas), size_tareasAux);

					t_catalogoInfo* otraInfo = malloc(sizeof(*otraInfo));
					otraInfo->tipoInfo = TAREAS;
					otraInfo->offset = sizeof(*nuevoPCB);
					otraInfo->size = size_tareasAux;
					otraInfo->ultimo = espacioLibreEnPagina >= strlen(tareas) + 1 ? 1 : 0;
					otraInfo->inicioTCB = -1;
					list_add(paginaNueva->listadoInfo, (void*)otraInfo);

					actualizarVariasblesAuxiliares(paginaNueva, i, &espacioLibreEnPagina, &size_tareasAux, &contadorString, &tareasAux);
				}
				else{
					t_pagina* paginaNueva = malloc(sizeof(*paginaNueva));
					crearPaginaNueva(paginaNueva, i);

					espacioLibreEnPagina = SIZE_PAGINA;
					int sizeAux = espacioLibreEnPagina > size_tareasAux ? size_tareasAux : espacioLibreEnPagina;
					char* aux = string_substring(tareasAux, 0, sizeAux);
					char vectorTareas[sizeAux];
					strcpy(vectorTareas, aux);

					memcpy(MEMORIA + paginaNueva->nroFrameEnMemoria * SIZE_PAGINA, &(vectorTareas), sizeAux);

					t_catalogoInfo* info = malloc(sizeof(*info));
					info->tipoInfo = TAREAS;
					info->offset = 0;
					info->size = sizeAux;
					info->ultimo = espacioLibreEnPagina > strlen(tareasAux) + 1 ? 1 : 0;
					info->inicioTCB = -1;
					list_add(paginaNueva->listadoInfo, (void*)info);

					size_tareasAux = sizeAux;
					actualizarVariasblesAuxiliares(paginaNueva, i, &espacioLibreEnPagina, &size_tareasAux, &contadorString, &tareasAux);
				}
			}
			free(nuevoPCB);
			free(tareasAux);
			return true;
			break;

		case SEGMENTACION:;

			nuevoPCB = malloc(sizeof(*nuevoPCB));
			memoriaNecesaria_Size = sizeof(*nuevoPCB) + strlen(tareas) + 1;

			pthread_mutex_lock(&mutex_sizeMemoria);
			if (memoriaNecesaria_Size > MEMORIA_SIZE){
				pthread_mutex_unlock(&mutex_sizeMemoria);
				log_memoriaInsuficiente();
				free(nuevoPCB);
				return false;
			}
			pthread_mutex_unlock(&mutex_sizeMemoria);

		//CREACION DE TABLA DE SEGMENTOS DE LA PATOTA
			t_tablaDeSegmentos* nuevaTSporPatota = malloc(sizeof(*nuevaTSporPatota));
			nuevaTSporPatota->idPatota = idPatota;
			nuevaTSporPatota->segmentos = list_create();
			pthread_mutex_init(&nuevaTSporPatota->mutex_segmentos, NULL);

			pthread_mutex_lock(&mutex_tablaDeSegmentos);
			list_add(TABLAdeSEGMENTOS, (void*)nuevaTSporPatota);
			pthread_mutex_unlock(&mutex_tablaDeSegmentos);
			Log_nuevaTScreada(idPatota);


		/*CREACION DE SEGMENTO PCB*/
			nuevoPCB->pid = idPatota;
			t_particion* nuevoSegmento = (t_particion*) asignarMemoria((void*)sizeof(*nuevoPCB));

			nuevoSegmento->id = idPatota;
			nuevoSegmento->tipoSegmento = PCB;
			nuevoSegmento->nroSegmento = 0;

		/*CREACION DE SEGMENTO TAREAS*/
			int sizeTareas = string_length(tareas) + 1;
			char vectorTareas[sizeTareas];
			strcpy(vectorTareas, tareas);

			t_particion* otroSegmento = (t_particion*) asignarMemoria((void*)sizeTareas);
			otroSegmento->id = idPatota;
			otroSegmento->tipoSegmento = TAREAS;
			otroSegmento->nroSegmento = 1;
			memcpy(MEMORIA + otroSegmento->base, &(vectorTareas), sizeTareas);

			pthread_mutex_lock(&(nuevaTSporPatota->mutex_segmentos));
			list_add(nuevaTSporPatota->segmentos, (void*) nuevoSegmento);
			list_add(nuevaTSporPatota->segmentos, (void*) otroSegmento);
			pthread_mutex_unlock(&(nuevaTSporPatota->mutex_segmentos));

		/*ACTUALIZACION DE PARTICIONES EN LISTA ALL */
			pthread_mutex_lock(&mutex_ALLParticiones);
			int indice = get_segmentoPorCondicion(ALL_PARTITIONS, nuevoSegmento->base, BASE);	//PCB
			list_replace(ALL_PARTITIONS,indice, (void*)nuevoSegmento);
			indice = get_segmentoPorCondicion(ALL_PARTITIONS, otroSegmento->base, BASE);		//TAREAS
			list_replace(ALL_PARTITIONS,indice, (void*)otroSegmento);
			pthread_mutex_unlock(&mutex_ALLParticiones);

			nuevoPCB->tareasPatota = otroSegmento->nroSegmento;
			guardarPCBenMemoria(nuevoPCB, nuevoSegmento->base);

			pthread_mutex_lock(&mutex_sizeMemoria);
			MEMORIA_SIZE -= memoriaNecesaria_Size;
			pthread_mutex_unlock(&mutex_sizeMemoria);

			free(nuevoPCB);
			return true;
			break;
	}

	log_esquemaMemoriaInvalido();
	return false;
}

/*	-------------------------------------------------------------------
						CREAR TRIPULANTE
	-------------------------------------------------------------------*/
bool crearTripulante(int idPatota, int idTripulante, int poscX, int poscY){
	uint32_t memoriaNecesaria_Size;
	t_tcb* nuevoTCB = malloc(sizeof(*nuevoTCB));

	switch (ESQUEMA_MEM) {
		case PAGINACION:;
			pthread_mutex_lock(&mutex_tablaDePaginas);
			t_tablaDePaginas* tablaDePaginas = get_tablaDePaginas(idPatota);
			pthread_mutex_unlock(&mutex_tablaDePaginas);

			pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
			t_pagina* ultimaPagina = get_paginaPorCondicion(tablaDePaginas->tripulantes, ULTIMA);
			pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));

			int sizeTCB =  sizeof(uint32_t)*5 + sizeof(char);

			if(ultimaPagina->nroFrameEnMemoria == -1){
				traerPaginaAMemoria(ultimaPagina);
			}
			else
				actualizarPaginaXAlgoritmoReemplazo(ultimaPagina);

			nuevoTCB->tid = idTripulante;
			nuevoTCB->estado = 'N';
			nuevoTCB->posicionX = poscX;
			nuevoTCB->posicionY = poscY;
			nuevoTCB->proximaInstruccion = 0;

			pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
			t_pagina* pcbPagina = get_paginaPorCondicion(tablaDePaginas->tripulantes, PCB);
			pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));

			if(pcbPagina->nroFrameEnMemoria == -1){
				traerPaginaAMemoria(pcbPagina);
			}
			else
				actualizarPaginaXAlgoritmoReemplazo(pcbPagina);

			nuevoTCB->ptoPCB = pcbPagina->nroFrameEnMemoria * SIZE_PAGINA ;


		//GUARDAR TCB EN MEMORIA
			//CASO 1 -> hay espacio en la ultima pagina de la tabla de paginas de la patota --- (｡◕‿◕｡)
			if(ultimaPagina->espacioLibre >= sizeTCB){
				t_catalogoInfo* info = malloc(sizeof(*info));
				info->tipoInfo = TCB;
				info->offset = SIZE_PAGINA - ultimaPagina->espacioLibre;
				info->size = sizeTCB;
				info->ultimo = true;
				info->inicioTCB = tid;
				list_add(ultimaPagina->listadoInfo, (void*)info);

				guardarTCBenMemoria_Paginacion(nuevoTCB, ultimaPagina->nroFrameEnMemoria, info->offset);

				//Actualizacion de LISTAS -> ultima pagina
				ultimaPagina->espacioLibre -= info->size;
				actualizarPagina(tablaDePaginas, ultimaPagina, ULTIMA);
				free(nuevoTCB);
				return true;
			}

			if(!espacioDisponible(1)){
				log_memoriaInsuficiente();
				free(nuevoTCB);
				return false;
			}


		//Creacion de nueva pagina
			t_pagina* nuevaPagina = malloc(sizeof(*nuevaPagina));
			nuevaPagina->idPatota = idPatota;
			nuevaPagina->idPagina = list_size(tablaDePaginas->tripulantes);
			nuevaPagina->modificado = true;
			nuevaPagina->nroFrameEnSwap = encontrarPaginaLibreEnSwap(true);
			nuevaPagina->nroFrameEnMemoria = encontrarPaginaLibreEnMemoria(true);
			nuevaPagina->listadoInfo = list_create();

			if(nuevaPagina->nroFrameEnMemoria == -1){
				paginacion_asignarMemoria(nuevaPagina);
			}
			else{
				agregarPaginaEnMemoria(nuevaPagina);
			}

		//CASO 2 -> se carga una parte de TCB en la ultima pagina y el resto en una nueva Pagina --- (งツ)ว
			if(ultimaPagina->espacioLibre < sizeTCB &&  ultimaPagina->espacioLibre >= sizeof(uint32_t)){
				int contador = tid;
				int offset = 0;
				int sizeVariable = sizeof(uint32_t);

				t_catalogoInfo* info = malloc(sizeof(*info));
				info->tipoInfo = TCB;
				info->offset = SIZE_PAGINA - ultimaPagina->espacioLibre;
				info->ultimo = 0;
				info->inicioTCB = tid;

				//completando la ultima pagina de la patota
				while(ultimaPagina->espacioLibre >= sizeVariable){
					int base = ultimaPagina->nroFrameEnMemoria * SIZE_PAGINA + info->offset;
					switch (contador) {
						case estado:
							offset = generarOFFSET(info, base, contador);
							memcpy(MEMORIA + offset , &(nuevoTCB->estado), sizeVariable);
							break;
						case tid:
						case posicionX ... proximaInstruccion:
							offset = generarOFFSET(info, base, contador);
							switch (contador) {
								case tid:
									memcpy(MEMORIA + offset , &(nuevoTCB->tid), sizeVariable);
									info->size = sizeVariable;
									break;
								case posicionX:
									memcpy(MEMORIA + offset , &(nuevoTCB->posicionX), sizeVariable);
									break;
								case posicionY:
									memcpy(MEMORIA + offset , &(nuevoTCB->posicionY), sizeVariable);
									break;
								case proximaInstruccion:
									memcpy(MEMORIA + offset , &(nuevoTCB->proximaInstruccion), sizeVariable);
									break;
							}
					}

					if(contador != tid)
						info->size += sizeVariable;

					ultimaPagina->espacioLibre -= sizeVariable;
					sizeVariable = contador == tid ? sizeof(char) : sizeof(uint32_t);
					contador++;
				}
				list_add(ultimaPagina->listadoInfo, (void*)info);

				//Actualizacion de LISTAS -> ultima pagina
				actualizarPagina(tablaDePaginas, ultimaPagina, ULTIMA);


			//iniciando NUEVA PAGINA de la patota
				t_catalogoInfo* nuevoInfo = malloc(sizeof(*nuevoInfo));
				nuevoInfo->tipoInfo = TCB;
				nuevoInfo->offset = 0;
				nuevoInfo->ultimo = 1;
				nuevoInfo->inicioTCB = contador;

				while(contador <= ptoPCB){
					int base = nuevaPagina->nroFrameEnMemoria * SIZE_PAGINA;
					switch (contador) {
						case estado:
							offset = generarOFFSET(info, base, contador);
							memcpy(MEMORIA + offset, &(nuevoTCB->estado), sizeof(char));
							break;
						case posicionX ... ptoPCB:
							offset = generarOFFSET(info, base, contador);
							switch (contador) {
								case posicionX:
									memcpy(MEMORIA + offset , &(nuevoTCB->posicionX), sizeVariable);
									break;
								case posicionY:
									memcpy(MEMORIA + offset , &(nuevoTCB->posicionY), sizeVariable);
									break;
								case proximaInstruccion:
									memcpy(MEMORIA + offset , &(nuevoTCB->proximaInstruccion), sizeVariable);
									break;
								case ptoPCB:
									memcpy(MEMORIA + offset , &(nuevoTCB->ptoPCB), sizeVariable);
									break;
							}
							break;
					}
					if(contador == info->inicioTCB)
						info->size = sizeVariable;
					else
						info->size += sizeVariable;

					if(contador == estado)
						sizeVariable = sizeof(uint32_t);
				}
				list_add(nuevaPagina->listadoInfo, (void*)nuevoInfo);
				nuevaPagina->espacioLibre = SIZE_PAGINA - nuevoInfo->size;

			}
			else{
			//CASO 3 -> Se carga en una nueva pagina --- ( ͡° ͜ʖ ͡°)
				nuevaPagina->espacioLibre = SIZE_PAGINA - sizeTCB;
				t_catalogoInfo* info = malloc(sizeof(*info));
				info->tipoInfo = TCB;
				info->offset = 0;
				info->size = sizeTCB;
				info->ultimo = true;
				info->inicioTCB = tid;
				list_add(nuevaPagina->listadoInfo, (void*)info);

				guardarTCBenMemoria_Paginacion(nuevoTCB, nuevaPagina->nroFrameEnMemoria, info->offset);
			}

			//Actualizacion de LA TABLA DE PAGINAS DE LA PATOTA
			pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
			list_add(tablaDePaginas->tripulantes, (void*)nuevaPagina);
			pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));

			free(nuevoTCB);
			return true;
			break;

		case SEGMENTACION:;

			memoriaNecesaria_Size = sizeof(*nuevoTCB);

			pthread_mutex_lock(&mutex_sizeMemoria);
			if (memoriaNecesaria_Size > MEMORIA_SIZE){
				pthread_mutex_unlock(&mutex_sizeMemoria);
				log_memoriaInsuficiente();
				return false;
			}
			pthread_mutex_unlock(&mutex_sizeMemoria);

		/*consulta del nro segmento de la patota*/
			pthread_mutex_lock(&mutex_tablaDeSegmentos);
			t_tablaDeSegmentos* tablaDeSegmentos = get_tablaDeSegmentos(idPatota);
			pthread_mutex_unlock(&mutex_tablaDeSegmentos);

		/*CREACION DE TCB */
			nuevoTCB->tid = idTripulante;
			nuevoTCB->estado = 'N';
			nuevoTCB->posicionX = poscX;
			nuevoTCB->posicionY = poscY;
			nuevoTCB->proximaInstruccion = 0;
			nuevoTCB->ptoPCB = 0;

		/*creacion de segmento del tripulante*/
			t_particion* nuevoSegmento = (t_particion*) asignarMemoria((void*)sizeof(*nuevoTCB));

			nuevoSegmento->id = idPatota;
			nuevoSegmento->tipoSegmento = TCB;
			guardarTCBenMemoria_Segmentacion(nuevoTCB, nuevoSegmento->base);

			pthread_mutex_lock(&tablaDeSegmentos->mutex_segmentos);
			nuevoSegmento->nroSegmento = list_size(tablaDeSegmentos->segmentos);
			list_add(tablaDeSegmentos->segmentos, (void*)nuevoSegmento);
			pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);

			pthread_mutex_lock(&mutex_ALLParticiones);
			int indiceTCB = get_segmentoPorCondicion(ALL_PARTITIONS, nuevoSegmento->base, BASE);
			list_replace(ALL_PARTITIONS, indiceTCB, (void*)nuevoSegmento);
			pthread_mutex_unlock(&mutex_ALLParticiones);

		/*ACTUALIZACION DE MEMORIA SIZE*/
			pthread_mutex_lock(&mutex_sizeMemoria);
			MEMORIA_SIZE -= memoriaNecesaria_Size;
			pthread_mutex_unlock(&mutex_sizeMemoria);

			free(nuevoTCB);
			return true;
			break;
	}
	return false;
}


/*	-------------------------------------------------------------------
						ACTUALIZAR UBICACION
	-------------------------------------------------------------------*/
void guardarAtributoTCB(int idTripulante, int atributo, int nuevoValor, t_tablaDePaginas* tablaDePaginas, int* posc1, int* posc2){
	pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
	t_pagina* pagina = get_paginaPorTCByCondicion(tablaDePaginas->tripulantes, idTripulante, atributo);	// -OK
	//pagina->modificado = true;

	//Actualizacion de LA TABLA DE PAGINAS DE LA PATOTA
	int indice = getIndicePagina(tablaDePaginas->tripulantes, pagina);
	list_replace(tablaDePaginas->tripulantes, indice, (void*)pagina);
	pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));

	t_catalogoInfo* info = estaTCBenLista(pagina->listadoInfo, idTripulante, pagina->nroFrameEnMemoria);
	int offset = generarOFFSET(info, pagina->nroFrameEnMemoria * SIZE_PAGINA + info->offset, atributo);

	//Actualizacion en Memoria
	uint32_t actualPosc;
	memcpy(&(actualPosc), MEMORIA + offset, sizeof(uint32_t));
	*posc1 = actualPosc;

	if(atributo == proximaInstruccion)
		nuevoValor += actualPosc;

	memcpy(MEMORIA + offset, &(nuevoValor),sizeof(uint32_t));
	memcpy(&(actualPosc), MEMORIA + offset, sizeof(uint32_t));
	*posc2 = actualPosc;
}

bool actualizarUbicacionTripulante(int idPatota, int idTripulante, int poscX, int poscY){
	int poscX1, poscX2, poscY1, poscY2;
	switch (ESQUEMA_MEM) {
		case PAGINACION:;
			pthread_mutex_lock(&mutex_tablaDePaginas);
			t_tablaDePaginas* tablaDePaginas = get_tablaDePaginas(idPatota);	// - OK
			pthread_mutex_unlock(&mutex_tablaDePaginas);

			guardarAtributoTCB(idTripulante, posicionX, poscX, tablaDePaginas, &poscX1, &poscX2);
			guardarAtributoTCB(idTripulante, posicionY, poscY, tablaDePaginas, &poscY1, &poscY2);

			log_posicionActualizada(idTripulante, poscX1, poscY1, poscX2, poscY2);
			return true;
			break;

		case SEGMENTACION:;
			pthread_mutex_lock(&mutex_tablaDeSegmentos);
			t_tablaDeSegmentos* tablaDeSegmentos = get_tablaDeSegmentos(idPatota);
			pthread_mutex_unlock(&mutex_tablaDeSegmentos);

		/*obteniendo la info del tripulante*/
			pthread_mutex_lock(&tablaDeSegmentos->mutex_segmentos);
			int indiceTripulante = get_segmentoPorCondicion(tablaDeSegmentos->segmentos, idTripulante, TCB);

			if(indiceTripulante < 0){
				pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);
				log_tripulanteNoExiste(idTripulante);
				return false;
			}
			t_particion* tripulanteAactualizar = (t_particion*)list_get(tablaDeSegmentos->segmentos, indiceTripulante);
			pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);

			memcpy(&(poscX1), MEMORIA + tripulanteAactualizar->base + sizeof(uint32_t) + sizeof(char), sizeof(uint32_t));
			memcpy(&(poscY1), MEMORIA + tripulanteAactualizar->base + (sizeof(uint32_t)*2) + sizeof(char), sizeof(uint32_t));

			memcpy(MEMORIA + tripulanteAactualizar->base + sizeof(uint32_t) + sizeof(char), &(poscX), sizeof(uint32_t));
			memcpy(MEMORIA + tripulanteAactualizar->base + (sizeof(uint32_t)*2) + sizeof(char), &(poscY), sizeof(uint32_t));

			memcpy(&(poscX2), MEMORIA + tripulanteAactualizar->base + sizeof(uint32_t) + sizeof(char), sizeof(uint32_t));
			memcpy(&(poscY2), MEMORIA + tripulanteAactualizar->base + (sizeof(uint32_t)*2) + sizeof(char), sizeof(uint32_t));

			log_posicionActualizada(idTripulante, poscX1, poscY1, poscX2, poscY2);
			return true;
			break;
	}
	return false;
}


/*	-------------------------------------------------------------------
						ACTUALIZAR ESTADO TCB
	-------------------------------------------------------------------*/
char nuevoEstado(int status){
	char nuevoEstado;
	switch (status){
		case N:
			nuevoEstado = 'N';
			break;
		case R:
			nuevoEstado = 'R';
			break;
		case E:
			nuevoEstado = 'E';
			break;
		case B:
			nuevoEstado = 'B';
			break;
	}
	return nuevoEstado;
}

void guardarEstado(int idTripulante, char status, t_tablaDePaginas* tablaDePaginas, char* estadoInicial, char* estadoFinal){
	pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
	t_pagina* pagina = get_paginaPorTCByCondicion(tablaDePaginas->tripulantes, idTripulante, estado);	// -OK

	//Actualizacion de LA TABLA DE PAGINAS DE LA PATOTA
	int indice = getIndicePagina(tablaDePaginas->tripulantes, pagina);
	list_replace(tablaDePaginas->tripulantes, indice, (void*)pagina);
	pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));

	t_catalogoInfo* info = estaTCBenLista(pagina->listadoInfo, idTripulante, pagina->nroFrameEnMemoria);
	int offset = generarOFFSET(info, pagina->nroFrameEnMemoria * SIZE_PAGINA + info->offset, estado);

	//Actualizacion en Memoria
	char actualPosc;
	memcpy(&(actualPosc), MEMORIA + offset, sizeof(char));
	*estadoInicial = actualPosc;

	memcpy(MEMORIA + offset, &(status),sizeof(char));
	memcpy(&(actualPosc), MEMORIA + offset, sizeof(char));
	*estadoFinal = actualPosc;
}

bool actualizarEstadoTripulante(int idPatota, int idTripulante, int status){
	char nuevoStatus = nuevoEstado(status);
	char estadoInicial, estadoFinal;
	switch (ESQUEMA_MEM) {
		case PAGINACION:;
			pthread_mutex_lock(&mutex_tablaDePaginas);
			t_tablaDePaginas* tablaDePaginas = get_tablaDePaginas(idPatota);	// - OK
			pthread_mutex_unlock(&mutex_tablaDePaginas);

			guardarEstado(idTripulante, nuevoStatus, tablaDePaginas, &estadoInicial, &estadoFinal);
			log_actualizarEstado(idTripulante, idPatota, estadoInicial, estadoFinal);
			return true;
			break;

		case SEGMENTACION:;
			pthread_mutex_lock(&mutex_tablaDeSegmentos);
			t_tablaDeSegmentos* tablaDeSegmentos = get_tablaDeSegmentos(idPatota);
			pthread_mutex_unlock(&mutex_tablaDeSegmentos);

		/*obteniendo la info del tripulante*/
			pthread_mutex_lock(&tablaDeSegmentos->mutex_segmentos);
			int indiceTripulante = get_segmentoPorCondicion(tablaDeSegmentos->segmentos, idTripulante, TCB);

			if(indiceTripulante < 0){
				pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);
				log_tripulanteNoExiste(idTripulante);
				return false;
			}
			t_particion* tripulanteAactualizar = (t_particion*)list_get(tablaDeSegmentos->segmentos, indiceTripulante);
			pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);

			memcpy(&(estadoInicial), MEMORIA + tripulanteAactualizar->base + sizeof(uint32_t), sizeof(char));
			memcpy(MEMORIA + tripulanteAactualizar->base + sizeof(uint32_t), &(nuevoStatus), sizeof(char));
			memcpy(&(estadoFinal), MEMORIA + tripulanteAactualizar->base + sizeof(uint32_t), sizeof(char));

			log_actualizarEstado(idTripulante, idPatota, estadoInicial, estadoFinal);
			return true;
			break;
	}
	return false;
}


/*	-------------------------------------------------------------------
						PROXIMA TAREA
	-------------------------------------------------------------------*/
t_tarea generarProximaTarea(int idTripulante, char* tareasPatota, int proxInstruccion, uint32_t* nuevoPTOtareas){
	t_tarea unaTarea;
	char* str1 = string_substring_from(tareasPatota, proxInstruccion);	//Leer string desde el puntero que esta en el TCB
/*8*/		//log_info(LOGGER, "%s\n", str1);//todo: borrar

	//"GENERAR_OXIGENO 12;2;3;5\nCONSUMIR_OXIGENO 120;2;3;1"
/*NOMBRE DE LA TAREA*/
	char** tokens1 =  string_n_split(str1, 2 , " ");			//Separa el string en tokens, con delimitador ESPACIO
	char* nombre = string_duplicate(tokens1[0]);//string_new();
	//nombre = tokens1[0];	//


	//strcpy(&proximaTarea->tarea.nombre, nombre);
	unaTarea.nombre = nombre;

	char** tokens2 =  string_n_split(tokens1[1], 2 , "\n");		//Separa el string en tokens, con delimitador \n
	char** parametros = string_split(tokens2[0], ";");			//Separa el string en tokens, con delimitador ;

/*PARAMETROS DE TAREA*/
	unaTarea.parametros = atoi(parametros[0]);
	unaTarea.posicionX = atoi(parametros[1]);
	unaTarea.posicionY = atoi(parametros[2]);
	unaTarea.duracion = atoi(parametros[3]);
	//proximaTarea->tid = idTripulante;

	char* str2 = NULL;
	if(tokens2[1] == str2) unaTarea.esUltimaTarea = true; else  unaTarea.esUltimaTarea = false;

	if(strcmp(unaTarea.nombre, "GENERAR_OXIGENO") == 0 	|| strcmp(unaTarea.nombre, "CONSUMIR_OXIGENO") == 0||
		strcmp(unaTarea.nombre, "GENERAR_COMIDA") == 0	|| strcmp(unaTarea.nombre, "CONSUMIR_COMIDA") == 0 ||
		strcmp(unaTarea.nombre, "GENERAR_BASURA") == 0	|| strcmp(unaTarea.nombre, "DESCARTAR_BASURA") == 0)
		unaTarea.requiereIO = true;
	else
		unaTarea.requiereIO = false;

	*nuevoPTOtareas = strlen(tokens1[0]) + 1 + strlen(tokens2[0]) + !unaTarea.esUltimaTarea;

	for(int i = 0; parametros[i] != NULL; i++ ){
			free(parametros[i]);
		}
	free(parametros);
	for(int i = 0; tokens2[i] != NULL; i++ ){
				free(tokens2[i]);
			}
	free(tokens2);
	for(int i = 0; tokens1[i] != NULL; i++ ){
				free(tokens1[i]);
			}
	free(tokens1);
	free(str1);
	return unaTarea;
}

t_RSolicitarProximaTarea proximaTareaTripulante(int idPatota, int idTripulante){

	t_RSolicitarProximaTarea tareaVacia(){
		t_tarea unaTarea;
		unaTarea.nombre = "";
		unaTarea.parametros = 0;
		unaTarea.posicionX = -1;
		unaTarea.posicionY = -1;
		unaTarea.duracion = -1;
		unaTarea.esUltimaTarea = 1;
		unaTarea.requiereIO = 0;

		t_RSolicitarProximaTarea proximaTarea;
		proximaTarea.tarea = unaTarea;
		proximaTarea.tid = idTripulante;
		return proximaTarea;
	}

	char* armarStringTareas(t_tablaDePaginas* tablaDePaginas){
		pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
		t_link_element* elemento = tablaDePaginas->tripulantes->head;
		t_pagina* pagina = (t_pagina*) elemento->data;

		t_catalogoInfo* info = (t_catalogoInfo*)list_get(pagina->listadoInfo, 1);
		int sizeTareas = 0;
		char* tareas = string_new();

		while(elemento != NULL) {
			sizeTareas = info->size;
			char vectorTareas[sizeTareas + 1];
			vectorTareas[sizeTareas] = '\0';
			memcpy(&(vectorTareas), MEMORIA + pagina->nroFrameEnMemoria * SIZE_PAGINA + info->offset, sizeTareas);
			string_append(&tareas, vectorTareas);

			if(info->ultimo == 1){
				pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));
				return tareas;
			}
			elemento = elemento->next;
			pagina = elemento == NULL ? NULL : elemento->data;
			info = (t_catalogoInfo*)list_get(pagina->listadoInfo, 0);
		}
		pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));
		return tareas;
	}

	char* tareasPatota = string_new();
	uint32_t proxInstruccion, nuevoPtrotareas;

	switch (ESQUEMA_MEM){
	/*	case PAGINACION:;
			pthread_mutex_lock(&mutex_tablaDePaginas);
			t_tablaDePaginas* tablaDePaginas = get_tablaDePaginas(idPatota);
			pthread_mutex_unlock(&mutex_tablaDePaginas);

			t_catalogoInfo* info;
			tareasPatota = armarStringTareas(tablaDePaginas);

			//consulto en la pagina donde se encuentra el TCB cual es proxima instruccion de tareas a ejecutar
			pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
			t_pagina* pagina = get_paginaPorTCByCondicion(tablaDePaginas->tripulantes, idTripulante, proximaInstruccion);
			pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));

			//traer de memoria la proxima instruccion
			info = estaTCBenLista(pagina->listadoInfo, idTripulante, pagina->nroFrameEnMemoria);
			int offset = generarOFFSET(info, pagina->nroFrameEnMemoria * SIZE_PAGINA + info->offset, 5);
			memcpy(&(proxInstruccion), MEMORIA + offset, sizeof(uint32_t));

			generarProximaTarea(idTripulante, tareasPatota, proxInstruccion, proximaTarea, &nuevoPtrotareas);

			//actualizacion de TCB
			int ptro1, ptro2;
			guardarAtributoTCB(idTripulante, proximaInstruccion, nuevoPtrotareas, tablaDePaginas, &ptro1, &ptro2);

		//log_info(LOGGER, "TID: %d, TAREAS ptro inicial: %d - ptro final: %d", idTripulante, ptro1, ptro2);
			log_tarea(proximaTarea->tarea, idTripulante, ptro1, ptro2);
			free(tareasPatota);*/
			break;

		case SEGMENTACION:;
			pthread_mutex_lock(&mutex_tablaDeSegmentos);
			t_tablaDeSegmentos* tablaDeSegmentos = get_tablaDeSegmentos(idPatota);
			pthread_mutex_unlock(&mutex_tablaDeSegmentos);

			pthread_mutex_lock(&tablaDeSegmentos->mutex_segmentos);
			uint32_t indiceTripulante = get_segmentoPorCondicion(tablaDeSegmentos->segmentos, idTripulante, TCB);
			pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);

			if(indiceTripulante < 0){
				log_tripulanteNoExiste(idTripulante);
				tareaVacia();
				log_tareaGeneradaFAIL(idTripulante);
				break;
			}

			uint32_t ptoPCB, ptotareasPatota;

		/*traer la data del segmento TAREAS de la PATOTA*/
			pthread_mutex_lock(&tablaDeSegmentos->mutex_segmentos);
			t_particion* tripulanteAactualizar = (t_particion*)list_get(tablaDeSegmentos->segmentos, indiceTripulante);
			memcpy(&(ptoPCB), MEMORIA + tripulanteAactualizar->base + (sizeof(uint32_t)*4) + sizeof(char), sizeof(uint32_t));
			memcpy(&(proxInstruccion), MEMORIA + tripulanteAactualizar->base + (sizeof(uint32_t)*3) + sizeof(char), sizeof(uint32_t));

			t_particion* laPatota = (t_particion*)list_get(tablaDeSegmentos->segmentos, ptoPCB);
			memcpy(&(ptotareasPatota), MEMORIA + laPatota->base + sizeof(uint32_t), sizeof(uint32_t));

			t_particion* lasTareas = (t_particion*)list_get(tablaDeSegmentos->segmentos, ptotareasPatota);
			int sizeTareas = lasTareas->size;
			char vectorTareas[sizeTareas];
			memcpy(&(vectorTareas), MEMORIA + lasTareas->base, sizeTareas);
			pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);

			//tareasPatota = vectorTareas;
			//log_info(LOGGER,"%s", vectorTareas );
			t_tarea unaTarea = generarProximaTarea(idTripulante, vectorTareas, proxInstruccion, &nuevoPtrotareas);

			nuevoPtrotareas += proxInstruccion;
			memcpy(MEMORIA + tripulanteAactualizar->base + (sizeof(uint32_t)*3) + sizeof(char), &(nuevoPtrotareas), sizeof(uint32_t));
			memcpy(&(nuevoPtrotareas), MEMORIA + tripulanteAactualizar->base + (sizeof(uint32_t)*3) + sizeof(char), sizeof(uint32_t));

			t_RSolicitarProximaTarea proximaTarea;
			proximaTarea.tarea = unaTarea;
			proximaTarea.tid = idTripulante;
			log_tarea(unaTarea, idTripulante, proxInstruccion, nuevoPtrotareas);

			free(tareasPatota);
			return proximaTarea;
			break;
	}

}


/*	-------------------------------------------------------------------
						ELIMINAR TRIPULANTE
	-------------------------------------------------------------------*/
bool eliminarTripulante(int idPatota, int idTripulante){
	void expulsionCASO2(t_tablaDePaginas* tablaDePaginas, t_pagina* pagina, t_catalogoInfo* info, uint32_t indiceInfo){
		//limpieza en Memoria
		memset(MEMORIA + pagina->nroFrameEnMemoria * SIZE_PAGINA, 0, info->size);	//todo: ver que no se borre el info

		//Actualizacion de lista de criterio de reemplazo
		actualizarPaginaXAlgoritmoReemplazo(pagina);

		//actualizacion en tabla de paginas de la patota
		pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
		list_remove(pagina->listadoInfo, indiceInfo);
		int indicePagina = getIndicePagina(tablaDePaginas->tripulantes, pagina);
		list_replace(tablaDePaginas->tripulantes, indicePagina, (void*)pagina);
		pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));

		//char* msj = codigo == EXPULSAR_TRIPULANTE? "Expulsion EXITOSA" : "Terminacion EXITOSA";
		log_expulsionTripulantePaginacion("TID: ", idTripulante, "Expulsion EXITOSA", pagina->nroFrameEnMemoria, pagina->nroFrameEnSwap);
	}

	void expulsionCASO1(t_tablaDePaginas* tablaDePaginas, t_pagina* pagina, uint32_t id, char* tidOpid, char* accion){
		//limpieza en Swap
		if(pagina->nroFrameEnSwap > -1){
			frameLibreEnSwap(pagina->nroFrameEnSwap);
			pthread_mutex_lock(&mutex_bitmapSWAP);
			SWAP_BITMAP[pagina->nroFrameEnSwap] = 0;
			pthread_mutex_unlock(&mutex_bitmapSWAP);
		}

		//limpieza en Memoria
		frameLibreEnMemoria(pagina->nroFrameEnMemoria);
		pthread_mutex_lock(&mutex_bitmapMEMORIA);
		MEMORIA_BITMAP[pagina->nroFrameEnMemoria] = 0;
		pthread_mutex_unlock(&mutex_bitmapMEMORIA);

		//limpieza en lista de Algoritmo de reemplazo
		switch(ALGORITMO_REEM){
			case LRU:
				eliminarPaginaLRU(pagina);
				break;
			case CLOCK:
				eliminarPaginaClock(pagina);
				break;
		}

		log_expulsionTripulantePaginacion(tidOpid, id, accion, pagina->nroFrameEnMemoria, pagina->nroFrameEnSwap);
		list_clean(pagina->listadoInfo);

		//limpieza en tabla de paginas de la patota
		pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
		int indicePagina = getIndicePagina(tablaDePaginas->tripulantes, pagina);
		list_remove(tablaDePaginas->tripulantes, indicePagina);
		pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));
	}
/*
	void eliminarPatota_Paginacion(t_tablaDePaginas* tablaDePaginas){
		pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
		t_pagina* pagina = get_paginaPorCondicion(tablaDePaginas->tripulantes, TCB);
		pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));

		if (pagina == NULL){
			int sizeTripulantes = list_size(tablaDePaginas->tripulantes);
			for(int i = sizeTripulantes - 1; i >= 0; i--){
				pagina = (t_pagina*)list_get(tablaDePaginas->tripulantes, i);
				if (pagina->nroFrameEnMemoria == -1){
					traerPaginaAMemoria(pagina);
				}
				else
					actualizarPaginaXAlgoritmoReemplazo(pagina);
				expulsionCASO1(tablaDePaginas, (t_pagina*)list_get(tablaDePaginas->tripulantes, i), tablaDePaginas->idPatota, "PID: ", "Terminacion EXITOSA");
			}
			pthread_mutex_lock(&mutex_tablaDePaginas);
			int idPatota =tablaDePaginas->idPatota;
			list_remove(TABLAdePAGINAS, get_indicetablaDePaginas(TABLAdePAGINAS, idPatota));
			pthread_mutex_unlock(&mutex_tablaDePaginas);

			log_info(LOGGER, "PATOTA: %d, Eliminada del Sistema", idPatota);
			free(pagina);
		}
	}
*/

	switch (ESQUEMA_MEM) {
		case PAGINACION:
			pthread_mutex_lock(&mutex_tablaDePaginas);
			t_tablaDePaginas* tablaDePaginas = get_tablaDePaginas(idPatota);	// - OK
			pthread_mutex_unlock(&mutex_tablaDePaginas);

			//se busca la pagina desde donde arranca el PCB
			pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
			t_pagina* pagina = get_paginaPorTCByCondicion(tablaDePaginas->tripulantes, idTripulante, tid); //trae pagina a memoria
			pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));

			if(pagina == NULL){
				log_tripulanteNoExiste(idTripulante);
				return false;
				break;
			}

		//CASO 1 -> TCB esta solo, la pagina queda limpia
			if(list_size(pagina->listadoInfo) == 1){
				expulsionCASO1(tablaDePaginas, pagina, idTripulante, "TID: ", "Expulsion EXITOSA");
				//eliminarPatota_Paginacion(tablaDePaginas);
				imprimirLista(TABLAdePAGINAS);	//TODO:BORRAR
				free(pagina);
				return true;
			}

		//CASO 2 -> TCB completo en una pagina con mas elementos
			int sizeTCB =  sizeof(uint32_t)*5 + sizeof(char);
			t_catalogoInfo* info = estaTCBenLista(pagina->listadoInfo, idTripulante, pagina->nroFrameEnMemoria);
			uint32_t indiceInfo = get_indiceInfo(pagina->listadoInfo, idTripulante, pagina->nroFrameEnMemoria);

			if(info->offset != 0 && info->size == sizeTCB){
				expulsionCASO2(tablaDePaginas, pagina, info, indiceInfo);
				//eliminarPatota_Paginacion(tablaDePaginas);
				imprimirLista(TABLAdePAGINAS);	//TODO:BORRAR
				memoriaDump(1); //TODO:BORRAR
				free(info);
				return true;
			}

		//CASO 3 ->  TCB inicia en una pagina y termina en otra
			/*se dan 2 casos en la nueva pagina:
			 * 		CASO A) el resto del TCB se elimina y la pagina queda vacia,
			 * 		CASO B) el resto del TCB se elimina y hay mas data en la pagina */

			if(!info->ultimo){
				//actualizacion 1ra pagina
				expulsionCASO2(tablaDePaginas, pagina, info, indiceInfo);
				free(info);

				//actualizacion 2da pagina
				pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
				int indicePag = getIndicePagina(tablaDePaginas->tripulantes, pagina);
				t_pagina* paginaSiguiente = (t_pagina*)list_get(tablaDePaginas->tripulantes, indicePag + 1);
				paginaSiguiente->modificado = true;
				pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));

				if(list_size(paginaSiguiente->listadoInfo) == 1){
					expulsionCASO1(tablaDePaginas, paginaSiguiente, idTripulante, "TID: ", "Expulsion EXITOSA");
					free(paginaSiguiente);
				}
				else{
					t_catalogoInfo* nuevoInfo = (t_catalogoInfo*)list_get(paginaSiguiente->listadoInfo, 0);
					expulsionCASO2(tablaDePaginas, paginaSiguiente, nuevoInfo, 0);
					free(nuevoInfo);
				}
				//eliminarPatota_Paginacion(tablaDePaginas);
			}
			//imprimirLista(TABLAdePAGINAS);	//TODO:BORRAR
			return true;
			break;

		case SEGMENTACION:;

			void eliminarSegmento(t_tablaDeSegmentos* tablaDeSegmentos, t_particion* particion, int indiceParticion){
			/*LIBERAR MEMORIA*/
				memset(MEMORIA + particion->base, 0, particion->size);

			/*ACTUALIZAR LISTAS y CONSOLIDAR*/
				particion->id = 0;
				particion->tipoSegmento = -1;
				particion->free = 1;
				particion->nroSegmento = -1;

				pthread_mutex_lock(&mutex_OCCUPIEDParticiones);
				uint32_t indiceTripulanteOCCUPIED = get_segmentoPorCondicion(OCCUPIED_PARTITIONS, particion->base, BASE);
				list_remove(OCCUPIED_PARTITIONS,indiceTripulanteOCCUPIED );
				pthread_mutex_unlock(&mutex_OCCUPIEDParticiones);

				pthread_mutex_lock(&mutex_FREEParticiones);
				uint32_t indiceTripulanteFREE = list_add(FREE_PARTITIONS, (void*) particion);
				pthread_mutex_unlock(&mutex_FREEParticiones);

				consolidarParticionesLibres(particion, &indiceTripulanteFREE);

				pthread_mutex_lock(&tablaDeSegmentos->mutex_segmentos);
				list_remove(tablaDeSegmentos->segmentos, indiceParticion);
				pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);
			}

			pthread_mutex_lock(&mutex_tablaDeSegmentos);
			t_tablaDeSegmentos* tablaDeSegmentos = get_tablaDeSegmentos(idPatota);
			pthread_mutex_unlock(&mutex_tablaDeSegmentos);

			pthread_mutex_lock(&tablaDeSegmentos->mutex_segmentos);
			int indiceTripulante = get_segmentoPorCondicion(tablaDeSegmentos->segmentos, idTripulante, TCB);

			if(indiceTripulante < 0){
				log_tripulanteNoExiste(idTripulante);
				pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);
				return false;
				break;
			}
			t_particion* tripulanteAexpulsar = (t_particion*)list_get(tablaDeSegmentos->segmentos, indiceTripulante);
			pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);

			int nroSegmento = tripulanteAexpulsar->nroSegmento;
			eliminarSegmento(tablaDeSegmentos, tripulanteAexpulsar, indiceTripulante);
			//char* msj = codigo == EXPULSAR_TRIPULANTE?  : "Terminacion EXITOSA";
			log_expulsionTripulanteSegmentacion("TID: ", idTripulante, "Expulsion EXITOSA" , idPatota, nroSegmento);
			imprimirLista(ALL_PARTITIONS); //todo:borrar
			printf("\n");

			/*pthread_mutex_lock(&tablaDeSegmentos->mutex_segmentos);
			int size = list_size(tablaDeSegmentos->segmentos);
			pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);

			//inicia verificacion para ELIMINAR SEGMENTOS [PCB y TAREAS] de PATOTA
			if( size == 2){
				pthread_mutex_lock(&tablaDeSegmentos->mutex_segmentos);
				t_particion* particion = (t_particion*)list_get(tablaDeSegmentos->segmentos, 1);
				pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);

				eliminarSegmento(tablaDeSegmentos, particion, 1);
				log_error(LOGGER, "Patota: %d, Seg: %d => [TAREAS] Eliminacion Exitosa", idPatota, 1);
				imprimirLista(ALL_PARTITIONS);
				printf("\n");

				pthread_mutex_lock(&tablaDeSegmentos->mutex_segmentos);
				particion = (t_particion*)list_get(tablaDeSegmentos->segmentos, 0);
				pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);
				eliminarSegmento(tablaDeSegmentos, particion, 0);
				log_error(LOGGER, "Patota: %d, Seg: %d => [PCB] Eliminacion Exitosa", idPatota, 0);
				//free(particion);
				imprimirLista(ALL_PARTITIONS);
				printf("\n");


				pthread_mutex_lock(&mutex_tablaDeSegmentos);
				int indiceTabla = get_IndiceTablaDeSegmentos(idPatota);
				list_remove(TABLAdeSEGMENTOS, indiceTabla);
				pthread_mutex_unlock(&mutex_tablaDeSegmentos);
				free(tablaDeSegmentos);
				log_warning(LOGGER, "ELIMINO LA PATOTA");
			}
			else
				pthread_mutex_unlock(&tablaDeSegmentos->mutex_segmentos);*/

	/*11*/		//imprimirLista(ALL_PARTITIONS);
			return true;
			break;

		default:;
			log_errorEsquemaMemoria();
			return false;
			break;
	}
}


/*	-------------------------------------------------------------------
						FUNCIONES AUXILIARES
	-------------------------------------------------------------------*/
int stringAcomandoInterno(char* comandoRecibido){
	comandoInterno comandoActual = -1;

	for(int i = 0; i < sizeof(conversionComandoInterno) / sizeof(conversionComandoInterno[0]); i++) {
		if(!strcmp(comandoRecibido, conversionComandoInterno[i].str)){
			comandoActual = conversionComandoInterno[i].codigo;
			break;
		}
	}
	return comandoActual == -1 ? ERROR_PROCESO :(int)comandoActual;
}


void* asignarMemoria(void* parametro){
	void* asignado = NULL;

	pthread_mutex_lock(&mutex_Memoria);
	if (ESQUEMA_MEM == PAGINACION) {
		t_pagina* pagina = (t_pagina*)parametro;
		paginacion_asignarMemoria(pagina);
	}
	else if (ESQUEMA_MEM == SEGMENTACION) {
		int size = (int)parametro;
		asignado = segmentacion_asignarMemoria(size);
	}
	else
		log_errorEsquemaMemoria();
	pthread_mutex_unlock(&mutex_Memoria);

	return asignado;
}

void guardarPCBenMemoria(t_pcb* nuevoPCB, uint32_t baseONroFrame){
	switch(ESQUEMA_MEM){
		case PAGINACION:
			memcpy(MEMORIA + baseONroFrame * SIZE_PAGINA, &(nuevoPCB->pid), sizeof(uint32_t));
			memcpy(MEMORIA + baseONroFrame * SIZE_PAGINA + sizeof(uint32_t), &(nuevoPCB->tareasPatota), sizeof(uint32_t));
			//memcpy(MEMORIA + baseONroFrame * SIZE_PAGINA + (sizeof(uint32_t) * 2), &(nuevoPCB->cantSecciones), sizeof(uint32_t));
			break;
		case SEGMENTACION:
			memcpy(MEMORIA + baseONroFrame, &(nuevoPCB->pid), sizeof(uint32_t));
			memcpy(MEMORIA + baseONroFrame + sizeof(uint32_t), &(nuevoPCB->tareasPatota), sizeof(uint32_t));
			//memcpy(MEMORIA + baseONroFrame + (sizeof(uint32_t) * 2), &(nuevoPCB->cantSecciones), sizeof(uint32_t));
			break;
	}
}

void guardarTCBenMemoria_Paginacion(t_tcb* nuevoTCB, uint32_t nroFrame, int offset){
	if(ESQUEMA_MEM == PAGINACION){
		uint32_t nuevoOffset = offset + nroFrame * SIZE_PAGINA;
		memcpy(MEMORIA + nuevoOffset, &(nuevoTCB->tid), sizeof(uint32_t));
		memcpy(MEMORIA + nuevoOffset + sizeof(uint32_t), &(nuevoTCB->estado), sizeof(char));
		memcpy(MEMORIA + nuevoOffset + sizeof(uint32_t) + sizeof(char), &(nuevoTCB->posicionX), sizeof(uint32_t));
		memcpy(MEMORIA + nuevoOffset + (sizeof(uint32_t)*2) + sizeof(char), &(nuevoTCB->posicionY), sizeof(uint32_t));
		memcpy(MEMORIA + nuevoOffset + (sizeof(uint32_t)*3) + sizeof(char), &(nuevoTCB->proximaInstruccion), sizeof(uint32_t));
		memcpy(MEMORIA + nuevoOffset + (sizeof(uint32_t)*4) + sizeof(char), &(nuevoTCB->ptoPCB), sizeof(uint32_t));
	}
}

void guardarTCBenMemoria_Segmentacion(t_tcb* nuevoTCB, uint32_t base){
	if (ESQUEMA_MEM == SEGMENTACION){
			memcpy(MEMORIA + base, &(nuevoTCB->tid), sizeof(uint32_t));
			memcpy(MEMORIA + base + sizeof(uint32_t), &(nuevoTCB->estado), sizeof(char));
			memcpy(MEMORIA + base + sizeof(uint32_t) + sizeof(char), &(nuevoTCB->posicionX), sizeof(uint32_t));
			memcpy(MEMORIA + base + (sizeof(uint32_t)*2) + sizeof(char), &(nuevoTCB->posicionY), sizeof(uint32_t));
			memcpy(MEMORIA + base + (sizeof(uint32_t)*3) + sizeof(char), &(nuevoTCB->proximaInstruccion), sizeof(uint32_t));
			memcpy(MEMORIA + base + (sizeof(uint32_t)*4) + sizeof(char), &(nuevoTCB->ptoPCB), sizeof(uint32_t));
	}
}

void vectorFrames(t_tablaDePaginas* tablaDePaginas, t_list* lista){
	pthread_mutex_lock(&(tablaDePaginas->mutex_tripulantes));
	t_link_element* elemento = lista->head;
	t_pagina* pagina = (t_pagina*) elemento->data;

	while(elemento != NULL) {
		t_frame frame;
		frame.idPatota = pagina->idPatota;;
		frame.pagina = pagina->idPagina;
		frame.estado = 1;

		FRAMES_MEMORIA[pagina->nroFrameEnMemoria] = frame;

		elemento = elemento->next;
		pagina = elemento == NULL ? NULL : elemento->data;
	}
	pthread_mutex_unlock(&(tablaDePaginas->mutex_tripulantes));
}

void imprimirPaginacion(){
	char* status;
	for(int i = 0; i < MEMORIA_PARTICIONES; i++){
		status = FRAMES_MEMORIA[i].estado == 1? "OCUPADO":"LIBRE";
		printf("\nFrame %-10d	Estado: %-10s	Patota: %-10d 	Pagina: %-10d", i, status, FRAMES_MEMORIA[i].idPatota, FRAMES_MEMORIA[i].pagina);
	}
}

void vectorDeFrames(t_pagina* pagina){
	t_frame frame;
	frame.idPatota = pagina->idPatota;;
	frame.pagina = pagina->idPagina;
	frame.estado = 1;

	FRAMES_MEMORIA[pagina->nroFrameEnMemoria] = frame;
}

void imprimirLista(t_list* lista){
	void imprimirSegmentacion(t_particion* particion){
		printf("\nPatota: %-10d	segmento: %-10d base: %-10d	size: %-10d", particion->id, particion->nroSegmento, particion->base, particion->size);
	}

	/*if(list_is_empty(lista)){
		log_warning(LOGGER, "NO hay NADA QUE IMPRIMIR EN MEMORIA");
	}
	else{*/
		if(ESQUEMA_MEM == SEGMENTACION) {
			printf("\n");
			list_iterate(lista, (void*)imprimirSegmentacion);
		}
		else{
			for(int i = 0; i<MEMORIA_PARTICIONES; i++){
				t_frame frame;
				frame.idPatota = -1;
				frame.pagina = -1;
				frame.estado = 0;
				FRAMES_MEMORIA[i] = frame;
			}

			switch (ALGORITMO_REEM) {
				case LRU:
					list_iterate(PAGINASenMEMORIA, (void*)vectorDeFrames);
					break;
				case CLOCK:;
					for(int i = 0; i < MEMORIA_PARTICIONES; i++){
						t_elemento elemento =  PUNTERO[i];

						if(elemento.uso == 1){
							t_pagina* pagina  = elemento.pagina;
							t_frame frame;
							frame.idPatota = pagina->idPatota;
							frame.pagina = pagina->idPagina;
							frame.estado = 1;
							FRAMES_MEMORIA[pagina->nroFrameEnMemoria] = frame;
						}
					}
					break;
			}
			imprimirPaginacion();
			printf("\n");
		}
	//}

}



/*	-------------------------------------------------------------------
						DUMP
	-------------------------------------------------------------------*/
void escribirInfoDeTiempoEnDump(FILE* dumpFile){
  time_t hoy = time(NULL);
  struct tm* tiempoInfo;
  setenv("TZ", "America/Buenos_Aires", 1);
  tiempoInfo = localtime(&hoy);
  fprintf(dumpFile, "%d/%d/%d %d:%d:%d\n", tiempoInfo->tm_mday, tiempoInfo->tm_mon + 1, tiempoInfo->tm_year + 1900, tiempoInfo->tm_hour, tiempoInfo->tm_min, tiempoInfo->tm_sec);
}


void escribirInfoDeLasParticiones(FILE* dumpFile){
	if (ESQUEMA_MEM == PAGINACION) {
		fprintf(dumpFile, "ESQUEMA PAGINACION\n\n");

		for(int i = 0; i<MEMORIA_PARTICIONES; i++){
			t_frame frame;
			frame.idPatota = -1;
			frame.pagina = -1;
			frame.estado = 0;
			FRAMES_MEMORIA[i] = frame;
		}

		//if(!(list_is_empty(PAGINASenMEMORIA))){
		switch (ALGORITMO_REEM) {
			case LRU:
				if(!(list_is_empty(PAGINASenMEMORIA)))
					list_iterate(PAGINASenMEMORIA, (void*)vectorDeFrames);
				break;
			case CLOCK:;

				for(int i = 0; i < MEMORIA_PARTICIONES; i++){
					t_elemento elemento =  PUNTERO[i];

					if(elemento.uso == 1){
						t_pagina* pagina  = elemento.pagina;
						t_frame frame;
						frame.idPatota = pagina->idPatota;
						frame.pagina = pagina->idPagina;
						frame.estado = 1;
						FRAMES_MEMORIA[pagina->nroFrameEnMemoria] = frame;
					}
				}
				break;
		}
		//}
		char* estado;
		for(int i = 0; i < MEMORIA_PARTICIONES; i++){
			estado = FRAMES_MEMORIA[i].estado == 1? "OCUPADO":"LIBRE";
			fprintf(dumpFile, "Frame: %-10d Estado: %-20s Patota: %-10d Pagina: %-10d\n", i, estado, FRAMES_MEMORIA[i].idPatota, FRAMES_MEMORIA[i].pagina);
		}
	}
	else if (ESQUEMA_MEM == SEGMENTACION) {
		void imprimirSegmentacion(t_particion* particion){
			char* libre;
			if (particion->free)
				libre = "[LIBRE]";
			else
				libre = "[]";

			fprintf(dumpFile, "Proceso: %-10d Segmento: %-10d Inicio: %-10d Tamaño: %-10d %s\n", particion->id, particion->nroSegmento, particion->base, particion->size, libre);
		}
		fprintf(dumpFile, "ESQUEMA SEGMENTACION\n\n");
		list_iterate(ALL_PARTITIONS, (void*)imprimirSegmentacion);
		fprintf(dumpFile, "\nTOTAL PARTICIONES: %d.\n", list_size(ALL_PARTITIONS));
	}
}


void memoriaDump(int sig){
	pthread_mutex_lock(&mutex_Memoria);
	FILE* dumpFile = fopen(DUMP_PATH, "w");

	const char* line = "-----------------------------------------------------------------------------------------------------------------------------";

	fprintf(dumpFile,"%s\n", line);
	escribirInfoDeTiempoEnDump(dumpFile);
	escribirInfoDeLasParticiones(dumpFile);
	fprintf(dumpFile,"%s\n", line);

	log_dump();
	fclose(dumpFile);
	pthread_mutex_unlock(&mutex_Memoria);
}
