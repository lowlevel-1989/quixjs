# Benchmark y perfilado

Como medir quixjs y encontrar que mejorar: h5bench para el runtime solo, y
`--fps`, `--profile` y la consola de debug para un juego de verdad.

La regla: un cambio se queda solo si mide mejor en una portatil, con la misma
prueba antes y despues. Una mejora en el PC que no se ve en la consola no
cuenta.

## h5bench

[h5bench](https://github.com/Knifethrower/h5bench), de Knifethrower, mide
runtimes de HTML5 pagina por pagina: JavaScript, WebGL, canvas 2d, memoria,
tiempos... Las mismas paginas corren en navegadores, NW.js y otros runtimes, asi
que los numeros se pueden comparar con ellos.

```
make bench                         # js, webgl, canvas2d y memory
tools/h5bench.sh js memory         # solo estas paginas
LABEL=antes tools/h5bench.sh js    # un nombre para los resultados
```

`tools/h5bench.sh` descarga h5bench en `obj/h5bench` (una version fija, para
que las corridas se comparen), escribe el toml de cada pagina con
`quixjs --gen-config pagina.html` y la corre en una ventana con `--expose-gc`
(un `gc()` global, como en V8, para que las lecturas de memoria sean exactas).
Cada pagina se cierra sola al terminar. Los resultados van a
`results/h5bench/<etiqueta>-<pagina>.log`, una linea por medida:

```
H5B js json.stringify_1mb ms=28.7 min_ms=28.3 max_ms=34.4
H5B webgl draws.one_quad_each max60=160
H5B memory gc.short_lived_objects pause_ms=4.320 pauses=521 gc_pct=28.5 ops_s=6551866
H5B-CHECK canvas2d todataurl_png fail got_data:,
```

| medida     | que es |
|------------|--------|
| `ops_s`    | unidades de trabajo por segundo: mas es mejor |
| `ms`       | la mediana de varias corridas: menos es mejor |
| `max60`    | cuantas llamadas de dibujo, sprites o capas aguanta a 55 fps o mas |
| `pause_ms` | la parada mas larga del GC |
| `rss_mb`, `heap_mb` | la memoria del proceso y la del heap de JS |
| `rss_growth_mb` | la memoria que queda tras crear y soltar objetos (una fuga si crece) |

`H5B-CHECK ... fail` o `missing` es una API que no funciona o no esta. El README
de h5bench explica cada prueba.

### En una portatil

El script solo necesita `git` para descargar h5bench. Corre `make bench` una vez
en el PC, copia `obj/h5bench` y `tools/h5bench.sh` (en las mismas carpetas)
junto al quixjs del port, y en la consola:

```
cd /storage/roms/ports/fearandhunger
export LD_LIBRARY_PATH=$PWD/libs.aarch64 QUIXJS_FONT=$PWD/fonts/DejaVuSans-Bold.ttf
QUIXJS=./quixjs.aarch64 ARGS=--fullscreen LABEL=cubexx sh tools/h5bench.sh js webgl memory
```

### Comparar corridas

```
python3 obj/h5bench/tools/h5bench.py compare results/h5bench/antes-js.log results/h5bench/despues-js.log
python3 obj/h5bench/tools/h5bench.py table results/h5bench/*-js.log
```

`compare` lista lo que empeoro y lo que mejoro un 10% o mas; `table` pone
varias corridas lado a lado.

### Resultados de quixjs 0.2.2

| prueba | PC (i7-8750H, Intel UHD 630) | RG CubeXX (H700, Panfrost) | RGB30 (RK3566, libmali) |
|---|---|---|---|
| `json.stringify_1mb` | 4,6 ms | 28,7 ms | 20,0 ms |
| `json.parse_1mb` | 10,6 ms | 51,2 ms | 42,7 ms |
| `call.closure_1k` | 16k/s | 7,8k/s | 11k/s |
| `draws.one_quad_each` (max60) | 2944 | 160 | 448 |
| `sprites.batched_32px` (max60) | 57344 | 2816 | 5120 |
| pausa de `gc.short_lived_objects` | 2,2 ms | 4,3 ms | 4,4 ms |
| pausa de `gc.long_lived_records_replaced` | 52,6 ms | 465 ms | 351 ms |

## Un juego de verdad

h5bench dice que es lento en el runtime; un juego dice que importa. Mide el
juego en la misma escena antes y despues de un cambio, en la consola.

### --fps

`--fps` escribe una linea por segundo con el tiempo de cada parte de un frame
(`js`, `present`, `timers`...; ver [uso](usage.es.md#--fps)) y al final la media
de toda la corrida. `js` alto: el juego o las funciones de quixjs que llama.
`present` alto: la GPU o la copia a la pantalla.

### --profile

`--profile[=archivo]` usa el sampling profiler de JavaScriptCore y escribe
`quixjs-profile.txt` cada 10 s y al salir. Con `--debug`, `prof start`,
`prof stop` y `prof save` hacen lo mismo solo para una parte del juego. El
informe tiene:

| seccion | que mirar |
|---------|-----------|
| JIT tier | cuanto codigo corre optimizado (DFG, FTL) o no (LLInt, Baseline, None) |
| quixjs C functions | cada binding (WebGL, canvas, audio...) con su tiempo, llamadas y microsegundos por llamada: el C que hay que acelerar |
| JS memory created | que funciones reservan mas memoria: presion sobre el GC |
| objects made by quixjs | los objetos que crea el runtime por segundo |
| self / total | las funciones de JS por su propio tiempo y con lo que llaman: los hooks que escribir |

El informe da medias; un tiron es un frame. Con el profiler en marcha, cada
frame de mas de 100 ms (o de `QUIXJS_SLOW_FRAME_MS`: con 40, los que se
pierden un refresco a 30 fps) escribe ademas que corrio en el:

```
[slow frame js] 464 ms
  self: 130 ms readPixels, 80 ms drawImage, 80 ms texImage2D, 50 ms blur, ...
  in: 430 ms (anonymous) (TDDP_FluidTimestep.js:86), ..., 220 ms (anonymous) (YEP_MainMenuManager.js:3763), ...
```

`self` es el tiempo de cada funcion (las de C por su nombre), `in` las
funciones que estaban en la pila (sin las que estan en todas las muestras:
el loop), la de mas tiempo primero. Junto a `[slow frame]` de `--fps` (js,
present, imagenes decodificadas), dice que parte del juego es el tiron.

### make TRACE=1

Medidas en los puntos criticos del runtime, que solo se compilan si se piden:

```
make TRACE=1                                   # PC
make portmaster PORT=fearandhunger TRACE=1   # la de las portatiles
```

Cada punto escribe `[trace] <que>: <ms> ms` cuando tarda al menos su minimo.
Una compilacion normal (sin `TRACE`, o `TRACE=0`) no tiene ninguno: ni una
linea en el binario. Cambiar `TRACE` recompila todo.

| punto | minimo | que mide |
|-------|--------|----------|
| `texImage2D WxH premultiply` | 0,5 ms | premultiplicar una imagen o un canvas antes de subirlo (`binding/webgl.c`) |
| `texImage2D WxH image/canvas upload` | 0,5 ms | la subida en si (el driver copia y ordena los pixeles para la GPU) |
| `texSubImage2D WxH` | 1 ms | volver a subir un canvas (el contenido de una ventana, texto) |
| `bufferSubData N bytes` | 1 ms | escribir vertices; si tarda, el driver espero a la GPU |
| `readPixels WxH` | 1 ms | leer de la GPU (la foto detras del menu) |
| `image decode WxH (worker)` | 5 ms | decodificar un PNG en su thread (`binding/image/decode.c`) |
| `collectGarbage full/due` | 1 ms | un GC pedido desde JS (`quix.memory.collect`: los hooks del GC) |
| `blur WxH` | 1 ms | el desenfoque nativo de RPG Maker MV (`engines/rpgmaker-mv/natives/blur.c`) |

Un punto nuevo: `TRACE_START(t)` y `TRACE_END(t, min_ms, "que", ...)` de
`runtime/core/debug/trace.h` alrededor del codigo, y una fila en esta tabla.

### Memoria

- `process.memoryUsage()` (en juegos `nwjs`) da `rss`, `heapUsed`, `heapTotal` y
  `external`; `quix.memory.usage()` ademas da `maxRss`, el pico.
- En la consola de debug, `mem` ensena el heap de JS y sus objetos, y `gc`
  pasa el GC y dice cuanto tardo y que libero.
- Desde fuera, `/proc/<pid>/status`: `VmRSS` ahora, `VmHWM` el pico.

### La consola de debug

Con `--debug`, `tools/console.py` maneja el juego desde un script: `eval` para
cargar una partida o ir a una escena, `press` para las teclas, `pacing` para el
intervalo real entre frames. Una prueba que hace los mismos pasos cada vez es
lo que hace comparables el antes y el despues. Los comandos:
[tests](tests.es.md#consola-de-debug).

## Donde se va el tiempo

- [Motor de JavaScript](javascript-engine.es.md): JavaScriptCore, su JIT y los
  cambios que le hace quixjs.
- [runtime/game/hooks](../runtime/game/hooks/README.es.md): cambiar una funcion lenta de un
  juego o motor por C, sin tocar sus archivos.
- [Codigo](../runtime/README.es.md): que hace cada parte del runtime.
