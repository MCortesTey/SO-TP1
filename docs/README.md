# TP1 - Sistemas Operativos (72.11) - ChompChamps

Este proyecto forma parte de la asignatura 72.11 - Sistemas Operativos, y consiste en la implementación de un juego de estrategia llamado ChompChamps. El juego permite enfrentar diferentes tipos de jugadores en un tablero rectangular, implementando un sistema de comunicación entre procesos mediante memoria compartida y semáforos. 

## Instrucciones de Compilación y Ejecución

Sigue los pasos a continuación para compilar y ejecutar el proyecto correctamente dentro del contenedor Docker provisto por la cátedra:

1. Dirígete al directorio raíz del proyecto en el contenedor Docker

2. Para compilar el proyecto utiliza:
```bash
make all
```
3. Para ejectutar el proyecto utiliza:
```bash
./ChompChamps [parámetros]
```

#### Parámetros

Los parámetros entre corchetes son opcionales y tienen un valor por defecto:

- `[-w width]`: Ancho del tablero. Default y mínimo: 10
- `[-h height]`: Alto del tablero. Default y mínimo: 10
- `[-d delay]`: Milisegundos que espera el máster cada vez que se imprime el estado. Default: 200
- `[-t timeout]`: Timeout en segundos para recibir solicitudes de movimientos válidos. Default: 10
- `[-s seed]`: Semilla utilizada para la generación del tablero. Default: time(NULL)
- `[-v view]`: Ruta del binario de la vista. Default: Sin vista.
- `-p player1 player2`: Ruta/s de los binarios de los jugadores. Mínimo: 1, Máximo: 9

#### Jugadores Disponibles

- `player_first_possible`
- `player_best_score`
- `player_random`
- `player_clock`
- `player_killer`
- `player_jason`

## Compilación

El proyecto utiliza un Makefile para la compilación. Algunos de los comandos son:

- `make`: Compila todo el proyecto
- `make clean`: Limpia los archivos generados
- `make analyze`: Ejecuta el analizador estático PVS-Studio

## Requisitos 

Docker: El contenedor Docker provisto por la cátedra es necesario para ejecutar el entorno de compilación y pruebas de manera controlada.