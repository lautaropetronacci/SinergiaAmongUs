/*
 * iMongoStore.c
 *
 *  Created on: 4 may. 2021
 *      Author: utnso
 // preguntar tamaño de bloques del .config
*/

//TODO: cambiar modalidad de SuperBloque, files y blocks a modo variable global y que esten abiertos mientras corra el programa
//y cuando finalize libere toda la memoria solicitada.

#include "iMongoStore.h"

pthread_mutex_t mutex_Tripulantes  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ModificarBlocks  = PTHREAD_MUTEX_INITIALIZER;

int main(void){

	inicializacion();


	signal(SIGUSR1, &enviarAvisoDeSabotaje);

	realizarConexiones();

	return 23;
}


void inicializacion(){

	iniciarConfiguracion();
	leerConfiguracion();
	iniciarLogger();
	realizarEstructuras();
	crearArchivos();
	printf("hola");
	//log_info(logger, "PRUEBA");
	pthread_create(&actualizarBloques, NULL, (void*)sincronizarBlocks, NULL);

	//crearSemaforos();

}

void iniciarConfiguracion() {
	config = config_create(DIR_CONFIG);
}


void leerConfiguracion(){

	PUNTO_MONTAJE = config_get_string_value(config, "PUNTO_MONTAJE");
	TIEMPO_SINCRONIZACION = config_get_int_value(config, "TIEMPO_SINCRONIZACION");
	POSICIONES_SABOTAJE = config_get_array_value(config, "POSICIONES_SABOTAJE");
	IP_ESCUCHA = config_get_string_value(config, "IP");
	PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO");
	IP_DISCORDIADOR = config_get_string_value(config, "IP_DISCORDIADOR");
	PUERTO_DISCORDIADOR = config_get_string_value(config, "PUERTO_DISCORDIADOR");
	TAMANIO_BLOQUES = config_get_int_value(config, "TAMANIO_BLOQUES");
	CANTIDAD_BLOQUES = config_get_int_value(config, "CANTIDAD_BLOQUES");
}


void iniciarLogger() {
	logger = log_create(DIR_LOG, "iMongoStore", true, LOG_LEVEL_INFO);
	
}

void realizarEstructuras(){
	contactoIMongoStore.ip = IP_ESCUCHA;
	contactoIMongoStore.puerto = PUERTO_ESCUCHA;
	contactoIMongoStore.modulo = IMONGOSTORE;

	ADDRBLOCKS = devolverPathDirecto("/Blocks.ims");
	ADDRSUPERBLOCKS = devolverPathDirecto("/SuperBloque.ims");

	sabotajesRecibidos = 0;

}


void realizarConexiones(){

    int socketServidor = iniciarServidor(IP_ESCUCHA,PUERTO_ESCUCHA);
	log_info(logger, "El modulo MONGO ya esta listo para recibir mensajes");
    while(1) {

        int socketPotencial = esperarCliente(socketServidor);
        if(socketPotencial > 0) {
            int* socketCliente = (int*) malloc(sizeof(int));
            *socketCliente = socketPotencial;
            pthread_create(&AtencionDiscordiador,NULL,(void*)atenderCliente,socketCliente);
            pthread_detach(AtencionDiscordiador);
        }
    }
    liberarConexion(socketServidor);
}



void atenderCliente(int* socketCliente) {
    Paquete* paqueteRecibido = recibirPaquete(*socketCliente, contactoIMongoStore); /*declarar contacto miranHQ en VG e inicializarla en leer CONFIG*/
    pthread_mutex_lock(&mutex_Tripulantes);
    gestionarMensaje(paqueteRecibido, *socketCliente);
    pthread_mutex_unlock(&mutex_Tripulantes);
    close(*socketCliente);
    free(socketCliente);
}



void gestionarMensaje(Paquete* paqueteRecibido, int conexion){
				//esta aca porque no la podemos declarar dentro del segundo case del switch
				// log_info(logger, "entro a gestionarMensaje");

	switch(paqueteRecibido->header.tipoMensaje){
        case HANDSHAKE:;
            // gestionarHandshake(paqueteRecibido);
        	log_info(logger, "Handshake recibido del discordiador");

        	//t_handshake* handshakeRecibido = (t_handshake*)paqueteRecibido->mensaje;
        	//free(handshakeRecibido->informacion);
        	//free(handshakeRecibido);
            break;

        case ATENDER_TRIPULANTE:;
        	uint32_t* tid = (uint32_t*)paqueteRecibido->mensaje;
        	crearBitacora(*tid);
        	log_warning(logger, "SE DEBE CREAR LA BITACORA DEL TRIPULANTE %d ",*tid);
        	free(tid);

        	break;


        case CONSULTAR_EXISTENCIA_ARCHIVO_TAREA:;
        	char* nombreArchivo = (char*)paqueteRecibido->mensaje;
        	log_warning(logger, "Consulta tarea nombre %s",nombreArchivo);

        	char * pathCONSULTAR_EXISTENCIA_ARCHIVO_TAREA = devolverPathArchivoTarea(nombreArchivo);
        	bool existeArchivo = consultarExistenciaArchivo(pathCONSULTAR_EXISTENCIA_ARCHIVO_TAREA);
        	free(pathCONSULTAR_EXISTENCIA_ARCHIVO_TAREA);

        	enviarMensaje(conexion,R_CONSULTAR_EXISTENCIA_ARCHIVO_TAREA,&existeArchivo);

        	free(nombreArchivo);/////////////////////////////////////////
        	break;

        case CREAR_ARCHIVO_TAREA:;
        	//esperando tener el struct t_archivoTarea de la branch discordiador

        	t_archivoTarea* tipoArchivo = (t_archivoTarea*)paqueteRecibido->mensaje;
        	log_warning(logger, "CREAR ARCHIVO %s , cantidad %d , caracterLlenado: %c",tipoArchivo->nombreArchivo, tipoArchivo->cantidad, tipoArchivo->caracterLlenado);

        	generarArchivoDeCero(tipoArchivo);

        	bool existeArchivo2 = true;
        	enviarMensaje(conexion,R_CONSULTAR_EXISTENCIA_ARCHIVO_TAREA,&existeArchivo2);

        	free(tipoArchivo->nombreArchivo);
        	free(tipoArchivo);

        	break;

        case LLENADO_ARCHIVO:;
        	t_archivoTarea* datosArchivo = (t_archivoTarea*)paqueteRecibido->mensaje;
        	log_warning(logger, "LLENADO ARCHIVO %s , cantidad %d , caracterLlenado: %c",datosArchivo->nombreArchivo,datosArchivo->cantidad, datosArchivo->caracterLlenado);

        	char* nombreArchivoALlenar = datosArchivo->nombreArchivo;
			int cantidad = datosArchivo->cantidad;
			char caracterLlenado = datosArchivo->caracterLlenado;

			agregarCaracter(nombreArchivoALlenar, cantidad, caracterLlenado);

			free(datosArchivo->nombreArchivo);
			free(datosArchivo);
        	break;


        case VACIADO_ARCHIVO:;
    		t_archivoTarea* datosArchivoV = (t_archivoTarea*)paqueteRecibido->mensaje;
    		log_warning(logger, "VACIADO ARCHIVO %s , cantidad %d , caracterLlenado: %c",datosArchivoV->nombreArchivo,datosArchivoV->cantidad, datosArchivoV->caracterLlenado);

    		char* nombreArchivoAVaciar = datosArchivoV->nombreArchivo;
			int cantidadAVaciar = datosArchivoV->cantidad;
			//char caracterVaciado = datosArchivoV->caracterLlenado;

			borrarCaracteres(nombreArchivoAVaciar, cantidadAVaciar);

			free(datosArchivoV->nombreArchivo);
			free(datosArchivoV);

        	break;

        case ELIMINAR_ARCHIVO_TAREA:;
    		t_archivoTarea* datosArchivoEliminar = (t_archivoTarea*)paqueteRecibido->mensaje;
    		log_error(logger, "ELIMINAR ARCHIVO %s , cantidad %d , caracterLlenado: %c",datosArchivoEliminar->nombreArchivo,datosArchivoEliminar->cantidad, datosArchivoEliminar->caracterLlenado);
    		descartarBasura();
    		free(datosArchivoEliminar->nombreArchivo);
    		free(datosArchivoEliminar);
        	break;

        case PERMITIR_LECTURAS:;
        	break;

        case PERMITIR_ESCRITURAS_CONCURRENTES:;
        	break;

        case ACTUALIZAR_BITACORA:;
        	t_actualizarBitacora* mensajeBitacora = (t_actualizarBitacora*)paqueteRecibido->mensaje;

        	pthread_mutex_lock(&mutex_ModificarBlocks);
        	agregarAccionesTripulanteABlocks(mensajeBitacora->reporte,mensajeBitacora->tid);
        	pthread_mutex_unlock(&mutex_ModificarBlocks);

        	log_warning(logger,"Actualizar Bitacora Tripulante %d : ''%s''", mensajeBitacora->tid, mensajeBitacora->reporte);

        	free(mensajeBitacora->reporte);
        	free(mensajeBitacora);
        	break;

        case COMENZAR_PROTOCOLO_FSCK:;
        	chequeoSabotajes();
        	break;

        case OBTENER_BITACORA:;
        	uint32_t* idTripulante = (uint32_t*)paqueteRecibido->mensaje;

        	char * bitacora = devolverBitacora(*idTripulante);

        	enviarMensaje(conexion, R_OBTENER_BITACORA, bitacora);

        	free(bitacora);
        	free(idTripulante);

        	break;

        case TERMINAR:;
        	log_info(logger,"terminar");

        	break;

    default:
        log_error(logger,"DEFAULT EN MONGO");
            break;
    }

	free(paqueteRecibido);
}

void gestionarHandshake(Paquete* paqueteRecibido){ 						//terminar
	t_handshake* handshakeRecibido = (t_handshake*)paqueteRecibido->mensaje;
	uint32_t patota = (uint32_t)handshakeRecibido->informacion;
	logConexion(patota);
}

void atenderDiscordiador(int* socketCliente) {
    Paquete* paqueteRecibido = recibirPaquete(*socketCliente, contactoIMongoStore);
    gestionarMensaje(paqueteRecibido, *socketCliente);
    close(*socketCliente);
}

