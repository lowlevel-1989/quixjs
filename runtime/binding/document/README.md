# document

English | [Español](README.es.md)

The minimal DOM, split by topic so it can be read: which canvases go to the
screen, the elements, finding them and the document itself.

## Why ../document.c only includes these files

`binding/document.c` is an index file: it includes the parts of this folder in
order and is compiled as a single file. The parts are not compiled by
themselves (`cmake/sources.cmake` only lists `document.c`).

The order matters: a part uses what the parts before it define. A new part
goes into `document.c` in its place, after what it uses. Each part includes
`common.h` (with `#pragma once`), so that it can be read and opened in an
editor by itself.

## Parts, in the order of document.c

| file | what it has |
|------|-------------|
| `common.h` | what the parts share: the elements registered by id and the state of the DOM (the element prototype, the canvases on screen) |
| `screen.c` | which canvases go to the screen: the ones added to the document and visible, in order |
| `element.c` | the elements: their methods (`appendChild`, `setAttribute`, `getBoundingClientRect`...), their prototype and creating them (`createElement`, `createTextNode`) |
| `query.c` | finding elements: the ones registered by id, `getElementById` (also `[html]` of the toml), `getElementsByTagName` and `querySelector` |
| `dom.c` | the document: `title`, the `HTMLElement` constructors, creating it and freeing it |
