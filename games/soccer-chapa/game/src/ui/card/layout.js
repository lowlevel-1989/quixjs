// games/soccer-chapa/game/src/ui/card/layout.js: donde va cada cosa en la
// carta de cada posicion (coordenadas de la carta, 320 x 448). Las ventanas
// las dibuja games/soccer-chapa/game/tools/ui/layouts.py: si cambian alli,
// cambiar aqui.
//   cap    centro y tamano de la chapa
//   name   x, y, giro, tamano de letra, cursiva, largo maximo, anclaje (0:
//          a la izquierda; sin el, centrado) y 'stack' (una palabra por linea)
//   stats  x, y de la fila de stats (centrada); con 'col', en columna
//          alineada a la derecha; con 'table', una fila de la tabla por stat
//   index  la insignia con la media (el promedio de las 4 stats): x, y, giro;
//          a la misma distancia de la linea de dentro del marco en todas
SC.ui.card.layout = {
  gk: {
    cap: [160, 193, 0.6], name: [160, 330, 0, 18],
    stats: [160, 408], index: [[55, 55, 0]]
  },
  def: {
    cap: [190, 180, 0.56], name: [40, 272, -Math.PI / 2, 26, false, 290],
    stats: [184, 408], index: [[55, 55, 0]]
  },
  mid: {
    cap: [160, 180, 0.62], name: [160, 318, 0, 18],
    stats: [160, 350], index: [[61, 61, 0]]
  },
  // las miticas, por modelo (games/soccer-chapa/game/tools/ui/specials.py)
  mythic0: { cap: [160, 194, 0.68], name: [160, 369, 0, 18], stats: [160, 414], index: [[77, 107, 0]] },
  mythic1: { cap: [196, 170, 0.78], name: [38, 224, -Math.PI / 2, 38, false, 370], stats: [196, 414], index: [[196, 356, 0]] },
  mythic2: { cap: [160, 170, 0.58], name: [160, 378, 0, 16], stats: [160, 408], index: [[52, 52, 0]] },
  mythic3: {
    cap: [76, 78, 0.44], name: [134, 78, 0, 19, false, 100, 0, 'stack'],
    stats: [176, 258, 'table'], index: [[262, 78, 0]]
  },
  mythic4: { cap: [160, 176, 0.72], name: [160, 370, 0, 22, false, 260], stats: [160, 406], index: [[258, 300, 0]] },
  fwd: {
    cap: [160, 230, 0.7], name: [160, 374, 0, 20],
    stats: [160, 412], index: [[55, 55, 0]]
  },
  // las de lo que no es chapa (games/soccer-chapa/game/tools/ui/items.py): solo el dibujo y el nombre
  ball: { cap: [160, 178, 1], name: [160, 372, 0, 20] },
  goal: { cap: [160, 240, 1], name: [160, 368, 0, 20] },   // la boca, sobre la linea de gol de la carta (y 286)
  kit: { cap: [160, 190, 1], name: [160, 369, 0, 20] },
  name: { cap: [160, 190, 1], name: [160, 369, 0, 20] }
};
