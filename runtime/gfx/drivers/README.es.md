# drivers

[English](README.md) | Español

Todos los drivers tienen la misma interfaz (`GpuDriver`, en `../driver.h`) y
cada uno la rellena en su archivo. El codigo comun no mira que driver hay:
llama a los puntos de la interfaz en su sitio. `../driver.c` elige el driver
al crear el contexto GL (`[gpu] driver: ...` en la salida,
`quix.gpuDriver()` en JS).

## Glosario

- **Mesa**: los drivers graficos libres de Linux, uno por familia de GPU
  ([lista](https://docs.mesa3d.org/systems.html), [codigo](https://gitlab.freedesktop.org/mesa/mesa)).
- **panfrost**: el de Mesa para las Mali modernas (Mali-T, Mali-G)
  ([docs](https://docs.mesa3d.org/drivers/panfrost.html),
  [codigo](https://gitlab.freedesktop.org/mesa/mesa/-/tree/main/src/gallium/drivers/panfrost)).
- **libmali**: el de ARM para sus Mali, cerrado
  ([ARM](https://developer.arm.com/downloads/-/mali-drivers/user-space)).
- **present_ahead**: otro thread presenta la imagen anterior mientras el
  juego dibuja la siguiente.

## Drivers

| driver     | que es                         | probado en |
|------------|--------------------------------|------------|
| `panfrost` | Mesa con "Panfrost" en GL_RENDERER | RG CubeXX (Mali-G31, Mesa 24.3), Powkiddy RGB30 (Mali-G52, Mesa 24.3) |
| `mali`     | "Mali" en GL_RENDERER, sin Mesa | Powkiddy RGB30 (Mali-G52) |
| `mali-g31` | "Mali-G31" en GL_RENDERER, sin Mesa (libmali en un RK3326: R36S y clones) | RX6S (Mali-G31, ROCKNIX) |
| `default`  | el resto (el PC); sin optimizar | PC (Intel UHD 630, Mesa) |

## Como se elige el driver

1. Al arrancar el juego, `gl_init` (`../gl.c`) crea el contexto de OpenGL ES.
2. Le pide al driver dos textos suyos: `GL_RENDERER` (que GPU y que driver)
   y `GL_VERSION` (que version de OpenGL ES y de quien).
3. `gpu_driver_detect` (`../driver.c`) los mira una vez:

   ```c
   int mesa = has(version, "mesa");
   if (mesa && has(renderer, "panfrost")) current = &gpu_driver_panfrost;
   else if (!mesa && has(renderer, "mali-g31")) current = &gpu_driver_mali_g31;
   else if (!mesa && has(renderer, "mali")) current = &gpu_driver_mali;
   else current = &gpu_driver_default;
   ```

   `has` busca la palabra sin mirar mayusculas: cada version de un driver
   puede escribir su nombre de otra forma ("Panfrost", "panfrost"...).
4. La salida lo dice (`[gpu] driver: panfrost`) y desde entonces
   `gpu_driver()` devuelve ese driver.

Lo que dicen nuestras consolas (de sus logs):

| consola | GL_RENDERER | GL_VERSION | driver |
|---------|-------------|------------|--------|
| RG CubeXX | `Mali-G31 (Panfrost)` | `OpenGL ES 3.1 Mesa 24.3.4` | panfrost |
| Powkiddy RGB30 (gpu.driver=panfrost) | `Mali-G52 r1 (Panfrost)` | `OpenGL ES 3.1 Mesa 24.3.4` | panfrost |
| Powkiddy RGB30 (gpu.driver=libmali) | `Mali-G52` | `OpenGL ES 3.2 v1.g13p0-01eac0...` | mali |
| RX6S (clon de la R36S) | `Mali-G31` | `OpenGL ES 3.2 v1.g13p0-01eac0...` | mali-g31 |
| PC | `Mesa Intel(R) UHD Graphics 630 (CFL GT2)` | `OpenGL ES 3.2 Mesa 26.0.6` | default |

## La interfaz

`GpuDriver` (`../driver.h`): lo que cada driver rellena en su archivo.

| campo           | que es | se usa en |
|-----------------|--------|-----------|
| `thread_wait(fence)` | funcion: como espera ese thread a que la GPU termine cada imagen. Sin ella (`0`), el driver no usa el thread y todo va por `present` | `backend/sdl/backend.c`, el thread y `thread_start` |
| `present(p, f)` | funcion: como se presenta una imagen sin ese thread (`present_ahead = false`, o si el compositor lo frena) | `backend/sdl/backend.c`, `gpu_present()` |

`present` recibe la imagen (`GpuFrame`) y lo que el backend sabe hacer con
ella (`Presenter`), explicados abajo.

## Las piezas compartidas (common.c)

Las funciones que usan los drivers estan en `common.c` (y `common.h`). Cada
driver elige las suyas; lo que solo use un driver va en su propio archivo
(de momento no hay nada asi).

| pieza | para | que hace |
|-------|------|----------|
| `drv_present_now` | `present` | espera a que la GPU termine la imagen y la muestra |
| `drv_present_previous` | `present` | muestra la imagen anterior, ya terminada, y guarda esta para la siguiente |
| `drv_wait_gpu` | `thread_wait` | espera la GPU (`eglWaitSyncKHR`); el thread sigue |
| `drv_wait_cpu` | `thread_wait` | espera la CPU del thread a que la GPU termine |

Lo que elige cada driver, y por que (lo medido):

| | panfrost | mali | mali-g31 | default |
|-|----------|------|----------|---------|
| `thread_wait` | `drv_wait_gpu`: con `drv_wait_cpu`, en la RG CubeXX de ~70 a ~390 frames tardios en batalla | `drv_wait_cpu`: con `drv_wait_gpu`, los menus pestaneaban | ninguna (sin thread): con el, el juego esperaba ~16 ms por imagen; en el mapa de ~17 a ~27 fps | `drv_wait_gpu` |
| `present` | `drv_present_previous`: en la RGB30 con panfrost, de 353 a 76 frames tardios en el mapa y de 940 a 194 en batalla | `drv_present_previous`: en la RGB30, de ~240 a ~60 en el mapa y de ~270 a ~85 en batalla | `drv_present_previous`, como mali | `drv_present_now` |

## Como espera el thread a la GPU (thread_wait)

Cuando el juego acaba un frame, la CPU solo ha mandado ordenes de dibujo: la
GPU las va haciendo despues, un rato mas tarde. El thread que presenta tiene
que copiar ese frame a la ventana, pero no antes de que la GPU termine de
pintarlo: copiaria un frame a medias. Por eso, al entregar el frame, se pone
una marca (fence) detras de sus ordenes; cuando la GPU llega a la marca, el
frame esta terminado. "Esperar" es esperar a esa marca, y hay dos formas.

**Esperar desde la GPU** (`drv_wait_gpu`):

1. El thread le dice a la GPU: "cuando termines este frame, copialo a la
   ventana".
2. El thread no espera: sigue con lo suyo.
3. La GPU termina de pintar el frame.
4. La GPU, sola, copia el frame a la ventana.

```c
// el thread que presenta, en cada frame (simplificado)
eglWaitSyncKHR(dpy, fence, 0);  // a la GPU: "no sigas hasta pasar la marca"; vuelve enseguida
draw_layers(...);               // orden: copiar el frame a la ventana (a la cola de la GPU)
SDL_GL_SwapWindow(window);      // cambio de imagen
```

**Esperar desde la CPU** (`drv_wait_cpu`):

1. El thread se queda parado.
2. La GPU termina de pintar el frame y avisa.
3. El thread le dice a la GPU: "copia el frame a la ventana".
4. La GPU copia el frame a la ventana.

```c
// el thread que presenta, en cada frame (simplificado)
eglClientWaitSyncKHR(dpy, fence, 0, EGL_FOREVER);  // el thread se para aqui hasta que la GPU pasa la marca
draw_layers(...);                                  // orden: copiar el frame a la ventana
SDL_GL_SwapWindow(window);                         // cambio de imagen
```

La unica diferencia en el codigo es la primera linea: `eglWaitSyncKHR` pone
la espera en la cola de la GPU y vuelve; `eglClientWaitSyncKHR` no vuelve
hasta que la GPU pasa la marca. Estan en `drv_wait_gpu` y `drv_wait_cpu`
(`common.c`, que llaman a `gl_fence_wait_gpu` y `gl_fence_wait_cpu` de
`../gl.c`); el thread es `present_thread` en `backend/sdl/backend.c`.

La diferencia: desde la GPU, la orden de copiar ya esta en la cola cuando el
frame termina, y la copia sale enseguida. Desde la CPU, entre el aviso de la
GPU y la orden de copiar pasa un poco de tiempo (el thread tiene que
despertar), pero el thread sabe seguro que el frame esta terminado.

Que usa cada driver, y por que (probamos las dos formas en cada uno):

- **panfrost usa `drv_wait_gpu`.** Con `drv_wait_cpu`, el thread despertaba
  tarde: la copia llegaba despues del refresco de la pantalla y el frame se
  veia un refresco mas tarde. En la RG CubeXX, frames tardios en batalla: ~70
  con `drv_wait_gpu`, ~390 con `drv_wait_cpu`.
- **mali (libmali) usa `drv_wait_cpu`.** Con `drv_wait_gpu`, libmali a veces
  copiaba el frame a la ventana antes de que estuviera terminado: en pantalla
  salia un frame a medias y los menus de RPG Maker parpadeaban. Con
  `drv_wait_cpu` no pasa (en la RGB30, 31 capturas seguidas del menu de
  batalla, todas completas).
- **mali-g31 no usa el thread.** Con libmali en una Mali-G31 (RX6S), el
  juego esperaba ~16 ms en cada imagen a que el thread mostrara la anterior.
  Sin el thread, con semilla, de ~400 a ~100 frames de mas de 60 ms en el
  mapa y de ~510 a ~210 en batalla (en ~1000). La RG CubeXX tiene la misma
  GPU con panfrost, y alli el thread si ayuda: es cosa de libmali en la G31.

## GpuFrame y Presenter

Los dos estan definidos en `../driver.h`. El backend (`backend/sdl/backend.c`)
los crea y se los pasa a `present` del driver.

### GpuFrame: una imagen lista para presentar

```c
typedef struct GpuFrame {
  int later;   // 1: se puede mostrar mas tarde; 0: hay que mostrarla ya
} GpuFrame;
```

El driver solo ve este campo. El backend guarda el resto (las capas y la marca
de la GPU) en su propia estructura, `SdlFrame`, que lleva `GpuFrame` como
primer campo.

### Como sabe quixjs si `later` es 1

El juego dibuja cada frame en su canvas WebGL. Por dentro, ese canvas es una
textura: una imagen en la memoria de la GPU.

Con `drv_present_previous` (sin el thread) cada frame se muestra un frame mas
tarde. Con una sola textura no se podria: cuando tocara mostrar el frame 1, el
juego ya estaria borrando esa textura para dibujar el frame 2 encima.

Por eso el canvas tiene tres texturas, que aqui llamamos A, B y C. Las tres
estan siempre en la memoria de la GPU; ninguna pasa por la CPU. En cada
momento cada una tiene un papel:

- **donde dibuja el juego**: el JS del juego corre en la CPU y manda ordenes
  de dibujo (WebGL); la GPU las hace y pinta en esta textura el frame que se
  esta haciendo;
- **esperando**: un frame ya terminado (la GPU acabo de pintarlo), que se
  mostrara en el siguiente;
- **en pantalla**: el frame que se ve ahora; la GPU lo copio de esta textura
  a la ventana (`show`, en el backend).

Al acabar cada frame, quixjs les cambia el papel. Asi va, frame a frame:

| frame que se hace | donde dibuja el juego | esperando        | en pantalla      |
|-------------------|-----------------------|------------------|------------------|
| 1                 | A                     | (nada)           | (nada)           |
| 2                 | B                     | A, con el frame 1 | (nada)          |
| 3                 | C                     | B, con el frame 2 | A, con el frame 1 |
| 4                 | A                     | C, con el frame 3 | B, con el frame 2 |
| 5                 | B                     | A, con el frame 4 | C, con el frame 3 |

Al acabar cada frame: la textura donde dibujo el juego pasa a esperar, la que
esperaba pasa a la pantalla, y la que estaba en pantalla queda libre para que
el juego dibuje el frame siguiente. Al empezar, la pantalla tarda dos frames en
tener algo. El juego nunca dibuja en una textura que este esperando o en
pantalla. El cambio de papeles lo hace la CPU, sin mover pixeles: solo cambia
a que textura apunta el canvas (`webgl_canvas_handoff`, `binding/webgl.c`,
llamado al acabar cada frame desde `present_screen`, `core/main.c`).

`later` es 1 si todo lo que hay en pantalla tiene su textura apartada asi. Es
0 en dos casos:

- el canvas WebGL tiene `preserveDrawingBuffer`: el juego pidio seguir
  dibujando sobre lo que ya tenia, asi que no se le puede cambiar la textura;
- en pantalla hay un canvas 2d o un video: no son texturas apartadas, son
  pixeles en la memoria de la CPU que el juego cambia en el frame siguiente.

La cuenta la hace `sdl_present_layers` (`backend/sdl/backend.c`). Los canvas
que estan en pantalla los da `document_screen_canvases`
(`binding/document.c`): los del `body` que no tienen `display: none`,
`visibility: hidden` ni `opacity: 0`.

En Fear & Hunger solo se ve `GameCanvas` (WebGL, sin preserveDrawingBuffer),
asi que `later` es 1, salvo en la pantalla de carga (se ve el `UpperCanvas` 2d
de RPG Maker) y con un video.

### Presenter: lo que el backend sabe hacer con una imagen

```c
typedef struct {
  void (*wait)(GpuFrame *f);
  void (*show)(GpuFrame *f);
  void (*drop)(GpuFrame *f);
} Presenter;
```

| operacion | que hace | en `backend.c` |
|-----------|----------|----------------|
| `wait(f)` | la CPU espera a que la GPU termine de dibujar `f` | `frame_wait` |
| `show(f)` | dibuja `f` en la ventana y la muestra (con vsync) | `frame_show` |
| `drop(f)` | `f` no se vera: la suelta | `frame_drop` |

El driver las combina en su `present`. Las reglas:

- cada imagen acaba en `wait` y `show`, o en `drop`;
- si `later` es 0, la imagen se muestra en la misma llamada;
- el driver puede guardar la imagen para la llamada siguiente, solo esa: el
  backend alterna dos y reutiliza la de antes.

Ejemplo, `mali.c`: muestra la imagen guardada (la anterior) y guarda la nueva.

Un punto nuevo: el campo en `GpuDriver`, su funcion `gpu_<punto>()` en
`../driver.c` (llama al del driver), la llamada en su sitio, su valor en cada
driver y su fila aqui. Si varios drivers hacen lo mismo, la pieza va en
`common.c`.

Un driver nuevo: su archivo, su `extern` en `../driver.h`, su caso en
`gpu_driver_detect` (`../driver.c`) y su columna aqui.
