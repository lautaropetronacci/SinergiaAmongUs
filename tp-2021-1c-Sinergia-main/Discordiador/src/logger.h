/*
 * logger.h
 *
 *  Created on: 11 may. 2021
 *      Author: utnso
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "commonsSinergicas.h"


typedef enum{
	FIFO,
	RR
} TIPO_ALGORITMO ;


// Config

char * IP_MI_RAM_HQ;
char * PUERTO_MI_RAM_HQ;
char * IP_I_MONGO_STORE;
char * PUERTO_I_MONGO_STORE;
int GRADO_MULTITAREA;
TIPO_ALGORITMO ALGORITMO;
int QUANTUM;
int DURACION_SABOTAJE;
int RETARDO_CICLO_CPU;
char * IP_ESCUCHA;
char * PUERTO_ESCUCHA;
bool PLANIFICACION_PAUSADA;



// Info para el Logger y el Config File

#define DIR_CONFIG "/home/utnso/tp-2021-1c-Sinergia/Discordiador/discordiador.config"
#define DIR_LOG "/home/utnso/tp-2021-1c-Sinergia/Discordiador/discordiador.log"

t_log* logger;
t_config* config;
t_contacto contactoDiscordiador;

TIPO_ALGORITMO algoritmoEnum(char* algoritmoChar);

//Sabotajes
bool BAJO_SABOTAJE;

void leerConfiguracion();
void iniciarConfiguracion();
void iniciarLogger();


void logInicioDelServidor();
void logServidorFinalizadoCorrectamente();
void logServidorFinalizadoIncorrectamente();

void logFinDeProgramaCorrecto();
void logComandoInvalidoIngresado();

void logPlanificacionIniciada();
void logPlanificacionPausada();

void logComandoIngresado(char* comando);

void logTripulantesListados();
void logTripulanteExpulsado(uint32_t idTripulante);
void logTripulanteNOExpulsado(uint32_t idTripulante);

void logIniciacionTripulanteNegadaPorRAM(int idTripulante);

void logBitacoraObtenida(char* idTripulante);

#endif /* LOGGER_H_ */
