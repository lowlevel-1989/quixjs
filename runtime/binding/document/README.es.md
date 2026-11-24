# document

[English](README.md) | Español

El DOM minimo, partido por temas para que se pueda leer: que canvas van a la
pantalla, los elementos, buscarlos y el document mismo.

## Por que ../document.c solo incluye estos archivos

`binding/document.c` es un archivo indice: incluye las partes de esta carpeta
en orden y se compila como un solo archivo. Las partes no se compilan solas
(`cmake/sources.cmake` solo lista `document.c`).

El orden importa: una parte usa lo que definen las anteriores. Una parte nueva
va en `document.c` en su sitio, despues de lo que usa. Cada parte incluye
`common.h` (con `#pragma once`), para que se pueda leer y abrir en el editor
por separado.

## Partes, en el orden de document.c

| archivo | que tiene |
|---------|-----------|
| `common.h` | lo que comparten las partes: los elementos registrados por id y el estado del DOM (el prototipo de los elementos, los canvas en pantalla) |
| `screen.c` | que canvas van a la pantalla: los agregados al documento y visibles, en orden |
| `element.c` | los elementos: sus metodos (`appendChild`, `setAttribute`, `getBoundingClientRect`...), su prototipo y crearlos (`createElement`, `createTextNode`) |
| `query.c` | buscar elementos: los registrados por id, `getElementById` (tambien `[html]` del toml), `getElementsByTagName` y `querySelector` |
| `dom.c` | el document: `title`, los constructores de `HTMLElement`, crearlo y liberarlo |
