/*
 * logger.c
 *
 *  Created on: 10 may. 2021
 *      Author: utnso
 */

#include "logger.h"


/*	-------------------------------------------------------------------
							INFO
	-------------------------------------------------------------------*/
void log_inicializacionCompleta(){
	log_info(LOGGER,"inicializacion exitosa de:\n LOGGER\n CONFIG\n Memoria\n Mapa");
}

void log_miRamHqReady(){
	log_info(LOGGER,"El modulo MiRamHQ esta listo para recibir mensajes.");

}


	/*MENSAJES ACCIONES PRINCIPALES*/
void log_msjHandshake(int modulo){
	switch(modulo){
		case DISCORDIADOR: ;
			log_info(LOGGER, "Se recibio un Handshake del Discodiador");
			DISCORDIADOR_CONECTADO = 1;
			break;

		case TRIPULANTE:
			log_info(LOGGER, "Se recibio un Handshake del Tripulante");
			break;
		default:
			break;
	}
}

void log_patotaIniciada(uint32_t idPatota, bool success){
	char* msj = success ? "Inicio EXITOSO" : "Inicio FALLIDO";
	log_info(LOGGER, "Patota %d, %s", idPatota, msj);
}

void log_tripulanteIniciado(uint32_t idTripulante, bool success){
	char* msj = success ? "Inicio EXITOSO" : "Inicio FALLIDO";
	log_info(LOGGER, "Tripulante %d, %s", idTripulante, msj);
}

void log_tareaGeneradaHIT(uint32_t idTripulante){
	log_info(LOGGER, "Tripulante %d, Tarea Generada con EXITO", idTripulante);
}

void log_tarea(t_tarea tarea, uint32_t idTripulante, uint32_t ptro1, uint32_t ptro2){
	char* io = tarea.requiereIO? "SI" : "NO";
	char* ultima = tarea.esUltimaTarea? "SI": "NO";
	log_info(LOGGER, "TID: %d, TAREAS | ptro inicial: %d - ptro final: %d", idTripulante, ptro1, ptro2);
	log_warning(LOGGER, "TID: %d -> Tarea: %s, Par: %d, [%d|%d], duracion: %d, IO: %s, ultima: %s",idTripulante, tarea.nombre, tarea.parametros, tarea.posicionX, tarea.posicionY, tarea.duracion, io, ultima);
}
/*
void log_tripulanteExpulsado(uint32_t idTripulante, int codigo, bool success){
	char* msj = codigo == EXPULSAR_TRIPULANTE? "Expulsion EXITOSA" : "Terminacion EXITOSA";
	log_info(LOGGER, "tripulante %d, %s", idTripulante, msj);
}

void log_tripulanteExpulsadoFAIL(uint32_t idTripulante, int codigo){
	char* msj = codigo == EXPULSAR_TRIPULANTE? "Expulsion FALLIDA" : "TERMINACION FALLIDA";
	log_info(LOGGER, "tripulante %d, %s", idTripulante, msj);
}
*/
void log_actualizarEstado(uint32_t tid, uint32_t patota, int estadoInicial, int estadoFinal){

	char* nuevoEstado(int status){
		switch (status){
			case 'N':
				return "NEW";
				break;
			case 'R':
				return "READY";
				break;
			case 'E':
				return "EXECUTE";
				break;
			case 'B':
				return "BLOCK";
				break;
			case 'F':
				return "EXIT";
				break;
		}
		return "ERROR";
	}

	char* estado1 = nuevoEstado(estadoInicial);
	char* estado2 = nuevoEstado(estadoFinal);
	log_info(LOGGER, "TID: %d, Estado Inicial: %s, Estado Actualizado: %s", tid, estado1, estado2);
	if (estadoFinal == 'F') log_warning(LOGGER, "TID: %d patota:%d, Finalizo Tareas", tid, patota);
}

void log_posicionActualizada(uint32_t idTripulante, int poscX1, int poscY1, int poscX2, int poscY2){
	log_info(LOGGER, "TID: %d, POSICION INICIAL (%d | %d) - POSICION ACTUALIZADA (%d | %d)", idTripulante, poscX1, poscY1, poscX2, poscY2);
}


	/*MENSAJES SEGMENTACION*/
void log_compactacion(){
	log_warning(LOGGER, "La memoria ha sido compactada");
}

void Log_nuevaTScreada(uint32_t idPatota){
	log_info(LOGGER,"Nueva tabla de segmentos creada para la Patota: %d", idPatota);
}

void log_expulsionTripulanteSegmentacion(char* pidOtid, uint32_t id, char* accion, int patota, int nroSegmento){
	log_warning(LOGGER, "%s %d, %s -> Actualizacion en (PAT: %d, SEG: %d)", pidOtid, id, accion, patota, nroSegmento);
}



	/*MENSAJES PAGINACION*/
void log_transferenciaASwap(){
	log_warning(LOGGER, "No hay más espacio en memoria real, comienza el proceso de swap");
}

void Log_nuevaTPcreada(uint32_t idPatota){
	log_info(LOGGER,"Nueva tabla de paginas creada para la Patota: %d", idPatota);
}

void log_paginaVictimaEnMemoria(uint32_t nroFrame){
	log_info(LOGGER, "Se selecciono a la pagina que ocupa el frame %d en memoria como victima", nroFrame);
}

void log_expulsionTripulantePaginacion(char* pidOtid, uint32_t id, char* accion,  int nroFrameEnMemoria, int nroFrameEnSwap){
	log_warning(LOGGER, "%s %d, %s -> Actualizacion en (FRAME MEMORIA: %d, FRAME SWAP: %d)", pidOtid, id, accion, nroFrameEnMemoria, nroFrameEnSwap);
}





	/*	DUMP	*/
void log_dump(){
	log_warning(LOGGER,"La Dump de cache ha sido solicitada");
}

	/*OTROS MENSAJES*/


void log_tripulanteNoExiste(uint32_t tid){
	log_error(LOGGER, "el tripulante TID: %d NO existe", tid);
}


/*	-------------------------------------------------------------------
							WARNING
	-------------------------------------------------------------------*/
void log_memoriaInsuficiente(){
	log_warning(LOGGER, "No hay más espacio disponible en memoria");
}

void log_patotaInexistenteEnSistema(uint32_t idPatota){
	log_warning(LOGGER, "NO se encuentra registro en el sistema para Patota No. %d", idPatota);
}

void log_algoritmoDeRemplazoInvalido(){
	log_warning(LOGGER, "Algoritmo de reemplazo NO valido. INgresar en archivo de config LRU O CLOCK");
}

void log_esquemaMemoriaInvalido(){
	log_warning(LOGGER, "Esquema de Memoria NO valido. Ingresar en archivo de config SEGMENTACION o PAGINACION");
}

void log_criterioDeReemplazoInvalido(){
	log_warning(LOGGER, "Criterio de reemplazo NO valido. Ingresar en archivo de config FIRST_FIT o BEST_FIT");
}


/*	-------------------------------------------------------------------
							ERROR
	-------------------------------------------------------------------*/
void log_errorEsquemaMemoria(){
	log_error(LOGGER, "Error en el esquema de Memoria");
}

void log_errorMapeoSwap(){
	log_error(LOGGER, "Error en el mapeo de la MEMORIA SWAP");
}

void log_errorDeAsignacionPorAlgtmRemplazo(){
	log_error(LOGGER, "Error en la asignacion de Memoria por error en el Algoritmo de Remplazo");

}
