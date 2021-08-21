
#ifndef MIRAMHQ_H_
#define MIRAMHQ_H_


#include "commonsSinergicas.h"
#include <commons/string.h>
#include <string.h>
#include <semaphore.h>
#include "Memoria.h"
#include "logger.h"


// Info para el Logger y el Config File
#define DIR_CONFIG "/home/utnso/tp-2021-1c-Sinergia/MiRAMHQ/miRAMHQ.config"
#define DIR_LOG "/home/utnso/tp-2021-1c-Sinergia/MiRAMHQ/miRAMHQ.log"

sem_t estados;

// Hilos
pthread_t AtencionClientes;
pthread_t sockets;
pthread_t mapa;

// Config
char* IP_ESCUCHA;
char* PUERTO_ESCUCHA;
char* IP_DISCORDIADOR;
char* PUERTO_DISCORDIADOR;
t_contacto contactoMiRamHQ;

// MEMORIA
int TAMANIO_MEMORIA;
char* ESQUEMA_MEMORIA;
int TAMANIO_PAGINA;
int TAMANIO_SWAP;
char* PATH_SWAP;
char* ALGORITMO_REEMPLAZO;
char* CRITERIO_REEMPLAZO;
char* DUMPPATH;
int available;
int socketServidor;


// OTRAS VG
t_config* CONFIG;
int pruebaMensajes;



// Funciones
void iniciarConfiguracion();
void leerConfiguracion();
void iniciarLogger();
bool inicializarMemoria(void);
void inicializarMapa();
void levantarServidor();
void admonSockets();
void atenderCLiente(int* socketCliente);
void gestionarMensaje (Paquete* paqueteRecibido, int socketCliente);
void gestionarHandshake(int modulo);
void iniciarPatota(int pid, char* tareas, int socketCliente);
void iniciarTripulante(int patota, int tid, int posicionX, int posicionY, int socketCliente);
void recibirUbicacionDelTripulante(int patota, int tid, int posicionX, int posicionY);
void actualizarEstado(int patota, int tid, int estado);
void solicitarProximaTarea(int patota, int tid, int socketCliente);
void expulsarTripulante(int patota, int tid);
void terminarmodulo();
void terminarPrograma();


#endif /* MIRAMHQ_H_ */
