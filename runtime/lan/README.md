# lan

English | [Español](README.es.md)

The local network for games: `quix.lan` finds the devices on the same network
that have the same game open, and sends short messages between them (for
example the position of a ship several times per second). No ports to open
and no addresses to type: it uses the network of Transfer Save
(`transfer/net.c`: the same UDP socket, its announcements and its packet
format), with the role `G` and `GAME` packets. Only between devices with the
same game (`[game] id`, or the title).

```js
quix.lan.open();                 // true if it works
quix.lan.id; quix.lan.name;      // this device: its 4-character id and its name
quix.lan.peers();                // [{id, name}]: the other devices now
quix.lan.send('text');           // to all of them (or send(text, id): to one)
quix.lan.receive();              // [{from, data}]: what arrived since the last call
quix.lan.close();
```

`peers()` and `receive()` read the network: call them every frame. A message
can be lost or arrive twice: send the whole state each time, not changes. Up
to 8 other devices; a device that sends nothing for 4 seconds leaves the
list. Messages up to 4 KB.

| file     | what it does |
|----------|--------------|
| `lan.c`  | the devices of the same game and the queue of messages, over `transfer/net.c` |
| `../binding/lan.c` | `quix.lan` in JS |

Used by the HexGL port (`games/hexgl/multiplayer.js`): races between devices,
with the ships of the others as ghosts.
