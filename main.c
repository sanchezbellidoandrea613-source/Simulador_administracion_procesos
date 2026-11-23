#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#include "proceso.h"
#include "hash.h"
#include "quicksort_cpu.h"
#include "quicksort_ram.h"

/* Limites de la simulacion. */
#define RAM_TOTAL 8192.0f
#define MAX_HISTORIAL_FINALIZADOS 40
#define INTERVALO_PANTALLA_MS 500

/* MODIFICADO: Solicitud y NodoSolicitud ahora estan en proceso.h. */

/* Agrega una solicitud al final de la cola FIFO de admision.
 * No existe un limite artificial de 100 solicitudes. */
static int guardarSolicitud(NodoSolicitud **frente,
                           NodoSolicitud **ultimo,
                           Solicitud solicitud) {
    NodoSolicitud *nuevo = (NodoSolicitud *)malloc(sizeof(NodoSolicitud));
    if (nuevo == NULL) return 0;
    nuevo->datos = solicitud;
    nuevo->siguiente = NULL;
    if (*ultimo != NULL) (*ultimo)->siguiente = nuevo;
    else *frente = nuevo;
    *ultimo = nuevo;
    return 1;
}

/* Libera las solicitudes que aun no se han admitido al salir. */
static void liberarSolicitudes(NodoSolicitud **frente) {
    while (*frente != NULL) {
        NodoSolicitud *viejo = *frente;
        *frente = viejo->siguiente;
        free(viejo);
    }
}

/* Suma la RAM de los procesos que no han terminado. */
static float calcularRAMActiva(Proceso *cabeza) {
    float ram = 0.0f;

    while (cabeza != NULL) {
        if (cabeza->estado != ESTADO_FINALIZADO) {
            ram += cabeza->uso_ram;
        }
        cabeza = cabeza->siguiente;
    }

    return ram;
}

/*
 * Conserva un historial limitado de procesos terminados.
 * Los procesos activos nunca se eliminan aqui.
 */
static void limpiarHistorialFinalizados(Proceso **cabeza) {
    Proceso *actual = *cabeza;
    int finalizados = 0;

    /* Cuenta los procesos terminados. */
    while (actual != NULL) {
        if (actual->estado == ESTADO_FINALIZADO) {
            finalizados++;
        }
        actual = actual->siguiente;
    }

    /* Elimina los registros mas antiguos si hay mas de 40. */
    while (finalizados > MAX_HISTORIAL_FINALIZADOS) {
        int pidEliminar = -1;

        actual = *cabeza;

        while (actual != NULL) {
            if (actual->estado == ESTADO_FINALIZADO) {
                pidEliminar = actual->pid;
                break;
            }
            actual = actual->siguiente;
        }

        if (pidEliminar == -1) {
            break;
        }

        /* Elimina primero la referencia de Hash. */
        eliminarHashPorPid(pidEliminar);

        /* Despues libera el proceso de la lista. */
        if (eliminarProcesoPorPid(cabeza, pidEliminar)) {
            finalizados--;
        } else {
            break;
        }
    }
}

/* Obtiene la consola de Windows. */
static HANDLE obtenerConsola(void) {
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

/* Oculta o muestra el cursor segun sea necesario. */
static void cambiarVisibilidadCursor(int visible) {
    HANDLE consola = obtenerConsola();
    CONSOLE_CURSOR_INFO info;

    if (GetConsoleCursorInfo(consola, &info)) {
        info.bVisible = visible ? TRUE : FALSE;
        SetConsoleCursorInfo(consola, &info);
    }
}

/* NUEVO: Windows Terminal admite secuencias ANSI de pantalla alternativa. */
static int pantallaAlternativa = 0;

/* Activa el modo VT y entra a una pantalla independiente del historial.
 * Esto evita que las actualizaciones se acumulen en el scrollback. */
static void iniciarPantallaSimulador(void) {
    HANDLE consola = obtenerConsola();
    DWORD modo;
    if (GetConsoleMode(consola, &modo) &&
        SetConsoleMode(consola, modo | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        pantallaAlternativa = 1;
        printf("\x1b[?1049h\x1b[?25l");
        fflush(stdout);
    }
}

/* Redibuja desde la esquina superior sin agregar nuevas pantallas.
 * Si VT no esta disponible, usa el borrado clasico de Windows. */
static void limpiarPantallaConsola(void) {
    if (pantallaAlternativa) {
        printf("\x1b[H\x1b[2J");
        fflush(stdout);
    } else {
        system("cls");
    }
}

/* Restaura el terminal y su historial original al salir. */
static void finalizarPantallaSimulador(void) {
    if (pantallaAlternativa) {
        printf("\x1b[?25h\x1b[?1049l");
        fflush(stdout);
        pantallaAlternativa = 0;
    }
}

/*
 * Compensa el tiempo de las consultas de Hash y Quick Sort.
 * Asi los procesos no avanzan mientras el usuario lee.
 */
static void compensarPausa(
    Proceso *cabeza,
    Puerta *cpu,
    time_t inicio,
    time_t fin,
    time_t *siguienteCreacion
) {
    time_t duracion = fin - inicio;
    Proceso *actual = cabeza;

    if (duracion <= 0) {
        return;
    }

    /* Ajusta el turno actual de la CPU. */
    if (cpu->ocupada) {
        cpu->inicioAtencion += duracion;
        cpu->finAtencion += duracion;
    }

    /* Ajusta los procesos que esperan E/S. */
    while (actual != NULL) {
        if (actual->estado == ESTADO_ESPERA) {
            actual->finEspera += duracion;
        }
        actual = actual->siguiente;
    }

    /* Ajusta la siguiente creacion automatica. */
    *siguienteCreacion += duracion;
}

/* ==================================================
   FUNCION PRINCIPAL
   ================================================== */

int main(void) {
    Cola unifila;
    Proceso *cabezaProcesos = NULL;
    Puerta puertaUnica;

    time_t inicioSimulador;
    time_t siguienteCreacion;

    int pidActual = 1001;
    int salir = 0;
    /* Cola dinamica: no limita artificialmente los procesos admitidos. */
    NodoSolicitud *primeraSolicitud = NULL;
    NodoSolicitud *ultimaSolicitud = NULL;
    unsigned long cantidadPendientes = 0;
    unsigned long generados = 0, admitidos = 0, descartados = 0;

    DWORD ultimaPantalla = 0;

    /*
     * Catalogo original de procesos.
     * Se conservan todos los nombres.
     */
    const char *nombresProcesos[] = {
        "bash", "firefox", "gcc", "vim", "python",
        "htop", "gedit", "ssh", "docker", "chrome",
        "nginx", "apache2", "mysql", "postgres",
        "redis", "node", "java", "javac", "php", "ruby",
        "perl", "git", "make", "cmake", "gdb",
        "code", "netbeans", "eclipse", "idea", "sublime",
        "nano", "emacs", "cat", "grep", "find",
        "ls", "cp", "mv", "rm", "mkdir", "tar", "gzip",
        "curl", "wget", "ping", "top", "ps",
        "free", "df", "du", "systemd", "cron", "rsyslog",
        "dbus-daemon", "sshd", "NetworkManager",
        "bluetoothd", "cupsd", "polkitd", "udisksd",
        "pulseaudio", "pipewire", "Xorg", "wayland",
        "gnome-shell", "nautilus", "thunderbird", "vlc",
        "audacity", "gimp", "inkscape", "libreoffice",
        "evince", "okular", "filezilla", "transmission",
        "qbittorrent", "telegram", "discord",
        "slack", "spotify", "obs", "ffmpeg",
        "convert", "identify", "mongod", "mariadb",
        "elasticsearch", "rabbitmq", "memcached",
        "tomcat", "wildfly", "uwsgi", "gunicorn",
        "flask", "pytest", "jupyter", "ipython",
        "npm", "yarn"
    };

    /* Catalogo original de usuarios. */
    const char *usuarios[] = {
        "root", "elvis", "daemon", "www-data",
        "postgres", "nobody", "sys", "usuario"
    };

    int totalNombres = (int)(
        sizeof(nombresProcesos) /
        sizeof(nombresProcesos[0])
    );

    int totalUsuarios = (int)(
        sizeof(usuarios) /
        sizeof(usuarios[0])
    );

    /* Inicializa la generacion aleatoria. */
    srand((unsigned int)time(NULL));

    /* MODIFICADO: se inicia directamente con el planificador combinado. */
    /* Inicializa la cola y la tabla Hash. */
    inicializarCola(&unifila);
    inicializarHash();

    /* Inicializa la unica CPU simulada. */
    memset(&puertaUnica, 0, sizeof(puertaUnica));
    puertaUnica.puerta = 1;

    /* El planificador queda fijo durante la corrida. */
    puertaUnica.planificador = PLANIFICADOR_RR_PRIORIDAD;

    /* Inicia el reloj de la simulacion. */
    inicioSimulador = time(NULL);
    siguienteCreacion = inicioSimulador;

    /* NUEVO: usa una pantalla alternativa para evitar tablas apiladas. */
    iniciarPantallaSimulador();
    limpiarPantallaConsola();
    cambiarVisibilidadCursor(0);

    /* ==================================================
       CICLO PRINCIPAL DEL SIMULADOR
       ================================================== */

    while (!salir) {
        time_t ahora = time(NULL);

        /* Actualiza el proceso que esta en CPU. */
        actualizarPuerta(
            &unifila,
            &puertaUnica,
            cabezaProcesos
        );

        /* Despierta procesos que terminaron E/S. */
        actualizarEsperas(
            &unifila,
            cabezaProcesos
        );

        /* Libera historial antiguo si supera 40. */
        limpiarHistorialFinalizados(
            &cabezaProcesos
        );

        /* GENERACION CONTINUA: una solicitud cada 1 a 5 segundos. */
        if (ahora >= siguienteCreacion) {
            Solicitud solicitud;
            solicitud.pid = pidActual++;
            solicitud.nombre = nombresProcesos[rand() % totalNombres];
            solicitud.cpu = (float)(rand() % 461 + 20) / 10.0f;
            solicitud.ram = (float)(rand() % 193 + 64);
            solicitud.usuario = usuarios[rand() % totalUsuarios];
            solicitud.hilos = rand() % 8 + 1;
            solicitud.tiempo = rand() % 16 + 5;
            solicitud.prioridad = rand() % NUM_COLAS + 1; /* Prioridad inicial 1..5. */
            solicitud.llegada = ahora;
            generados++;

            /* Las solicitudes siguen llegando aunque la RAM este ocupada.
             * Si falla la reserva REAL de memoria, se registra un rechazo. */
            if (guardarSolicitud(&primeraSolicitud, &ultimaSolicitud,
                                solicitud)) {
                cantidadPendientes++;
            } else {
                descartados++;
            }
            siguienteCreacion = ahora + (rand() % 5 + 1);
        }

        /* Admite por orden de llegada, solo si hay RAM simulada disponible.
         * No hay limite fijo de 12 procesos activos. */
        while (primeraSolicitud != NULL) {
            Solicitud *solicitud = &primeraSolicitud->datos;
            float ramOcupada = calcularRAMActiva(cabezaProcesos);
            Proceso *nuevoP;
            NodoSolicitud *atendida;

            /* MODIFICADO: permitir hasta el 100% de 8192 MB. */
            if (ramOcupada + solicitud->ram > RAM_TOTAL) {
                break; /* La solicitud permanece pendiente. */
            }

            if (!insertarProceso(&cabezaProcesos, solicitud->pid,
                                 solicitud->nombre, solicitud->cpu,
                                 solicitud->ram, ESTADO_LISTO)) {
                break; /* Si falla malloc, no se pierde la solicitud. */
            }
            nuevoP = buscarProcesoPorPid(cabezaProcesos, solicitud->pid);
            if (nuevoP != NULL) {
                strncpy(nuevoP->usuario, solicitud->usuario,
                        sizeof(nuevoP->usuario) - 1);
                nuevoP->usuario[sizeof(nuevoP->usuario) - 1] = '\0';
                nuevoP->hilos = solicitud->hilos;
                nuevoP->horaCreacion = solicitud->llegada;
                nuevoP->horaFin = 0;
                nuevoP->tiempoRestante = solicitud->tiempo;
                nuevoP->prioridad = solicitud->prioridad;
                nuevoP->prioridadOriginal = solicitud->prioridad;
                nuevoP->nivelCola = solicitud->prioridad - 1;
                insertarHash(nuevoP);
                encolarProceso(&unifila, nuevoP);
            }
            admitidos++;

            /* Saca de pendientes solamente la solicitud admitida. */
            atendida = primeraSolicitud;
            primeraSolicitud = atendida->siguiente;
            if (primeraSolicitud == NULL) ultimaSolicitud = NULL;
            free(atendida);
            cantidadPendientes--;
        }

        /* Asigna CPU segun el planificador elegido. */
        atenderProcesoUnicaPuerta(
            &unifila,
            &puertaUnica,
            cabezaProcesos
        );

        /* Revisa las opciones del usuario. */
        if (_kbhit()) {
            char tecla = _getch();

            /* Permite letras mayusculas y minusculas. */
            if (tecla >= 'a' && tecla <= 'z') {
                tecla = (char)(
                    tecla - ('a' - 'A')
                );
            }

            /*
             * B, C y M abren una consulta temporal.
             * Al terminar, regresa a la misma pantalla.
             */
            if (
                tecla == 'B' ||
                tecla == 'C' ||
                tecla == 'M'
            ) {
                time_t inicioPausa = time(NULL);

                /* MODIFICADO: salir temporalmente de la pantalla alternativa.
                 * Asi Hash y Quick Sort se imprimen en la pantalla normal,
                 * que permite desplazarse por todos los resultados. */
                finalizarPantallaSimulador();
                limpiarPantallaConsola();
                cambiarVisibilidadCursor(1);

                /* Busqueda por nombre usando Hash. */
                if (tecla == 'B') {
                    char nombreBusqueda[100];

                    printf(
                        "========== BUSQUEDA HASH ==========\n"
                    );
                    printf("Nombre del proceso: ");
                    fflush(stdout);

                    if (fgets(
                        nombreBusqueda,
                        sizeof(nombreBusqueda),
                        stdin
                    ) != NULL) {
                        nombreBusqueda[
                            strcspn(nombreBusqueda, "\n")
                        ] = '\0';

                        buscarProcesosPorNombre(
                            nombreBusqueda
                        );
                    }

                /* Quick Sort por porcentaje de CPU. */
                } else if (tecla == 'C') {
                    ordenarProcesosPorCPU(
                        cabezaProcesos
                    );

                /* Quick Sort por consumo de RAM. */
                } else {
                    ordenarProcesosPorRAM(
                        cabezaProcesos
                    );
                }

                printf(
                    "\nPresiona una tecla para continuar..."
                );
                fflush(stdout);
                _getch();

                /* Evita que el tiempo de consulta afecte CPU. */
                compensarPausa(
                    cabezaProcesos,
                    &puertaUnica,
                    inicioPausa,
                    time(NULL),
                    &siguienteCreacion
                );

                /* MODIFICADO: regresar a la pantalla alternativa solo
                 * despues de que el usuario haya revisado los resultados.
                 * La consulta no cambia el ordenamiento ni la simulacion. */
                iniciarPantallaSimulador();
                limpiarPantallaConsola();
                cambiarVisibilidadCursor(0);
                ultimaPantalla = 0;

            /* Q finaliza la simulacion. */
            } else if (tecla == 'Q') {
                salir = 1;
            }
        }

        /*
         * NUEVO:
         * Actualiza la misma pantalla cada 500 ms.
         * No se agregan tablas debajo de otras.
         */
        if (
            !salir &&
            GetTickCount() - ultimaPantalla >=
                INTERVALO_PANTALLA_MS
        ) {
            /* Borra la tabla anterior. */
            limpiarPantallaConsola();

            /* Dibuja los datos actuales. */
            mostrarHtop(
                cabezaProcesos,
                &unifila, /* MODIFICADO: pasa la cola real a la tabla. */
                &puertaUnica,
                inicioSimulador,
                primeraSolicitud /* NUEVO: muestra ESP en la tabla. */
            );

            /* Indicadores de llegada y admision, sin llenar la pantalla. */
            printf("Solicitudes: %lu | Admitidas: %lu | Pendientes (ESP): %lu | Rechazadas: %lu\n",
                   generados, admitidos, cantidadPendientes, descartados);
            /* MODIFICADO: la RAM puede ocuparse hasta el 100%. */

            /* Mantiene las opciones originales. */
            printf(
                "\n========== OPCIONES DEL SIMULADOR ==========\n"
            );
            printf(
                "B - Buscar proceso por nombre (HASH)\n"
            );
            printf(
                "C - Ordenar procesos por CPU (QUICK SORT)\n"
            );
            printf(
                "M - Ordenar procesos por RAM (QUICK SORT)\n"
            );
            printf(
                "Q - Salir del simulador\n"
            );

            fflush(stdout);
            ultimaPantalla = GetTickCount();
        }

        /* Evita consumir demasiada CPU real. */
        Sleep(50);
    }

    /* ==================================================
       CIERRE Y LIBERACION DE MEMORIA
       ================================================== */

    /* MODIFICADO: restaura primero la pantalla normal de Windows Terminal. */
    finalizarPantallaSimulador();
    cambiarVisibilidadCursor(1);

    /* MODIFICADO: borra el menu anterior que reaparecia al presionar Q. */
    system("cls");

    /* Libera solicitudes no admitidas para evitar fugas de memoria. */
    liberarSolicitudes(&primeraSolicitud);

    /* Libera los nodos de la cola. */
    liberarCola(&unifila);

    /* Libera las referencias de Hash. */
    liberarHash();

    /* Libera los procesos originales. */
    liberarListaProcesos(&cabezaProcesos);

    /* MODIFICADO: muestra el mensaje final en la pantalla ya limpia. */
    printf("Simulador finalizado.\n");

    return 0;
}
