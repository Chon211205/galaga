# Galaga en C++

Este proyecto es una recreación del clásico juego Galaga, desarrollado en C++ para ejecutarse desde la terminal. El juego utiliza caracteres ASCII, colores ANSI, control de teclado en tiempo real, sistema de vidas, enemigos, disparos, rondas y almacenamiento local de puntajes.

## Descripción

Galaga en C++ es un videojuego de consola donde el jugador controla una nave espacial que debe destruir enemigos mientras evita perder todas sus vidas. El programa cuenta con un menú principal, diferentes modos de juego, instrucciones, sistema de puntajes y guardado de resultados.

El juego está diseñado para ejecutarse en sistemas tipo Linux, por lo que puede correrse correctamente en WSL, Linux nativo o cualquier terminal compatible con ANSI.

## Características principales

* Juego ejecutable desde la terminal.
* Interfaz visual con caracteres ASCII.
* Uso de colores ANSI.
* Menú principal interactivo.
* Diferentes modos de juego.
* Sistema de vidas.
* Sistema de puntuación.
* Registro de mejores puntajes.
* Guardado local de puntajes.
* Control de teclado en tiempo real.
* Uso de hilos con pthread.
* Manejo de archivos.
* Compatible con WSL y Linux.

## Tecnologías utilizadas

* C++
* STL
* Pthreads
* Terminal ANSI
* WSL
* Linux
* Manejo de archivos
* Programación estructurada

## Librerías utilizadas

```cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <string>
#include <iomanip>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
```

## Requisitos

Para compilar y ejecutar el programa necesitas:

* WSL o Linux.
* Compilador g++.
* Terminal compatible con colores ANSI.

En Ubuntu o WSL puedes instalar el compilador con:

```bash
sudo apt update
sudo apt install g++ -y
```

## Cómo compilar

Si el archivo se llama `galaga.cpp`, usa:

```bash
g++ -std=c++17 -pthread galaga.cpp -o galaga
```

Si el archivo se llama `Pasted code.cpp`, usa comillas porque el nombre contiene un espacio:

```bash
g++ -std=c++17 -pthread "Pasted code.cpp" -o galaga
```

También puedes compilar mostrando advertencias con:

```bash
g++ -std=c++17 -Wall -Wextra -pthread "Pasted code.cpp" -o galaga
```

## Cómo ejecutar

Después de compilar, ejecuta el programa con:

```bash
./galaga
```

## Ejecución en WSL

Si el archivo está en el Escritorio de Windows, entra a la carpeta usando:

```bash
cd /mnt/c/Users/leejo/Desktop
```

Si el archivo está en Descargas, usa:

```bash
cd /mnt/c/Users/leejo/Downloads
```

Luego compila:

```bash
g++ -std=c++17 -pthread "Pasted code.cpp" -o galaga
```

Y ejecuta:

```bash
./galaga
```

## Controles

| Tecla                  | Acción                              |
| ---------------------- | ----------------------------------- |
| A                      | Mover la nave a la izquierda        |
| D                      | Mover la nave a la derecha          |
| Espacio                | Disparar                            |
| Q                      | Salir de la partida                 |
| Enter                  | Seleccionar opción o volver al menú |
| Flechas arriba y abajo | Navegar por el menú                 |

## Modos de juego

El juego incluye diferentes modos para variar la dificultad y duración de la partida.

### Modo 40 enemigos

En este modo el objetivo es destruir 40 enemigos. Es una partida corta, ideal para practicar o jugar rápidamente.

### Modo 50 enemigos

En este modo el objetivo es destruir 50 enemigos. Tiene una dificultad intermedia y requiere más precisión.

### Modo 60 enemigos

En este modo el objetivo es destruir 60 enemigos. Es más largo y desafiante que los modos anteriores.

### Modo infinito

En este modo no existe un límite fijo de enemigos. El jugador avanza por rondas y los enemigos continúan apareciendo hasta que se pierdan todas las vidas.

## Sistema de puntajes

El juego cuenta con un sistema de puntuación que permite registrar los mejores resultados del jugador. Cada enemigo destruido suma puntos. Al finalizar una partida, el programa puede guardar información como el nombre del jugador, el puntaje obtenido, la ronda alcanzada, el tiempo jugado y la fecha.

Los puntajes se almacenan localmente en el archivo:

```bash
~/.puntajes_galaga
```

El sistema ordena los puntajes tomando en cuenta:

1. Mayor puntaje.
2. Mayor ronda alcanzada.
3. Menor tiempo utilizado.

## Estructura general del programa

El programa está dividido en varias partes importantes que permiten organizar mejor su funcionamiento.

### Interfaz de usuario

Incluye funciones encargadas de limpiar la pantalla, ocultar y mostrar el cursor, centrar texto, dibujar cajas y mostrar el título del juego.

### Menú principal

Permite navegar entre las opciones principales del programa, como iniciar una partida, ver instrucciones, consultar puntajes o salir del juego.

### Sistema de puntajes

Se encarga de cargar, guardar, ordenar y mostrar los mejores puntajes obtenidos por los jugadores.

### Lógica del juego

Controla el movimiento de la nave, los disparos, la aparición de enemigos, las colisiones, las vidas, el puntaje, las rondas y las condiciones de victoria o derrota.

### Manejo de teclado

Utiliza `termios` y `fcntl` para leer teclas en tiempo real sin necesidad de presionar Enter durante la partida.

### Uso de hilos

El programa utiliza `pthread` para crear un hilo auxiliar que trabaja junto con la lógica principal del juego.

## Posibles problemas y soluciones

### El comando g++ no existe

Si al compilar aparece un error indicando que `g++` no está instalado, ejecuta:

```bash
sudo apt update
sudo apt install g++ -y
```

### El archivo no compila por el nombre

Si el archivo tiene espacios en el nombre, debes escribirlo entre comillas:

```bash
g++ -std=c++17 -pthread "Pasted code.cpp" -o galaga
```

### La pantalla se ve desordenada

Si la interfaz se ve mal, agranda la ventana de la terminal. También se recomienda usar Windows Terminal en lugar de la consola clásica.

### Los colores no aparecen correctamente

Verifica que estés usando una terminal compatible con secuencias ANSI.

### El programa no se ejecuta

Asegúrate de que el archivo ejecutable exista. Puedes revisar con:

```bash
ls
```

Si aparece el archivo `galaga`, ejecútalo con:

```bash
./galaga
```

## Ejemplo completo de uso

```bash
cd /mnt/c/Users/leejo/Desktop
g++ -std=c++17 -pthread "Pasted code.cpp" -o galaga
./galaga
```

## Aprendizajes del proyecto

Este proyecto permite practicar varios conceptos importantes de C++, entre ellos:

* Uso de estructuras de datos.
* Manejo de archivos.
* Programación con hilos.
* Control de entrada por teclado.
* Uso de terminal.
* Lógica de videojuegos.
* Condiciones de victoria y derrota.
* Organización de código.
* Manipulación de texto en consola.

## Autor

Proyecto desarrollado como práctica de programación en C++, aplicando conceptos de manejo de terminal, estructuras de datos, archivos, hilos y lógica de videojuegos.

## Conclusión

Galaga en C++ demuestra cómo se puede crear un videojuego interactivo desde la terminal utilizando herramientas propias del lenguaje y del sistema operativo. Aunque se ejecuta en consola, el programa incluye elementos importantes como menús, colores, controles en tiempo real, enemigos, disparos, vidas y puntajes. Esto lo convierte en un proyecto útil para reforzar conocimientos de programación estructurada, manejo de archivos, uso de hilos y desarrollo de lógica de videojuegos.
