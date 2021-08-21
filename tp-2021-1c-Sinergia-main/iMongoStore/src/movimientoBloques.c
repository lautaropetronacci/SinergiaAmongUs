/*
 * movimientoBloques.c
 *
 *  Created on: 21 jul. 2021
 *      Author: utnso
 */

#include "movimientoBloques.h"

char* devolverPathDirecto(char* nombreArchivo){
	char* aux = string_new();
	string_append(&aux, PUNTO_MONTAJE);
	string_append(&aux, nombreArchivo);

	return aux;
}

void crearArchivos(){
	mkdir(PUNTO_MONTAJE, 0777);
	char* direccionFiles = string_from_format("%s/files", PUNTO_MONTAJE);
	mkdir(direccionFiles, 0777);
	string_append(&direccionFiles,"/Bitacoras");
	mkdir(direccionFiles, 0777);

	free(direccionFiles);
	if(!consultarExistenciaArchivo(ADDRSUPERBLOCKS)){
		generarSuperBloque();
	}
	if(!consultarExistenciaArchivo(ADDRBLOCKS)){
		generarArchivoBlocks();
	}

	fdBlocks = open(ADDRBLOCKS, O_CREAT | O_RDWR, 00600);
		if (fdBlocks == -1){
			printf("error con shared memory");
			exit(1);
		}

	fstat(fdBlocks, &statbufBlocks);
	BLOCKS = mmap(NULL, statbufBlocks.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdBlocks, 0);
}

void generarSuperBloque(){ // crea el archivo solicitado

	log_info(logger,"Se genero el archivo SuperBloque.ims en el directorio: %s", ADDRSUPERBLOCKS);

	int cantidadDeBloques;
		if(CANTIDAD_BLOQUES%8 == 0){
			cantidadDeBloques = CANTIDAD_BLOQUES/8;
		}
		else{
			cantidadDeBloques = (CANTIDAD_BLOQUES/8)+1;
		}

	void* supBloq;

	int fd;
	fd = open(ADDRSUPERBLOCKS, O_CREAT | O_RDWR, 00600);
	if (fd == -1){
		printf("Error con la shared memory");
		exit(1);
	}

	ftruncate(fd, (2*sizeof(uint32_t)+cantidadDeBloques));

	supBloq = mmap(NULL, (2*sizeof(uint32_t)+cantidadDeBloques), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	uint32_t* block_size = supBloq;
	uint32_t* block_count = supBloq + sizeof(uint32_t);
	void* bitarray_mem = supBloq + 2*sizeof(uint32_t);

	t_bitarray* bitmap = bitarray_create_with_mode((char*)bitarray_mem , cantidadDeBloques , LSB_FIRST);
	*block_size = TAMANIO_BLOQUES;
	*block_count = CANTIDAD_BLOQUES;
	off_t maximo = bitarray_get_max_bit(bitmap);

	for(int i = 0; i < maximo; i++){
		bitarray_clean_bit(bitmap, i);
	}


	msync(supBloq, (2*sizeof(uint32_t)+cantidadDeBloques), MS_SYNC);
	munmap(supBloq,cantidadDeBloques);
	bitarray_destroy(bitmap);
	close(fd);

}

void generarArchivoBlocks(){
	int fd;
	fd = open(ADDRBLOCKS, O_CREAT | O_RDWR, 00600);
	if (fd == -1){
		printf("error con shared memory");
		exit(1);
	}
	ftruncate(fd, (TAMANIO_BLOQUES*CANTIDAD_BLOQUES));
	close(fd);
	log_info(logger,"Se genero el archivo Bloque.ims en el directorio: %s", ADDRBLOCKS);
}

void* sincronizarBlocks(){
	while(1){
		msync(BLOCKS,statbufBlocks.st_size, MS_SYNC);
		sleep(TIEMPO_SINCRONIZACION);
	}
}




