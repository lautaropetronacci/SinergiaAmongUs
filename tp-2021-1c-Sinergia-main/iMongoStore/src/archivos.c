/*
 * archivos.c
 *
 *  Created on: 21 jul. 2021
 *      Author: utnso
 */

#include "archivos.h"

void liberarEspacioListaDeBloques(char** listaDeBloques){
	int tamanioLista = 0;
		while(listaDeBloques[tamanioLista] != NULL ){
			tamanioLista++;
		}

	for (int i = 0; i < tamanioLista; i++) {
		free(listaDeBloques[i]);
	}

	free(listaDeBloques);
}



void actualizarArchivo(t_config* metadata, int espacioALlenar){
	//t_config* metadata = config_create(pathArchivo);
	int size = config_get_int_value(metadata, "SIZE");
	char aux[8];
	sprintf(aux, "%d", size+espacioALlenar);

	char** listaDeBloques = config_get_array_value(metadata, "BLOCKS");

	config_set_value(metadata, "SIZE", aux);

	char* listaDeCaracteres = devolverCadenaMD5(listaDeBloques);

	char* md5 = crearMD5(listaDeCaracteres);

	config_set_value(metadata, "MD5_ARCHIVO", md5);

	log_info(logger, "Se actualizó el hash a:%s, el size a:%s", md5, aux);

	free(listaDeCaracteres);
	free(md5);

	config_save(metadata);
	//config_destroy(metadata);
	liberarEspacioListaDeBloques(listaDeBloques);
}


char* devolverCadenaMD5(char** listaDeBloques){
	if (BLOCKS == MAP_FAILED){
		printf("error map (291)");
	}

	char * cadena = string_new();

	for(int i = 0; listaDeBloques[i] != NULL; i++){
		char b[TAMANIO_BLOQUES+1];
		strncpy(b, BLOCKS+((atoi(listaDeBloques[i])-1)*TAMANIO_BLOQUES),TAMANIO_BLOQUES);
		b[TAMANIO_BLOQUES] = '\0';
		string_append(&cadena, b);
	}
	return cadena;
}


char* crearMD5(char * cadena){

	FILE *fpipe;
	char *command = string_from_format("echo -n %s | md5sum",cadena);
	char c[32];

	if (0 == (fpipe = (FILE*)popen(command, "r")))
	{
		perror("popen() failed.");
		exit(EXIT_FAILURE);
	}
	free(command);

	fread(&c, sizeof c, 1, fpipe);

	char* aux = string_new();
	string_append(&aux, c);
	pclose(fpipe);
	return aux;
}



char* devolverPathArchivoTarea(char* nombreArchivo){
	char * aux = string_new();
	string_append(&aux, PUNTO_MONTAJE);
	string_append(&aux, "/files/");
	string_append(&aux,nombreArchivo);

	return aux;
}



bool consultarExistenciaArchivo(char* pathArchivo){  //consulta en los files si existe el archivo que le pregunta el discordiador
	FILE * file;
	file = fopen(pathArchivo, "r");
	   if (file)
	   {
	       fclose(file);
	       return true;
	   }
	   else
	   {
		   return false;
	   }


}

void generarArchivoDeCero(t_archivoTarea* tipoArchivo){ // crea el archivo solicitado

	char caracterLlenado = tipoArchivo->caracterLlenado;

	char * aux = devolverPathArchivoTarea(tipoArchivo->nombreArchivo);

	FILE * newFile;

	newFile = fopen(aux,"w+");

	fprintf(newFile,"SIZE=0\nBLOCK_COUNT=0\nBLOCKS=[]\nCARACTER_LLENADO=%c\nMD5_ARCHIVO= ",caracterLlenado);

	log_info(logger, "Se generó el archivo: %s", aux);

	free(aux);
	fclose(newFile);
}











void agregarCaracter(char* nombreArchivo, int caracteresAescribir, char caracterLlenado){
	char* pathArchivo = devolverPathArchivoTarea(nombreArchivo);
	t_config* metadata = config_create(pathArchivo);
	free(pathArchivo);

	int bloque = buscarBloqueDisponible(metadata);


	while (caracteresAescribir > 0){
		int espacioEnElBloque = espacioDisponible(metadata);
		int espacioALlenar = min(caracteresAescribir, espacioEnElBloque); //TODO

		if(espacioALlenar){       		//chequeo que haya espacio en los bloques            se puede sacar con un while(espacioBloque >0);
			aniadirCaracteres(caracterLlenado, espacioALlenar, bloque, (TAMANIO_BLOQUES-espacioEnElBloque));  //TODO                         //agrego caracter
			actualizarArchivo(metadata, espacioALlenar);	 //TODO
			caracteresAescribir -= espacioALlenar;
		}else{
			bloque = buscarBloqueDisponible(metadata);
			if(bloque == 0){
				errorDeBloques();
				return;
			}
		}
	}

	logCaracteresAgregados(caracteresAescribir);

	config_save(metadata);
	config_destroy(metadata);
}








int buscarBloqueDisponible(t_config* metadata){
	//t_config* metadata = config_create(pathArchivo);
	int size = config_get_int_value(metadata, "SIZE");

	char** listaDeBloques = config_get_array_value(metadata,"BLOCKS");

	int tamanioLista = 0;
		while(listaDeBloques[tamanioLista] != NULL ){
			tamanioLista++;
		}

	int cantidadDeBloquesArchivo = tamanioLista;

	int numeroDeBloque = 0;
	if ((cantidadDeBloquesArchivo * TAMANIO_BLOQUES) > size){			//Me fijo que no haya espacio en el bloque actual
		//config_destroy(metadata);
		numeroDeBloque = atoi(listaDeBloques[cantidadDeBloquesArchivo-1]);
		liberarEspacioListaDeBloques(listaDeBloques);
		return numeroDeBloque;
	}
	else {

		int cantidadDeBloques;
		if(CANTIDAD_BLOQUES%8 == 0){
			cantidadDeBloques = CANTIDAD_BLOQUES/8;
		}
		else{
			cantidadDeBloques = (CANTIDAD_BLOQUES/8)+1;
		}



		int fd;
		fd = open(ADDRSUPERBLOCKS, O_RDWR, 00600);
		if (fd == -1){
			printf("Error con la shared memory");
			exit(1);
		}
		void* supBloq; //= malloc(sizeof(superBloque));
		supBloq = mmap(NULL, (2*sizeof(uint32_t)+cantidadDeBloques), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		void* bitarray_mem = supBloq + 2*sizeof(uint32_t);

		t_bitarray* bitmap = bitarray_create_with_mode((char*)bitarray_mem , cantidadDeBloques , LSB_FIRST);
		//off_t maximo = bitarray_get_max_bit(bitmap);

		bool bitDisponible = 1;
		for(; numeroDeBloque < CANTIDAD_BLOQUES && bitDisponible; numeroDeBloque++){	//Busco un bit disponible (0)
			bitDisponible = bitarray_test_bit(bitmap, numeroDeBloque);
		}

		if(numeroDeBloque >= CANTIDAD_BLOQUES){							//Chequeo no haber recorrido todos los bits

			//free(supBloq);
			munmap(supBloq,cantidadDeBloques);
			close(fd);
			//config_destroy(metadata);
			liberarEspacioListaDeBloques(listaDeBloques);
			return 0; //Error, no hay mas espacio
		}

		bitarray_set_bit(bitmap, numeroDeBloque-1);						//Cambio el bit a "en uso"


		msync(supBloq, (2*sizeof(uint32_t)+cantidadDeBloques), MS_SYNC);
		munmap(supBloq,(2*sizeof(uint32_t)+cantidadDeBloques));
		bitarray_destroy(bitmap);
		close(fd);
	}

	char * stringLista = string_new();									//creo el string de la lista
	string_append(&stringLista, "[");									//*
	if(cantidadDeBloquesArchivo){
		string_append(&stringLista, listaDeBloques[0]);						//*
	}																	//*
	for(int i = 1; i < cantidadDeBloquesArchivo; i++){					//*
		string_append(&stringLista, ",");								//*
		string_append(&stringLista, listaDeBloques[i]);					//*
	}																	//*

	if(cantidadDeBloquesArchivo){
		string_append(&stringLista, ",");								//*
	}

	char* numeroString = string_itoa(numeroDeBloque);
	string_append(&stringLista, numeroString);			//*
	free(numeroString);
	string_append(&stringLista, "]");									//*

	config_set_value(metadata, "BLOCKS", stringLista);					//Asigno el string de la lista
	printf("%s", stringLista);

	free(stringLista);													//Libero el string de la lista

	if(config_has_property(metadata, "BLOCK_COUNT")){
		char * prueba = string_itoa(cantidadDeBloquesArchivo+1);
		config_set_value(metadata, "BLOCK_COUNT", prueba);
		free(prueba);
	}
					//Aumento la catidad de bloques

	liberarEspacioListaDeBloques(listaDeBloques);

	config_save(metadata);
	//config_destroy(metadata);

	log_error(logger, "Bloque encontrado: %d", numeroDeBloque);
    return numeroDeBloque;
}



int espacioDisponible(t_config* metadata){
	//t_config* metadata = config_create(pathArchivo);
	int size = config_get_int_value(metadata, "SIZE");
	int cantidadDeBloques = config_get_int_value(metadata, "BLOCK_COUNT");
	int espacioDisponible = TAMANIO_BLOQUES - size%TAMANIO_BLOQUES;
	//config_destroy(metadata);

	if (espacioDisponible == TAMANIO_BLOQUES){
		if(size/TAMANIO_BLOQUES == cantidadDeBloques)
			return 0;

		else if(size/TAMANIO_BLOQUES < cantidadDeBloques)
			return TAMANIO_BLOQUES;
	}
	return espacioDisponible;
}


int espacioDisponibleBitacora(t_config* metadata){
	//t_config* metadata = config_create(pathBitacora);
	int size = config_get_int_value(metadata, "SIZE");
	char** listaDeBloques = config_get_array_value(metadata,"BLOCKS");

	int cantidadDeBloques = 0;
	while(listaDeBloques[cantidadDeBloques] != NULL ){
		cantidadDeBloques++;
	}

	int espacioDisponible = TAMANIO_BLOQUES - size%TAMANIO_BLOQUES;


	if (espacioDisponible == TAMANIO_BLOQUES){
		if(size/TAMANIO_BLOQUES == cantidadDeBloques){
			liberarEspacioListaDeBloques(listaDeBloques);
			return 0;
		}

		else if(size/TAMANIO_BLOQUES < cantidadDeBloques){
			liberarEspacioListaDeBloques(listaDeBloques);
			return TAMANIO_BLOQUES;
		}

	}
	liberarEspacioListaDeBloques(listaDeBloques);
	return espacioDisponible;
}



void aniadirCaracteres(char caracterLlenado, int espacioALlenar, int bloque, int offsetBloque){
	char buf[espacioALlenar];
	for(int i = 0; i < espacioALlenar; i++){//Se podría pasar a la funcióna anterior a esta para no tener que repetir el for constantemente
		buf[i] = caracterLlenado;
	}

	int offset = ((bloque-1)*TAMANIO_BLOQUES+offsetBloque);

	memcpy(BLOCKS + offset, buf, espacioALlenar);
}

void errorDeBloques(){
	log_error(logger, "ERROR: No hay mas bloques disponibles");
}


void borrarCaracteres(char* nombreArchivo, int espacioAVaciar){
	char* pathArchivo = devolverPathArchivoTarea(nombreArchivo);
	t_config* metadata = config_create(pathArchivo);
	free(pathArchivo);

	int bloque = buscarUltimoBloque(metadata);

	int size = config_get_int_value(metadata, "SIZE");
	int tamanioOcupado = size%TAMANIO_BLOQUES;

	if(tamanioOcupado == 0)
		tamanioOcupado = TAMANIO_BLOQUES;


	while(espacioAVaciar > 0){

		int tamanioARestar = min(espacioAVaciar, tamanioOcupado);
		if(!bloque){
			   log_error(logger, "Se trata de eliminar más caracteres de los existentes");
			   break;
		   }
		if(espacioAVaciar > tamanioARestar){        						// caso donde no alcanza el bloque para

		   eliminarCaracteresDelBlocks(bloque, tamanioARestar, tamanioOcupado);                   // completar el pedido
		   eliminarBloque(metadata,bloque);

		   actualizarBitmap(bloque);
		   actualizarArchivo(metadata, -tamanioARestar);


		   bloque = buscarUltimoBloque(metadata);

		   espacioAVaciar -= tamanioARestar;
		   tamanioOcupado = TAMANIO_BLOQUES;
		}

		else if(tamanioOcupado > tamanioARestar){
			eliminarCaracteresDelBlocks(bloque, tamanioARestar, tamanioOcupado);
			actualizarArchivo(metadata, -tamanioARestar);

			espacioAVaciar -= tamanioARestar;
		}

		else if(tamanioOcupado == tamanioARestar){
			eliminarCaracteresDelBlocks(bloque, tamanioARestar, tamanioOcupado);
			eliminarBloque(metadata,bloque);

			actualizarBitmap(bloque);
			actualizarArchivo(metadata, -tamanioARestar);

			espacioAVaciar -= tamanioARestar;
		}
	}
	logBorrarCaracteres();
	config_save(metadata);
	config_destroy(metadata);
}

int buscarUltimoBloque(t_config* metadata){
	char** listaDeBloques = config_get_array_value(metadata, "BLOCKS");
	int block_count = config_get_int_value(metadata, "BLOCK_COUNT");

	if(block_count == 0){
		liberarEspacioListaDeBloques(listaDeBloques);
		return 0;
	}
	if(listaDeBloques[block_count - 1] == NULL){
		liberarEspacioListaDeBloques(listaDeBloques);
		return 0;
	}

	int aux = atoi(listaDeBloques[block_count - 1]);

	liberarEspacioListaDeBloques(listaDeBloques);

	return aux;
}

void eliminarCaracteresDelBlocks(int bloque, int cantidadDeCaracteres, int size){
	int posicionBloque = TAMANIO_BLOQUES * (bloque -1);

	int tamanioOcupado;
	if(size%TAMANIO_BLOQUES == 0){
			tamanioOcupado = TAMANIO_BLOQUES;}
	else {tamanioOcupado = size%TAMANIO_BLOQUES;}

	memset(BLOCKS + posicionBloque + tamanioOcupado - cantidadDeCaracteres,0,cantidadDeCaracteres );
}

void actualizarBitmap(int bloque){
	void* supBloq; //= malloc(sizeof(superBloque));//asigno variable a superBloque y asigno memoria

	int fd;										//abro superBloque
	fd = open(ADDRSUPERBLOCKS, O_RDWR, 00600);
	if (fd == -1){
		printf("Error con la shared memory");
		exit(1);
	}

	int cantidadDeBloques;
	if(CANTIDAD_BLOQUES%8 == 0){
		cantidadDeBloques = CANTIDAD_BLOQUES/8;
	}
	else{
		cantidadDeBloques = (CANTIDAD_BLOQUES/8)+1;
	}

	supBloq = mmap(NULL, (2*sizeof(uint32_t)+cantidadDeBloques), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	void* bitarray_bitmap = supBloq + 2*sizeof(uint32_t);

	t_bitarray* bitmap = bitarray_create_with_mode((char*)bitarray_bitmap , cantidadDeBloques , LSB_FIRST);

	bitarray_clean_bit(bitmap, bloque-1);

	msync(supBloq, (2*sizeof(uint32_t)+cantidadDeBloques), MS_SYNC);
	munmap(supBloq,(2*sizeof(uint32_t)+cantidadDeBloques));
	bitarray_destroy(bitmap);
	close(fd);
}

void eliminarBloque(t_config* metadata, int bloque){
	int blockCount = config_get_int_value(metadata, "BLOCK_COUNT");
	char** listaDeBloques = config_get_array_value(metadata, "BLOCKS");

	blockCount -= 1;
	char * prueba = string_itoa(blockCount);
	config_set_value(metadata, "BLOCK_COUNT", prueba);
	free(prueba);

	char * stringLista = string_new();

	if(blockCount <= 0){
		string_append(&stringLista, "[]");
		config_set_value(metadata, "BLOCKS", stringLista);
		free(stringLista);
		liberarEspacioListaDeBloques(listaDeBloques);
		config_save(metadata);
		return;
	}

	string_append(&stringLista, "[");
	string_append(&stringLista, listaDeBloques[0]);

	for(int i = 1; i<blockCount;i++){
		string_append(&stringLista, ",");
		string_append(&stringLista, listaDeBloques[i]);
	}
	string_append(&stringLista, "]");

	config_set_value(metadata, "BLOCKS", stringLista);
	free(stringLista);

	liberarEspacioListaDeBloques(listaDeBloques);
	config_save(metadata);
}



void crearBitacora(uint32_t tid){
		char * aux = devolverPathBitacora(tid);

		FILE * newFile;

		newFile = fopen(aux,"w+");

		fprintf(newFile,"SIZE=0\nBLOCKS=[]");

		free(aux);
		fclose(newFile);
}

char* devolverPathBitacora(uint32_t tid){
	char * aux = string_new();
	char * idAux = string_itoa(tid);

	string_append(&aux, PUNTO_MONTAJE);
	string_append(&aux, "/files/Bitacoras/Tripulante");					//TODO podriamos cambiarlos con fprintf si funciona piola tmb
	string_append(&aux,idAux);
	string_append(&aux,".ims");

	free(idAux);

	return aux;
}

char* devolverDirectorioBitacora(){
	char * aux = string_new();

	string_append(&aux, PUNTO_MONTAJE);
	string_append(&aux, "/files/Bitacoras/");					//TODO podriamos cambiarlos con fprintf si funciona piola tmb

	return aux;
}

void agregarAccionesTripulanteABlocks(char* reporte, uint32_t tid){
	char * pathBitacora = devolverPathBitacora(tid);
	t_config* metadata = config_create(pathBitacora);
	free(pathBitacora);

	int tamanioReporte = string_length(reporte);
	int bloque = buscarBloqueDisponible(metadata);
	int offset = 0;

	while(tamanioReporte > 0){
		int espacioEnBloque = espacioDisponibleBitacora(metadata);
		int espacioALlenar = min(tamanioReporte,espacioEnBloque);

		if(espacioALlenar){
			char * textoACopiar = string_substring(reporte, offset, espacioALlenar);

			copiarSubstringEnBlocks(metadata, bloque, textoACopiar, espacioALlenar);
			free(textoACopiar);

			offset += espacioALlenar;
			actualizarArchivoBitacora(metadata, espacioALlenar);
			tamanioReporte -= espacioALlenar;


		}
		else{
			bloque = buscarBloqueDisponible(metadata);
				if(bloque == 0){
					errorDeBloques(); //TODO
					return;
				}
		}
	}
	log_info(logger, "Se actualiza la bitacora del tripulante %d", tid);
	config_save(metadata);
	config_destroy(metadata);

}


void copiarSubstringEnBlocks(t_config* bitacora, int bloque, char* textoACopiar, int espacioALlenar){
	//t_config* bitacora = config_create(pathBitacora);
	int size = config_get_int_value(bitacora, "SIZE");
	char** listaDeBloques = config_get_array_value(bitacora,"BLOCKS");


	int tamanioLista = 0;
	while(listaDeBloques[tamanioLista] != NULL ){
		tamanioLista++;
	}

	int offsetBloque = size%TAMANIO_BLOQUES;															//TODO faltaria un riki testing

	int offsetTotal = ((bloque-1)*TAMANIO_BLOQUES+offsetBloque);

	char buf[espacioALlenar];

	strncpy(buf, textoACopiar, espacioALlenar);

	memcpy(BLOCKS+offsetTotal, buf, espacioALlenar);

	liberarEspacioListaDeBloques(listaDeBloques);

}

void actualizarArchivoBitacora(t_config* metadata, int espacioALlenar){
	//t_config* metadata = config_create(pathBitacora);
	int size = config_get_int_value(metadata, "SIZE");
	char aux[8];
	sprintf(aux, "%d", size+espacioALlenar);

	config_set_value(metadata, "SIZE", aux);

	config_save(metadata);
	//config_destroy(metadata);
}


char * devolverBitacora(uint32_t idTripulante){
    char * pathBitacora = devolverPathBitacora(idTripulante);

    t_config* bitacora = config_create(pathBitacora);
    int size = config_get_int_value(bitacora, "SIZE");
    char** listaDeBloques = config_get_array_value(bitacora,"BLOCKS");

    free(pathBitacora);
    config_destroy(bitacora);
    char* bitacoraCompleta = string_new();

    if(size == 0){
    	liberarEspacioListaDeBloques(listaDeBloques);
    	return bitacoraCompleta;
    }

    int tamanioLista = 0;
        while(listaDeBloques[tamanioLista] != NULL ){
            tamanioLista++;
        }

    int i=0;
    int bloqueAux;


    if(size%TAMANIO_BLOQUES == 0){
        while(i != (tamanioLista)){
        	//char* bitacoraAux = malloc(TAMANIO_BLOQUES);
            bloqueAux = atoi(listaDeBloques[i]);

            char* bitacoraAux = string_substring(BLOCKS, TAMANIO_BLOQUES * (bloqueAux-1), TAMANIO_BLOQUES);
            //strncpy(bitacoraAux, (BLOCKS + TAMANIO_BLOQUES * (bloqueAux - 1)), TAMANIO_BLOQUES);
            string_append(&bitacoraCompleta, bitacoraAux);

            free(bitacoraAux);
            i++;
        }
    }

    else{
        while(i != (tamanioLista-1)){
            bloqueAux = atoi(listaDeBloques[i]);

            char* bitacoraAux = string_substring(BLOCKS, TAMANIO_BLOQUES * (bloqueAux-1), TAMANIO_BLOQUES);
            //strncpy(bitacoraAux, (BLOCKS + TAMANIO_BLOQUES * (bloqueAux - 1)), TAMANIO_BLOQUES);
            string_append(&bitacoraCompleta, bitacoraAux);

            free(bitacoraAux);
            i++;
        }

        int offset = size%TAMANIO_BLOQUES;

        bloqueAux = atoi(listaDeBloques[tamanioLista-1]);

        char* bitacoraAux = string_substring(BLOCKS, TAMANIO_BLOQUES * (bloqueAux-1), offset);

        string_append(&bitacoraCompleta, bitacoraAux);
        free(bitacoraAux);

    }

    liberarEspacioListaDeBloques(listaDeBloques);
    return bitacoraCompleta;
}


void descartarBasura(){
    char* pathBasura = devolverPathArchivoTarea("Basura.ims");
    if(!consultarExistenciaArchivo(pathBasura)){
        log_info(logger, "El archivo basura.ims no existe");
        free(pathBasura);
        return;
    }

    t_config* basura = config_create(pathBasura);
    int size = config_get_int_value(basura, "SIZE");
    config_destroy(basura);

    borrarCaracteres("Basura.ims", size);
    int eliminar = remove(pathBasura);
    free(pathBasura);

    if(eliminar == 0) {
	  log_info(logger, "Archivo Basura.ims eliminado correctamente");
    } else {
	  log_error(logger, "Error: no fue posible eliminar Basura.ims");
    }
}
