
#ifndef PROCESO_C_INCLUDED
#define PROCESO_C_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "proceso.h"

/* Cada 6 segundos de CPU, el proceso realiza E/S. */
#define INTERVALO_ES 6

/* La espera de entrada/salida dura 3 segundos. */
#define DURACION_ES 3

/* Un proceso finalizado se muestra durante 5 segundos. */
#define SEGUNDOS_FINALIZADO 5

/* Numero maximo de procesos mostrados en pantalla. */
#define MAX_FILAS_HTOP 15 /* MODIFICADO: mostrar hasta 15 procesos. */

/* Cantidad total de RAM simulada, en MB. */
#define RAM_SIMULADA 8192.0f


/* ==================================================
   1. COLA DE PROCESOS
   ================================================== */

/* Cinco colas FIFO independientes; la prioridad decide QUE cola atender. */
void inicializarCola(Cola *q) {
    int i;
    for (i = 0; i < NUM_COLAS; i++) q->frente[i] = q->final[i] = NULL;
}
int estaVacia(Cola *q) {
    int i;
    if (!q) return 1;
    for (i = 0; i < NUM_COLAS; i++) if (q->frente[i]) return 0;
    return 1;
}
void encolarProceso(Cola *q, Proceso *p) {
    Nodo *n;
    int nivel;
    if (!q || !p || p->estado != ESTADO_LISTO || p->enCola) return;
    nivel = p->nivelCola;
    if (nivel < 0 || nivel >= NUM_COLAS) nivel = p->nivelCola = 0;
    n = (Nodo *)malloc(sizeof(*n));
    if (!n) return;
    n->proceso = p;
    n->siguiente = NULL;
    if (q->final[nivel]) q->final[nivel]->siguiente = n;
    else q->frente[nivel] = n;
    q->final[nivel] = n;
    p->enCola = 1;
    p->entradaCola = time(NULL);
}
/* FIFO dentro del primer nivel no vacio: no se busca prioridad en una cola. */
static Proceso *desencolarSiguiente(Cola *q) {
    int i;
    Nodo *n;
    Proceso *p;
    for (i = 0; i < NUM_COLAS; i++) {
        if (!q->frente[i]) continue;
        n = q->frente[i];
        q->frente[i] = n->siguiente;
        if (!q->frente[i]) q->final[i] = NULL;
        p = n->proceso;
        free(n);
        p->enCola = 0;
        return p;
    }
    return NULL;
}
/* Extrae un proceso de su cola, conservando los demas en orden FIFO. */
static void retirarDeCola(Cola *q, Proceso *p) {
    int i;
    for (i = 0; i < NUM_COLAS; i++) {
        Nodo *n = q->frente[i], *ant = NULL;
        while (n) {
            if (n->proceso == p) {
                if (ant) ant->siguiente = n->siguiente;
                else q->frente[i] = n->siguiente;
                if (q->final[i] == n) q->final[i] = ant;
                free(n);
                p->enCola = 0;
                return;
            }
            ant = n;
            n = n->siguiente;
        }
    }
}
void liberarCola(Cola *q) {
    int i;
    for (i = 0; i < NUM_COLAS; i++) {
        Nodo *n = q->frente[i];
        while (n) {
            Nodo *sig = n->siguiente;
            if (n->proceso) n->proceso->enCola = 0;
            free(n);
            n = sig;
        }
        q->frente[i] = q->final[i] = NULL;
    }
}
/* Aging: subir un nivel despues de esperar 40 segundos en LISTO. */
static void aplicarEnvejecimiento(Cola *q, Proceso *cabeza) {
    Proceso *p;
    time_t ahora = time(NULL);
    for (p = cabeza; p; p = p->siguiente) {
        if (p->estado == ESTADO_LISTO && p->enCola && p->nivelCola > 0 &&
            difftime(ahora, p->entradaCola) >= SEGUNDOS_AGING) {
            retirarDeCola(q, p);
            p->nivelCola--;
            p->prioridad = p->nivelCola + 1;
            p->quantumConsumido = 0;
            /* encolarProceso reinicia entradaCola: otros 40 s para otra promocion. */
            encolarProceso(q, p);
        }
    }
}

/* ==================================================
   2. LISTA ENLAZADA DE PROCESOS
   ================================================== */

/* Crea un proceso con sus datos iniciales. */
Proceso *crearProceso(
    int pid,
    const char *nombre,
    float cpu,
    float ram,
    char estado
) {
    Proceso *nuevo;

    /* Reserva memoria para un proceso. */
    nuevo = (Proceso *)malloc(sizeof(Proceso));

    /* Comprueba que la memoria este disponible. */
    if (nuevo == NULL) {
        return NULL;
    }

    /* Asigna el identificador del proceso. */
    nuevo->pid = pid;

    /* Copia el nombre sin desbordar el arreglo. */
    strncpy(
        nuevo->nombre,
        nombre,
        sizeof(nuevo->nombre) - 1
    );

    /* Asegura el fin de la cadena. */
    nuevo->nombre[
        sizeof(nuevo->nombre) - 1
    ] = '\0';

    /* Inicializa el nombre del usuario. */
    nuevo->usuario[0] = '\0';

    /* Guarda los recursos simulados. */
    nuevo->uso_cpu = cpu;
    nuevo->uso_ram = ram;

    /* Inicializa los hilos y el estado. */
    nuevo->hilos = 1;
    nuevo->estado = estado;

    /* Guarda la fecha de creacion. */
    nuevo->horaCreacion = time(NULL);

    /* Todavia no ha finalizado. */
    nuevo->horaFin = 0;

    /* Inicializa el tiempo de trabajo pendiente. */
    nuevo->tiempoRestante = 0;

    /*
     * Asigna una prioridad aleatoria del 1 al 5.
     * El numero 1 representa la mayor prioridad.
     */
    nuevo->prioridad = 1; /* Valor provisional; main asigna prioridad de solicitud. */
    nuevo->prioridadOriginal = nuevo->prioridad;
    nuevo->nivelCola = 0;
    nuevo->quantumConsumido = 0;
    nuevo->entradaCola = time(NULL);

    /* Inicializa el tiempo acumulado de CPU. */
    nuevo->tiempoCPUAcumulado = 0;
    nuevo->proximaES = INTERVALO_ES;

    /* Inicializa la espera de entrada/salida. */
    nuevo->finEspera = 0;

    /* El proceso todavia no esta en una cola. */
    nuevo->enCola = 0;

    /* No existe un siguiente proceso todavia. */
    nuevo->siguiente = NULL;

    return nuevo;
}

/* Inserta un proceso al final de la lista enlazada. */
int insertarProceso(
    Proceso **cabeza,
    int pid,
    const char *nombre,
    float cpu,
    float ram,
    char estado
) {
    Proceso *nuevo;
    Proceso *actual;

    /* Crea el nuevo proceso. */
    nuevo = crearProceso(
        pid, nombre, cpu, ram, estado
    );

    /* Comprueba que se haya creado correctamente. */
    if (nuevo == NULL) {
        return 0;
    }

    /* Si la lista esta vacia, sera el primero. */
    if (*cabeza == NULL) {
        *cabeza = nuevo;
    } else {

        /* Busca el ultimo proceso de la lista. */
        actual = *cabeza;

        while (actual->siguiente != NULL) {
            actual = actual->siguiente;
        }

        /* Conecta el proceso nuevo al final. */
        actual->siguiente = nuevo;
    }

    return 1;
}

/* Busca un proceso por su PID. */
Proceso *buscarProcesoPorPid(
    Proceso *cabeza,
    int pid
) {
    /* Recorre la lista buscando el identificador. */
    while (cabeza != NULL) {

        /* Devuelve el proceso si encuentra su PID. */
        if (cabeza->pid == pid) {
            return cabeza;
        }

        cabeza = cabeza->siguiente;
    }

    /* No se encontro el proceso. */
    return NULL;
}

/* Elimina de la lista un proceso finalizado. */
int eliminarProcesoPorPid(
    Proceso **cabeza,
    int pid
) {
    Proceso *actual = *cabeza;
    Proceso *anterior = NULL;

    /* Busca el proceso que se desea eliminar. */
    while (actual != NULL && actual->pid != pid) {
        anterior = actual;
        actual = actual->siguiente;
    }

    /* No existe un proceso con ese PID. */
    if (actual == NULL) {
        return 0;
    }

    /*
     * Proteccion:
     * No elimina procesos activos ni encolados.
     */
    if (actual->estado != ESTADO_FINALIZADO ||
        actual->enCola) {
        return 0;
    }

    /* Desconecta el proceso de la lista. */
    if (anterior == NULL) {
        *cabeza = actual->siguiente;
    } else {
        anterior->siguiente = actual->siguiente;
    }

    /* Libera su memoria. */
    free(actual);

    return 1;
}

/* Libera todos los procesos al cerrar el programa. */
void liberarListaProcesos(Proceso **cabeza) {
    Proceso *actual = *cabeza;

    /* Recorre y libera todos los elementos. */
    while (actual != NULL) {
        Proceso *siguiente = actual->siguiente;

        free(actual);
        actual = siguiente;
    }

    /* Deja vacia la lista principal. */
    *cabeza = NULL;
}


/* ==================================================
   3. PLANIFICADORES ROUND ROBIN Y PRIORIDAD
   ================================================== */

/* Las cinco colas usan Round Robin; la quinta rota sin descender. */
static int quantumPorNivel(int nivel) {
    static const int quantum[NUM_COLAS] = {
        QUANTUM_ALTA, QUANTUM_MEDIA, QUANTUM_TERCERA, QUANTUM_CUARTA, QUANTUM_QUINTA
    };
    if (nivel < 0 || nivel >= NUM_COLAS) return 0;
    return quantum[nivel];
}

/* MLFQ de cinco prioridades, de 1 (alta) a 5 (baja). */
void atenderProcesoUnicaPuerta(Cola *q, Puerta *cpu, Proceso *cabeza) {
    Proceso *p;
    int duracion, restanteQuantum;
    aplicarEnvejecimiento(q, cabeza);
    if (cpu->ocupada || estaVacia(q)) return;
    p = desencolarSiguiente(q);
    if (!p || p->estado != ESTADO_LISTO) return;
    if (p->tiempoRestante <= 0) {
        p->tiempoRestante = 0;
        p->estado = ESTADO_FINALIZADO;
        p->horaFin = time(NULL);
        return;
    }
    duracion = p->tiempoRestante;
    {
        restanteQuantum = quantumPorNivel(p->nivelCola) - p->quantumConsumido;
        if (restanteQuantum < 1) restanteQuantum = 1;
        if (duracion > restanteQuantum) duracion = restanteQuantum;
    }
    /* Turno indivisible: la E/S no interrumpe el quantum en curso. */
    cpu->ocupada = 1;
    cpu->procesoActual = p->pid;
    snprintf(cpu->nombreProceso, sizeof(cpu->nombreProceso), "%s", p->nombre);
    snprintf(cpu->operacionActual, sizeof(cpu->operacionActual), "CPU: %.1f%%", p->uso_cpu);
    cpu->inicioAtencion = time(NULL);
    cpu->finAtencion = cpu->inicioAtencion + duracion;
    p->estado = ESTADO_EJECUTANDO;
}

/*
 * Actualiza el proceso que esta usando la CPU.
 *
 * Puede pasar a:
 * FINALIZADO, ESPERA o LISTO.
 */
void actualizarPuerta(
    Cola *q,
    Puerta *cpu,
    Proceso *cabeza
) {
    Proceso *proceso;
    time_t ahora;
    time_t limite;
    int transcurrido;

    /* No hay nada que actualizar si la CPU esta libre. */
    if (!cpu->ocupada) {
        return;
    }

    /* Obtiene la hora actual. */
    ahora = time(NULL);

    /* Busca el proceso que esta en ejecucion. */
    proceso = buscarProcesoPorPid(
        cabeza,
        cpu->procesoActual
    );

    /* Proteccion si no se encuentra el proceso. */
    if (proceso == NULL) {
        cpu->ocupada = 0;
        cpu->procesoActual = 0;
        return;
    }

    /* No expropiar por prioridad: un proceso nuevo o promovido por aging
       debe esperar hasta que termine el turno del proceso actual. */
    aplicarEnvejecimiento(q, cabeza);
    /* Limita el tiempo al final del turno. */
    limite = ahora;

    if (limite > cpu->finAtencion) {
        limite = cpu->finAtencion;
    }

    /* Avanza exactamente una unidad de CPU por actualizacion.
       Si el reloj salto varios segundos, los recupera en ciclos
       separados sin descontarlos todos de golpe. */
    transcurrido = (int)difftime(limite, cpu->inicioAtencion);
    if (transcurrido > 1) transcurrido = 1;

    /* Evita tiempos negativos. */
    if (transcurrido < 0) {
        transcurrido = 0;
    }

    /* No descuenta mas tiempo del que falta. */
    if (transcurrido > proceso->tiempoRestante) {
        transcurrido = proceso->tiempoRestante;
    }

    /* Descuenta el trabajo realizado. */
    proceso->tiempoRestante -= transcurrido;

    /* Acumula el tiempo de CPU utilizado. */
    proceso->tiempoCPUAcumulado += transcurrido;
    proceso->quantumConsumido += transcurrido;

    /* Evita contar dos veces el mismo segundo. */
    cpu->inicioAtencion += transcurrido;

    /* Si el turno continua, no cambia de proceso. */
    if (cpu->inicioAtencion < cpu->finAtencion) {
        return;
    }

    /*
     * CASO 1:
     * El proceso ya termino todo su trabajo.
     */
    if (proceso->tiempoRestante <= 0) {

        proceso->tiempoRestante = 0;

        /* Marca el proceso como finalizado. */
        proceso->estado = ESTADO_FINALIZADO;

        /* Guarda la hora para mostrarlo 5 segundos. */
        proceso->horaFin = ahora;

    /*
     * CASO 2:
     * El proceso debe realizar entrada/salida.
     */
    } else if (
        proceso->tiempoCPUAcumulado >= proceso->proximaES
    ) {

        /* Se conserva el consumo del quantum entre bloqueos de E/S,
           para permitir que eventualmente se alcance el umbral del nivel. */
        if (proceso->quantumConsumido >= quantumPorNivel(proceso->nivelCola)) {
            if (proceso->nivelCola < NUM_COLAS - 1) proceso->nivelCola++;
            proceso->prioridad = proceso->nivelCola + 1;
            proceso->quantumConsumido = 0;
        }
        /* Reprograma el siguiente evento de E/S para otro turno. */
        while (proceso->proximaES <= proceso->tiempoCPUAcumulado)
            proceso->proximaES += INTERVALO_ES;
        /* Cambia el estado a ESPERA. */
        proceso->estado = ESTADO_ESPERA;

        /* Programa el final de la espera. */
        proceso->finEspera = ahora + DURACION_ES;

    /*
     * CASO 3:
     * El proceso aun tiene trabajo pendiente.
     */
    } else {

        /* Vuelve a estar listo. */
        proceso->estado = ESTADO_LISTO;

        /* Si agoto su quantum, baja de nivel o rota en cola 5.
           Si fue desalojado, conserva el quantum restante. */
        if (proceso->quantumConsumido >= quantumPorNivel(proceso->nivelCola)) {
            if (proceso->nivelCola < NUM_COLAS - 1) proceso->nivelCola++;
            proceso->prioridad = proceso->nivelCola + 1;
            proceso->quantumConsumido = 0;
        }
        /* Regresa a la cola para otro turno. */
        encolarProceso(q, proceso);
    }

    /* Libera la CPU para atender otro proceso. */
    cpu->ocupada = 0;
    cpu->procesoActual = 0;

    /* Limpia los datos del proceso anterior. */
    cpu->nombreProceso[0] = '\0';
    cpu->operacionActual[0] = '\0';
}

/* Reanuda los procesos que terminaron su espera de E/S. */
void actualizarEsperas(
    Cola *q,
    Proceso *cabeza
) {
    time_t ahora = time(NULL);

    /* Recorre la lista de procesos. */
    while (cabeza != NULL) {

        /* Comprueba si termino su espera. */
        if (cabeza->estado == ESTADO_ESPERA &&
            ahora >= cabeza->finEspera) {

            /* El proceso ya no esta esperando E/S. */
            cabeza->finEspera = 0;

            /* Regresa al estado LISTO. */
            cabeza->estado = ESTADO_LISTO;

            /* Se coloca nuevamente en la cola. */
            encolarProceso(q, cabeza);
        }

        /* Continua con el siguiente proceso. */
        cabeza = cabeza->siguiente;
    }
}


/* ==================================================
   4. FUNCIONES PARA MOSTRAR LA INTERFAZ HTOP
   ================================================== */

/* Convierte una cantidad de segundos a HH:MM:SS. */
static void convertirTiempo(
    long segundos,
    char *salida,
    size_t tamano
) {
    long horas;
    long minutos;
    long seg;

    /* No permite tiempos negativos. */
    if (segundos < 0) {
        segundos = 0;
    }

    /* Calcula horas, minutos y segundos. */
    horas = segundos / 3600;
    minutos = (segundos % 3600) / 60;
    seg = segundos % 60;

    /* Escribe el tiempo en el arreglo de salida. */
    snprintf(
        salida,
        tamano,
        "%02ld:%02ld:%02ld",
        horas,
        minutos,
        seg
    );
}

/* Crea una barra de 20 caracteres para CPU o RAM. */
static void dibujarBarra(
    float porcentaje,
    char *salida,
    size_t tamano
) {
    int i;
    int llenas;

    /* Comprueba que el arreglo tenga espacio. */
    if (tamano < 21) {
        return;
    }

    /* Ajusta los porcentajes al rango permitido. */
    if (porcentaje < 0.0f) {
        porcentaje = 0.0f;
    }

    if (porcentaje > 100.0f) {
        porcentaje = 100.0f;
    }

    /* Calcula cuantos caracteres deben llenarse. */
    llenas = (int)(porcentaje * 20.0f / 100.0f);

    /* Construye la barra visual. */
    for (i = 0; i < 20; i++) {
        salida[i] = (i < llenas) ? '#' : '-';
    }

    /* Termina la cadena. */
    salida[20] = '\0';
}

/*
 * Determina si un proceso debe mostrarse.
 *
 * Los procesos activos siempre son visibles.
 * Los finalizados se muestran durante 5 segundos.
 */
static int procesoVisible(
    Proceso *proceso,
    time_t ahora
) {
    /* Un proceso activo puede mostrarse. */
    if (proceso->estado != ESTADO_FINALIZADO) {
        return 1;
    }

    /* Si no tiene fecha de fin, no se muestra. */
    if (proceso->horaFin == 0) {
        return 0;
    }

    /* Comprueba si todavia esta dentro de los 5 segundos. */
    return difftime(
        ahora,
        proceso->horaFin
    ) <= SEGUNDOS_FINALIZADO;
}

/* Convierte el estado del proceso a texto. */
static const char *nombreEstado(char estado) {

    if (estado == ESTADO_LISTO) {
        return "L";
    }

    if (estado == ESTADO_EJECUTANDO) {
        return "R";
    }

    if (estado == ESTADO_ESPERA) {
        return "E/S";
    }

    if (estado == ESTADO_SUSPENDIDO) {
        return "SUSPENDIDO";
    }

    if (estado == ESTADO_FINALIZADO) {
        return "F";
    }

    return "DESCONOCIDO";
}

/*
 * MODIFICADO:
 * Muestra una fila con todos los datos del proceso.
 *
 * NUEVO:
 * Se agrega la columna PRIO.
 *
 * PRIO 1 = mayor prioridad.
 * PRIO 5 = menor prioridad.
 */
static void mostrarFilaProceso(Proceso *proceso) {
    char tiempo[20];

    /* CORREGIDO: muestra el tiempo restante en RR y Prioridad. */
    convertirTiempo(
        proceso->tiempoRestante,
        tiempo,
        sizeof(tiempo)
    );

    /*
     * Muestra las columnas:
     * PID, USUARIO, PROCESO, CPU%, RAM(MB),
     * HILOS, PRIO, TIEMPO y ESTADO.
     */
    printf(
        "%-6d %-12.12s %-15.15s %5.1f %8.0f "
        "%5d %4d %4d %-9s %-10s\n",
        proceso->pid,
        proceso->usuario,
        proceso->nombre,
        proceso->uso_cpu,
        proceso->uso_ram,
        proceso->hilos,
        proceso->prioridadOriginal,
        proceso->prioridad,
        tiempo,
        nombreEstado(proceso->estado)
    );
}

/*
 * Agrega un proceso al arreglo de filas visibles.
 * Evita que el mismo proceso aparezca dos veces.
 */
static void agregarFila(
    Proceso *filas[],
    int *cantidad,
    Proceso *proceso
) {
    int i;

    /* No agrega punteros nulos. */
    if (proceso == NULL) {
        return;
    }

    /* Comprueba que no este agregado anteriormente. */
    for (i = 0; i < *cantidad; i++) {
        if (filas[i] == proceso) {
            return;
        }
    }

    /* Solo agrega si queda espacio en la pantalla. */
    if (*cantidad < MAX_FILAS_HTOP) {
        filas[*cantidad] = proceso;
        (*cantidad)++;
    }
}

/*
 * Muestra la interfaz principal tipo HTOP.
 *
 * Incluye:
 * - Planificador elegido
 * - Tiempo activo del simulador
 * - Procesos activos y finalizados
 * - Hilos totales y en ejecucion
 * - Uso de CPU y RAM
 * - Tabla con la columna PRIO
 *
 * No modifica la lista original de procesos.
 */
void mostrarHtop(
    Proceso *cabeza,
    Cola *q, /* MODIFICADO: acceso al orden real de la cola de listos. */
    Puerta *cpu,
    time_t inicioSimulador,
    const NodoSolicitud *pendientes /* NUEVO: cola de espera por RAM. */
) {
    Proceso *actual;
    Proceso *procesoCPU = NULL;

    /* Arreglo auxiliar para los procesos visibles. */
    Proceso *filas[MAX_FILAS_HTOP] = {NULL};

    /* Contadores de los estados. */
    int activos = 0;
    int listos = 0;
    int ejecutando = 0;
    int esperando = 0;
    int finalizados = 0;

    /* Contadores de hilos. */
    int hilosTotales = 0;
    int hilosEjecutando = 0;

    /* Cantidad de filas mostradas. */
    int cantidadFilas = 0;
    int i;

    /* Variables para los recursos simulados. */
    float ramActiva = 0.0f;
    float cpuActual = 0.0f;
    float ramPorcentaje;

    /* Arreglos para las barras y el tiempo. */
    char barraCPU[21];
    char barraRAM[21];
    char uptime[20];

    /* Obtiene la hora actual. */
    time_t ahora = time(NULL);

    /* ==================================================
       CONTAR PROCESOS Y RECURSOS
       ================================================== */

    actual = cabeza;

    /* Recorre toda la lista de procesos. */
    while (actual != NULL) {

        /* Los finalizados ya no consumen RAM. */
        if (actual->estado == ESTADO_FINALIZADO) {

            finalizados++;

        } else {

            /* Cuenta los procesos que siguen activos. */
            activos++;

            /* Suma su consumo de RAM simulada. */
            ramActiva += actual->uso_ram;

            /* Suma sus hilos. */
            hilosTotales += actual->hilos;

            /* Cuenta los procesos LISTOS. */
            if (actual->estado == ESTADO_LISTO) {

                listos++;

            /* Cuenta el proceso que usa la CPU. */
            } else if (
                actual->estado == ESTADO_EJECUTANDO
            ) {

                ejecutando++;
                procesoCPU = actual;

                /* Cuenta sus hilos en ejecucion. */
                hilosEjecutando += actual->hilos;

            /* Cuenta los procesos esperando. */
            } else if (
                actual->estado == ESTADO_ESPERA ||
                actual->estado == ESTADO_SUSPENDIDO
            ) {

                esperando++;
            }
        }

        /* Continua con el siguiente proceso. */
        actual = actual->siguiente;
    }

    /* ==================================================
       CALCULAR CPU, RAM Y UPTIME
       ================================================== */

    /* Obtiene el CPU% del proceso en ejecucion. */
    if (cpu->ocupada && procesoCPU != NULL) {
        cpuActual = procesoCPU->uso_cpu;
    }

    /* Calcula el porcentaje de RAM utilizada. */
    ramPorcentaje =
        (ramActiva / RAM_SIMULADA) * 100.0f;

    /* Dibuja la barra de CPU. */
    dibujarBarra(
        cpuActual,
        barraCPU,
        sizeof(barraCPU)
    );

    /* Dibuja la barra de RAM. */
    dibujarBarra(
        ramPorcentaje,
        barraRAM,
        sizeof(barraRAM)
    );

    /* Calcula el tiempo desde el inicio. */
    convertirTiempo(
        (long)difftime(ahora, inicioSimulador),
        uptime,
        sizeof(uptime)
    );

    /* ==================================================
       ENCABEZADO DE LA INTERFAZ
       ================================================== */

    printf(
        "SIMULADOR TOP / LINUX"
        "                  Uptime: %s\n",
        uptime
    );

    /* MODIFICADO: un solo planificador con quantum fijo. */
    printf("MLFQ | C1 RR 4s | C2 RR 8s | C3 RR 12s | C4 RR 16s | C5 RR 20s | Aging 40s\n");

    /* Muestra los contadores de procesos activos. */
    printf(
        "Tareas: %d activas, %d ejecutando, "
        "%d listas, %d esperando\n",
        activos,
        ejecutando,
        listos,
        esperando
    );

    /* Muestra los procesos terminados del historial. */
    printf(
        "Terminadas en historial: %d\n",
        finalizados
    );

    /* Muestra los hilos de los procesos activos. */
    printf(
        "Hilos: %d totales, %d en ejecucion\n",
        hilosTotales,
        hilosEjecutando
    );

    /* Barra del porcentaje de CPU simulado. */
    printf(
        "CPU: [%s] %5.1f%%\n",
        barraCPU,
        cpuActual
    );

    /* Barra y consumo de RAM simulada. */
    printf(
        "RAM: [%s] %5.1f%% "
        "(%.0f / %.0f MB simulados)\n",
        barraRAM,
        ramPorcentaje,
        ramActiva,
        RAM_SIMULADA
    );

    printf("\n");

    /* ==================================================
       ENCABEZADO DE LA TABLA DE PROCESOS
       ================================================== */

    /*
     * MODIFICADO:
     * Se agrega PRIO despues de HILOS.
     */
    printf(
        "%-6s %-12s %-15s %5s %8s "
        "%5s %4s %4s %-9s %-10s\n",
        "PID",
        "USUARIO",
        "PROCESO",
        "CPU%",
        "RAM(MB)",
        "HILOS",
        "ORIG",
        "PRIO",
        "RESTANTE",
        "ESTADO"
    );

    /* Linea separadora de las columnas. */
    printf(
        "----------------------------------------"
        "----------------------------------------"
        "----------\n"
    );

    /* ==================================================
       SELECCIONAR LOS PROCESOS VISIBLES
       ================================================== */

    /* MODIFICADO: primero aparece el proceso que ocupa la CPU. */
    agregarFila(filas, &cantidadFilas, procesoCPU);

    /* Mostrar los listos por colas reales, respetando FIFO en cada nivel. */
    {
        int nivel;
        for (nivel = 0; nivel < NUM_COLAS; nivel++) {
            Nodo *nodoActual = q ? q->frente[nivel] : NULL;
            while (nodoActual && cantidadFilas < MAX_FILAS_HTOP) {
                if (nodoActual->proceso &&
                    nodoActual->proceso->estado == ESTADO_LISTO)
                    agregarFila(filas, &cantidadFilas, nodoActual->proceso);
                nodoActual = nodoActual->siguiente;
            }
        }
    }

    /* MODIFICADO: despues aparecen los procesos en espera/suspendidos. */
    actual = cabeza;
    while (actual != NULL && cantidadFilas < MAX_FILAS_HTOP) {
        if (actual->estado == ESTADO_ESPERA ||
            actual->estado == ESTADO_SUSPENDIDO) {
            agregarFila(filas, &cantidadFilas, actual);
        }
        actual = actual->siguiente;
    }

    /* MODIFICADO: reservar una fila para ESP si hay pendientes. */
    if (pendientes != NULL && cantidadFilas >= MAX_FILAS_HTOP) {
        cantidadFilas = MAX_FILAS_HTOP - 1;
    }
    /* Los finalizados recientes aparecen al final. */
    actual = cabeza;
    while (actual != NULL && cantidadFilas < MAX_FILAS_HTOP - (pendientes != NULL ? 1 : 0)) {
        if (actual->estado == ESTADO_FINALIZADO &&
            procesoVisible(actual, ahora)) {
            agregarFila(filas, &cantidadFilas, actual);
        }
        actual = actual->siguiente;
    }

    /* ==================================================
       MOSTRAR LOS PROCESOS SELECCIONADOS
       ================================================== */

    /* Imprime las filas con la nueva columna PRIO. */
    for (i = 0; i < cantidadFilas; i++) {
        mostrarFilaProceso(filas[i]);
    }

    /* NUEVO: solicitudes ESP sin RAM consumida ni duplicar procesos. */
    {
        const NodoSolicitud *pendiente = pendientes;
        while (pendiente != NULL && cantidadFilas < MAX_FILAS_HTOP) {
            char restante[20];
            convertirTiempo(pendiente->datos.tiempo, restante, sizeof(restante));
            printf("%-6d %-12.12s %-15.15s %5.1f %8.0f %5d %4d %4d %-9s %-10s\n",
                   pendiente->datos.pid, pendiente->datos.usuario,
                   pendiente->datos.nombre, pendiente->datos.cpu,
                   pendiente->datos.ram, pendiente->datos.hilos,
                   pendiente->datos.prioridad, pendiente->datos.prioridad, restante, "ESP");
            cantidadFilas++;
            pendiente = pendiente->siguiente;
        }
    }

    /* Mensaje cuando no hay procesos para mostrar. */
    if (cantidadFilas == 0) {
        printf("No hay procesos activos.\n");
    }

    /* Indica cuantos procesos aparecen en la tabla. */
    printf(
        "\nFilas mostradas: %d | Procesos activos: %d\n",
        cantidadFilas,
        activos
    );
}

#endif /* PROCESO_C_INCLUDED */
