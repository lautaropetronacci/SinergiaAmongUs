/*
 * gui.h
 *
 *  Created on: 24 may. 2021
 *      Author: utnso
 */

#ifndef GUI_H_
#define GUI_H_

#include <nivel-gui/nivel-gui.h>
#include <nivel-gui/tad_nivel.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <curses.h>
#include "estructurasDeMemoria.h"



#define ASSERT_CREATE(nivel, id, err)                                                   \
    if(err) {                                                                           \
        nivel_destruir(nivel);                                                          \
        nivel_gui_terminar();                                                           \
        fprintf(stderr, "Error al crear '%c': %s\n", id, nivel_gui_string_error(err));  \
        return EXIT_FAILURE;                                                            \
    }

extern pthread_mutex_t mutex_mapa;

typedef struct{
	NIVEL* nivel;
	char id;
	int x;
	int y;
}t_mapaTrip;

NIVEL* nivel;
int cols, rows;
int err;
t_list* tripulantesEnMapa;

/* FUNCIONES */
void dibujarMapa();
void agregarTripulante(uint32_t tid, uint32_t posX, uint32_t posY);
int get_indice(uint32_t tid);
void borrarTripulante(uint32_t tid);



#endif /* GUI_H_ */
