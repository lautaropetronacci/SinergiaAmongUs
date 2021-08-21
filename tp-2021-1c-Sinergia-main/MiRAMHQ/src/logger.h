/*
 * logger.h
 *
 *  Created on: 10 may. 2021
 *      Author: utnso
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <commons/log.h>
#include <commonsSinergicas.h>



t_log* LOGGER;
int DISCORDIADOR_CONECTADO;



/*	-------------------------------------------------------------------
							INFO
	-------------------------------------------------------------------*/
void log_inicializacionCompleta();
void log_miRamHqReady();

	/*MENSAJES ACCIONES PRINCIPALES*/
void log_msjHandshake(int modulo);
void log_patotaIniciada(uint32_t idPatota, bool success);
void log_tripulanteIniciado(uint32_t idTripulante, bool success);
void log_tareaGeneradaHIT(uint32_t idTripulante);
void log_tareaGeneradaFAIL(uint32_t idTripulante);
void log_tarea(t_tarea tarea, uint32_t idTripulante, uint32_t ptro1, uint32_t ptro2);
void log_tripulanteExpulsado(uint32_t idTripulante, int codigo, bool success);
void log_actualizarEstado(uint32_t tid, uint32_t patota, int estadoInicial, int estadoFinal);

	/*MENSAJES SEGMENTACION*/
void log_compactacion();
void Log_nuevaTScreada(uint32_t idPatota);
void log_expulsionTripulanteSegmentacion(char* pidOtid, uint32_t id, char* accion, int patota, int nroSegmento);


	/*MENSAJES PAGINACION*/
void log_transferenciaASwap();
void Log_nuevaTPcreada(uint32_t idPatota);
void log_expulsionTripulantePaginacion(char* pidOtid, uint32_t id, char* accion,  int nroFrameEnMemoria, int nroFrameEnSwap);
void log_posicionActualizada(uint32_t idTripulante, int poscX1, int poscY1, int poscX2, int poscY2);


void log_paginaVictimaEnMemoria(uint32_t nroFrame);

	/*DUMP*/
void log_dump();

	/*OTROS MENSAJES*/
void log_tripulanteNoExiste(uint32_t tid);


/*	-------------------------------------------------------------------
							WARNING
	-------------------------------------------------------------------*/
void log_patotaInexistenteEnSistema(uint32_t idPatota);
void log_memoriaInsuficiente();
void log_algoritmoDeRemplazoInvalido();
void log_esquemaMemoriaInvalido();
void log_criterioDeReemplazoInvalido();


/*	-------------------------------------------------------------------
							ERROR
	-------------------------------------------------------------------*/
void log_errorEsquemaMemoria();
void log_errorMapeoSwap();
void log_errorDeAsignacionPorAlgtmRemplazo();


#endif /* LOGGER_H_ */
