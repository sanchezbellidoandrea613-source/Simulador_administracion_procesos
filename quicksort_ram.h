#ifndef QUICKSORT_RAM_H_INCLUDED
#define QUICKSORT_RAM_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include "proceso.h"

/* Intercambia dos punteros a procesos. */
static void intercambiarRAM(Proceso **a, Proceso **b) {

    Proceso *auxiliar = *a;

    *a = *b;
    *b = auxiliar;
}

/* Particiona el arreglo tomando el elemento central como pivote. */
static int particionarRAM(Proceso **arreglo, int inicio, int fin) {

    float pivote = arreglo[inicio + (fin - inicio) / 2]->uso_ram;

    int i = inicio;
    int j = fin;

    while (i <= j) {

        // Busca valores mayores que el pivote.
        while (arreglo[i]->uso_ram > pivote) {
            i++;
        }

        // Busca valores menores que el pivote.
        while (arreglo[j]->uso_ram < pivote) {
            j--;
        }

        // Intercambia los elementos.
        if (i <= j) {

            intercambiarRAM(&arreglo[i], &arreglo[j]);

            i++;
            j--;
        }
    }

    return i;
}

/* Algoritmo Quick Sort descendente para RAM. */
static void quickSortRAM(Proceso **arreglo, int inicio, int fin) {

    if (inicio >= fin) {
        return;
    }

    int indice = particionarRAM(arreglo, inicio, fin);

    if (inicio < indice - 1) {
        quickSortRAM(arreglo, inicio, indice - 1);
    }

    if (indice < fin) {
        quickSortRAM(arreglo, indice, fin);
    }
}

/* Cuenta los procesos registrados. */
static int contarProcesosRAM(Proceso *cabeza) {

    int contador = 0;

    while (cabeza != NULL) {

        contador++;

        cabeza = cabeza->siguiente;
    }

    return contador;
}

/* Ordena y muestra todos los procesos por consumo de RAM. */
void ordenarProcesosPorRAM(Proceso *cabeza) {

    int cantidad = contarProcesosRAM(cabeza);

    if (cantidad == 0) {

        printf("\nNo hay procesos registrados.\n");

        return;
    }

    // Reserva memoria para el arreglo auxiliar.
    Proceso **arreglo = (Proceso **)malloc(
        cantidad * sizeof(Proceso *)
    );

    if (arreglo == NULL) {

        printf("\nError: no se pudo reservar memoria.\n");

        return;
    }

    // Copia los punteros de la lista al arreglo.
    int i = 0;

    while (cabeza != NULL) {

        arreglo[i] = cabeza;

        cabeza = cabeza->siguiente;

        i++;
    }

    // Ejecuta Quick Sort.
    quickSortRAM(arreglo, 0, cantidad - 1);

    // Muestra los resultados.
    printf("\n============================================\n");
    printf("     PROCESOS ORDENADOS POR MEMORIA RAM\n");
    printf("          ORDEN DESCENDENTE\n");
    printf("============================================\n");

    printf("%-8s %-16s %-12s %-12s %-10s\n",
           "PID", "PROCESO", "CPU (%)", "RAM (MB)", "ESTADO");

    printf("---------------------------------------------------------------\n");

    for (i = 0; i < cantidad; i++) {

        printf("%-8d %-16.16s %-12.1f %-12.0f %-10c\n",
               arreglo[i]->pid,
               arreglo[i]->nombre,
               arreglo[i]->uso_cpu,
               arreglo[i]->uso_ram,
               arreglo[i]->estado);
    }

    printf("\nTotal de procesos: %d\n", cantidad);

    // Libera el arreglo auxiliar.
    free(arreglo);
}

#endif
