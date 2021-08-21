/*
 * sabotajes.c
 *
 *  Created on: 21 jul. 2021
 *      Author: utnso
 */
#include "sabotajes.h"

void enviarAvisoDeSabotaje(){

	//TODO: Poner un mutex/semaforo que impida la realizacion de otras actividades

	int conexion = crearConexion(IP_DISCORDIADOR, PUERTO_DISCORDIADOR);

	char** posiciones = string_n_split(POSICIONES_SABOTAJE[sabotajesRecibidos], 2, "|");

	t_sabotaje posicionesSabotaje;

	posicionesSabotaje.posicionX = atoi(posiciones[0]);
	posicionesSabotaje.posicionY = atoi(posiciones[1]);

	free(posiciones[0]);free(posiciones[1]);free(posiciones);

	bool valorRetorno = enviarMensaje(conexion, NOTIFICAR_SABOTAJE, &posicionesSabotaje);

	if(valorRetorno){
	    log_info(logger, "Aviso de sabotaje enviado");
		Paquete* paqueteRecibido = recibirPaquete(conexion, contactoIMongoStore);
		t_RSolicitarProximaTarea* rtaTarea = (t_RSolicitarProximaTarea*)paqueteRecibido->mensaje;

		if(paqueteRecibido->header.tipoMensaje == COMENZAR_PROTOCOLO_FSCK){
			chequeoSabotajes();
			bool sabotajeResuelto = true;
			enviarMensaje(conexion, R_CONSULTAR_EXISTENCIA_ARCHIVO_TAREA, &sabotajeResuelto);
		}

		free(rtaTarea);
		free(paqueteRecibido);

	}else
	    printf("error en conexion con MONGO");

	liberarConexion(conexion);
}

void chequeoSabotajes(){
	if(sabotajeCantidadBloques()){}
	else if(sabotajeFiles()){}
	else if(sabotajeBitmap()){}

}


bool sabotajeCantidadBloques(){

	void* supBloq;

	int fd;
	fd = open(ADDRSUPERBLOCKS, O_RDWR, 00600);
	if (fd == -1){
		printf("Error con la shared memory");
		exit(1);
	}

	supBloq = mmap(NULL, sizeof(superBloque), PROT_READ | PROT_WRITE , MAP_SHARED, fd, 0);
	uint32_t* block_count = supBloq + sizeof(uint32_t);

	uint32_t cantidadDeBloques = statbufBlocks.st_size/TAMANIO_BLOQUES;

	if(cantidadDeBloques == *block_count){
		return false;
	}

	memcpy(supBloq + sizeof(uint32_t), &(cantidadDeBloques), sizeof(uint32_t));

	log_error(logger, "Sabotaje en la cantidad de bloques de SuperBloqs, cantidad de bloques: %d",*block_count);

	msync(supBloq, sizeof(superBloque), MS_SYNC);
	munmap(supBloq, sizeof(superBloque));
	close(fd);


	return true;
}




bool sabotajeBitmap(){

	int cantidadDeBloques;
		if(CANTIDAD_BLOQUES%8 == 0){
			cantidadDeBloques = CANTIDAD_BLOQUES/8;
		}
		else{
			cantidadDeBloques = (CANTIDAD_BLOQUES/8)+1;
		}

	void* supBloq;//asigno variable a superBloque y asigno memoria

	int fd;										//abro superBloque
	fd = open(ADDRSUPERBLOCKS, O_RDWR, 00600);
	if (fd == -1){
		printf("Error con la shared memory");
		exit(1);
	}

	supBloq = mmap(NULL, (2*sizeof(uint32_t)+cantidadDeBloques), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	t_bitarray* bitarray_bitmap = bitarray_create_with_mode(supBloq + 2*sizeof(uint32_t), cantidadDeBloques, LSB_FIRST);

	int tamanioBitmap = CANTIDAD_BLOQUES;


	t_list* listaAuxiliar = list_create();

	char* pathOxigeno = devolverPathArchivoTarea("Oxigeno.ims");
	char* pathComida = devolverPathArchivoTarea("Comida.ims");
	char* pathBasura = devolverPathArchivoTarea("Basura.ims");


	if(consultarExistenciaArchivo(pathOxigeno)){
		t_config* metadataOxigeno = config_create(pathOxigeno);
		agregarBloqueALista(listaAuxiliar, metadataOxigeno);
		config_destroy(metadataOxigeno);
	}

	if(consultarExistenciaArchivo(pathComida)){
		t_config* metadataComida = config_create(pathComida);
		agregarBloqueALista(listaAuxiliar ,metadataComida);
		config_destroy(metadataComida);
	}

	if(consultarExistenciaArchivo(pathBasura)){
		t_config* metadataBasura = config_create(pathBasura);
		agregarBloqueALista(listaAuxiliar,metadataBasura);
		config_destroy(metadataBasura);
	}

	free(pathOxigeno);free(pathComida);free(pathBasura);


	 // void list_add_all(t_list*, t_list* other); ->!



     struct dirent *entry;
	 DIR *dp;

	 char direccion[128];

	 sprintf(direccion, "%s/files/Bitacoras", PUNTO_MONTAJE);

	 dp = opendir(direccion);
	 if (dp == NULL){
		perror("error en opendir (sabotajes.c)");
		return -1;
	 }

	 bool errorEncontrado = false;
	 while((entry = readdir(dp)) != NULL && !errorEncontrado && entry->d_type == DT_REG){
		char* aux = devolverDirectorioBitacora();
		string_append(&aux,entry->d_name);
		//devolverPathArchivoTarea(entry->d_name);

		t_config* metadataBitacora = config_create(aux);

		free(aux);

		agregarBloqueALista(listaAuxiliar, metadataBitacora);
		config_destroy(metadataBitacora);
	 }

	 closedir(dp);




	//liberar memoria de bitacoras cuando se creen las bitacoras
	int tamanioLista = list_size(listaAuxiliar);
	//Chequeo si hay sabotaje
	if(compararResultadosBitmap(listaAuxiliar, bitarray_bitmap, tamanioBitmap, tamanioLista)){
		bitarray_destroy(bitarray_bitmap);
		munmap(supBloq, (2*sizeof(uint32_t)+cantidadDeBloques));
		close(fd);
		return false;
	}

	//Si hay, arreglo el sabotaje
	int i;

	bool cumpleCondicion(int* primerParametro){
		return (*primerParametro == i);
	}

	for(i=0;i<tamanioBitmap;i++){

		if (list_any_satisfy(listaAuxiliar, (void*)cumpleCondicion)){
			if(bitarray_test_bit(bitarray_bitmap, i) == false){
				bitarray_set_bit(bitarray_bitmap, i);
			}
		}
		else{
			if(bitarray_test_bit(bitarray_bitmap,i)){
				bitarray_clean_bit(bitarray_bitmap, i);
			}
		}
	}


	bitarray_destroy(bitarray_bitmap);
	munmap(supBloq, (2*sizeof(uint32_t)+cantidadDeBloques));
	close(fd);

	return true;
}



bool compararResultadosBitmap(t_list* listaAuxiliar, t_bitarray* bitarray, int tamanioBitmap, int tamanioLista){//si esta ok, tiene q devolver true. Apenas encuentre algo distinto, devuelve false

	int i;

	bool cumpleCondicion(int* primerParametro){
		return (*primerParametro == i);
	}

	int a = list_size(listaAuxiliar);
	for(int i = 0; i < a; i++){
		int* aux = list_get(listaAuxiliar, i);

		log_info(logger, "%d", * aux);
	}

	for(i=0;i<tamanioBitmap;i++){

		bool valorBit = bitarray_test_bit(bitarray,i);
		bool estaEnLista = list_any_satisfy(listaAuxiliar, (void*)cumpleCondicion);

		if(valorBit && estaEnLista == false ){
			return false;
		}
		if(valorBit == false && estaEnLista){
			return false;
		}
	}

	return true;
}



void agregarBloqueALista(t_list* listaAuxiliar,t_config* file){

	//int numeroDeBloques = config_get_int_value(file, "BLOCK_COUNT");
	char** listaDeBloques = config_get_array_value(file, "BLOCKS");

	int numeroDeBloques = 0;
		while(listaDeBloques[numeroDeBloques] != NULL ){
			numeroDeBloques++;
		}

	for(int i=0;i<numeroDeBloques;i++){
		//int numeroBloque = atoi(listaDeBloques[i])-1;
		//list_add(listaAuxiliar, (void*)&numeroBloque);

		int* numeroBloque = malloc(sizeof(int));
		*numeroBloque = atoi(listaDeBloques[i])-1;
		list_add(listaAuxiliar, (void*)numeroBloque);
	}

	liberarEspacioListaDeBloques(listaDeBloques);
}


int contadorDeBitacoras(){

    int file_count = 0;
    DIR * dirp;
    struct dirent * entry;
    char * directorio = devolverDirectorioBitacora();
    dirp = opendir(directorio); /* There should be error handling after this */

    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_REG) { /* If the entry is a regular file */
             file_count++;

        }
    }
    free(directorio);
    closedir(dirp);
    return file_count;
}


bool sabotajeFiles(){
	struct dirent *entry;
	DIR *dp;

	char direccion[128];

	sprintf(direccion, "%s/files", PUNTO_MONTAJE);

	dp = opendir(direccion);
	if (dp == NULL)
	{
		perror("error en opendir (sabotajes.c)");
		return -1;
	}

	bool errorEncontrado = false;
	while((entry = readdir(dp)) != NULL && !errorEncontrado){
		if(entry->d_type == DT_REG){
			char* direccionArchvio = devolverPathArchivoTarea(entry->d_name);
			t_config* metadata = config_create(direccionArchvio);
			free(direccionArchvio);

			//Buscar sabotaje en el size
			errorEncontrado = (sabotajeSize(metadata) || errorEncontrado);

			//Buscar sabotaje en el block count
			errorEncontrado = (sabotajeBlockCountYBlocks(metadata) || errorEncontrado);

			//Buscar sabotaje en el BLOCKS
			errorEncontrado = (sabotajeBlocks(metadata) || errorEncontrado);

			if(errorEncontrado){
				config_save(metadata);
			}
			config_destroy(metadata);
		}
	}

	closedir(dp);
	return errorEncontrado;
}





bool sabotajeSize(t_config* metadata){

	int size = config_get_int_value(metadata, "SIZE");

	char caracter[2];

	strcpy(caracter, config_get_string_value(metadata, "CARACTER_LLENADO"));

	char** listaDeBloques = config_get_array_value(metadata,"BLOCKS");

	int tamanioLista = 0;

	while(listaDeBloques[tamanioLista] != NULL ){
		tamanioLista++;
	}
	int sizeReal;
	if (tamanioLista){
		/*sizeReal = (tamanioLista - 1) * TAMANIO_BLOQUES;
		sizeReal += cantidadLlenado(atoi(listaDeBloques[tamanioLista-1]), caracter);
*/		sizeReal = 0;
		for(int i = 0; i < tamanioLista; i++){
			sizeReal += cantidadLlenado(atoi(listaDeBloques[i]), caracter);
		}


	}
	else{
		sizeReal = 0;
	}

	if(sizeReal == size){
		liberarEspacioListaDeBloques(listaDeBloques);
		return false;
	}
	else{
		config_set_value(metadata, "SIZE", string_itoa(sizeReal));
		log_error(logger,"Sabotaje en el SIZE del file");
	}
	liberarEspacioListaDeBloques(listaDeBloques);
	return true;
}

int cantidadLlenado(int bloque, char caracter[]){
	int cantidadCaracteres;

	for(cantidadCaracteres = 0; (memcmp((BLOCKS + ((bloque - 1) * TAMANIO_BLOQUES) + cantidadCaracteres), caracter, 1) == 0) && cantidadCaracteres < TAMANIO_BLOQUES; cantidadCaracteres++){
	}

	//printf("Hay un total de %d caracteres ocupados en este bloque (sabotajes.c)", cantidadCaracteres);
	return cantidadCaracteres;
}





bool sabotajeBlockCountYBlocks(t_config* metadata){

	int block_count = config_get_int_value(metadata, "BLOCK_COUNT");

	char** listaDeBloques = config_get_array_value(metadata,"BLOCKS");

	int tamanioLista = 0;
	while(listaDeBloques[tamanioLista] != NULL ){
		tamanioLista++;
	}

	if(tamanioLista == block_count){
		return false;
	}

	config_set_value(metadata, "BLOCK_COUNT", string_itoa(tamanioLista));
	liberarEspacioListaDeBloques(listaDeBloques);
	return true;
}



bool sabotajeBlocks(t_config* metadata){

	char md5[32];
	strcpy(md5, config_get_string_value(metadata, "MD5_ARCHIVO"));
	int size = config_get_int_value(metadata, "SIZE");
	char** listaDeBloques = config_get_array_value(metadata, "BLOCKS");
	int block_count = config_get_int_value(metadata, "BLOCK_COUNT");

	char caracter[2];
	strcpy(caracter, config_get_string_value(metadata, "CARACTER_LLENADO"));

	//TODO hay que fijarse que la funcion de hash no tenga error al leer los caracteres y ver como hacer para borrar caracteres


	char * cadenaDeCaracteres = obtenerCadenaSabotaje(listaDeBloques, size, block_count); //no libero aca el listaDebloques xq se usa abajo

	char * hashReal = crearMD5(cadenaDeCaracteres);
	if( strcmp( md5, hashReal ) == 0 || block_count == 0){
		free(cadenaDeCaracteres);
		liberarEspacioListaDeBloques(listaDeBloques);
		return false;
	}
	free(hashReal);
	free(cadenaDeCaracteres);


	//TODO: reparar el archvio (Reescribir los caracteres necesarios)
	restaurarArchivo(listaDeBloques, size, block_count, caracter[0]);

	char * cadenaAHashear = devolverCadenaMD5(listaDeBloques);
	strcpy(md5, crearMD5(cadenaAHashear));
	config_set_value(metadata, "MD5_ARCHIVO", md5);
	free(cadenaAHashear);
	liberarEspacioListaDeBloques(listaDeBloques);
	return true;
}


char * obtenerCadenaSabotaje(char** listaDeBloques, int size, int block_count){

	char * cadena = malloc(sizeof(char) * TAMANIO_BLOQUES * block_count);//* block_count * TAMANIO_BLOQUES);
	memset(cadena, 0, sizeof(char) * TAMANIO_BLOQUES * block_count);
	int tamanioLista = 0;
	while(listaDeBloques[tamanioLista] != NULL ){
		int menor = min(size, TAMANIO_BLOQUES);
		strncpy(cadena + tamanioLista*TAMANIO_BLOQUES, BLOCKS + ( ( atoi( listaDeBloques[tamanioLista] ) - 1) *TAMANIO_BLOQUES), menor);

		size -= min(size, TAMANIO_BLOQUES);
		tamanioLista++;

	}
	return cadena;
}

void restaurarArchivo(char** listaDeBloques, int size, int block_count, char caracter){

	int bloque;
	for(bloque = 1; bloque < block_count; bloque++){
		memset(BLOCKS + ((atoi(listaDeBloques[bloque-1]) - 1 ) * TAMANIO_BLOQUES), caracter, TAMANIO_BLOQUES);
		size -= TAMANIO_BLOQUES;
	}

	memset(BLOCKS + ((atoi(listaDeBloques[bloque-1]) - 1 ) * TAMANIO_BLOQUES), caracter, size);
	memset(BLOCKS + ((atoi(listaDeBloques[bloque-1]) - 1 ) * TAMANIO_BLOQUES + size), 0, (TAMANIO_BLOQUES - size));




/*
 * OTRA RESOLUCION POSIBLE:
 *
	int tamanioLista = 0;
	while(listaDeBloques[tamanioLista] != NULL ){
		memset(BLOCKS + ((atoi(listaDeBloques[tamanioLista]) - 1 ) * TAMANIO_BLOQUES), caracter, min(TAMANIO_BLOQUES, size));
		if(size < TAMANIO_BLOQUES){
			memset(BLOCKS + ((atoi(listaDeBloques[tamanioLista]) - 1 ) * TAMANIO_BLOQUES + size), 0, (TAMANIO_BLOQUES - size));
		}

		size -= min(TAMANIO_BLOQUES, size);
		tamanioLista++;
	}
 */

}

/*
memcpy(BLOCKS, "AAAAA", 5);
memcpy(BLOCKS + 5, "AA", 2);

memcpy(BLOCKS + 9, "AA", 2);
memcpy(BLOCKS+14, "AAAAA", 3);

char * cadena = malloc(sizeof(char) * 10);
memcpy(cadena, BLOCKS, 5);
memcpy(cadena+5, BLOCKS + 5, 5);

char * cadena2 = malloc(sizeof(char) * 10);
memcpy(cadena2, BLOCKS+9, 5);
memcpy(cadena2+5, BLOCKS + 14, 5);


char * cadena3 = string_new();
char * cadena4 = malloc(sizeof(char) * 10);
memcpy(cadena4, BLOCKS+9, 5);
string_append(&cadena3, cadena4);
free(cadena4);
cadena4 = malloc(sizeof(char) * 10);
memcpy(cadena4, BLOCKS+14, 5);
string_append(&cadena3, cadena4);
free(cadena4);



printf("\ncadena 1: %s\n",cadena);
printf("\ncadena 2: %s\n",cadena2);
printf("\ncadena 3: %s\n",cadena3);*/

/*signal(SIGUSR1, &enviarAvisoDeSabotaje);



	char caracterLlenado = 'A';

	char * aux = devolverPathArchivoTarea("EJEMPLO.ims");

	FILE * newFile;

	newFile = fopen(aux,"w+");

	fprintf(newFile,"SIZE=28\nBLOCK_COUNT=3\nBLOCKS=[1,4,2]\nCARACTER_LLENADO=%c\nMD5_ARCHIVO=%s",caracterLlenado,"caracterMD5");

	free(aux);
	fclose(newFile);

	//realizarConexiones();

	//int sincronizacionFinalizada = pthread_cancel(actualizarBloques);
	//if(sincronizacionFinalizada) printf("failed to cancel the thread\n");

	char md5[32];
	t_config* metadata = config_create(devolverPathArchivoTarea("EJEMPLO.ims"));
	strcpy(md5, config_get_string_value(metadata, "MD5_ARCHIVO"));
	int size = config_get_int_value(metadata, "SIZE");
	char** listaDeBloques = config_get_array_value(metadata, "BLOCKS");
	int block_count = config_get_int_value(metadata, "BLOCK_COUNT");

	char caracter[2];
	strcpy(caracter, config_get_string_value(metadata, "CARACTER_LLENADO"));


	memcpy(BLOCKS, "AAAAAAAAAAAAAAAAAAAA", 20);
	memcpy(BLOCKS + 30, "AAAAAAAA", 8);
	printf("\n%s\n%s\n", BLOCKS, BLOCKS + 30);

	char * cadenaDeCaracteres = obtenerCadenaSabotaje(listaDeBloques, size, block_count);
	printf("\n%s\n", cadenaDeCaracteres);
	printf("%d", strlen(cadenaDeCaracteres));


	//free(cadenaDeCaracteres);

	restaurarArchivo(listaDeBloques, size, block_count, caracter[0]);
	char * cadenaDeCaracteres2 = obtenerCadenaSabotaje(listaDeBloques, size, block_count);
	printf("\n%s\n", cadenaDeCaracteres2);
	printf("%d", strlen(cadenaDeCaracteres2));


	char * cadenaDeCaracteres3 = devolverCadenaMD5(listaDeBloques);
	printf("\n%s\n", cadenaDeCaracteres3);
	printf("%d", strlen(cadenaDeCaracteres3));

	//free(cadenaDeCaracteres2);


	msync(BLOCKS,statbufBlocks.st_size, MS_SYNC);
	close(fdBlocks);

	return 23;*/






