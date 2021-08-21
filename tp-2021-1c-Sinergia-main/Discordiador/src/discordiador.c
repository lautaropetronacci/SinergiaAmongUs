/*
 * discordiador.c
 *
 *  Created on: 4 may. 2021
 *      Author: utnso
 */


#include "discordiador.h"


int main(void){

	inicializacion();
	pthread_create(&consola, NULL, (void*) leerConsola, NULL);
	pthread_join(consola, NULL);

	logFinDeProgramaCorrecto();

	return 1;
}


/*	-------------------------------------------------------------------
							INICIALIZACIÓN
	-------------------------------------------------------------------	*/

void inicializacion(){

	iniciarConfiguracion();
	leerConfiguracion();
	iniciarLogger();
	realizarConexiones();
	realizarEstructuras();
	crearSemaforos();

}


void realizarConexiones(){

	socketServidor = iniciarServidor(IP_ESCUCHA,PUERTO_ESCUCHA);

	// Handshake con MiRAMHQ
	enviarHankshakeAMIRAMHQ(socketServidor);

	// Handshake con iMongoStore
	enviarHankshakeAiMongoStore(socketServidor);

	// Servidor para atender mensajes de iMongoStore y MiRAMHQ
	pthread_create(&servidor, NULL, (void*)levantarServidor, (void*) socketServidor);


}

void realizarEstructuras(){

	LISTA_LISTO = list_create();
	LISTA_BLOQUEADO_IO = list_create();
	LISTA_BLOQUEADO_EMERGENCIA = list_create();
	LISTA_TRABAJANDO = list_create();
	TRIPULANTES = list_create();

	ID_PATOTA = 0;
	ID_TRIPULANTE = 1;

	BAJO_SABOTAJE = false;
}


void crearSemaforos(){

	sem_init(&semListaTrabajando, 0, GRADO_MULTITAREA ); // TODO: checkear el +1, es para que coincida con el grado de multitarea
	sem_init(&semExecuteBoolCicloDeReloj, 0, 1);
	sem_init(&sem, 0, 1);
	sem_init(&semInicioPlanificacion, 0, 0);
	sem_init(&semSabotajeEnCurso, 0, 0);

}


// Administración de los tripulantes



// Administración de las entrada/salida



// Administración de los sabotajes





		/*	-------------------------------------------------------------------
										CONSOLA
			-------------------------------------------------------------------	*/


void leerConsola(){ // Es de tipo void* y retorna NULL para que no tire un error el p_thread_creat
	char* leido;
	char** parametrosPatota;


	while(1){

		leido = readline("Ingresar comando:\n") ; // Se pide al usuario que ingrese el comando


/*---------- Iniciar Patota ----------*/

		// Ejemplo: INICIA22222222222222222222222R_PATOTA 6 /home/utnso/tp-2021-1c-Sinergia/tareas/plantas.txt 1|1 3|4

		if(string_starts_with(leido,"INICIAR_PATOTA ")){ 				// Tambien se podria usar strings_starts_with !strncmp(leido, "INICIAR_PATOTA" ,14)

			logComandoIngresado("INICIAR_PATOTA");

			char* aux = string_substring_from(leido,15);
			parametrosPatota = string_split(aux, " ");
			int cantidadTripulantes = atoi(parametrosPatota[0]);
			char* pathArchivoTareas = parametrosPatota[1];

			ID_PATOTA++;

			if(notificarPatotaAMIRAMHQ(pathArchivoTareas,ID_PATOTA)){

				int i;

				for( i=2 ; parametrosPatota[i] != NULL && i < cantidadTripulantes + 2 ; i++){

					sem_wait(&sem);
					instanciarTripulante(ID_PATOTA,obtenerPosicion(X,parametrosPatota[i]),obtenerPosicion(Y,parametrosPatota[i])); // <- Ta ido

				}

				for(  ; i < cantidadTripulantes+2; i++ ){

					sem_wait(&sem);
					instanciarTripulante(ID_PATOTA,0,0);

				}

			}

			liberarArray(parametrosPatota);
			free(aux);
		}




/*---------- Listar Tripulantes ----------*/

		// Ejemplo: LISTAR_TRIPULANTES

		else if(!strcmp(leido, "LISTAR_TRIPULANTES")){

			logComandoIngresado("LISTAR_TRIPULANTES");

			listarTripulantes();
		}




/*---------- Expulsar Tripulante ----------*/

		// Ejemplo: EXPULSAR_TRIPULANTE 5

		else if(!strncmp(leido, "EXPULSAR_TRIPULANTE " ,20)){

			logComandoIngresado("EXPULSAR_TRIPULANTE");

			char* aux = string_substring_from(leido, 20);
			uint32_t idIngresado = atoi(aux);

			expulsarTripulante(idIngresado);

			free(aux);
		}



/*---------- Iniciar Planificación ----------*/

		// Ejemplo: INICIAR_PLANIFICACION

		else if(!strcmp(leido, "INICIAR_PLANIFICACION")){

			logComandoIngresado("INICIAR_PLANIFICACION");

			inicioPlanificacion();
			//while(1){listarTripulantes(); sleep(RETARDO_CICLO_CPU);}
		}



/*---------- Pausar Planificación ----------*/

		// Ejemplo: PAUSAR_PLANIFICACION

		else if(!strcmp(leido, "PAUSAR_PLANIFICACION")){

			logComandoIngresado("PAUSAR_PLANIFICACION");

			pausarPlanificacion();
		}



/*---------- Obtener Bitácora ----------*/

		// Ejemplo: OBTENER_BITACORA 1

		else if(!strncmp(leido, "OBTENER_BITACORA " ,17)){

			logComandoIngresado("OBTENER_BITACORA");

			char* idIngresado = string_substring_from(leido, 17);

			obtenerBitacora(idIngresado); // TODO: Dani por favor hace esto peeposhy uwu
		}



/*---------- Terminar ----------*/ // Comando agregado

		// Ejemplo: TERMINAR

		else if(!strcmp(leido, "TERMINAR")){ logComandoIngresado("TERMINAR");

			int socketRAM = conexionDiscord(); enviarMensaje(socketRAM, TERMINAR , NULL);
			int socketMongo = conexionDiscordMongo(); enviarMensaje(socketMongo, TERMINAR, NULL);

			int status = pthread_detach(servidor); //retorna 0 si tod0 OK, se termina el servidor
			status == 0 ? logServidorFinalizadoCorrectamente() : logServidorFinalizadoIncorrectamente();

			liberarConexion(socketServidor);

			list_destroy(TRIPULANTES);
			list_destroy(LISTA_LISTO);
			list_destroy(LISTA_BLOQUEADO_IO);
			list_destroy(LISTA_TRABAJANDO);
			list_destroy(LISTA_BLOQUEADO_EMERGENCIA);



			free(leido);
			return;
		}


// Comando Invalido o Comando Correcto con cantidad de parametros incorrecta.

		else{
			logComandoInvalidoIngresado();
		}

		free(leido);

	}

	return;

}




		/*	-------------------------------------------------------------------
		     						Funciones
		-------------------------------------------------------------------	*/


/*---------- Funciones para comando INICIAR_PATOTA ----------*/

bool notificarPatotaAMIRAMHQ(char* pathArchivoTareas, uint32_t idPatota){

	char* aux = string_new();;
	char buffer[100];
	bool status;

	FILE* archivoTareas;
	archivoTareas = fopen(pathArchivoTareas,"r");

    if (archivoTareas == NULL){

        log_error(logger,"No se encontró el archivo de tareas");
        status = false;

    } else {

		while (fgets(buffer, sizeof(buffer), archivoTareas) != NULL){

			if(!(string_contains(buffer," "))){
					char* aux2 = string_new();
					char** tokens = string_split(buffer,";");
					string_append(&aux2,tokens[0]);
					string_append(&aux2," 0");
					string_append(&aux2,";");
					string_append(&aux2,tokens[1]);
					string_append(&aux2,";");
					string_append(&aux2,tokens[2]);
					string_append(&aux2,";");
					string_append(&aux2,tokens[3]);
					liberarArray(tokens);

					string_append(&aux,aux2);

					free(aux2);
				}
				else {
					string_append(&aux,buffer);
				}

	    }

		status = enviarContenidoArchivoTareasAMIRAMHQ(aux, idPatota); // Mensaje a MiRAM



    archivoTareas == NULL ? free(archivoTareas) : fclose(archivoTareas) ;
    }

    return status;
}



bool enviarContenidoArchivoTareasAMIRAMHQ(char* contenidoArchivo, uint32_t pid) {

    t_iniciarPatota mensajeAEnviar;
    mensajeAEnviar.pid = pid;
    mensajeAEnviar.tareas = contenidoArchivo;
    bool success;

	int socketRAM = conexionDiscord();

	int status = enviarMensaje(socketRAM, INICIAR_PATOTA, &mensajeAEnviar);

	if(status > 0){

		Paquete* paquete_recibido = recibirPaquete(socketRAM, contactoDiscordiador);
		if(paquete_recibido->header.tipoMensaje == R_INICIAR_PATOTA){

			bool* mensaje = (bool*)paquete_recibido->mensaje;
			success = *mensaje? true : false;
			char* estado  = *mensaje? "EXITOSA": "FALLIDA" ;
			log_warning(logger, "Resultado de la inicialización de la patota %d: %s", pid, estado);
			//free(estado);
		}
		free(paquete_recibido->mensaje);
		free(paquete_recibido);
		close(socketRAM);
	}

	free(contenidoArchivo);

	return success;

}



int conexionDiscord(){
	int socketMiRAM = crearConexion(IP_MI_RAM_HQ,PUERTO_MI_RAM_HQ);
	while (socketMiRAM <= 0)	socketMiRAM = crearConexion(IP_MI_RAM_HQ,PUERTO_MI_RAM_HQ);

	//check_conexion(socketMiRAM);
	return socketMiRAM;
}

int conexionDiscordMongo(){
	int socketMongo = crearConexion(IP_I_MONGO_STORE,PUERTO_I_MONGO_STORE);
	while (socketMongo <= 0) 	socketMongo = crearConexion(IP_I_MONGO_STORE,PUERTO_I_MONGO_STORE);

	//check_conexion(socketMongo);
	return socketMongo;
}

void check_conexion(int conexion){
	if (conexion)	log_info(logger, "Conexion Exitosa");
	else			perror("Conexion");
}


int obtenerPosicion(ejePosicion eje, char* posicion){
	char** listaPosiciones =  string_split(posicion,"|"); // ["123","123154",NULL] "123|123154"
	int aux = atoi(listaPosiciones[eje]);
	liberarArray(listaPosiciones);
	return aux;
}


void liberarArray(char** array) {
	for(int i = 0; i < tamanioArray(array); i++) free(array[i]);
	free(array);
}


int tamanioArray(char** array) {
	int i = 0; while(array[i]) { i++; }
	return i;
}



/*---------- Funciones para comando LISTAR_TRIPULANTES ----------*/

void listarTripulantes(){

	char* fechaYHora;

	fechaYHora = temporal_get_string_time("%d/%m/%y %H:%M:%S");;

	log_info(logger,"Estado de la Nave: %s ",fechaYHora);
	log_info(logger,"╔════════════════════════════════════════════════════════════════╗");
	log_info(logger,"║%-25s  %-20s   %-10s    ║", "  Tripulante ID", " Patota ID", " Status");
	log_info(logger,"╠════════════════════════════════════════════════════════════════╣");

	int auxTrabajando = list_size(TRIPULANTES);

	for(int i = 0; i < auxTrabajando; i++){
		//infoTripulante* tripulanteAux = malloc(sizeof(infoTripulante));
		infoTripulante* tripulanteAux = list_get(TRIPULANTES, i);
		char* idTripulanteAux =  string_itoa(tripulanteAux->idTripulante);
		char* idPatotaAux =  string_itoa(tripulanteAux->idPatota);
		char* estadoAux = estadoCHAR(tripulanteAux->estado);
		log_info(logger,"║        %-24s%-17s   %-10.16s  ║", idTripulanteAux, idPatotaAux, estadoAux);

		free(idTripulanteAux);
		free(idPatotaAux);
		//free(estadoAux);
		//free(tripulanteAux);
	}

	log_info(logger,"╚════════════════════════════════════════════════════════════════╝");

	//logTripulantesListados();
	free(fechaYHora);

}

char* estadoCHAR(estadoTripulante estado){
	switch(estado){
	case NEW: 				return "NEW"; 				break;
	case EXECUTE: 			return "EXEC"; 				break;
	case READY: 			return "READY";				break;
	case BLOCK_IO: 			return "BLOCK I/O"; 		break;
	case BLOCK_EMERGENCIA: 	return "BLOCK EMERGENCIA"; 	break;
	case EXIT: 				return "EXIT"; 				break;
	case EXPULSADO: 		return "EXIT (EXP)"; 		break;
	default: 				return "ERROR";
	}
}



/*---------- Funciones para comando EXPULSAR_TRIPULANTE ----------*/

void expulsarTripulante(uint32_t idTripulante){

	bool tieneMismoId(infoTripulante* tripulanteAux){ return tripulanteAux->idTripulante == idTripulante; }

	infoTripulante* tripulanteAux2;

	pthread_mutex_lock(&mutexListaTripulantes);
	tripulanteAux2 = list_find(TRIPULANTES, (void*)tieneMismoId); // Ver si se saca o no de la lista cuando se lo elimina
	pthread_mutex_unlock(&mutexListaTripulantes);

	if(tripulanteAux2 != NULL){
		tripulanteAux2->fueEliminado = true;

		logTripulanteExpulsado(idTripulante);
	}
	else{

		logTripulanteNOExpulsado(idTripulante);
	}

	//free(tripulanteAux2);

}



/*---------- Funciones para comando INICIAR_PLANIFICACION ----------*/

void inicioPlanificacion(){

	PLANIFICACION_PAUSADA = false;
	sem_post(&semInicioPlanificacion);
	//logPlanificacionIniciada();
}



/*---------- Funciones para comando PAUSAR_PLANIFICACION ----------*/

void pausarPlanificacion(){

	PLANIFICACION_PAUSADA = true;
	sem_wait(&semInicioPlanificacion);
	//logPlanificacionPausada();
}



/*---------- Funciones para comando OBTENER_BITACORA ----------*/

void obtenerBitacora(char* idTripulante){
	// Consulta a i-Mongo-Store (obtendrá la bitácora del tripulante pasado por parámetro a través de una consulta a i-Mongo-Store.)

	int socketMongo = conexionDiscordMongo();
	uint32_t tid = atoi(idTripulante);
	int status = enviarMensaje(socketMongo, OBTENER_BITACORA, &tid);

	char * bitacora;
	if(status > 0){

	Paquete* paquete_recibido = recibirPaquete(socketMongo, contactoDiscordiador);
		if(paquete_recibido->header.tipoMensaje == R_OBTENER_BITACORA){

			bitacora = (char*)paquete_recibido->mensaje;
			log_info(logger, "La bitácora del tripulante %d es :/n %s ", idTripulante,bitacora);

			free(paquete_recibido->mensaje);
			free(paquete_recibido);
		}
		close(socketMongo);
	}

	// logBitacoraObtenida(idTripulante);
}



	/*	-------------------------------------------------------------------
		     						SABOTAJES
		-------------------------------------------------------------------	*/

void resolverSabotaje(t_sabotaje* posicionesSabotaje, int conexion){
	uint32_t posXSabotaje = posicionesSabotaje->posicionX;
	uint32_t posYSabotaje = posicionesSabotaje->posicionY;		//TODO: testear que lleguen los valores correctos
	t_tarea tareaSabotaje;
	t_tarea tareaAnterior;

	tareaSabotaje.esUltimaTarea=false;
	// tareaSabotaje->nombre= ACTIVAR_PROTOCOLO_FSK;
	tareaSabotaje.posicionX = posXSabotaje;
	tareaSabotaje.posicionY = posYSabotaje;


	pausarPlanificacion();

	moverTripulacionEstadoBloqueado(); 	// Pasar a todos a bloq emergencia

	// reiniciarSemaforos();

										//TODO: Los tripulantes que se encuentren en la cola de BLOQ dejarán de ejecutar ciclos de E/S ya que la
										//		misma se encuentra detenida por el sabotaje

	// Enviar al tripulante

		// Encontrar el tripulante mas cercano
		infoTripulante* tripulanteDesignado = encontrarTripulanteCercanoAlSabotaje(posXSabotaje,posYSabotaje);
		tareaAnterior = tripulanteDesignado->tareaARealizar;

		// Enviarlo a hacer la tarea
		tripulanteDesignado->tareaARealizar = tareaSabotaje;


		inicioPlanificacion();
		informarIMONGO(tripulanteDesignado->idTripulante,"Se corre en pánico hacia la ubicación del sabotaje\n");
		sem_wait(&semSabotajeEnCurso);	/*mutex() -> espero a que resuelva el sabotaje, el signal me lo envia al resolver el sabotaje
										-> pauso ejecucion
										le asigno la tarea anterior y regreso todo a su estado anterior
										-> siga ejecutando*/

		enviarMensaje(conexion,COMENZAR_PROTOCOLO_FSCK,NULL);



		//Pauso la planificacion para evitar errores al mover a todos los tripulantes de las distintas listas
		pausarPlanificacion();

		tripulanteDesignado->tareaARealizar = tareaAnterior;

		sacarTripulacionEstadoBloqueado();	//TODO

		//Reanudo la planificacion
		inicioPlanificacion();

}



infoTripulante* encontrarTripulanteCercanoAlSabotaje(uint32_t posX, uint32_t posY){

	infoTripulante* tripulanteMasCercano(infoTripulante* tripulante1, infoTripulante* tripulante2){
		if(distanciaAPosicion(tripulante1,posX,posY) > distanciaAPosicion(tripulante2,posX,posY)){
			return tripulante2;
		}else return tripulante1;
	}

	infoTripulante* tripulanteDesignado;
	tripulanteDesignado = list_get_minimum(LISTA_BLOQUEADO_EMERGENCIA, (void*) tripulanteMasCercano);

	return tripulanteDesignado;

}



void moverTripulacionEstadoBloqueado(){

    bool esMinimo(infoTripulante* tripulante1, infoTripulante* tripulante2){
        if(tripulante1->idTripulante < tripulante2->idTripulante){
            return true;
        }
        return false;
    }

    t_list* listaExceYReady = list_sorted(LISTA_TRABAJANDO, (void*) esMinimo);
    list_add_all(listaExceYReady, list_sorted(LISTA_LISTO, (void*) esMinimo));

    list_clean(LISTA_TRABAJANDO);
    list_clean(LISTA_LISTO);

    list_add_all(LISTA_BLOQUEADO_EMERGENCIA, listaExceYReady);
}

void sacarTripulacionEstadoBloqueado(){
	//list_add(LISTA_LISTO, list_remove(LISTA_BLOQUEADO_EMERGENCIA, 0));

	list_add_all(LISTA_LISTO, LISTA_BLOQUEADO_EMERGENCIA);
	list_clean(LISTA_BLOQUEADO_EMERGENCIA);
}



	/*	-------------------------------------------------------------------
									MENSAJES
		-------------------------------------------------------------------	*/

/*---------- Funciones para las conexiones ----------*/


void enviarHankshakeAiMongoStore(int socketServidor) {
    int conexion = crearConexion(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);

    bool status = enviarHandshake(conexion, contactoDiscordiador, NULL);

    if(status){
        conexion = esperarCliente(socketServidor);

        Paquete* paqueteRecibido = recibirPaquete(conexion, contactoDiscordiador);
        t_handshake* handshakeRecibido = (t_handshake*)paqueteRecibido->mensaje;

        SERVIDOR_MONGO_CONECTADO = handshakeRecibido->contacto.modulo == IMONGOSTORE;

        t_contacto aux = handshakeRecibido->contacto;

        free(aux.ip);
        free(aux.puerto);
        free(handshakeRecibido->informacion);
        free(paqueteRecibido->mensaje);
        free(paqueteRecibido);
        if(SERVIDOR_MONGO_CONECTADO) {log_warning(logger,"Conexión con MONGO");}
    } else {
        log_warning(logger,"Error en la conexión con MONGO");
    }

    liberarConexion(conexion);
}

void enviarHankshakeAMIRAMHQ(int socketServidor) {
    int conexion = crearConexion(IP_MI_RAM_HQ,PUERTO_MI_RAM_HQ);

    bool status = enviarHandshake(conexion, contactoDiscordiador, NULL);

    if(status){
        conexion = esperarCliente(socketServidor);

        Paquete* paqueteRecibido = recibirPaquete(conexion, contactoDiscordiador);
        t_handshake* handshakeRecibido = (t_handshake*)paqueteRecibido->mensaje;

        SERVIDOR_MIRAM_CONECTADO = handshakeRecibido->contacto.modulo == MIRAMHQ;
        t_contacto aux = handshakeRecibido->contacto;
        free(aux.ip);
        free(aux.puerto);
        free(handshakeRecibido->informacion);
        free(paqueteRecibido->mensaje);
        free(paqueteRecibido);

        if(SERVIDOR_MIRAM_CONECTADO) {log_warning(logger,"Conexión con RAM");
        }
    } else {
    	log_warning(logger,"Error en la conexión con MiRAMHQ");
    }

    liberarConexion(conexion);
}



void levantarServidor(int socketServidor) {
	//logInicioDelServidor();
	while(1) {

        int socketPotencial = esperarCliente(socketServidor);

        //Espera que se conecte un cliente
        if(socketPotencial > 0) {

        	int* socketCliente = (int*) malloc(sizeof(int));
            *socketCliente = socketPotencial;

            pthread_create(&threadAtencion,NULL,(void*)atenderCliente,socketCliente); // Si se conecta un cliente crea un hilo que se encarga de atenderlo
            pthread_detach(threadAtencion);

            free(socketCliente);

        }

        liberarConexion(socketPotencial);
    }
}


void atenderCliente(int* socketCliente) {
    Paquete* paqueteRecibido = recibirPaquete(*socketCliente, contactoDiscordiador);

    gestionarMensaje(paqueteRecibido, *socketCliente);

    close(*socketCliente);
}


void gestionarMensaje (Paquete* paqueteRecibido, int socketCliente){
    switch(paqueteRecibido->header.tipoMensaje) {

    	case HANDSHAKE:;

            t_handshake* handshakeRecibido = (t_handshake*)paqueteRecibido->mensaje;
            log_info(logger, "Se recibió un handshake");

            break;

    	case NOTIFICAR_SABOTAJE:;

    		log_warning(logger,"Se recibió el mensaje NOTIFICAR_SABOTAJE ALERTA!!!");
            resolverSabotaje((t_sabotaje*)paqueteRecibido->mensaje, socketCliente);

            break;


        default:;
        	printf("default de gestionarMensajes");
            break;

    };
}

