#ifndef PROCESO_H_INCLUDED
#define PROCESO_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Estados internos del simulador. */
#define ESTADO_LISTO 'L'
#define ESTADO_EJECUTANDO 'R'
#define ESTADO_ESPERA 'S'
#define ESTADO_SUSPENDIDO 'T'
#define ESTADO_FINALIZADO 'F'

/* MODIFICADO: planificador unico con quantum igual para todos. */
#define PLANIFICADOR_RR_PRIORIDAD 1
#define QUANTUM_RR 2

/* NUEVO: solicitudes en espera de RAM, sin ocupar RAM simulada. */
typedef struct {
    int pid;
    const char *nombre;
    float cpu;
    float ram;
    const char *usuario;
    int hilos;
    int tiempo;
    int prioridad;
    time_t llegada;
} Solicitud;

typedef struct NodoSolicitud {
    Solicitud datos;
    struct NodoSolicitud *siguiente;
} NodoSolicitud;

typedef struct Proceso {
    int pid;
    char nombre[64];
    char usuario[32];
    float uso_cpu;
    float uso_ram;
    int hilos;
    char estado;
    time_t horaCreacion;
    time_t horaFin;
    int tiempoRestante;
    int prioridad;
    int tiempoCPUAcumulado;
    time_t finEspera;
    int enCola;
    struct Proceso *siguiente;
} Proceso;

typedef struct Nodo {
    Proceso *proceso;
    struct Nodo *siguiente;
} Nodo;

typedef struct {
    Nodo *frente;
    Nodo *final;
} Cola;

typedef struct {
    int puerta;
    int ocupada;
    int procesoActual;
    char nombreProceso[50];
    char operacionActual[30];
    time_t finAtencion;
    time_t inicioAtencion;
    int planificador;
} Puerta;

void inicializarCola(Cola *q);
int estaVacia(Cola *q);
void encolarProceso(Cola *q, Proceso *procesoOriginal);
void liberarCola(Cola *q);
Proceso *crearProceso(int pid, const char *nombre, float cpu, float ram, char estado);
int insertarProceso(Proceso **cabeza, int pid, const char *nombre, float cpu, float ram, char estado);
int eliminarProcesoPorPid(Proceso **cabeza, int pid);
Proceso *buscarProcesoPorPid(Proceso *cabeza, int pid);
void liberarListaProcesos(Proceso **cabeza);
void atenderProcesoUnicaPuerta(Cola *q, Puerta *puertaUnica, Proceso *cabeza);
void actualizarPuerta(Cola *q, Puerta *puertaUnica, Proceso *cabeza);
void actualizarEsperas(Cola *q, Proceso *cabeza);
/* El planificador es fijo: ya no se cambia durante la simulacion. */
/* MODIFICADO: la tabla recibe la cola real para mostrar el orden de turnos. */
void mostrarHtop(Proceso *cabeza, Cola *q, Puerta *puertaUnica, time_t inicioSimulador, const NodoSolicitud *pendientes);
#endif // PROCESO_H_INCLUDED


