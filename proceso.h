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

/* Planificador MLFQ: cinco colas RR con quantum creciente. */
#define PLANIFICADOR_RR_PRIORIDAD 1
#define QUANTUM_ALTA 4
#define QUANTUM_MEDIA 8
#define QUANTUM_TERCERA 12
#define QUANTUM_CUARTA 16
#define QUANTUM_QUINTA 20
#define SEGUNDOS_AGING 40
#define NUM_COLAS 5

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
    int prioridad; /* prioridad actual usada por el planificador */
    int prioridadOriginal; /* prioridad asignada al ingresar */
    int tiempoCPUAcumulado;
    int proximaES; /* Siguiente umbral de E/S, revisado al terminar el turno */
    time_t finEspera;
    int enCola;
    int nivelCola; /* 0 a 4: prioridades 1 a 5 */
    int quantumConsumido; /* CPU gastada desde el ultimo cambio de nivel */
    time_t entradaCola;
    struct Proceso *siguiente;
} Proceso;

typedef struct Nodo {
    Proceso *proceso;
    struct Nodo *siguiente;
} Nodo;

typedef struct {
    Nodo *frente[NUM_COLAS];
    Nodo *final[NUM_COLAS];
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


