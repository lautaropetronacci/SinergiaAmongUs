/*
 * logger.c
 *
 *  Created on: 13 may. 2021
 *      Author: utnso
 */
#include "logger.h"

void logConexion(uint32_t patota){
    log_info(logger, "Se establecio conexion con el Discordiador, patota N° %d.", patota);
}

void logIMongoStore(){
    log_info(logger,"El modulo IMongoStore esta listo para recibir mensajes.");
}

void logCaracteresAgregados(int caracteresAescribir){
	log_info(logger,"%d nuevos caracteres agregados correctamente.", caracteresAescribir);
}

void logBorrarCaracteres(){
	log_info(logger,"Se eliminaron bloques correctamente.");
}
