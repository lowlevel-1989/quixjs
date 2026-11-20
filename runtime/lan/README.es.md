# lan

[English](README.md) | Español

La red local para juegos: `quix.lan` encuentra los dispositivos de la misma
red que tienen el mismo juego abierto y manda mensajes cortos entre ellos
(por ejemplo la posición de una nave varias veces por segundo). Sin abrir
puertos ni escribir direcciones: va sobre la red de Transfer Save
(`transfer/net.c`: el mismo socket UDP, sus anuncios y su formato de
paquetes), con el rol `G` y paquetes `GAME`. Solo entre dispositivos con el
mismo juego (`[game] id`, o el título).

```js
quix.lan.open();                 // true si va
quix.lan.id; quix.lan.name;      // este dispositivo: su id de 4 caracteres y su nombre
quix.lan.peers();                // [{id, name}]: los demás dispositivos ahora
quix.lan.send('texto');          // a todos (o send(texto, id): a uno)
quix.lan.receive();              // [{from, data}]: lo que llegó desde la última vez
quix.lan.close();
```

`peers()` y `receive()` leen la red: se llaman en cada imagen. Un mensaje
puede perderse o llegar dos veces: se manda el estado entero cada vez, no
cambios. Hasta 8 dispositivos más; uno que no manda nada en 4 segundos sale
de la lista. Mensajes de hasta 4 KB.

| archivo  | qué hace |
|----------|----------|
| `lan.c`  | los dispositivos del mismo juego y la cola de mensajes, sobre `transfer/net.c` |
| `../binding/lan.c` | `quix.lan` en JS |

Lo usa el port de HexGL (`games/hexgl/multiplayer.js`): carreras entre
dispositivos, con las naves de los demás como fantasmas.
