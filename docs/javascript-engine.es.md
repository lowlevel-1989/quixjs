# El motor de JavaScript

[English](javascript-engine.md) | Español

quixjs funciona sobre **JavaScriptCore**, el motor de WebKit (Safari, GNOME
Web, WPE), compilado por nosotros como libreria estatica y usado por su API en
C (`runtime/js/`, el unico sitio que conoce el motor).

**QuickJS fue nuestra primera opcion**, y quixjs funciono con el antes de la
version 0.2.0: es diminuto (alrededor de 1 MB, 9 MB de memoria) y muy facil de
implementar. Se descarto en favor de JavaScriptCore porque es un interprete sin JIT
(nunca convierte el JavaScript en codigo maquina), y los juegos que portamos
(RPG Maker MV) llaman a miles de funciones pequenas en cada imagen. Aun con
todas las optimizaciones que hicimos ([hooks de motor](../runtime/engines/README.es.md),
funciones nativas en C, un profiler mejor), Fear & Hunger no llegaba a 30 fps
en las portatiles. Una prueba con codigo tipo RPG Maker en la RG CubeXX
(Cortex-A53):

| motor                     | tiempo  | memoria maxima |
|---------------------------|---------|----------------|
| QuickJS                   | 2212 ms | 9 MB           |
| JavaScriptCore (JIT)      | 147 ms  | 208 MB         |
| V8 (Node, JIT)            | 107 ms  | 204 MB         |

En el juego, en el PC, una actualizacion del mapa mas pesado paso de 14-19 ms
(QuickJS) a 3,4 ms (JavaScriptCore), y las batallas de unos 22 fps en la
portatil a 60 estables en el PC. Sin JIT ni JavaScriptCore ni V8 mejoran a
QuickJS: lo que cuenta es el JIT. Elegimos JavaScriptCore antes que V8 porque
tiene una API en C sencilla que encaja con quixjs (en C), no va atado a Node, y
el port embebido WPE lo mantiene en buena forma en ARM.
