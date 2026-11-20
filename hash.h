
#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED

#include "proceso.h"

/*
 * Inicializa todas las posiciones de la tabla Hash.
 */
void inicializarHash(void);

/*
 * Inserta un proceso en la tabla Hash.
 *
 * Guarda un puntero al proceso original.
 */
void insertarHash(Proceso *proceso);

/*
 * Busca todos los procesos que coincidan
 * con el nombre solicitado.
 *
 * Devuelve la cantidad de coincidencias.
 */
int buscarProcesosPorNombre(const char *nombre);

/*
 * NUEVO:
 * Elimina de la tabla Hash la referencia
 * correspondiente al PID indicado.
 *
 * Esta funcion debe llamarse ANTES de liberar
 * el proceso original de la lista enlazada.
 */
int eliminarHashPorPid(int pid);

/*
 * Libera todos los nodos de la tabla Hash.
 *
 * No libera los procesos originales porque
 * pertenecen a la lista enlazada principal.
 */
void liberarHash(void);

#endif // HASH_H_INCLUDED

