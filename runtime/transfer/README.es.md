# transfer

[English](README.md) | Español

Transfer Save: pasa las partidas de un juego a otro dispositivo de la misma
red (un PC, una portátil), sin teclado y sin abrir puertos. Se abre desde el
menú de `--launcher`. Viene de tamatama
(https://github.com/lowlevel-1989/tamatama, `src/transfer`), que pasa un
fichero de 768 bytes; aquí son todos los ficheros de `[saves] files`, en un
paquete que se manda por trozos.

```
[game]
title = "Fear & Hunger"
id    = "fear-and-hunger"    # solo entre el mismo juego (por defecto: el título)

[saves]
dir   = "save"   # la carpeta de las partidas, desde game_dir (absoluta: tal cual; sin dir: game_dir)
files = ["*.rpgsave"]        # patrones dentro de dir
```

El archivo de logros (`[saves] achievements`) va en el paquete con las
partidas. Al recibirlo, los logros se vuelven a leer.

La lógica, sin pantallas:

| archivo       | de qué se encarga |
|---------------|-------------------|
| `net.c`       | el socket UDP, los anuncios y el formato de los paquetes |
| `sender.c`    | el envío, con su estado |
| `receiver.c`  | la recepción, con su estado |
| `saves.c`     | busca, empaqueta, guarda (con backup) y restaura las partidas |
| `identity.c`  | el nombre (usuario@hostname (sistema)) y la firma de 4 caracteres, nueva cada vez que se abre Send Save o Receive Save |

Y las pantallas, sobre `launcher/ui.c`:

| archivo      | de qué se encarga |
|--------------|-------------------|
| `transfer.c` | Transfer Save: Send Save, Receive Save y Restore Backup |

## El protocolo

Paquetes UDP de texto en el puerto 47474, todos con la misma primera línea,
`QUIX 1 <tipo> <firma de quien lo manda> [palabras]`, y a veces algo detrás:

```
HELLO S|R <nombre>     cada segundo, a cada dirección de la red y por
                       broadcast; S en Send Save, R en Receive Save
HERE S|R <nombre>      la respuesta a HELLO
OFFER <n>              el número de la oferta, y detrás, en líneas:
                       name, game, size, date (unix) y summary
WAIT                   la oferta ha llegado: se pregunta
OK                     Accept
NO <motivo>            declined, version (otro juego), format, busy o cancelled
DATA <i> <trozos> <bytes> <crc32>   y detrás el trozo i del paquete
                       (16 KB cada uno)
ACK <i>                el trozo i ha llegado: el siguiente
DONE / FAIL <motivo>   guardado, o no (format, crc, write)
CANCEL                 el que envía ya no espera
```

El que envía repite OFFER cada segundo y cada trozo cada 250 ms hasta que le
contestan; el que recibe contesta a cada repetición con su última respuesta.
Así no importa que se pierda un paquete. El CRC32 es del paquete entero.

El paquete es texto y después los bytes:

```
QUIXSAVE 1
<bytes> <ruta relativa>     una línea por fichero
<línea vacía>
<los ficheros, uno detrás de otro>
```

Por qué no hay que abrir puertos: un cortafuegos deja pasar la respuesta a lo
que su equipo ha mandado. Los dos lados mandan HELLO a cada dirección de su
red desde el puerto 47474 al 47474, así que lo que llega del otro ya cuenta
como respuesta. Solo falla en las redes que no dejan hablar entre sí a los
dispositivos (las de invitados, algunas de hoteles).

## Lo que protege

- Solo se escucha con Receive Save abierta, y nada se escribe sin Accept.
- Solo entre el mismo juego (`[game] id`).
- Solo se escriben los ficheros que casan con `[saves] files` (y el archivo
  de logros), y nunca fuera de `[saves] dir`: una ruta con `..` o absoluta
  invalida el paquete.
- Lo nuevo se escribe aparte (`<carpeta de partidas>/.quixjs/incoming`) y se
  cambia con rename. Las partidas que había pasan a
  `<carpeta de partidas>/.quixjs/backup`, y Restore Backup las vuelve a poner.

## Pruebas

`tests/transfer.sh` arranca dos procesos de quixjs en este equipo, uno en Receive Save y
otro en Send Save, manejados con `--script`. Con
`QUIXJS_TRANSFER_HOSTS=127.0.0.1` solo hablan por esa dirección, sin salir a
la red.
