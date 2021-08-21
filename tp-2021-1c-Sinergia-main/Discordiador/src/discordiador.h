/*
 * discordiador.h
 *
 *  Created on: 2 may. 2021
 *      Author: utnso
 */

#ifndef DISCORDIADOR_H_
#define DISCORDIADOR_H_

#include "commonsSinergicas.h"
#include "logger.h"
#include "tripulante.h"


#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <pthread.h>


#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <commons/collections/queue.h>
//#include <commons/collections/list.h>


// Conexiones con los otros modulos
bool SERVIDOR_MONGO_CONECTADO;
bool SERVIDOR_MIRAM_CONECTADO;
int socketServidor;



char* comando;
char** parametros;
char* fechaYHora;


// HILOS

pthread_t consola; // Hilo que lleva la consola
pthread_t servidor; //Hilo que lleva el servidor
pthread_t threadAtencion; // Hilo que atiende los mensajes que llegan al servidor


// FUNCIONES

void inicializacion();
void realizarConexiones();
void realizarEstructuras();
void crearSemaforos();



	// CONSOLA //
void leerConsola();

// Funciones para comando Iniciar Patota

typedef enum{
	X,
	Y
} ejePosicion;


bool notificarPatotaAMIRAMHQ(char* pathArchivoTAreas, uint32_t idPatota);
int obtenerPosicion(ejePosicion,char*);

void liberarArray(char** array);
int tamanioArray(char** array);

// Funciones para comando Listar Tripulantes

void listarTripulantes();
char* estadoCHAR(estadoTripulante estado);


// Funciones para comando Iniciar Planificación

void inicioPlanificacion();


// Funciones para comando Pausar Planificación

void pausarPlanificacion();


// Funciones para comando Expulsar Tripulante

bool existeTripulanteConID(uint32_t idTripulante);
void expulsarTripulante(uint32_t idTripulante);


// Funciones para comando Obtener Bitácora

void obtenerBitacora(char* idTripulante);


// Funciones para comando Terminar




	// TRIPULANTE //


	// SABOTAJES //

void resolverSabotaje(t_sabotaje*, int);
infoTripulante* encontrarTripulanteCercanoAlSabotaje(uint32_t posX, uint32_t posY);
void moverTripulacionEstadoBloqueado();
void sacarTripulacionEstadoBloqueado();

// Funciones para las conexiones

void enviarHankshakeAiMongoStore(int socketServidor);
void enviarHankshakeAMIRAMHQ(int socketServidor);
void levantarServidor(int socketServidor);
void atenderCliente(int* socketCliente);
void gestionarMensaje (Paquete* paqueteRecibido, int socketCliente);

bool enviarContenidoArchivoTareasAMIRAMHQ(char* contenidoArchivo, uint32_t pid);

void check_conexion(int conexion);

#endif /* DISCORDIADOR_H_ */
