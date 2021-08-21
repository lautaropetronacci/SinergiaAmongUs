/*
 * tripulante.h
 *
 *  Created on: 7 may. 2021
 *      Author: utnso
 */

#ifndef TRIPULANTE_H_
#define TRIPULANTE_H_

#include <stdbool.h>
#include <semaphore.h>
#include "commonsSinergicas.h"
#include "commons/collections/queue.h"
#include "logger.h"




extern pthread_mutex_t mutexListaTripulantes;

t_list* TRIPULANTES;

// Listas Planificacion

//t_queue* COLA_LISTO;
t_list* LISTA_LISTO;
t_list* LISTA_TRABAJANDO;
t_list* LISTA_BLOQUEADO_IO;
t_list* LISTA_BLOQUEADO_EMERGENCIA;

sem_t semListaTrabajando;
sem_t semExecuteBoolCicloDeReloj;
sem_t sem;
sem_t semInicioPlanificacion;
sem_t semSabotajeEnCurso;


typedef enum{
	NEW,
	EXECUTE,
	READY,
	BLOCK_IO,
	BLOCK_EMERGENCIA,
	EXIT,
	EXPULSADO
}estadoTripulante;

uint32_t ID_PATOTA;
uint32_t ID_TRIPULANTE;

pthread_t tripulante;

typedef struct{
	uint32_t idTripulante;
	uint32_t idPatota;
	uint32_t posicionX;
	uint32_t posicionY;
	estadoTripulante estado;
	uint32_t cicloDeRelojRestante;
	bool fueEliminado;
	t_tarea tareaARealizar;
}infoTripulante;


/* Enviado a miRAMHQ.h
typedef struct{
	uint32_t PID;
	uint32_t tamanioInfo;
	void* informacion;
}patotaControlBlock;

typedef struct{
	t_contacto contacto;
	bool deboResponder;
	patotaControlBlock* punteroAPCB;
}tripulanteControlBlock;
*/



//Funciones:

void instanciarTripulante(uint32_t,uint32_t,uint32_t);
void vidaTripulante(infoTripulante* infoTripulante);

bool informarAMIRAMHQ(uint32_t idPatota, uint32_t idTripulante, uint32_t posicionX, uint32_t posicionY);

void solicitarTarea(infoTripulante* tripulante);
//t_tarea* solicitarTareaAMIRAMHQ(uint32_t idTripulante, uint32_t idPatota);


// TAREAS

void realizarTarea(infoTripulante* tripulante);
void moverHaciaTarea(infoTripulante* tripulante);
bool llegoATarea(infoTripulante* tripulante);
uint32_t distanciaAPosicion( infoTripulante* tripulante ,uint32_t posX, uint32_t posY);
void notificarMovimiento(uint32_t idTripulante, uint32_t idPatota, uint32_t posicionXAnterior, uint32_t posicionYAnterior, uint32_t posicionX, uint32_t posicionY);

bool existeArchivo(char* nombreArchivo);
void crearArchivo(char* nombreArchivo, char caracterDeLlenado);
void informarInexistenciaArchivo(char* nombreArchivo);
void llenarArchivoEnIMONGO(char* nombreArchivo, char caracterDeLlenado, int cantidadLlenada);
void vaciarArchivoEnIMONGO(char* nombreArchivo, char caracterDeVaciado, int cantidad);
void eliminarArchivoEnIMONGO(char*);

int nombreTareaENUM(char* nombre);
void informarFinTripulante(uint32_t idTripulante, uint32_t idPatetente);
void informarCambioEstado(uint32_t idTripulante,uint32_t idPatota, t_estadoTripulante estadoTripulante);

void informarIMONGO(uint32_t tid,char* reporte);
void informarInicioTripulante(uint32_t idTripulante);

int conexionDiscord();
int conexionDiscordMongo();

#endif /* TRIPULANTE_H_ */
