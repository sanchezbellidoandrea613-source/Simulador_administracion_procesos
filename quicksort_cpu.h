
#ifndef QUICKSORT_CPU_H_INCLUDED
#define QUICKSORT_CPU_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include "proceso.h"

/* MODIFICADO: convierte el estado interno en su etiqueta visible. */
static const char *estadoVisibleCPU(char estado) {

    switch (estado) {
        case ESTADO_LISTO:
            return "L";

        case ESTADO_EJECUTANDO:
            return "R";

        case ESTADO_ESPERA:
            return "E/S";

        case ESTADO_FINALIZADO:
            return "F";

        default:
            return "?";
    }
}

/* Intercambia dos punteros a procesos. */
static void intercambiarCPU(Proceso **a, Proceso **b) {
    Proceso *auxiliar = *a;
    *a = *b;
    *b = auxiliar;
}

/* Particiona el arreglo tomando el elemento central como pivote. */
static int particionarCPU(Proceso **arreglo, int inicio, int fin) {

    float pivote = arreglo[inicio + (fin - inicio) / 2]->uso_cpu;

    int i = inicio;
    int j = fin;

    while (i <= j) {

        while (arreglo[i]->uso_cpu > pivote) {
            i++;
        }

        while (arreglo[j]->uso_cpu < pivote) {
            j--;
        }

        if (i <= j) {
            intercambiarCPU(&arreglo[i], &arreglo[j]);
            i++;
            j--;
        }
    }

    return i;
}

/* Algoritmo Quick Sort descendente para CPU. */
static void quickSortCPU(Proceso **arreglo, int inicio, int fin) {

    if (inicio >= fin) {
        return;
    }

    int indice = particionarCPU(arreglo, inicio, fin);

    if (inicio < indice - 1) {
        quickSortCPU(arreglo, inicio, indice - 1);
    }

    if (indice < fin) {
        quickSortCPU(arreglo, indice, fin);
    }
}

/* Cuenta los procesos de la lista enlazada. */
static int contarProcesosCPU(Proceso *cabeza) {

    int contador = 0;

    while (cabeza != NULL) {
        contador++;
        cabeza = cabeza->siguiente;
    }

    return contador;
}

/* Ordena y muestra todos los procesos por uso de CPU. */
void ordenarProcesosPorCPU(Proceso *cabeza) {

    int cantidad = contarProcesosCPU(cabeza);

    if (cantidad == 0) {
        printf("\nNo hay procesos registrados.\n");
        return;
    }

    // Reserva un arreglo auxiliar de punteros.
    Proceso **arreglo = (Proceso **)malloc(
        cantidad * sizeof(Proceso *)
    );

    if (arreglo == NULL) {
        printf("\nError: no se pudo reservar memoria.\n");
        return;
    }

    // Copia las direcciones de los procesos al arreglo.
    int i = 0;

    while (cabeza != NULL) {
        arreglo[i] = cabeza;
        cabeza = cabeza->siguiente;
        i++;
    }

    // Ejecuta Quick Sort.
    quickSortCPU(arreglo, 0, cantidad - 1);

    // Muestra los resultados.
    printf("\n============================================\n");
    printf("     PROCESOS ORDENADOS POR CPU\n");
    printf("          ORDEN DESCENDENTE\n");
    printf("============================================\n");

    printf("%-8s %-16s %-12s %-12s %-10s\n",
           "PID", "PROCESO", "CPU (%)", "RAM (MB)", "ESTADO");

    printf("---------------------------------------------------------------\n");

    for (i = 0; i < cantidad; i++) {

        /* MODIFICADO: muestra la etiqueta del estado y no el carácter interno. */
        printf("%-8d %-16.16s %-12.1f %-12.0f %-10s\n",
               arreglo[i]->pid,
               arreglo[i]->nombre,
               arreglo[i]->uso_cpu,
               arreglo[i]->uso_ram,
               estadoVisibleCPU(arreglo[i]->estado));
    }

    printf("\nTotal de procesos: %d\n", cantidad);

    // Libera únicamente el arreglo auxiliar.
    free(arreglo);
}

#endif
