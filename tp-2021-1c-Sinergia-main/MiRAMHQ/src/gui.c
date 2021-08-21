/*
 * gui.c
 *
 *  Created on: 24 may. 2021
 *      Author: utnso
 */

#include "gui.h"

void dibujarMapa(){
	void mostrarTripulante(void* elemento){
		t_mapaTrip* tripulante = (t_mapaTrip*) elemento;
		int err = item_mover(tripulante->nivel, tripulante->id, tripulante->x, tripulante->y);

		if(err == NGUI_ITEM_NOT_FOUND){
			personaje_crear(tripulante->nivel, tripulante->id, tripulante->x, tripulante->y);
			//item_mover(tripulante->nivel, tripulante->id, tripulante->x, tripulante->y);
		}
	}

	//int init, area;
	tripulantesEnMapa = list_create();

	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&cols, &rows);
	nivel = nivel_crear("prueba");

	while (ON) {
		nivel_gui_dibujar(nivel);
		pthread_mutex_lock(&mutex_mapa);
		list_iterate(tripulantesEnMapa, mostrarTripulante);
		pthread_mutex_unlock(&mutex_mapa);
	}
	nivel_destruir(nivel);
	nivel_gui_terminar();
	//return (init == NGUI_SUCCESS && area == NGUI_SUCCESS);
}

char obtenerID(uint32_t tid){
	switch (tid) {
		case 1 ... 9:
			return tid + 48;
			break;
		default: return tid + 55;
			break;
	}
}

void agregarTripulante(uint32_t tid, uint32_t posX, uint32_t posY){
	t_mapaTrip* tripulante = malloc(sizeof(*tripulante));
	tripulante->nivel = nivel;
	tripulante->id = obtenerID(tid);
	tripulante->x = posX;
	tripulante->y = posY;

	list_add(tripulantesEnMapa, (void*)tripulante);

}

int get_indice(uint32_t tid){
	t_link_element* elemento = tripulantesEnMapa->head;
	t_mapaTrip* tripulante = (t_mapaTrip*) elemento->data;
	int indice = 0;

	while(elemento != NULL) {
		char buscar = obtenerID(tid);
		if(tripulante->id == buscar)
			return indice;
		elemento = elemento->next;
		tripulante = elemento == NULL ? NULL : elemento->data;
	}
	return -1;
}

void actualizarTripulante(uint32_t tid, uint32_t posX, uint32_t posY){
	t_mapaTrip* tripulante;
	tripulante->nivel = nivel;
	tripulante->id = obtenerID(tid);
	tripulante->x = posX;
	tripulante->y = posY;


	int indice = get_indice(tid);
	list_replace(tripulantesEnMapa, indice, (void*)tripulante);
}

void borrarTripulante(uint32_t tid){
	item_borrar(nivel, obtenerID(tid));
	int indice = get_indice(tid);
	list_remove(tripulantesEnMapa, indice);
}

void vaciarListaMapa(){
	t_link_element* elemento = tripulantesEnMapa->head;
	t_mapaTrip* tripu = (t_mapaTrip*) elemento->data;

	while(tripu != NULL){
		free(tripu);

	}
}

