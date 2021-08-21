/*
 * sabotajes.h
 *
 *  Created on: 21 jul. 2021
 *      Author: utnso
 */

#ifndef SABOTAJES_H_
#define SABOTAJES_H_

#include "commonsSinergicas.h"
#include "logger.h"
#include "movimientoBloques.h"
#include "archivos.h"
#include "iMongoStore.h"

#include <dirent.h>

void enviarAvisoDeSabotaje();
void chequeoSabotajes();

bool sabotajeCantidadBloques();

bool sabotajeBitmap();
bool compararResultadosBitmap(t_list*  listaAuxiliar, t_bitarray* bitarray, int tamanioBitmap, int tamanioLista);
void agregarBloqueALista(t_list* , t_config* );


bool sabotajeFiles();

bool sabotajeSize(t_config* metadata);
int cantidadLlenado(int bloque, char caracter[]);

bool sabotajeBlockCountYBlocks(t_config* metadata);

bool sabotajeBlocks(t_config* metadata);
char * obtenerCadenaSabotaje(char** listaDeBloques, int size, int block_count);
void restaurarArchivo(char** listaDeBloques, int size, int block_count, char caracter);

#endif /* SABOTAJES_H_ */
