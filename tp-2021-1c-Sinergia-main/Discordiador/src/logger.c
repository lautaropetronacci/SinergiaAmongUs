/*
 * logger.c
 *
 *  Created on: 11 may. 2021
 *      Author: utnso
 */

#include "logger.h"

void leerConfiguracion(){

	IP_MI_RAM_HQ = config_get_string_value(config, "IP_MI_RAM_HQ");
	PUERTO_MI_RAM_HQ = config_get_string_value(config, "PUERTO_MI_RAM_HQ"); //TODO: recibimos los puertos como chars pero el enunciado dice que son tipo numerico
	IP_I_MONGO_STORE = config_get_string_value(config, "IP_I_MONGO_STORE");
	PUERTO_I_MONGO_STORE = config_get_string_value(config, "PUERTO_I_MONGO_STORE");
	GRADO_MULTITAREA = config_get_int_value(config, "GRADO_MULTITAREA");
	ALGORITMO = algoritmoEnum(config_get_string_value(config, "ALGORITMO"));
	QUANTUM = config_get_int_value(config, "QUANTUM");
	DURACION_SABOTAJE = config_get_int_value(config, "DURACION_SABOTAJE");
	RETARDO_CICLO_CPU = config_get_int_value(config, "RETARDO_CICLO_CPU");
	IP_ESCUCHA = config_get_string_value(config, "IP");
	PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO");

	contactoDiscordiador.ip = IP_ESCUCHA;
	contactoDiscordiador.puerto = PUERTO_ESCUCHA;
	contactoDiscordiador.modulo = DISCORDIADOR;



}


TIPO_ALGORITMO algoritmoEnum(char* algoritmoChar){
	if(strcmp(algoritmoChar,"RR") == 0 )
		return RR; // Por convención es FIFO el algoritmo
	return FIFO;
}


void iniciarConfiguracion() {
	config = config_create(DIR_CONFIG);
}

void iniciarLogger() {
	logger = log_create(DIR_LOG, "discordiador", true, LOG_LEVEL_INFO);
}





void logInicioDelServidor(){
	log_info(logger, "Se inició el servidor del Discordiador, esperando mensaje.");
}

void logServidorFinalizadoCorrectamente(){
	log_info(logger,"Se finalizó el servidor del Discordiador correctamente.");
}

void logServidorFinalizadoIncorrectamente(){
	log_warning(logger,"Error al finalizar el servidor del Discordiador // O nunca se inició el mismo.");
}


void logFinDeProgramaCorrecto(){
	log_info(logger, "El programa terminó correctamente :).");
}


void logComandoInvalidoIngresado(){
	log_info(logger, "Se ingresó un comando invalido en la consola del Discordiador o un comando valido con una cantidad incorrecta de parámetros.");
}

void logComandoIngresado(char* comando){
	log_info(logger, "Se ingresó el comando %s." , comando );
}

void logPlanificacionIniciada(){
	log_info(logger, "Se inició la Planificación.");
}

void logPlanificacionPausada(){
	log_info(logger, "Se pausó la Planificación.");
}

void logTripulantesListados(){
	log_info(logger, "Se listaron los tripulantes en consola.");
}

void logTripulanteExpulsado(uint32_t idTripulante){
	log_info(logger, "Se expulsó al tripulante con el id %d", idTripulante);
}

void logTripulanteNOExpulsado(uint32_t idTripulante){
	log_warning(logger, "NO se expulsó al tripulante con el id %d, ya que no existe", idTripulante);
}

void logBitacoraObtenida(char* idTripulante){
	log_info(logger, "Se obtuvo la bitacora del tripulante con el id %s", idTripulante);
}











void logIniciacionTripulanteNegadaPorRAM(int idTripulante){
	log_warning(logger, "No se inició un tripulante de la patota id %d ya que no RAM le negó la iniciación", idTripulante);
}


































