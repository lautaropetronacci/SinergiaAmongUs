/*
 * archivos.h
 *
 *  Created on: 21 jul. 2021
 *      Author: utnso
 */

#ifndef ARCHIVOS_H_
#define ARCHIVOS_H_

#include "commonsSinergicas.h"
#include "logger.h"
#include "iMongoStore.h"
#include "movimientoBloques.h"
#include "sabotajes.h"


void liberarEspacioListaDeBloques(char**);
void actualizarArchivo(t_config*, int);
void actualizarArchivoPorEliminacion(t_config*, int);
char* devolverCadenaMD5(char**);
char* crearMD5(char*);
char* devolverPathArchivoTarea(char*);
bool consultarExistenciaArchivo(char*);
void generarArchivoDeCero(t_archivoTarea*);
void agregarCaracter(char*, int, char);
int buscarBloqueDisponible(t_config*);
int espacioDisponible(t_config*);
int espacioDisponibleBitacora(t_config*);
void aniadirCaracteres(char, int, int, int);
void errorDeBloques();

void crearBitacora(uint32_t);
char* devolverPathBitacora(uint32_t);
char* devolverDirectorioBitacora();

void borrarCaracteres(char*, int);
void agregarAccionesTripulanteABlocks(char*, uint32_t);
void copiarSubstringEnBlocks(t_config* , int, char*, int);
void actualizarArchivoBitacora(t_config* , int);
void eliminarCaracteresDelBlocks(int, int, int);
void actualizarBitmap(int);
void eliminarBloque(t_config*,int);
int buscarUltimoBloque(t_config*);
char * devolverBitacora(uint32_t);

void descartarBasura();





#endif /* ARCHIVOS_H_ */
