// fs.writeFileSync con un texto y fs.readFileSync con encoding utf8: vuelve
// el mismo texto, tambien con ceros dentro (RPG Maker MZ guarda sus partidas
// asi: pako.deflate(json, { to: 'string' }), caracteres de 0 a 255) y con
// letras de fuera de ASCII. Antes se cortaba en el primer cero y Continue no
// veia ninguna partida.

var fs = require('fs');
var file = 'roundtrip.tmp';   // en la carpeta del juego (el directorio de nw.js); se borra al final

function roundTrip(text, what) {
  fs.writeFileSync(file, text);
  T.equal(fs.readFileSync(file, { encoding: 'utf8' }), text, what);
}

var bytes = '';
for (var i = 0; i < 256; i++) bytes += String.fromCharCode(i);
roundTrip('x\u0000y', 'a zero in the middle');
roundTrip(bytes + bytes, 'every char from 0 to 255 (a pako string)');
roundTrip('\u0000año 漢字 😀\u0000', 'zeros, accents, CJK and an emoji');
roundTrip('plain text', 'text without zeros');
fs.unlinkSync(file);
T.done();
