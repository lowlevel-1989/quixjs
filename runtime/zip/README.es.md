# zip

[English](README.md) | Español

Leer un `.zip` sin descomprimirlo en disco: su indice (el directorio central)
se lee una vez, y cada archivo se lee de su sitio dentro del zip con `pread`
(varios threads a la vez: los que cargan imagenes). Tambien un zip pegado al
final de otro archivo (el `Game.exe` de un juego de nw.js). Los archivos sin
compresion se leen tal cual; los comprimidos (deflate), con zlib. Sin zip64:
un zip de mas de 4 GB no cabe en una tarjeta FAT.

| archivo     | de que se encarga |
|-------------|-------------------|
| `zip.h`     | abrir, buscar un archivo por su nombre, leerlo entero o como `FILE*`, convertir |
| `zip.c`     | el directorio central, la tabla de busqueda y los datos de cada archivo |
| `convert.c` | pasar un zip comprimido a uno sin compresion, por trozos (los videos grandes no van enteros a memoria), con las mismas rutas y el mismo CRC |

Lo usa la rom ([rom](../rom/README.es.md)).
