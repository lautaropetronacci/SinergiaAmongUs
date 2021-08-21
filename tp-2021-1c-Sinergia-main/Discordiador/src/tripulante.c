/*
 * tripulante.c
 *
 *  Created on: 7 may. 2021
 *      Author: utnso
 */

#include "tripulante.h"

pthread_mutex_t mutexListaListo = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaBloqueadoIO = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexArchivoOxigeno = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexArchivoComida = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexArchivoBasura = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaTrabajando = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaTripulantes = PTHREAD_MUTEX_INITIALIZER;



void instanciarTripulante(uint32_t idPatota, uint32_t posicionX, uint32_t posicionY){

	infoTripulante* aux = malloc(sizeof(infoTripulante));

	aux -> fueEliminado = false;
	aux -> idTripulante = ID_TRIPULANTE ++;
	aux->idPatota = idPatota;
	aux->posicionX = posicionX;
	aux->posicionY = posicionY;
	aux->estado = NEW;
	aux->tareaARealizar.nombre = string_new();
	pthread_create(&tripulante ,NULL, (void*) vidaTripulante, aux);

}



/*	-------------------------------------------------------------------
						TRIPULANTES
-------------------------------------------------------------------	*/


void vidaTripulante(infoTripulante* tripulante){

	bool status = informarAMIRAMHQ(tripulante ->idTripulante, tripulante ->idPatota, tripulante ->posicionX, tripulante ->posicionY); // Informar al módulo Mi-RAM HQ que desea iniciar, indicando a qué patota pertenece, retorna false si miRAM no permite instanciar al tripulante


	if(status){
		//pthread_mutex_lock(&mutexPrueba);
		informarInicioTripulante(tripulante->idTripulante);
		//pthread_mutex_unlock(&mutexPrueba);

		tripulante->estado = NEW;


		pthread_mutex_lock(&mutexListaTripulantes);
		list_add(TRIPULANTES,tripulante);
		pthread_mutex_unlock(&mutexListaTripulantes);



		sem_post(&sem);

		bool tieneMismoId(infoTripulante* tripulanteAux){
			return tripulanteAux->idTripulante == tripulante->idTripulante;
		}



		//solicitarTarea(tripulante);
		int quantumDisponible;

		switch(ALGORITMO){


		/*	-------------------------------------------------------------------
										FIFO
			-------------------------------------------------------------------	*/

		case FIFO:


				while(1){



					switch(tripulante->estado){

				case NEW:

						sem_wait(&semInicioPlanificacion);
						sem_post(&semInicioPlanificacion);

						tripulante->estado = READY;
						informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, R);


						if(tripulante->fueEliminado){
							tripulante->estado = EXPULSADO;
						}else{
							solicitarTarea(tripulante);
						}

						break;


				case READY:

						pthread_mutex_lock(&mutexListaListo);
						list_add(LISTA_LISTO,tripulante);
						pthread_mutex_unlock(&mutexListaListo);


						while(list_get(LISTA_LISTO, 0) != tripulante){}
						sem_wait(&semListaTrabajando);

						if(BAJO_SABOTAJE){
							break;
						}

						pthread_mutex_lock(&mutexListaListo);
						list_remove(LISTA_LISTO, 0);
						pthread_mutex_unlock(&mutexListaListo);

						if(tripulante->fueEliminado){

							tripulante->estado = EXPULSADO;
							sem_post(&semListaTrabajando);
						}
						else{

							tripulante->estado = EXECUTE;

							pthread_mutex_lock(&mutexListaTrabajando);
							list_add(LISTA_TRABAJANDO,tripulante);
							pthread_mutex_unlock(&mutexListaTrabajando);

							informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, E);

							sleep(RETARDO_CICLO_CPU);
						}



						break;


				case EXECUTE:

						sem_wait(&semExecuteBoolCicloDeReloj);

						if(!llegoATarea(tripulante)) moverHaciaTarea(tripulante);
						else{

							if(tripulante->tareaARealizar.requiereIO){

								realizarTarea(tripulante);
								tripulante->estado = BLOCK_IO;
								informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, B);

								pthread_mutex_lock(&mutexListaTrabajando);
								list_remove_by_condition(LISTA_TRABAJANDO, (void*)tieneMismoId);
								pthread_mutex_unlock(&mutexListaTrabajando);

								pthread_mutex_lock(&mutexListaBloqueadoIO);
								list_add(LISTA_BLOQUEADO_IO, tripulante);
								pthread_mutex_unlock(&mutexListaBloqueadoIO);


							}

							else if( --tripulante->cicloDeRelojRestante == 0 ){

								realizarTarea(tripulante); //Realizo la tarea dentro del if ya que se utilizó todo el el timepo de ciclos de reloj

								if(tripulante->tareaARealizar.esUltimaTarea){

									tripulante->estado = EXIT;
								}
								else {

									char* reporte =  string_new();
									string_append(&reporte,"Se finaliza la tarea " );
									string_append(&reporte,tripulante->tareaARealizar.nombre);
									string_append(&reporte,"\n" );
									informarIMONGO(tripulante->idTripulante,reporte);
									free(reporte);

									solicitarTarea(tripulante);
									//tripulante->estado = READY;
									//informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, R);
								}

								pthread_mutex_lock(&mutexListaTrabajando);
								list_remove_by_condition(LISTA_TRABAJANDO, (void*)tieneMismoId);
								pthread_mutex_unlock(&mutexListaTrabajando);

								//sem_post(&semListaTrabajando); // cuando sale del estado EXECUTE el contador con el valor de GRADO_MULTITAREA se le hace signal


							}
						}


						if(tripulante->fueEliminado){
							tripulante->estado = EXPULSADO;
						}
						sem_post(&semExecuteBoolCicloDeReloj);


						if(PLANIFICACION_PAUSADA){
							sem_wait(&semInicioPlanificacion);
							sem_post(&semInicioPlanificacion);
						}

						if(tripulante->estado != EXECUTE){
							sem_post(&semListaTrabajando);
						}else sleep(RETARDO_CICLO_CPU); // REVISAR



						break;


				case BLOCK_IO:

						sleep(RETARDO_CICLO_CPU);

						if(tripulante == list_get(LISTA_BLOQUEADO_IO,0) /*&& BAJO_SABOTAJE*/){ // TODO ¿Se tendria que hacer un FIFO con los blocks? PeepoShy :3
							log_warning(logger,"El tripulante %d, se encuentra usando el dispositivo de E/S", tripulante->idTripulante);
							if( --tripulante->cicloDeRelojRestante == 0 ){

								if(tripulante->tareaARealizar.esUltimaTarea){
									tripulante->estado = EXIT;
								}
								else {

									char* reporte =  string_new();
									string_append(&reporte,"Se finaliza la tarea " );
									string_append(&reporte,tripulante->tareaARealizar.nombre);
									string_append(&reporte,"\n" );
									informarIMONGO(tripulante->idTripulante,reporte);
									free(reporte);

									solicitarTarea(tripulante);
									tripulante->estado = READY;
									informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, R);
								}

								pthread_mutex_lock(&mutexListaBloqueadoIO);
								list_remove_by_condition(LISTA_BLOQUEADO_IO, (void*)tieneMismoId);
								pthread_mutex_unlock(&mutexListaBloqueadoIO);

								//sem_post(&semListaTrabajando); // cuando sale del estado EXECUTE el contador con el valor de GRADO_MULTITAREA se le hace signal
							}

						}else{
							log_warning(logger,"El tripulante %d, se encuentra en la cola de espera del dispositivo de E/S", tripulante->idTripulante);
						}

						if(tripulante->fueEliminado){

							tripulante->estado = EXPULSADO;

							pthread_mutex_lock(&mutexListaBloqueadoIO);
							list_remove_by_condition(LISTA_BLOQUEADO_IO, (void*)tieneMismoId);
							pthread_mutex_unlock(&mutexListaBloqueadoIO);

						}


						break;


				case BLOCK_EMERGENCIA:


						sleep(RETARDO_CICLO_CPU);

						if(tripulante->fueEliminado){
							tripulante->estado = EXPULSADO;
						}

						break;


				case EXIT:
					//informarFinTripulante(tripulante ->idTripulante, tripulante ->idPatota);
					informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, F);
					log_error(logger,"El tripulante %d realizó todas sus tareas",tripulante->idTripulante);
					return;

				case EXPULSADO:
					//informarFinTripulante(tripulante ->idTripulante, tripulante ->idPatota);
					informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, F);
					log_error(logger,"El tripulante %d fue expulsado",tripulante->idTripulante);
					return;

					}
				}; // Fin de while
			break; // Fin de case FIFO


		/*	-------------------------------------------------------------------
									RR
		-------------------------------------------------------------------	*/

			case RR:

				while(1){

					switch(tripulante->estado){

				case NEW:

						sem_wait(&semInicioPlanificacion);
						sem_post(&semInicioPlanificacion);

						tripulante->estado = READY;
						informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, R);

						if(tripulante->fueEliminado){
							tripulante->estado = EXPULSADO;
						}else{
							solicitarTarea(tripulante);
						}


						break;


				case READY:

						quantumDisponible = QUANTUM;

						pthread_mutex_lock(&mutexListaListo);
						list_add(LISTA_LISTO,tripulante);
						pthread_mutex_unlock(&mutexListaListo);


						while(list_get(LISTA_LISTO, 0) != tripulante){}
						sem_wait(&semListaTrabajando); 						// semaforo contador de execute, que se inicia con el valor de GRADO_MULTITAREA

						if(BAJO_SABOTAJE){
							break;
						}

						pthread_mutex_lock(&mutexListaListo);
						list_remove(LISTA_LISTO, 0);
						pthread_mutex_unlock(&mutexListaListo);

						if(tripulante->fueEliminado){
							tripulante->estado = EXPULSADO;
							sem_post(&semListaTrabajando);
						}
						else{

							tripulante->estado = EXECUTE;

							pthread_mutex_lock(&mutexListaTrabajando);
							list_add(LISTA_TRABAJANDO,tripulante);
							pthread_mutex_unlock(&mutexListaTrabajando);

							informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, E);
							//solicitarTarea(tripulante);
							sleep(RETARDO_CICLO_CPU);
						}



						break;


				case EXECUTE:

						sem_wait(&semExecuteBoolCicloDeReloj);


						if(!llegoATarea(tripulante)){ moverHaciaTarea(tripulante);}
						else{

							if(tripulante->tareaARealizar.requiereIO){
								realizarTarea(tripulante);
								tripulante->estado = BLOCK_IO;
								informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, B);

								pthread_mutex_lock(&mutexListaTrabajando);
								list_remove_by_condition(LISTA_TRABAJANDO, (void*)tieneMismoId);
								pthread_mutex_unlock(&mutexListaTrabajando);

								pthread_mutex_lock(&mutexListaBloqueadoIO);
								list_add(LISTA_BLOQUEADO_IO, tripulante);
								pthread_mutex_unlock(&mutexListaBloqueadoIO);
							}


							else if( --tripulante->cicloDeRelojRestante == 0 ){

								realizarTarea(tripulante); //Realizo la tarea dentro del if ya que se utilizó todo el el timepo de ciclos de reloj

								if(tripulante->tareaARealizar.esUltimaTarea){

									tripulante->estado = EXIT;
								}
								else {


									char* reporte =  string_new();
									string_append(&reporte,"Se finaliza la tarea " );
									string_append(&reporte,tripulante->tareaARealizar.nombre);
									string_append(&reporte,"\n" );
									informarIMONGO(tripulante->idTripulante,reporte);

									free(reporte);

									 solicitarTarea(tripulante); // REVISAR
									tripulante->estado = READY;
									informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, R);
								}

								pthread_mutex_lock(&mutexListaTrabajando);
								list_remove_by_condition(LISTA_TRABAJANDO, (void*)tieneMismoId);
								pthread_mutex_unlock(&mutexListaTrabajando);

								//sem_post(&semListaTrabajando); // cuando sale del estado EXECUTE el contador con el valor de GRADO_MULTITAREA se le hace signal
							}
						}

						if(tripulante->fueEliminado){
							tripulante->estado = EXPULSADO;
						}
						sem_post(&semExecuteBoolCicloDeReloj);

						if(PLANIFICACION_PAUSADA){
							sem_wait(&semInicioPlanificacion);
							sem_post(&semInicioPlanificacion);
						}

						//log_info(logger, "Al tripulante %d le quedan %d de quantum.", tripulante->idTripulante, quantumDisponible-1);

						if(--quantumDisponible == 0 && tripulante->estado == EXECUTE){
							sleep(RETARDO_CICLO_CPU);
							tripulante->estado = READY;
							informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, R);

							pthread_mutex_lock(&mutexListaTrabajando);
							list_remove_by_condition(LISTA_TRABAJANDO, (void*)tieneMismoId);
							pthread_mutex_unlock(&mutexListaTrabajando);
						}

						if(tripulante->estado != EXECUTE)  {		//if(tripulante->estado == EXIT || tripulante->estado == READY){
							sem_post(&semListaTrabajando);
						}

						if(tripulante->estado != EXECUTE && tripulante->estado != READY) sleep(RETARDO_CICLO_CPU);


						break;


				case BLOCK_IO:

						sleep(RETARDO_CICLO_CPU);
						if(tripulante == list_get(LISTA_BLOQUEADO_IO,0) /*&& BAJO_SABOTAJE*/){ // TODO ¿Se tendria que hacer un FIFO con los blocks? PeepoShy :3

							if( --tripulante->cicloDeRelojRestante == 0 ){

								if(tripulante->tareaARealizar.esUltimaTarea){

									tripulante->estado = EXIT;
								}
								else {


									char* reporte =  string_new();
									string_append(&reporte,"Se finaliza la tarea " );
									string_append(&reporte,tripulante->tareaARealizar.nombre);
									string_append(&reporte,"\n" );
									informarIMONGO(tripulante->idTripulante,reporte);

									free(reporte);


									solicitarTarea(tripulante);
									tripulante->estado = READY;
									informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, R);
								}

								pthread_mutex_lock(&mutexListaBloqueadoIO);
								list_remove_by_condition(LISTA_BLOQUEADO_IO, (void*)tieneMismoId);
								pthread_mutex_unlock(&mutexListaBloqueadoIO);

								//sem_post(&semListaTrabajando); // cuando sale del estado EXECUTE el contador con el valor de GRADO_MULTITAREA se le hace signal
							}

						}else{
							log_warning(logger,"El tripulante %d, se encuentra en la cola de espera del dispositivo de E/S", tripulante->idTripulante);
						}

						if(tripulante->fueEliminado){

							tripulante->estado = EXPULSADO;

							pthread_mutex_lock(&mutexListaBloqueadoIO);
							list_remove_by_condition(LISTA_BLOQUEADO_IO, (void*)tieneMismoId);
							pthread_mutex_unlock(&mutexListaBloqueadoIO);

						}

						break;


				case BLOCK_EMERGENCIA:


						sleep(RETARDO_CICLO_CPU);
						if(tripulante->fueEliminado){
							tripulante->estado = EXPULSADO;
						}
						break;


				case EXIT:
					//informarFinTripulante(tripulante ->idTripulante, tripulante ->idPatota);
					informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, F);
					log_error(logger,"El tripulante %d finalizó ya que realizó todas sus tareas",tripulante->idTripulante);
					return;

				case EXPULSADO:
					//informarFinTripulante(tripulante ->idTripulante, tripulante ->idPatota);
					informarCambioEstado(tripulante->idTripulante,tripulante->idPatota, F);
					log_error(logger,"El tripulante %d fue expulsado correctamente",tripulante->idTripulante);
					return;

					}
				}; // Fin de while
			break; // Fin de case RR
		};

		}

	else {

		logIniciacionTripulanteNegadaPorRAM(tripulante->idPatota);
		free(tripulante);

		return;
	}
}


		/*	-------------------------------------------------------------------
									TAREAS
		-------------------------------------------------------------------	*/


void solicitarTarea(infoTripulante* infoTripulante){	// TODO Solicitar la primera tarea a realizar al módulo Mi-RAM HQ.

	t_tarea aux ;

	t_tripulante unTripulante;
			unTripulante.patota = infoTripulante -> idPatota;
			unTripulante.tid = infoTripulante -> idTripulante;

			int socketRAM = conexionDiscord();

			int status = enviarMensaje(socketRAM, SOLICITAR_PROXIMA_TAREA, &unTripulante);


			if(status > 0){


				 Paquete* paqueteRecibido = recibirPaquete(socketRAM, contactoDiscordiador);
				 t_RSolicitarProximaTarea* rtaTarea = (t_RSolicitarProximaTarea*)paqueteRecibido->mensaje;



				if(paqueteRecibido->header.tipoMensaje == R_SOLICITAR_PROXIMA_TAREA){

					aux = rtaTarea->tarea;

					// log_info(logger, "tarea recibida: %s, duracion %d, ultima: %d, IO: %d , %d|%d ", rtaTarea->tarea.nombre, rtaTarea->tarea.duracion, rtaTarea->tarea.esUltimaTarea, rtaTarea->tarea.requiereIO, rtaTarea->tarea.posicionX,rtaTarea->tarea.posicionY);

					//free(rtaTarea);
				}

				free(rtaTarea);
				free(paqueteRecibido);
				close(socketRAM);

			}


	infoTripulante->cicloDeRelojRestante = aux.duracion;
	infoTripulante->tareaARealizar.duracion = aux.duracion;
	infoTripulante->tareaARealizar.esUltimaTarea = aux.esUltimaTarea;
	free(infoTripulante->tareaARealizar.nombre);
	infoTripulante->tareaARealizar.nombre = aux.nombre;
	infoTripulante->tareaARealizar.parametros = aux.parametros;
	infoTripulante->tareaARealizar.posicionX = aux.posicionX;
	infoTripulante->tareaARealizar.posicionY = aux.posicionY;
	infoTripulante->tareaARealizar.requiereIO = aux.requiereIO;

	// log_info(logger, "tarea %s, duracion %d, ultima: %d, IO: %d , %d|%d ",infoTripulante->tareaARealizar.nombre,);
	return;
}



void realizarTarea(infoTripulante* tripulante){

	//sleep(RETARDO_CICLO_CPU);
	t_tarea tarea = tripulante->tareaARealizar;

	switch(nombreTareaENUM(tarea.nombre)){

		case GENERAR_OXIGENO:

			pthread_mutex_lock(&mutexArchivoOxigeno);
			if(!existeArchivo("Oxigeno.ims"))crearArchivo("Oxigeno.ims",'O');

			llenarArchivoEnIMONGO("Oxigeno.ims", 'O', tarea.parametros);
			pthread_mutex_unlock(&mutexArchivoOxigeno);
			log_error(logger,"El tripulante %d realizó GENERAR_OXIGENO",tripulante->idTripulante);
			break;


		case CONSUMIR_OXIGENO:

			pthread_mutex_lock(&mutexArchivoOxigeno);
			//!existeArchivo("Oxigeno.ims") ? informarInexistenciaArchivo("Oxigeno.ims") : vaciarArchivoEnIMONGO("Oxigeno.ims", 'O', tarea.parametros);
			if(!existeArchivo("Oxigeno.ims")){
				informarInexistenciaArchivo("Oxigeno.ims");
			}else{
				vaciarArchivoEnIMONGO("Oxigeno.ims", 'O', tarea.parametros);
			}
			pthread_mutex_unlock(&mutexArchivoOxigeno);
			log_error(logger,"El tripulante %d realizó CONSUMIR_OXIGENO",tripulante->idTripulante);

			break;


		case GENERAR_COMIDA:

			pthread_mutex_lock(&mutexArchivoComida);
			if(!existeArchivo("Comida.ims")){ crearArchivo("Comida.ims",'C');}
			llenarArchivoEnIMONGO("Comida.ims", 'C', tarea.parametros);
			pthread_mutex_unlock(&mutexArchivoComida);
			log_error(logger,"El tripulante %d realizó GENERAR_COMIDA",tripulante->idTripulante);

			break;


		case CONSUMIR_COMIDA:

			pthread_mutex_lock(&mutexArchivoComida);
			//!existeArchivo("Comida.ims") ? informarInexistenciaArchivo("Comida.ims") : vaciarArchivoEnIMONGO("Comida.ims",'C', tarea.parametros);
			if(!existeArchivo("Comida.ims")){
				informarInexistenciaArchivo("Comida.ims");
			}else{
				vaciarArchivoEnIMONGO("Comida.ims",'C', tarea.parametros);
			}

			pthread_mutex_unlock(&mutexArchivoComida);
			log_error(logger,"El tripulante %d realizó CONSUMIR_COMIDA",tripulante->idTripulante);

			break;


		case GENERAR_BASURA:

			pthread_mutex_lock(&mutexArchivoBasura);
			if(!existeArchivo("Basura.ims")) crearArchivo("Basura.ims",'B');
			llenarArchivoEnIMONGO("Basura.ims", 'B', tarea.parametros);
			pthread_mutex_unlock(&mutexArchivoBasura);
			log_error(logger,"El tripulante %d realizó GENERAR_BASURA",tripulante->idTripulante);

			break;

		case DESCARTAR_BASURA:

			pthread_mutex_lock(&mutexArchivoBasura);
			//!existeArchivo("Basura.ims") ? informarInexistenciaArchivo("Basura.ims") : eliminarArchivoEnIMONGO("Basura.ims"); //TODO cambiar de vaciar a eliminar
			if(!existeArchivo("Basura.ims")){
				informarInexistenciaArchivo("Basura.ims");
			}else{
				eliminarArchivoEnIMONGO("Basura.ims");
			}
			pthread_mutex_unlock(&mutexArchivoBasura);
			log_error(logger,"El tripulante %d realizó DESCARTAR_BASURA",tripulante->idTripulante);

			break;

		case ACTIVAR_PROTOCOLO_FSK:

			//ENVIAR MENSAJE A IMONGO
			//SE ESPERA LA RTA DE IMONGO

			informarIMONGO(tripulante->idTripulante,"Se resuelve el sabotaje\n");
			sem_post(&semSabotajeEnCurso);

			break;




		default: // Tarea genérica
			log_error(logger,"El tripulante %d realizó una tarea genérica",tripulante->idTripulante);
			break;

		};

}



int nombreTareaENUM(char* nombre){
	if			(strcmp(nombre,"GENERAR_OXIGENO")  == 0)		{ return GENERAR_OXIGENO;
	}else if 	(strcmp(nombre,"CONSUMIR_OXIGENO") == 0)		{ return CONSUMIR_OXIGENO;
	}else if 	(strcmp(nombre,"GENERAR_COMIDA")   == 0)		{ return GENERAR_COMIDA;
	}else if 	(strcmp(nombre,"CONSUMIR_COMIDA")  == 0)		{ return CONSUMIR_COMIDA;
	}else if 	(strcmp(nombre,"GENERAR_BASURA")   == 0)		{ return GENERAR_BASURA;
	}else if 	(strcmp(nombre,"DESCARTAR_BASURA") == 0)		{ return DESCARTAR_BASURA;
	}else 														  return TAREA_GENERICA;
}




// Movimiento del Tripulante

void moverHaciaTarea(infoTripulante* tripulante) {

	//sleep(RETARDO_CICLO_CPU);

	uint32_t posicionXTripulante = tripulante->posicionX;
	uint32_t posicionYTripulante = tripulante->posicionY;

	uint32_t posicionXTarea = tripulante->tareaARealizar.posicionX;
	uint32_t posicionYTarea = tripulante->tareaARealizar.posicionY;


	if (posicionXTripulante != posicionXTarea) {

		int diferencia_en_x = posicionXTarea - posicionXTripulante;
		if (diferencia_en_x > 0) {
			tripulante->posicionX = posicionXTripulante + 1;
		} else if (diferencia_en_x < 0) {
			tripulante->posicionX = posicionXTripulante - 1;
		}

	} else if (posicionYTripulante != posicionYTarea) {

		int diferencia_en_y = posicionYTarea - posicionYTripulante;
		if (diferencia_en_y > 0) {
			tripulante->posicionY = posicionYTripulante + 1;
		} else if (diferencia_en_y < 0) {
			tripulante->posicionY = posicionYTripulante - 1;
		}
	}

	notificarMovimiento(tripulante->idTripulante,tripulante->idPatota, posicionXTripulante, posicionYTripulante, tripulante->posicionX, tripulante->posicionY);
	log_warning(logger,"El tripulante %d se mueve de (%d|%d) hacia (%d|%d)",tripulante->idTripulante,posicionXTripulante, posicionYTripulante, tripulante->posicionX, tripulante->posicionY);
}


bool llegoATarea(infoTripulante* tripulante) {

	uint32_t posicionXTripulante = tripulante->posicionX;
	uint32_t posicionYTripulante = tripulante->posicionY;

	uint32_t posicionXTarea = tripulante->tareaARealizar.posicionX;
	uint32_t posicionYTarea = tripulante->tareaARealizar.posicionY;

	bool status = (posicionXTripulante == posicionXTarea) && (posicionYTripulante == posicionYTarea);


	if(status){

		char* reporte = string_new();
		string_append(&reporte,"Comienza ejecución de tarea ");
		string_append(&reporte,tripulante->tareaARealizar.nombre);
		string_append(&reporte,"\n");


		informarIMONGO(tripulante->idTripulante,reporte);
		free(reporte);
		//free(tripulante->tareaARealizar.nombre);
	}

	return status;
}


uint32_t distanciaAPosicion( infoTripulante* tripulante ,uint32_t posX, uint32_t posY){

	uint32_t modulo(int num){
		uint32_t aux = - num;
		if (num > 0){
			return num;
		}else return aux;
	}

	uint32_t difY = modulo(posY - tripulante->posicionY);
	uint32_t difX = modulo(posX - tripulante->posicionX);

	return (difX + difY);
}




void notificarMovimiento(uint32_t idTripulante, uint32_t idPatota, uint32_t posicionXAnterior, uint32_t posicionYAnterior, uint32_t posicionX, uint32_t posicionY){



	t_iniciarTripulante tripulante; // se reutiliza la struct de otro mensaje
			tripulante.patota = idPatota;
			tripulante.tid = idTripulante;
			tripulante.posicionX = posicionX;
			tripulante.posicionY = posicionY;


	//	Avisar movimiento a MIRAMHQ

			int socketRAM = conexionDiscord();
			enviarMensaje(socketRAM, RECIBIR_LA_UBICACION_DEL_TRIPULANTE, &tripulante);

	//	Avisar movimiento a iMongo
			char reporte[64]; sprintf(reporte,"Se mueve de %d|%d a %d|%d\n", posicionXAnterior,posicionYAnterior,posicionX,posicionY);
			informarIMONGO(idTripulante,reporte);

}




/*---------- Funciones relacionadas con MIRAMHQ ----------*/

bool informarAMIRAMHQ(uint32_t idTripulante, uint32_t idPatota, uint32_t posicionX, uint32_t posicionY){

	bool success;

	t_iniciarTripulante tripulante;
			tripulante.patota = idPatota;
			tripulante.tid = idTripulante;
			tripulante.posicionX = posicionX;
			tripulante.posicionY = posicionY;

			int socketRAM = conexionDiscord();
			int status = enviarMensaje(socketRAM, INICIAR_TRIPULANTE, &tripulante);

			if(status > 0){
				Paquete* paquete_recibido = recibirPaquete(socketRAM, contactoDiscordiador);
				if(paquete_recibido->header.tipoMensaje == R_INICIAR_TRIPULANTE){

					bool* mensaje = (bool*)paquete_recibido->mensaje;
					success = *mensaje? true : false;
					char* estado = string_new();
					if(*mensaje){
						string_append(&estado,"EXITOSA");
					}else{
						string_append(&estado,"FALLIDA");
					}
					log_info(logger, "Resultado de la inicialización del tripulante %d: %s", idTripulante, estado);
					free(mensaje);
					free(estado);
					free(paquete_recibido);
				}
				close(socketRAM);
			}

	return success;
}



void informarFinTripulante(uint32_t idTripulante, uint32_t idPatota){

	t_tripulante tripulante;
	tripulante.patota = idPatota;
	tripulante.tid = idTripulante;

	int socketRAM = conexionDiscord();
	enviarMensaje(socketRAM, TERMINAR_TRIPULANTE, &tripulante);
	close(socketRAM);

	log_warning(logger,"Se informó el fin del tripulante %d de la patota %d",idTripulante, idPatota);

}



void informarCambioEstado(uint32_t idTripulante,uint32_t idPatota, t_estadoTripulante estadoTripulante){


	t_actualizarEstado* aux = malloc(sizeof(t_actualizarEstado));
	aux->estado = estadoTripulante;
	aux->tripulante.patota = idPatota;
	aux->tripulante.tid = idTripulante;


	int socketRAM = conexionDiscord();
		enviarMensaje(socketRAM, ACTUALIZAR_ESTADO, aux);
		close(socketRAM);

	free(aux);

}





/*---------- Funciones relacionadas con IMONGO ----------*/

void informarInicioTripulante(uint32_t idTripulante){
	int socketMONGO = conexionDiscordMongo();
	enviarMensaje(socketMONGO, ATENDER_TRIPULANTE, &idTripulante);
	liberarConexion(socketMONGO);
}



bool existeArchivo(char* nombreArchivo){

	int conexion = conexionDiscordMongo();
	int status = enviarMensaje(conexion,CONSULTAR_EXISTENCIA_ARCHIVO_TAREA,nombreArchivo);

	bool success;

	if(status > 0){
		Paquete* paquete_recibido = recibirPaquete(conexion, contactoDiscordiador);
		if(paquete_recibido->header.tipoMensaje == R_CONSULTAR_EXISTENCIA_ARCHIVO_TAREA){

			bool* mensaje = (bool*)paquete_recibido->mensaje;

			success = *mensaje? true : false;
			free(paquete_recibido->mensaje);
			free(paquete_recibido);
		}
		close(conexion);
	}

	return success;
}


void crearArchivo(char* nombreArchivo, char caracterDeLlenado){

	t_archivoTarea archivoACrear;
	archivoACrear.nombreArchivo = nombreArchivo;
	archivoACrear.caracterLlenado = caracterDeLlenado;
	archivoACrear.cantidad = 0;


	int conexion = crearConexion(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);
	int status = enviarMensaje(conexion,CREAR_ARCHIVO_TAREA,&archivoACrear);

	if(status > 0){
			Paquete* paquete_recibido = recibirPaquete(conexion, contactoDiscordiador);
			if(paquete_recibido->header.tipoMensaje == R_CONSULTAR_EXISTENCIA_ARCHIVO_TAREA){

				free(paquete_recibido->mensaje);
				free(paquete_recibido);
			}
			close(conexion);
		}


	liberarConexion(conexion);
}


void informarInexistenciaArchivo(char* nombreArchivo){
	log_warning(logger,"No existe el archivo %s.ims",nombreArchivo);
}


void llenarArchivoEnIMONGO(char* nombreArchivo, char caracterDeLlenado, int cantidad){

	t_archivoTarea archivoACrear;
	archivoACrear.nombreArchivo = nombreArchivo;
	archivoACrear.caracterLlenado = caracterDeLlenado;
	archivoACrear.cantidad = cantidad;

	int conexion = crearConexion(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);
	enviarMensaje(conexion,LLENADO_ARCHIVO,&archivoACrear); // Mensaje al IMONGO
	liberarConexion(conexion);

}


void vaciarArchivoEnIMONGO(char* nombreArchivo, char caracterDeVaciado, int cantidad){

	t_archivoTarea archivoAVaciar;
	archivoAVaciar.nombreArchivo = string_duplicate(nombreArchivo);
	archivoAVaciar.caracterLlenado = caracterDeVaciado;
	archivoAVaciar.cantidad = cantidad;


	int conexion = crearConexion(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);
	enviarMensaje(conexion,VACIADO_ARCHIVO,&archivoAVaciar);
	liberarConexion(conexion);
	free(archivoAVaciar.nombreArchivo);


}


void eliminarArchivoEnIMONGO(char* nombreArchivo){

	t_archivoTarea archivoACrear;
	archivoACrear.nombreArchivo = nombreArchivo;
	archivoACrear.caracterLlenado = 'X';
	archivoACrear.cantidad = -1;

	int conexion = crearConexion(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);
	enviarMensaje(conexion,ELIMINAR_ARCHIVO_TAREA,&archivoACrear);
	liberarConexion(conexion);

}

void informarIMONGO(uint32_t tid,char* reporte){

	t_consultarExistenciaArchivoTarea mensajeAEnviar;
	mensajeAEnviar.tid = tid;
	mensajeAEnviar.nombreArchivo = reporte;

	int socketMongo = conexionDiscordMongo();
	enviarMensaje(socketMongo, ACTUALIZAR_BITACORA, &mensajeAEnviar);
	close(socketMongo);

}




