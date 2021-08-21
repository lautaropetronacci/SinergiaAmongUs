/*
 * movimientoBloques.h
 *
 *  Created on: 21 jul. 2021
 *      Author: utnso
 */

#ifndef MOVIMIENTOBLOQUES_H_
#define MOVIMIENTOBLOQUES_H_

#include "commonsSinergicas.h"
#include "logger.h"
#include "iMongoStore.h"
#include "archivos.h"
#include "sabotajes.h"

char* devolverPathDirecto(char*);
void crearArchivos();
void generarSuperBloque();
void generarArchivoBlocks();
void* sincronizarBlocks();


#endif /* MOVIMIENTOBLOQUES_H_ */
