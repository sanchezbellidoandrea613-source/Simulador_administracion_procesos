
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

/*
 * CONSERVADO:
 * La tabla Hash tiene 100 posiciones.
 */
#define TAM_HASH 100

/*
 * Estructura que representa cada nodo
 * almacenado dentro de la tabla Hash.
 */
typedef struct NodoHash {

    /*
     * Puntero al proceso original.
     *
     * No guardamos una copia.
     */
    Proceso *proceso;

    /*
     * Siguiente nodo de la misma posicion.
     *
     * Permite resolver colisiones.
     */
    struct NodoHash *siguiente;

} NodoHash;

/*
 * Arreglo de punteros que representa
 * las 100 posiciones de la tabla.
 */
static NodoHash *tablaHash[TAM_HASH];

/*
 * =====================================================
 * FUNCION HASH
 * =====================================================
 */

/*
 * CONSERVADO:
 * Calcula una posicion utilizando el nombre.
 *
 * Utiliza el mismo calculo del codigo original.
 */
static unsigned int funcionHash(const char *nombre) {

    unsigned long hash = 5381;

    /*
     * Recorre los caracteres del nombre.
     */
    while (*nombre != '\0') {

        hash = hash * 33 + (unsigned char)*nombre;

        nombre++;
    }

    /*
     * El modulo asegura que la posicion
     * se encuentre entre 0 y 99.
     */
    return (unsigned int)(hash % TAM_HASH);
}

/*
 * =====================================================
 * INICIALIZACION
 * =====================================================
 */

/*
 * Inicializa todas las posiciones en NULL.
 *
 * NULL indica que no hay nodos registrados.
 */
void inicializarHash(void) {

    int i;

    for (i = 0; i < TAM_HASH; i++) {

        tablaHash[i] = NULL;
    }
}

/*
 * =====================================================
 * INSERCION
 * =====================================================
 */

/*
 * Inserta un proceso en la tabla Hash.
 *
 * Se conserva el manejo de colisiones
 * mediante listas enlazadas.
 */
void insertarHash(Proceso *proceso) {

    unsigned int posicion;

    NodoHash *nuevo;

    /*
     * Verifica que el proceso exista.
     */
    if (proceso == NULL) {
        return;
    }

    /*
     * Calcula la posicion correspondiente.
     */
    posicion = funcionHash(proceso->nombre);

    /*
     * Reserva memoria para un nuevo nodo.
     */
    nuevo = (NodoHash *)malloc(sizeof(NodoHash));

    if (nuevo == NULL) {

        printf(
            "[ERROR] No hay memoria para la tabla Hash.\n"
        );

        return;
    }

    /*
     * Guarda la direccion del proceso original.
     */
    nuevo->proceso = proceso;

    /*
     * Conecta el nodo con los que ya estaban
     * almacenados en esa posicion.
     */
    nuevo->siguiente = tablaHash[posicion];

    /*
     * Coloca el nuevo nodo al inicio.
     */
    tablaHash[posicion] = nuevo;
}

/*
 * =====================================================
 * BUSQUEDA POR NOMBRE
 * =====================================================
 */

/*
 * Busca todos los procesos con el nombre indicado.
 *
 * Es posible encontrar varios porque distintos
 * procesos pueden compartir el mismo nombre,
 * pero tienen PID diferentes.
 */
int buscarProcesosPorNombre(const char *nombre) {

    unsigned int posicion;

    NodoHash *actual;

    int encontrados = 0;

    /*
     * Comprueba que el nombre sea valido.
     */
    if (nombre == NULL || nombre[0] == '\0') {
        return 0;
    }

    /*
     * Calcula la posicion correspondiente.
     */
    posicion = funcionHash(nombre);

    /*
     * Comienza a recorrer la lista de esa posicion.
     */
    actual = tablaHash[posicion];

    printf("\n--- RESULTADOS DE BUSQUEDA ---\n");

    while (actual != NULL) {

        /*
         * Compara el nombre solicitado
         * con el nombre del proceso.
         */
        if (strcmp(
                actual->proceso->nombre,
                nombre
            ) == 0) {

            /*
             * Muestra los datos del proceso.
             */
            printf(
                "\nPID: %d",
                actual->proceso->pid
            );

            printf(
                "\nNombre: %s",
                actual->proceso->nombre
            );

            printf(
                "\nUsuario: %s",
                actual->proceso->usuario
            );

            printf(
                "\nCPU: %.1f%%",
                actual->proceso->uso_cpu
            );

            printf(
                "\nRAM: %.0f MB",
                actual->proceso->uso_ram
            );

            printf(
                "\nHilos: %d",
                actual->proceso->hilos
            );

            /*
             * NUEVO:
             * Muestra la prioridad del proceso.
             */
            printf(
                "\nPrioridad: %d",
                actual->proceso->prioridad
            );

            /*
             * Muestra su estado actual.
             */
            printf(
                "\nEstado: %c",
                actual->proceso->estado
            );

            /*
             * Muestra su tiempo restante.
             */
            printf(
                "\nTiempo restante: %d segundos\n",
                actual->proceso->tiempoRestante
            );

            encontrados++;
        }

        actual = actual->siguiente;
    }

    /*
     * Informa cuando no existen coincidencias.
     */
    if (encontrados == 0) {

        printf(
            "No se encontro el proceso: %s\n",
            nombre
        );
    }

    return encontrados;
}

/*
 * =====================================================
 * ELIMINACION SEGURA POR PID
 * =====================================================
 */

/*
 * NUEVO:
 * Elimina la referencia a un proceso por su PID.
 *
 * IMPORTANTE:
 * Esta funcion NO libera el proceso original.
 *
 * Solamente libera el nodo de la tabla Hash.
 *
 * De esta manera, la lista enlazada puede
 * eliminar despues el proceso sin dejar
 * referencias invalidas.
 */
int eliminarHashPorPid(int pid) {

    int i;

    /*
     * Recorre todas las posiciones de la tabla.
     *
     * Utilizamos el PID porque identifica
     * de manera unica a cada proceso.
     */
    for (i = 0; i < TAM_HASH; i++) {

        NodoHash *actual = tablaHash[i];

        NodoHash *anterior = NULL;

        /*
         * Recorre los nodos de esta posicion.
         */
        while (actual != NULL) {

            /*
             * Comprueba si encontramos el PID.
             */
            if (actual->proceso != NULL &&
                actual->proceso->pid == pid) {

                /*
                 * Si es el primer nodo,
                 * actualizamos la cabeza.
                 */
                if (anterior == NULL) {

                    tablaHash[i] = actual->siguiente;

                } else {

                    /*
                     * Si esta en medio o al final,
                     * conectamos sus vecinos.
                     */
                    anterior->siguiente =
                        actual->siguiente;
                }

                /*
                 * Liberamos solamente el nodo Hash.
                 */
                free(actual);

                /*
                 * Devuelve 1 para indicar
                 * que la referencia fue eliminada.
                 */
                return 1;
            }

            /*
             * Avanza al siguiente nodo.
             */
            anterior = actual;

            actual = actual->siguiente;
        }
    }

    /*
     * Devuelve 0 si no encontro el PID.
     */
    return 0;
}

/*
 * =====================================================
 * LIBERACION DE MEMORIA
 * =====================================================
 */

/*
 * Libera todos los nodos de la tabla Hash.
 *
 * Los procesos originales se liberaran
 * posteriormente desde la lista enlazada.
 */
void liberarHash(void) {

    int i;

    for (i = 0; i < TAM_HASH; i++) {

        NodoHash *actual = tablaHash[i];

        while (actual != NULL) {

            NodoHash *temporal = actual;

            actual = actual->siguiente;

            /*
             * Solo libera el nodo Hash.
             */
            free(temporal);
        }

        /*
         * Deja vacia esta posicion.
         */
        tablaHash[i] = NULL;
    }
}

