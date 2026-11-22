# zip

English | [Español](README.es.md)

Read a `.zip` without unzipping it on disk: its index (the central directory)
is read once, and each file is read from its place inside the zip with
`pread` (several threads at once: the ones that load images). A zip pasted
at the end of another file (the `Game.exe` of a nw.js game) is read too.
Files without compression are read as they are; compressed ones (deflate),
with zlib. No zip64: a zip over 4 GB does not fit on a FAT card.

| file        | what it does |
|-------------|--------------|
| `zip.h`     | open, find a file by name, read it whole or as a `FILE*`, convert |
| `zip.c`     | the central directory, the search table and the data of each file |
| `convert.c` | pass a compressed zip to one without compression, in chunks (big videos do not go whole into memory), with the same paths and CRC |

It is used by the rom ([rom](../rom/README.md)).
