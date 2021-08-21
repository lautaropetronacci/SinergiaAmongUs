/*
 * miRAMHQ.c
 *
 *  Created on: 4 may. 2021
 *      Author: utnso
 */


#include <stdio.h>
#include "miRAMHQ.h"

pthread_mutex_t mutex_Tripulantes  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_mapa = PTHREAD_MUTEX_INITIALIZER;

int main(void){
	signal(SIGUSR1, &memoriaDump);
	signal(SIGUSR2, &memoriaCompactar);

	iniciarConfiguracion();
	leerConfiguracion();
	iniciarLogger();

	if(!inicializarMemoria())
		return -1;

	//inicializarMapa();


	log_inicializacionCompleta();
	log_miRamHqReady();
	levantarServidor();


	return 1;
}


/*	-------------------------------------------------------------------
					FUNCIONES PRINCIPALES
	-------------------------------------------------------------------*/
void iniciarConfiguracion() {
	CONFIG = config_create(DIR_CONFIG);
}

void leerConfiguracion(){
	IP_ESCUCHA = config_get_string_value(CONFIG, "IP");
	PUERTO_ESCUCHA = config_get_string_value(CONFIG, "PUERTO");
	IP_DISCORDIADOR = config_get_string_value(CONFIG, "IP_DISCORDIADOR");
	PUERTO_DISCORDIADOR = config_get_string_value(CONFIG, "PUERTO_DISCORDIADOR");

	contactoMiRamHQ.ip = IP_ESCUCHA;
	contactoMiRamHQ.puerto = PUERTO_ESCUCHA;
	contactoMiRamHQ.modulo = MIRAMHQ;
}

void iniciarLogger() {
	LOGGER = log_create(DIR_LOG, "miRAMHQ", true, LOG_LEVEL_INFO);
}

bool inicializarMemoria(void){
		TAMANIO_MEMORIA = config_get_int_value(CONFIG, "TAMANIO_MEMORIA");
		ESQUEMA_MEMORIA = config_get_string_value(CONFIG, "ESQUEMA_MEMORIA");
		TAMANIO_PAGINA = config_get_int_value(CONFIG, "TAMANIO_PAGINA");
		TAMANIO_SWAP = config_get_int_value(CONFIG, "TAMANIO_SWAP");
		PATH_SWAP = config_get_string_value(CONFIG, "PATH_SWAP");
		ALGORITMO_REEMPLAZO = config_get_string_value(CONFIG, "ALGORITMO_REEMPLAZO");
		CRITERIO_REEMPLAZO = config_get_string_value(CONFIG, "CRITERIO_REEMPLAZO");
		DUMPPATH = config_get_string_value(CONFIG,"DUMP_FILE");

		bool sucess = cargarMemoria(TAMANIO_MEMORIA,
									ESQUEMA_MEMORIA,
									TAMANIO_PAGINA,
									TAMANIO_SWAP,
									PATH_SWAP,
									ALGORITMO_REEMPLAZO,
									CRITERIO_REEMPLAZO,
									DUMPPATH);
		free(ALGORITMO_REEMPLAZO);
		free(CRITERIO_REEMPLAZO);
		sem_init(&estados, 0, 1);
	return sucess;
}

void inicializarMapa(){
	pthread_create(&mapa, NULL, (void*) dibujarMapa, NULL);
	pthread_detach(mapa);
}

bool ordenar(void* socket1, void* socket2) {
	int unSocket = (int) socket1;
	int otroSocket = (int) socket2;
	return unSocket < otroSocket;
}

void levantarServidor() {
	ON = true;
	listaSockets = list_create();
	socketServidor = iniciarServidor(IP_ESCUCHA, PUERTO_ESCUCHA);
	int socketPotencial ;//= esperarCliente(socketServidor);

	//list_add_sorted(listaSockets, (void*)socketPotencial, ordenar);

	pthread_create(&sockets,NULL,(void*)admonSockets, NULL);
	pthread_detach(sockets);

	while(ON) {
    	if(ON == false) break;
        socketPotencial = esperarCliente(socketServidor);
        list_add(listaSockets, (void*)socketPotencial);
    }

}

void admonSockets(){
	while(1){
		sleep(0.3);
		if(list_size(listaSockets) > 0){
			list_sort(listaSockets, ordenar);
		   int* socketCliente = (int*) malloc(sizeof(int));
			*socketCliente = (int)list_get(listaSockets, 0);
			if(*socketCliente > 0){
				pthread_create(&AtencionClientes,NULL,(void*)atenderCLiente, socketCliente);
				pthread_detach(AtencionClientes);
			}
			list_remove(listaSockets, 0);
		}
    }
}

void atenderCLiente(int* socketCliente) {
    Paquete* paqueteRecibido = recibirPaquete(*socketCliente, contactoMiRamHQ); /*declarar contacto miranHQ en VG e inicializarla en leer CONFIG*/
    pthread_mutex_lock(&mutex_Tripulantes);
    gestionarMensaje(paqueteRecibido, *socketCliente);
    pthread_mutex_unlock(&mutex_Tripulantes);

    close(*socketCliente);
    free(socketCliente);
}

void gestionarMensaje (Paquete* paqueteRecibido, int socketCliente){
    switch(paqueteRecibido->header.tipoMensaje) {
    	case HANDSHAKE:;

    		t_handshake* handshakeRecibido = (t_handshake*)paqueteRecibido->mensaje;
    		log_msjHandshake(handshakeRecibido->contacto.modulo);
    		free(handshakeRecibido);
			break;

        case INICIAR_PATOTA:;

        	t_iniciarPatota* patota = (t_iniciarPatota*)paqueteRecibido->mensaje;
        	iniciarPatota(patota->pid, patota->tareas, socketCliente);
        	free(patota->tareas);
        	free(patota);
            break;

        case INICIAR_TRIPULANTE:;

        	t_iniciarTripulante* tripulante = (t_iniciarTripulante*)paqueteRecibido->mensaje;
            iniciarTripulante(tripulante->patota, tripulante->tid, tripulante->posicionX, tripulante->posicionY, socketCliente);
            free(tripulante);
            break;

        case RECIBIR_LA_UBICACION_DEL_TRIPULANTE:;

        	t_iniciarTripulante* ubicacionMensaje = (t_iniciarTripulante*)paqueteRecibido->mensaje;
			recibirUbicacionDelTripulante(ubicacionMensaje->patota, ubicacionMensaje->tid, ubicacionMensaje->posicionX, ubicacionMensaje->posicionY);
			free(ubicacionMensaje);
			break;

        case ACTUALIZAR_ESTADO:;

        	t_actualizarEstado* mensaje = (t_actualizarEstado*)paqueteRecibido->mensaje;
        	actualizarEstado(mensaje->tripulante.patota, mensaje->tripulante.tid, mensaje->estado);
        	free(mensaje);
        	break;

        case SOLICITAR_PROXIMA_TAREA:;
        	t_tripulante* mensajeTarea = (t_tripulante*)paqueteRecibido->mensaje;
			solicitarProximaTarea(mensajeTarea->patota, mensajeTarea->tid, socketCliente);
			free(mensajeTarea);
			break;

        case EXPULSAR_TRIPULANTE:;
        	t_tripulante* tripulanteAexpulsar = (t_tripulante*)paqueteRecibido->mensaje;
        	expulsarTripulante(tripulanteAexpulsar->patota, tripulanteAexpulsar->tid);
        	free(tripulanteAexpulsar);
        	break;

        case TERMINAR:
        	terminarmodulo();
        	break;
        default:;
        	log_error(LOGGER, "mensaje no reconocido");
            break;
    }
    free(paqueteRecibido);
}


void gestionarHandshake(int modulo){}


void iniciarPatota(int pid, char* tareas, int socketCliente){
	bool success;

	if(DISCORDIADOR_CONECTADO){
		success = crearPatota(pid, tareas);
		log_patotaIniciada(pid, success);
	}
	enviarMensaje(socketCliente, R_INICIAR_PATOTA, &success);

	if(strcmp(ESQUEMA_MEMORIA, "PAGINACION") == 0){
		imprimirLista(TABLAdePAGINAS);	//TODO:BORRAR
	}
	else{
		imprimirLista(ALL_PARTITIONS);	//TODO: BORRAR
		printf("\n");
	}
	memoriaDump(1); //TODO:BORRAR
}


void iniciarTripulante(int patota, int tid, int posicionX, int posicionY, int socketCliente){
	bool success;

	if(DISCORDIADOR_CONECTADO){
		success = crearTripulante(patota, tid, posicionX, posicionY);

		log_tripulanteIniciado(tid, success);
	}
	pthread_mutex_lock(&mutex_mapa);
	//agregarTripulante(tid, posicionX, posicionY);
	pthread_mutex_unlock(&mutex_mapa);

	enviarMensaje(socketCliente, R_INICIAR_TRIPULANTE, &success);

	if(strcmp(ESQUEMA_MEMORIA, "PAGINACION") == 0){
		imprimirLista(TABLAdePAGINAS);	//TODO:BORRAR
	}
	else{
		imprimirLista(ALL_PARTITIONS);	//TODO: BORRAR
		printf("\n");
	}
}


void recibirUbicacionDelTripulante(int patota, int tid, int posicionX, int posicionY){
	if(DISCORDIADOR_CONECTADO){
		actualizarUbicacionTripulante(patota, tid, posicionX, posicionY);
	}
	pthread_mutex_lock(&mutex_mapa);
	//actualizarTripulante(tid, posicionX, posicionY);
	pthread_mutex_unlock(&mutex_mapa);
}


void actualizarEstado(int patota, int tid, int estado){
	if(DISCORDIADOR_CONECTADO){
		char* nuevoEstado(int status){
				switch (status){
					case N:
						return "NEW";
						break;
					case R:
						return "READY";
						break;
					case E:
						return "EXECUTE";
						break;
					case B:
						return "BLOCK";
						break;
					case F:
						return "EXIT";
						break;
				}
				return "ERROR";
			}

			char* estado1 = nuevoEstado(estado);
		log_error(LOGGER, "estado nuevo %s", estado1);
		actualizarEstadoTripulante(patota, tid, estado);
	}
}


void solicitarProximaTarea(int patota, int tid, int socketCliente){
	if(DISCORDIADOR_CONECTADO){
		t_RSolicitarProximaTarea proximaTarea = proximaTareaTripulante(patota, tid);

		enviarMensaje(socketCliente, R_SOLICITAR_PROXIMA_TAREA, &proximaTarea);
		log_tareaGeneradaHIT(tid);
		free(proximaTarea.tarea.nombre);
	}
}


void expulsarTripulante(int patota, int tid){
	if(DISCORDIADOR_CONECTADO)
		eliminarTripulante(patota, tid);

	pthread_mutex_lock(&mutex_mapa);
	//borrarTripulante(tripulanteAexpulsar->tid);
	pthread_mutex_unlock(&mutex_mapa);

	if(strcmp(ESQUEMA_MEMORIA, "PAGINACION") == 0){
		imprimirLista(TABLAdePAGINAS);	//TODO:BORRAR
	}
	else{
		imprimirLista(ALL_PARTITIONS);	//TODO: BORRAR
		printf("\n");
	}
}


void terminarmodulo(){
	if(strcmp(ESQUEMA_MEMORIA, "PAGINACION") == 0){
		imprimirLista(TABLAdePAGINAS);	//TODO:BORRAR
	}
	else{
		imprimirLista(ALL_PARTITIONS);	//TODO: BORRAR
		printf("\n");
	}
	compactarMemoria(1);//TODO:BORRAR
	if(strcmp(ESQUEMA_MEMORIA, "PAGINACION") == 0){
		imprimirLista(TABLAdePAGINAS);	//TODO:BORRAR
	}
	else{
		imprimirLista(ALL_PARTITIONS);	//TODO: BORRAR
		printf("\n");
	}



	/*switch(ESQUEMA_MEM){
		case PAGINACION:
			list_destroy(TABLAdePAGINAS);
			if(ALGORITMO_REEM == LRU){
				list_destroy(PAGINASenMEMORIA);
			}
			else
				free(PUNTERO);
			break;

		case SEGMENTACION:
			list_destroy(TABLAdeSEGMENTOS);
			list_destroy(FREE_PARTITIONS);
			list_destroy(OCCUPIED_PARTITIONS);
			list_destroy(ALL_PARTITIONS);*/
			//list_clean_and_destroy_elements();
			list_destroy(backupListOCCUPIED);
			//break;
	//}
	liberarConexion(socketServidor);
	ON = false;
	//list_destroy(tripulantesEnMapa);
    log_warning(LOGGER, "Finalizacion EXITOSA del modulo miRAMHQ (ᵔᴥᵔ)");
	terminarPrograma();
}

void terminarPrograma() {
	log_destroy(LOGGER);
	config_destroy(CONFIG);
}

