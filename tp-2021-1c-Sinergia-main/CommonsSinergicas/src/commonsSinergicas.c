/*
 * commonsSinergicas.c
 *
 *  Created on: 2 may. 2021
 *      Author: utnso
 */


#include "commonsSinergicas.h"


int crearConexion(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		socket_cliente = -1;

	freeaddrinfo(server_info);

	return socket_cliente;
}

int iniciarServidor(char *ip, char* puerto) {
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int esperarCliente(int socket_servidor) {
	struct sockaddr_in dir_cliente;

	socklen_t tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	return socket_cliente;
}

void liberarConexion(int socket) {
	close(socket);
}

bool enviarPaquete(int socketCliente, Paquete* paquete) {
	int cantAEnviar = sizeof(Header) + paquete->header.tamanioMensaje;
	void* datos = malloc(cantAEnviar);
	int offset = 0;

	serializarVariable(datos, &(paquete->header), sizeof(Header), &offset);

	if(paquete->header.tamanioMensaje > 0){
		serializarVariable(datos, (paquete->mensaje), paquete->header.tamanioMensaje, &offset);
	}
	int bytesEnviados = 0;
	int totalEnviado = 0;
	bool valorRetorno=true;

	do {
		bytesEnviados = send(socketCliente, datos + totalEnviado, cantAEnviar - totalEnviado, 0);
		totalEnviado += bytesEnviados;
		if(bytesEnviados==-1){
			valorRetorno=false;
			break;
		}
	} while (totalEnviado != cantAEnviar);
	free(datos);
	return valorRetorno;
}

bool enviarMensaje(int socketFD, codigoMensaje tipoMensaje, void* estructura){

	Paquete* paquete = malloc(sizeof(Paquete));
	paquete->header.tipoMensaje = tipoMensaje;
	bool valorRetorno;
	int tamDatos = 0;


	void* datos = serializarMensaje(estructura, tipoMensaje, &tamDatos);

	paquete->header.tamanioMensaje = tamDatos;
	paquete->mensaje=datos;

	valorRetorno=enviarPaquete(socketFD, paquete);

	free(paquete->mensaje);
	free(paquete);
	//free(estructura);
	return valorRetorno;
}

bool enviarHandshake(int socketFD, t_contacto modulo, void* mensaje){
	t_handshake informacionHandshake;
	informacionHandshake.contacto = modulo;
	informacionHandshake.informacion = mensaje;
	informacionHandshake.deboResponder = true;
	bool valorRetorno = enviarMensaje(socketFD, HANDSHAKE, &informacionHandshake);
	return valorRetorno;
}


void serializarVariable(void* AEnviar, void* ASerializar, int tamanio, int *offset) {
	memcpy(AEnviar + *offset, ASerializar, tamanio);
	*offset += tamanio;
}


void* serializarMensaje(void* estructura, codigoMensaje codigo, int* tamanio){

	void* mensajeSerializado;
	uint32_t tamanio1, tamanio2;
	int offset = 0;

	switch(codigo) {
		case HANDSHAKE:; //OK

			t_handshake* handshake = estructura;

			tamanio1 = strlen(handshake->contacto.ip)+1;
			tamanio2 = strlen(handshake->contacto.puerto)+1;

			*tamanio +=	  sizeof(handshake->deboResponder)
						+ sizeof(handshake->contacto.modulo)
						+ sizeof(uint32_t)*2
						+ tamanio1
						+ tamanio2;

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, &(handshake->contacto.modulo), sizeof(handshake->contacto.modulo), &offset);
			serializarChar(mensajeSerializado, handshake->contacto.ip, tamanio1, &offset);
			serializarChar(mensajeSerializado, handshake->contacto.puerto, tamanio2, &offset);
			serializarVariable(mensajeSerializado, &(handshake->deboResponder), sizeof(handshake->deboResponder), &offset);
			break;

		case ACTUALIZAR_BITACORA:;

			t_actualizarBitacora* infoBitacora = estructura;

			tamanio1 = strlen(infoBitacora->reporte)+1;
			*tamanio += sizeof(uint32_t) + tamanio1 + sizeof(tamanio1);

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, &(infoBitacora->tid), sizeof(infoBitacora->tid), &offset);
			serializarChar(mensajeSerializado, infoBitacora->reporte, tamanio1, &offset);
			break;

		case R_OBTENER_BITACORA:
		case CONSULTAR_EXISTENCIA_ARCHIVO_TAREA:;

			char* existenciaArchivo = estructura;

			tamanio1 = strlen(existenciaArchivo)+1;
			*tamanio = tamanio1 + sizeof(tamanio1);

			mensajeSerializado = malloc(*tamanio);
			serializarChar(mensajeSerializado, existenciaArchivo, tamanio1, &offset);

		/*	t_consultarExistenciaArchivo* existenciaArchivo = estructura;
			tamanio1 = strlen(existenciaArchivo->nombreArchivo)+1;
			*tamanio = tamanio1 + sizeof(uint32_t);
			mensajeSerializado = malloc(*tamanio);
			serializarChar(mensajeSerializado, existenciaArchivo->nombreArchivo, tamanio1, &offset);
*/
			break;

		case CREAR_ARCHIVO_TAREA:
		case LLENADO_ARCHIVO:
		case ELIMINAR_ARCHIVO_TAREA:
		case VACIADO_ARCHIVO:;

			t_archivoTarea* archivoTarea = estructura;

			tamanio1 = strlen(archivoTarea->nombreArchivo)+1;
			*tamanio += sizeof(uint32_t) + tamanio1 + sizeof(char) +  sizeof(int);

			mensajeSerializado = malloc(*tamanio);

			serializarChar(mensajeSerializado, archivoTarea->nombreArchivo, tamanio1, &offset);
			serializarVariable(mensajeSerializado, &(archivoTarea->caracterLlenado), sizeof(archivoTarea->caracterLlenado), &offset);
			serializarVariable(mensajeSerializado, &(archivoTarea->cantidad), sizeof(archivoTarea->cantidad), &offset);
			break;

		case NOTIFICAR_SABOTAJE:;

			t_sabotaje* posiciones = estructura;

			*tamanio += sizeof(uint32_t) * 2;

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, &(posiciones->posicionX), sizeof(posiciones->posicionX), &offset);
			serializarVariable(mensajeSerializado, &(posiciones->posicionY), sizeof(posiciones->posicionY), &offset);

			break;

		case INICIAR_PATOTA:;

			t_iniciarPatota* infoPatota = estructura;

			tamanio1 = strlen(infoPatota->tareas)+1;
			*tamanio += sizeof(uint32_t) + tamanio1 + sizeof(tamanio1);

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, &(infoPatota->pid), sizeof(infoPatota->pid), &offset);
			serializarChar(mensajeSerializado, infoPatota->tareas, tamanio1, &offset);
			break;

		case INICIAR_TRIPULANTE:
		case RECIBIR_LA_UBICACION_DEL_TRIPULANTE:;

			t_iniciarTripulante* infoTripulante = estructura;

			*tamanio += sizeof(uint32_t) * 4;

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, &(infoTripulante->patota), sizeof(infoTripulante->patota), &offset);
			serializarVariable(mensajeSerializado, &(infoTripulante->tid), sizeof(infoTripulante->tid), &offset);
			serializarVariable(mensajeSerializado, &(infoTripulante->posicionX), sizeof(infoTripulante->posicionX), &offset);
			serializarVariable(mensajeSerializado, &(infoTripulante->posicionY), sizeof(infoTripulante->posicionY), &offset);
			break;

		case OBTENER_BITACORA:
		case ATENDER_TRIPULANTE:;

			uint32_t* tid = estructura;

			*tamanio += sizeof(uint32_t);

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, tid, sizeof(tid), &offset);
			break;

		case SOLICITAR_PROXIMA_TAREA:
		case EXPULSAR_TRIPULANTE:
		case TERMINAR_TRIPULANTE:;	//TO TEST

			t_tripulante* tripulante = estructura;

			*tamanio += sizeof(uint32_t) * 2;

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, &(tripulante->patota), sizeof(tripulante->patota), &offset);
			serializarVariable(mensajeSerializado, &(tripulante->tid), sizeof(tripulante->tid), &offset);
			break;

		case R_CONSULTAR_EXISTENCIA_ARCHIVO_TAREA:
		case R_INICIAR_PATOTA:
		case R_INICIAR_TRIPULANTE:
		case R_RECIBIR_LA_UBICACION_DEL_TRIPULANTE:
		case R_EXPULSAR_TRIPULANTE:
		case R_TERMINAR_TRIPULANTE:;//TO TEST

			bool* operacionExitosa = estructura;

			*tamanio += sizeof(operacionExitosa);

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, operacionExitosa, sizeof(operacionExitosa), &offset);
			break;

		case R_SOLICITAR_PROXIMA_TAREA:;	//TO TEST

			t_RSolicitarProximaTarea* proximaTarea = estructura;

			tamanio1 = strlen(proximaTarea->tarea.nombre) + 1;

			*tamanio += sizeof(uint32_t) * 6
						+ tamanio1
						+ sizeof(proximaTarea->tarea.esUltimaTarea) * 2;

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, &(proximaTarea->tid), sizeof(proximaTarea->tid), &offset);
			serializarChar(mensajeSerializado, proximaTarea->tarea.nombre, strlen(proximaTarea->tarea.nombre) + 1, &offset);
			serializarVariable(mensajeSerializado, &(proximaTarea->tarea.parametros), sizeof(proximaTarea->tarea.parametros), &offset);
			serializarVariable(mensajeSerializado, &(proximaTarea->tarea.posicionX), sizeof(proximaTarea->tarea.posicionX), &offset);
			serializarVariable(mensajeSerializado, &(proximaTarea->tarea.posicionY), sizeof(proximaTarea->tarea.posicionY), &offset);
			serializarVariable(mensajeSerializado, &(proximaTarea->tarea.duracion), sizeof(proximaTarea->tarea.duracion), &offset);
			serializarVariable(mensajeSerializado, &(proximaTarea->tarea.esUltimaTarea), sizeof(proximaTarea->tarea.esUltimaTarea), &offset);
			serializarVariable(mensajeSerializado, &(proximaTarea->tarea.requiereIO), sizeof(proximaTarea->tarea.requiereIO), &offset);
			break;

		case TERMINAR:;

			*tamanio = 0;
			mensajeSerializado = NULL;
			break;

		/*case R_OBTENER_BITACORA:;

			t_RObtenerBitacora* rObtenerBitacora = estructura;

			tamanio1 = strlen(rObtenerBitacora->bitacora)+1;
			*tamanio += sizeof(uint32_t) + tamanio1 + sizeof(tamanio1);

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, &(rObtenerBitacora->tid), sizeof(rObtenerBitacora->tid), &offset);
			serializarChar(mensajeSerializado, rObtenerBitacora->bitacora, tamanio1, &offset);
			break;*/

		case ACTUALIZAR_ESTADO:;

			t_actualizarEstado* mensaje = estructura;

			*tamanio += sizeof(uint32_t) * 3;

			mensajeSerializado = malloc(*tamanio);

			serializarVariable(mensajeSerializado, &(mensaje->tripulante.patota), sizeof(mensaje->tripulante.patota), &offset);
			serializarVariable(mensajeSerializado, &(mensaje->tripulante.tid), sizeof(mensaje->tripulante.tid), &offset);
			serializarVariable(mensajeSerializado, &(mensaje->estado), sizeof(mensaje->estado), &offset);


			//free(mensaje);


			break;

		default:; //fprintf(stderr, "Error: no se serializó el mensaje ya que no entró en algun CASE en serializarMensaje(En commonsSinergicas.h), Cariños... Dani y Maxi <3 peepoShy\n");
	}

	return mensajeSerializado;
}


void serializarChar(void* mensajeSerializado, char* mensaje, uint32_t tamanio, int* offset){
	serializarVariable(mensajeSerializado, &(tamanio), sizeof(tamanio), offset);
	serializarVariable(mensajeSerializado, mensaje, tamanio, offset);
}


//Recibir datos
int recibirMensaje(void* paquete, int socketFD, uint32_t cantARecibir) {
	void* datos = malloc(cantARecibir);
	int datosRecibidos = 0;
	int totalRecibido = 0;

	do {
		datosRecibidos = recv(socketFD, datos + totalRecibido, cantARecibir - totalRecibido, 0);
		totalRecibido += datosRecibidos;
	} while (totalRecibido != cantARecibir && datosRecibidos > 0);

	memcpy(paquete, datos, cantARecibir);
	free(datos);

	if (datosRecibidos < 0) {
		printf("Cliente Desconectado\n");
		close(socketFD); // Hasta aquí llegamos :)
	}
	else if (datosRecibidos == 0) {
		printf("Fin de Conexion en socket %d\n", socketFD);
		close(socketFD); // Hasta aquí llegamos :)
	}

	return datosRecibidos;
}

Paquete* recibirPaquete(int socketFD, t_contacto contacto) {
	Paquete* paquete = malloc(sizeof(Paquete));
	void* mensajeRecibido;

	recibirMensaje(&(paquete->header), socketFD, sizeof(Header));

	if (paquete->header.tamanioMensaje == 0){
		paquete->mensaje = NULL;
		return paquete;
	}

	mensajeRecibido = malloc(paquete->header.tamanioMensaje);
	recibirMensaje(mensajeRecibido, socketFD, paquete->header.tamanioMensaje);
	deserializarMensaje(mensajeRecibido, paquete, contacto);
	free(mensajeRecibido); //TODO: revisar
	return paquete;
}


// Deserializacion
void deserializarMensaje(void* mensajeRecibido, Paquete* paqueteRecibido, t_contacto contacto){
	int offset = 0;

	switch(paqueteRecibido->header.tipoMensaje){
		case HANDSHAKE: ;
			t_handshake* handshakeRecibido = malloc(sizeof(*handshakeRecibido));

			copiarVariable(&(handshakeRecibido->contacto.modulo), mensajeRecibido, &offset, sizeof(handshakeRecibido->contacto.modulo));
			deserializarChar(mensajeRecibido, &offset, &(handshakeRecibido->contacto.ip));
			deserializarChar(mensajeRecibido, &offset, &(handshakeRecibido->contacto.puerto));
			copiarVariable(&(handshakeRecibido->deboResponder), mensajeRecibido, &offset, sizeof(handshakeRecibido->deboResponder));
			handshakeRecibido->informacion = NULL;

			if (handshakeRecibido->deboResponder){
				t_handshake respuesta;
				respuesta.contacto = contacto;
				respuesta.deboResponder = false;
				respuesta.informacion = NULL;
				int cliente = crearConexion(handshakeRecibido->contacto.ip, handshakeRecibido->contacto.puerto);
				enviarMensaje(cliente, HANDSHAKE, &respuesta);

				/*TODO:
				  switch(handshakeRecibido->contacto.modulo){
					case TRIPULANTE: ;

					default : break;
				}*/

				liberarConexion(cliente);
			} else {
				printf("\nConexion exitosa\n");
			}

			paqueteRecibido->mensaje = handshakeRecibido;
			//free(handshakeRecibido);
			break;

		case INICIAR_PATOTA:;	//OK

			t_iniciarPatota* patotaAguardar = malloc(sizeof(*patotaAguardar));

			copiarVariable(&(patotaAguardar->pid), mensajeRecibido, &offset, sizeof(patotaAguardar->pid));
			deserializarChar(mensajeRecibido, &offset, &(patotaAguardar->tareas));

			paqueteRecibido->mensaje = patotaAguardar;
			//free(patotaAguardar);
			break;

		case INICIAR_TRIPULANTE:
		case RECIBIR_LA_UBICACION_DEL_TRIPULANTE:;

			t_iniciarTripulante* infoTripulante = malloc(sizeof(*infoTripulante));

			copiarVariable(&(infoTripulante->patota), mensajeRecibido, &offset, sizeof(infoTripulante->patota));
			copiarVariable(&(infoTripulante->tid), mensajeRecibido, &offset, sizeof(infoTripulante->tid));
			copiarVariable(&(infoTripulante->posicionX), mensajeRecibido, &offset, sizeof(infoTripulante->posicionX));
			copiarVariable(&(infoTripulante->posicionY), mensajeRecibido, &offset, sizeof(infoTripulante->posicionY));

			paqueteRecibido->mensaje = infoTripulante;
			//free(infoTripulante);
			break;

		case OBTENER_BITACORA:
		case ATENDER_TRIPULANTE:;

			uint32_t* tid = malloc(sizeof(uint32_t));

			copiarVariable(tid, mensajeRecibido, &offset, sizeof(uint32_t));

			paqueteRecibido->mensaje = tid;
			//free(tid);
			break;

		case SOLICITAR_PROXIMA_TAREA:
		case EXPULSAR_TRIPULANTE:
		case TERMINAR_TRIPULANTE:;	//TO TEST

			t_tripulante* tripulante = malloc(sizeof(*tripulante));

			copiarVariable(&(tripulante->patota), mensajeRecibido, &offset, sizeof(tripulante->patota));
			copiarVariable(&(tripulante->tid), mensajeRecibido, &offset, sizeof(tripulante->tid));

			paqueteRecibido->mensaje = tripulante;
			//free(tripulante);
			break;

		case R_CONSULTAR_EXISTENCIA_ARCHIVO_TAREA:
		case R_INICIAR_PATOTA:
		case R_INICIAR_TRIPULANTE:
		case R_RECIBIR_LA_UBICACION_DEL_TRIPULANTE:
		case R_EXPULSAR_TRIPULANTE:
		case R_TERMINAR_TRIPULANTE:;	//TO TEST

			bool* operacionExitosa = malloc(sizeof(_Bool));

			copiarVariable(operacionExitosa, mensajeRecibido, &offset, sizeof(operacionExitosa));

			paqueteRecibido->mensaje = operacionExitosa;
			//free(operacionExitosa);
			break;

		case R_SOLICITAR_PROXIMA_TAREA:;	//TO TEST

			t_RSolicitarProximaTarea* proximaTarea = malloc(sizeof(*proximaTarea));

			copiarVariable(&(proximaTarea->tid), mensajeRecibido, &offset, sizeof(proximaTarea->tid));
			deserializarChar(mensajeRecibido, &offset, &(proximaTarea->tarea.nombre));
			copiarVariable(&(proximaTarea->tarea.parametros), mensajeRecibido, &offset, sizeof(proximaTarea->tarea.parametros));
			copiarVariable(&(proximaTarea->tarea.posicionX), mensajeRecibido, &offset, sizeof(proximaTarea->tarea.posicionX));
			copiarVariable(&(proximaTarea->tarea.posicionY), mensajeRecibido, &offset, sizeof(proximaTarea->tarea.posicionY));
			copiarVariable(&(proximaTarea->tarea.duracion), mensajeRecibido, &offset, sizeof(proximaTarea->tarea.duracion));
			copiarVariable(&(proximaTarea->tarea.esUltimaTarea), mensajeRecibido, &offset, sizeof(proximaTarea->tarea.esUltimaTarea));
			copiarVariable(&(proximaTarea->tarea.requiereIO), mensajeRecibido, &offset, sizeof(proximaTarea->tarea.requiereIO));

			paqueteRecibido->mensaje = proximaTarea;
			//free(proximaTarea);
			break;

		case ACTUALIZAR_ESTADO:;	//TO TEST

			t_actualizarEstado* estadoActualizar = malloc(sizeof(*estadoActualizar));

			copiarVariable(&(estadoActualizar->tripulante.patota), mensajeRecibido, &offset, sizeof(estadoActualizar->tripulante.patota));
			copiarVariable(&(estadoActualizar->tripulante.tid), mensajeRecibido, &offset, sizeof(estadoActualizar->tripulante.tid));
			copiarVariable(&(estadoActualizar->estado), mensajeRecibido, &offset, sizeof(estadoActualizar->estado));

			paqueteRecibido->mensaje = estadoActualizar;
			//free(estadoActualizar);
			break;

	//----------------------------------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------------------
		case R_OBTENER_BITACORA:
		case CONSULTAR_EXISTENCIA_ARCHIVO_TAREA:;

			 char* nombreArchivo;
			 deserializarChar(mensajeRecibido, &offset, &nombreArchivo);
		/*
			t_consultarExistenciaArchivo* nombreArchivo = malloc(sizeof(*nombreArchivo));
			deserializarChar(mensajeRecibido, &offset, &(nombreArchivo->nombreArchivo));*/
			paqueteRecibido->mensaje = nombreArchivo;
			break;


		case CREAR_ARCHIVO_TAREA:
		case LLENADO_ARCHIVO:
		case ELIMINAR_ARCHIVO_TAREA:
		case VACIADO_ARCHIVO:;

			t_archivoTarea* archivoTarea = malloc(sizeof(*archivoTarea));

			deserializarChar(mensajeRecibido, &offset, &(archivoTarea->nombreArchivo));
			copiarVariable(&(archivoTarea->caracterLlenado), mensajeRecibido, &offset, sizeof(archivoTarea->caracterLlenado));
			copiarVariable(&(archivoTarea->cantidad), mensajeRecibido, &offset, sizeof(archivoTarea->cantidad));

			paqueteRecibido->mensaje = archivoTarea;
			//free(archivoTarea);
			break;


		case ACTUALIZAR_BITACORA:; // OK

			t_actualizarBitacora* reporteAGuardar = malloc(sizeof(*reporteAGuardar));

			copiarVariable(&(reporteAGuardar->tid), mensajeRecibido, &offset, sizeof(reporteAGuardar->tid));
			deserializarChar(mensajeRecibido, &offset, &(reporteAGuardar->reporte));

			paqueteRecibido->mensaje = reporteAGuardar;
			//free(reporteAGuardar);
			break;


		case NOTIFICAR_SABOTAJE:;
		t_sabotaje* posiciones = malloc(sizeof(*posiciones));
		copiarVariable(&(posiciones->posicionX), mensajeRecibido, &offset, sizeof(posiciones->posicionX));
		copiarVariable(&(posiciones->posicionY), mensajeRecibido, &offset, sizeof(posiciones->posicionY));

		paqueteRecibido->mensaje = posiciones;
		//free(posiciones);
		break;
	//----------------------------------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------------------


		default: fprintf(stderr, "Error: Conexion fallida\n"); break;
	}
}

void copiarVariable(void* variable, void* stream, int* offset, int tamanio) {
	memcpy(variable, stream + *offset, tamanio);
	*offset += tamanio;
}

void deserializarChar(void* stream, int* offset, char** receptor){
	uint32_t tamanio;
	copiarVariable(&(tamanio), stream, offset, sizeof(tamanio));
	*receptor = malloc(tamanio);
	copiarVariable(*receptor, stream, offset, tamanio);
	//free(receptor); //TODO:revisar
}











// Funcionres randoms que dani crea donde no deberia


int max(int a,int b){
	if(a > b)
		return a;
	return b;
}


int min(int a,int b){
	if(a < b)
		return a;
	return b;
}

