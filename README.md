# Simulador de Administración de Procesos

Simulador de administración de procesos desarrollado en **lenguaje C**, inspirado en la herramienta **htop de Linux**. Permite representar el funcionamiento de un sistema operativo mediante la planificación de CPU, la administración de memoria RAM y el monitoreo de procesos en tiempo real.

Implementa un algoritmo de planificación mediante **colas multinivel realimentadas (MLFQ)**, utilizando Round Robin, quantums variables y envejecimiento de prioridades.

## Objetivo

Simular el funcionamiento de un administrador de procesos, mostrando cómo se asignan los recursos del sistema, cómo se ejecutan los procesos, cómo cambian sus estados y cómo evolucionan sus prioridades durante la ejecución.

## Características principales

- Planificación mediante **colas multinivel realimentadas (MLFQ)**.
- Cinco colas independientes, una por cada nivel de prioridad.
- Algoritmo **Round Robin** dentro de cada cola.
- Quantums variables de **4, 8, 12, 16 y 20 segundos**.
- Cinco niveles de prioridad, donde **1 es la más alta y 5 la más baja**.
- Ejecución sin interrupciones por prioridad durante el quantum.
- Envejecimiento de procesos (**Aging**) cada 40 segundos de espera continua en estado listo.
- Visualización de la prioridad original y la prioridad actual.
- Administración de **8192 MB de memoria RAM simulada**.
- Generación automática y continua de solicitudes.
- Cola de espera para solicitudes sin memoria disponible.
- Liberación de memoria RAM al finalizar los procesos.
- Visualización del consumo simulado de CPU y RAM.
- Simulación de operaciones de entrada y salida (E/S).
- Búsqueda de procesos mediante **Hash**.
- Ordenamiento mediante **Quick Sort** por CPU y RAM.
- Interfaz de consola inspirada en htop.

## Algoritmo de planificación

El simulador implementa el algoritmo **Multilevel Feedback Queue (MLFQ)**, también conocido como colas multinivel realimentadas.

Este algoritmo organiza los procesos en diferentes colas según su prioridad actual, utilizando Round Robin dentro de cada una y permitiendo que los procesos cambien de nivel durante la simulación.

### Colas de prioridad

Cada cola cuenta con un quantum diferente:

| Cola | Prioridad | Quantum |
|---|---|---|
| Cola 1 | Muy alta | 4 segundos |
| Cola 2 | Alta | 8 segundos |
| Cola 3 | Media | 12 segundos |
| Cola 4 | Baja | 16 segundos |
| Cola 5 | Muy baja | 20 segundos |

El planificador selecciona los procesos de la cola de mayor prioridad que contenga procesos listos. Dentro de cada cola se respeta el orden FIFO (First In, First Out).

### Funcionamiento de Round Robin

Cada proceso recibe un tiempo de ejecución determinado por el quantum de su cola.

- Si el proceso termina antes de consumir su quantum, pasa al estado finalizado.
- Si consume completamente su quantum y aún tiene trabajo pendiente, desciende al siguiente nivel de prioridad.
- Si pertenece a la cola 5 y consume su quantum, permanece en ese nivel.
- Si requiere una operación de entrada y salida, pasa temporalmente al estado de espera al finalizar su turno.

Una vez que un proceso comienza su turno, la llegada de otro proceso de mayor prioridad no interrumpe su ejecución.

### Envejecimiento (Aging)

El envejecimiento es un mecanismo que permite reducir el riesgo de **inanición (starvation)** de los procesos con menor prioridad.

Cuando un proceso permanece durante **40 segundos** en una cola de listos sin recibir atención, asciende un nivel de prioridad.

Después de cada ascenso, su tiempo de espera y el consumo de quantum se reinician.

Este mecanismo permite que los procesos que han esperado demasiado tengan mayores oportunidades de acceder a la CPU.

### Prioridad original y prioridad actual

El simulador registra dos tipos de prioridad:

- **Prioridad original:** prioridad asignada al proceso al momento de generar su solicitud. Permanece constante durante la simulación.
- **Prioridad actual:** prioridad utilizada por el planificador, que puede cambiar debido al consumo del quantum o al envejecimiento.

Ambas prioridades se muestran en la interfaz para observar el comportamiento de los procesos.

## Administración de memoria

El simulador dispone de **8192 MB de RAM simulada**.

Las solicitudes se generan continuamente en intervalos aleatorios de 1 a 5 segundos, con un requerimiento de memoria de entre 64 y 256 MB.

Los procesos son admitidos únicamente cuando existe suficiente memoria RAM disponible. Si no hay espacio suficiente, las solicitudes permanecen en una cola de espera.

La cola de solicitudes funciona mediante el principio **FIFO**, respetando el orden de llegada.

Cuando un proceso finaliza, libera la memoria simulada que tenía asignada, permitiendo admitir nuevas solicitudes.

La generación de solicitudes continúa incluso cuando la memoria RAM simulada se encuentra ocupada. Las solicitudes no son rechazadas por falta de RAM simulada, sino que permanecen pendientes hasta que exista espacio disponible.

## Estados de los procesos

| Estado | Descripción |
|---|---|
| L | Listo para ejecutarse |
| R | En ejecución |
| E/S | En espera de entrada/salida |
| ESP | Solicitud esperando disponibilidad de RAM |
| F | Finalizado |

Los procesos que requieren una operación de entrada y salida permanecen en espera durante 3 segundos antes de regresar a su cola de prioridad correspondiente.

Los procesos finalizados permanecen visibles temporalmente durante 5 segundos y se conserva un historial de hasta 40 procesos finalizados.

## Estructuras de datos

### Lista simplemente enlazada

Almacena los procesos admitidos y su información, incluyendo PID, nombre, estado, prioridades, consumo de recursos y tiempo restante de ejecución.

Es la estructura principal del simulador.

### Colas múltiples de procesos listos

Se utilizan cinco colas independientes, una por cada nivel de prioridad.

Cada cola trabaja mediante el principio FIFO y almacena referencias a los procesos de la lista principal.

### Cola de solicitudes

Almacena las solicitudes pendientes de admisión cuando no existe suficiente memoria RAM simulada.

### Tabla Hash

Permite realizar búsquedas de procesos mediante su nombre. Utiliza encadenamiento mediante listas enlazadas para resolver colisiones.

### Arreglos de punteros

Se utilizan como estructuras auxiliares para ordenar los procesos mediante Quick Sort, sin modificar la lista principal ni el orden de ejecución de las colas.

## Algoritmos de búsqueda y ordenamiento

- **Hash:** permite localizar procesos mediante su nombre utilizando una tabla Hash.
- **Quick Sort por CPU:** ordena los procesos de manera descendente según su porcentaje de uso de CPU simulado.
- **Quick Sort por RAM:** ordena los procesos de manera descendente según su consumo de memoria RAM simulada.

Los algoritmos de búsqueda y ordenamiento no modifican las prioridades ni el orden de planificación de los procesos.

## Parámetros de la simulación

| Parámetro | Valor |
|---|---|
| CPU simuladas | 1 |
| Memoria RAM total | 8192 MB |
| Colas de prioridad | 5 |
| Llegada de solicitudes | 1 a 5 segundos |
| Tiempo inicial de ejecución | 5 a 20 segundos |
| RAM por proceso | 64 a 256 MB |
| Hilos por proceso | 1 a 8 |
| Envejecimiento | 40 segundos |
| Duración de espera por E/S | 3 segundos |
| Actualización de la interfaz | 500 milisegundos |
| Máximo de filas en la interfaz | 15 |
| Historial de procesos finalizados | 40 registros |

## Limitaciones

- **Inanición:** el envejecimiento reduce el riesgo de inanición de los procesos de menor prioridad, pero no garantiza un tiempo máximo de espera bajo cualquier carga de trabajo.
- **CPU única:** únicamente se simula una CPU, por lo que no existe ejecución paralela de procesos.
- **Recursos simulados:** los valores de CPU y RAM no corresponden al consumo real de recursos del sistema operativo.
- **Cola de solicitudes:** puede crecer continuamente si la generación de solicitudes supera la capacidad de atención del simulador.
- **Admisión FIFO:** una solicitud que requiere más memoria de la disponible puede retrasar la admisión de las solicitudes posteriores.
- **Visualización:** la interfaz muestra un máximo de 15 filas, aunque pueden existir más procesos registrados.
- **Planificación fija:** no es posible cambiar el algoritmo de planificación durante la ejecución.
- **Compatibilidad:** el programa utiliza bibliotecas específicas de Windows, por lo que no es directamente compatible con Linux.

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

Durante las consultas de búsqueda y ordenamiento, la simulación se pausa temporalmente y se reanuda al regresar a la interfaz principal.

## Estructura del proyecto

| Archivo | Descripción |
|---|---|
| `main.c` | Programa principal, generación de solicitudes, administración de memoria y control del simulador |
| `proceso.c` | Gestión de procesos, colas multinivel, planificación, envejecimiento e interfaz |
| `proceso.h` | Estructuras, constantes y declaraciones de funciones |
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
- Punteros y memoria dinámica.
- Listas simplemente enlazadas.
- Colas FIFO.
- Arreglos de punteros.
- Tablas Hash.
- Algoritmo Quick Sort.
- Colas multinivel realimentadas (MLFQ).
- Algoritmo Round Robin.
- Envejecimiento de prioridades (Aging).

## Propósito académico

Este proyecto fue desarrollado con fines educativos para comprender los principios de administración de procesos, planificación de CPU, gestión de memoria y estructuras de datos en los sistemas operativos.

También permite analizar el funcionamiento de las colas multinivel realimentadas, la asignación de quantums según la prioridad, los cambios dinámicos de prioridad y la importancia del envejecimiento como mecanismo para reducir la inanición de procesos.

La implementación integra estructuras de datos y algoritmos de búsqueda, ordenamiento y planificación dentro de un mismo sistema de simulación.

## Autores

Proyecto académico desarrollado por estudiantes de tercer semestre de Ingeniería en Sistemas Computacionales.

