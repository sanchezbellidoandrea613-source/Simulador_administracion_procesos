 # Simulador de Administración de Procesos

Simulador de administración de procesos desarrollado en **lenguaje C**, inspirado en la herramienta **htop de Linux**. Permite representar el funcionamiento de un sistema operativo mediante la planificación de CPU, la administración de memoria RAM y el monitoreo de procesos en tiempo real.

## Objetivo

Simular el funcionamiento de un administrador de procesos, mostrando cómo se asignan los recursos del sistema, cómo se ejecutan los procesos y cómo cambian sus estados durante la ejecución.

## Características principales

- Planificación combinada **Round Robin con Prioridad**.
- Quantum fijo de **2 segundos**.
- Cinco niveles de prioridad, donde **1 es la más alta y 5 la más baja**.
- Administración de **8192 MB de memoria RAM simulada**.
- Generación automática de procesos.
- Cola de espera para solicitudes sin memoria disponible.
- Visualización del consumo de CPU y RAM.
- Búsqueda de procesos mediante **Hash**.
- Ordenamiento mediante **Quick Sort** por CPU y RAM.
- Interfaz de consola inspirada en htop.

## Algoritmo de planificación

El simulador combina dos algoritmos de planificación:

**Prioridad:** determina qué proceso debe ejecutarse primero, dando preferencia a los procesos con mayor prioridad.

**Round Robin:** distribuye el tiempo de CPU mediante un quantum fijo de 2 segundos, permitiendo que los procesos de la misma prioridad se turnen equitativamente.

### Limitaciones

El algoritmo utiliza prioridades estrictas, por lo que los procesos de prioridad baja pueden experimentar **inanición (starvation)** si continuamente existen procesos listos de mayor prioridad.

Actualmente no se implementa aging o envejecimiento de prioridades.

## Administración de memoria

El simulador dispone de **8192 MB de RAM simulada**.

Los procesos son admitidos cuando existe suficiente memoria. Si no hay espacio disponible, permanecen pendientes hasta que se libere memoria.

Cuando un proceso finaliza, libera la memoria simulada que tenía asignada, permitiendo admitir nuevas solicitudes.

## Estados de los procesos

| Estado | Descripción |
|---|---|
| L | Listo para ejecutarse |
| R | En ejecución |
| E/S | En espera de entrada/salida |
| ESP | Esperando disponibilidad de RAM |
| F | Finalizado |

## Requisitos

- Sistema operativo Windows.
- Code::Blocks con compilador MinGW/GCC.
- Consola compatible con las funciones de visualización utilizadas.

## Cómo ejecutar

1. Descargar o clonar este repositorio.
2. Abrir el archivo `PROYECTO INTEGRADOR.cbp` en Code::Blocks.
3. Verificar que estén incluidos todos los archivos fuente y de cabecera.
4. Compilar el proyecto utilizando **Build and Run (F9)**.
5. Visualizar el simulador en la consola.

## Controles

| Tecla | Función |
|---|---|
| B | Buscar procesos mediante Hash |
| C | Ordenar procesos por CPU con Quick Sort |
| M | Ordenar procesos por RAM con Quick Sort |
| Q | Salir del simulador |

## Estructura del proyecto

| Archivo | Descripción |
|---|---|
| `main.c` | Programa principal y control del simulador |
| `proceso.c` | Gestión y planificación de procesos |
| `proceso.h` | Estructuras y declaraciones de funciones |
| `hash.c` | Implementación de búsqueda Hash |
| `hash.h` | Declaraciones relacionadas con Hash |
| `quicksort_cpu.h` | Ordenamiento por uso de CPU |
| `quicksort_ram.h` | Ordenamiento por uso de RAM |
| `PROYECTO INTEGRADOR.cbp` | Proyecto de Code::Blocks |

## Tecnologías utilizadas

- Lenguaje C.
- Code::Blocks.
- MinGW/GCC.
- Consola de Windows.
- Listas enlazadas y colas.
- Tablas Hash.
- Algoritmos Round Robin, Prioridad y Quick Sort.

## Propósito académico

Este proyecto fue desarrollado con fines educativos para comprender los principios de administración de procesos, planificación de CPU, gestión de memoria y estructuras de datos en los sistemas operativos.

También permite analizar las ventajas y limitaciones de combinar un algoritmo equitativo, como Round Robin, con un algoritmo basado en prioridades.

## Autores

Proyecto académico desarrollado por estudiantes de tercer semestre de Ingeniería en Sistemas Computacionales.

