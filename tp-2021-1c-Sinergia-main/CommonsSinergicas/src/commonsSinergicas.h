/*
 * commonsSinergicas.h
 *
 *  Created on: 2 may. 2021
 *      Author: utnso
 */

#ifndef SRC_COMMONSSINERGICAS_H_
#define SRC_COMMONSSINERGICAS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <stdbool.h>
#include <netinet/in.h>

#include<commons/string.h>
#include<commons/config.h>
#include<commons/temporal.h>
#include<readline/readline.h>

#include <pthread.h>

// Mensajes

typedef enum{
	HANDSHAKE,

	//Discordiador:
	INICIAR_PATOTA,
	LISTAR_TRIPULANTES,
	EXPULSAR_TRIPULANTE,
	INICIAR_PLANIFICACION,
	PAUSAR_PLANIFICACION,
	OBTENER_BITACORA,
	TERMINAR_TRIPULANTE,
	TERMINAR,

	//Tripulante:
	INFORMAR_INICIO,

	ACTUALIZAR_BITACORA, //INFORMAR_MOVIMIENTO, COMIENZA_EJECUCION, FINALIZA_TAREA, CORRE_UBICACION_SABOTAJE, RESUELVE_SABOTAJE

	INFORMAR_UBICACION,
	SOLICITAR_PRIMERA_TAREA,
	SOLICITAR_PROXIMA_TAREA,
	INVOCAR_FSCK,
	INICIAR_TRIPULANTE,
	RECIBIR_LA_UBICACION_DEL_TRIPULANTE,
	ACTUALIZAR_ESTADO,
	CONSULTAR_EXISTENCIA_ARCHIVO_TAREA,
	R_CONSULTAR_EXISTENCIA_ARCHIVO_TAREA,
	CREAR_ARCHIVO_TAREA,
	LLENADO_ARCHIVO,
	VACIADO_ARCHIVO,
	ELIMINAR_ARCHIVO_TAREA,




	//IMongoStore
	ATENDER_TRIPULANTE,
	//RECIBIR_SENIAL,
	COMENZAR_PROTOCOLO_FSCK,
	//FINALIZAR_PROTOCOLO_FSCK,
	//NOTIFICAR_DICORDIADOR,

	PERMITIR_LECTURAS,
	PERMITIR_ESCRITURAS_CONCURRENTES,
	INICIAR_SERVIDOR,
	NOTIFICAR_SABOTAJE,
	TRABAJAR_FILE_SYSTEM_LIMPIO,
	RECUPERAR_FILE_SYSTEM_EXISTENTE,
	GENERAR_ARCHIVOS_DE_BLOQUES,
	CAMBIAR_POSICION,
	COMENZAR_EJECUCION_TAREA,
	FINALIZAR_TAREA,
	CORRER_EN_PANICO_A_SABOTAJE,
	RESOLVER_SABOTAJE,

	/*(ver parte de archivos)
	(ver parte anterior a reparacion)*/

	REPARACION_SUPERBLOQUE,
	REPARACION_BITMAP,
	REPARACION_SIZE,
	REPARACION_BLOCK_COUNT,
	REPARACION_BLOCKS,


	//Respuestas
	R_INICIAR_PATOTA,
	R_INICIAR_TRIPULANTE,
	R_RECIBIR_LA_UBICACION_DEL_TRIPULANTE,
	R_SOLICITAR_PROXIMA_TAREA,
	R_EXPULSAR_TRIPULANTE,
	R_TERMINAR_TRIPULANTE,
	R_OBTENER_BITACORA,




	ERROR_PROCESO
} codigoMensaje;

typedef enum{
	GENERAR_OXIGENO,
	CONSUMIR_OXIGENO,
	GENERAR_COMIDA,
	CONSUMIR_COMIDA,
	GENERAR_BASURA,
	DESCARTAR_BASURA,
	TAREA_GENERICA,
	ACTIVAR_PROTOCOLO_FSK
}t_tipoTarea;

typedef enum{
	DISCORDIADOR,
	TRIPULANTE,
	MIRAMHQ,
	IMONGOSTORE
}t_modulo;

typedef enum{
	N,
	R,
	E,
	B,
	F
}t_estadoTripulante;

/*	-------------------------------------------------------------------
					ESTRUCTURAS DE COMUNICAICON
	-------------------------------------------------------------------	*/

typedef struct {
	codigoMensaje tipoMensaje;
	int tamanioMensaje;
}__attribute__((packed)) Header;

typedef struct {
	Header header;
	void* mensaje;
}__attribute__((packed)) Paquete;


typedef struct {
	t_modulo modulo;
	char* ip;
	char* puerto;
}t_contacto;



/*	-------------------------------------------------------------------
	NOTA: dejar con uint32_t porque así estaba en la cosigna, tanto
		para en TCB como PCB
	-------------------------------------------------------------------*/
typedef struct {
	uint32_t pid;
	uint32_t tareasPatota;
}t_pcb;

typedef struct {
	uint32_t tid;
	char estado;
	uint32_t posicionX;
	uint32_t posicionY;
	uint32_t proximaInstruccion;
	uint32_t ptoPCB;
}t_tcb;

/*	-------------------------------------------------------------------*/


typedef struct {
    char* 		nombre;
    uint32_t	parametros;
    uint32_t 	posicionX;
    uint32_t	posicionY;
    uint32_t 	duracion;
    bool 		esUltimaTarea;
    bool 		requiereIO;
} t_tarea;

typedef struct{
	t_contacto 	contacto;
	bool 		deboResponder;
	void* 		informacion;
}t_handshake;

typedef struct {
	uint32_t 	pid;
	char* 		tareas;
}t_iniciarPatota;

typedef struct {
	uint32_t 	tid;
	char* 		bitacora;
}t_RObtenerBitacora;

typedef struct {
	uint32_t 	patota;
	uint32_t 	tid;
	uint32_t 	posicionX;
	uint32_t 	posicionY;
}t_iniciarTripulante;

typedef struct {
	uint32_t 	patota;
	uint32_t	tid;
}t_tripulante;

typedef struct {
	uint32_t tid;
	t_tarea tarea;
}t_RSolicitarProximaTarea;


typedef struct{
	uint32_t 	tid;
	char* 		nombreArchivo;
}t_consultarExistenciaArchivoTarea;

typedef struct {
	char* nombreArchivo;
	char caracterLlenado;
	int cantidad;
} t_archivoTarea;

typedef struct{
	t_tripulante tripulante;
	uint32_t estado;
}t_actualizarEstado;

/*----------Bitácora----------*/

typedef struct {
	uint32_t 	tid;
	char* 		reporte;
}t_actualizarBitacora;

// estructuras para archivos

typedef struct {
	uint32_t block_size;
	uint32_t blocks;
	int bitmap[];
} t_superBloque;


typedef struct{
	uint32_t cantidadDeBloques; //la cantidad de bitmaps tiene que ser igual a la cantidad de bloques y al sizeof bitmap
	uint32_t tamanioBloque;		//y el tamaño de bloque igual al block_size
} t_block;

typedef struct{
	uint32_t posicionX; //la cantidad de bitmaps tiene que ser igual a la cantidad de bloques y al sizeof bitmap
	uint32_t posicionY;		//y el tamaño de bloque igual al block_size
} t_sabotaje;


// Funciones

int crearConexion(char *ip, char* puerto);

int iniciarServidor(char *ip, char* puerto);

int esperarCliente(int socket_servidor);

void liberarConexion(int socket);

bool enviarPaquete(int socketCliente, Paquete* paquete);

bool enviarMensaje(int socketFD, codigoMensaje tipoMensaje, void* estructura);

bool enviarHandshake(int socketFD, t_contacto modulo, void* mensaje);

void serializarVariable(void* a_enviar, void* a_serializar, int tamanio, int *offset);

void* serializarMensaje(void* estructura, codigoMensaje codigo, int* tamanio);

void serializarChar(void* mensajeSerializado, char* mensaje, uint32_t tamanio, int* offset);

//Recibir datos
int recibirMensaje(void* paquete, int socketFD, uint32_t cantARecibir);

Paquete* recibirPaquete(int socketFD, t_contacto contacto);

void deserializarMensaje(void* mensajeRecibido, Paquete* paqueteRecibido, t_contacto contacto);

void copiarVariable(void* variable, void* stream, int* offset, int size);

void deserializarChar(void* stream, int* offset, char** receptor);











































// Funcionres randoms que dani crea donde no deberia

int max(int a, int b);

int min(int a, int b);

#endif /* SRC_COMMONSSINERGICAS_H_ */
