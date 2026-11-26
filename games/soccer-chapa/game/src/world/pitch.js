// games/soccer-chapa/game/src/world/pitch.js: las medidas del campo, en las
// mismas unidades del render de Blender (games/soccer-chapa/game/tools/render/scene.py,
// modo pitch), y donde cae cada cosa de la imagen games/soccer-chapa/game/assets/pitch.jpg.
SC.world.pitch = {
  L: 7.0, W: 4.6,           // largo y ancho entre lineas
  goalW: 1.0, goalD: 0.32,  // la porteria: boca y fondo
  areaD: 0.4, areaW: 1.2,   // el area pequeña: fondo y ancho (games/soccer-chapa/game/tools/render/scene.py)
  wall: 0.0,                // el borde donde rebota todo: en las lineas (cada
                            // campo lo dibuja a su manera: madera, muralla...)
  capR: 0.16, ballR: 0.08,      // la bola, la mitad de la chapa
  // la imagen: 2380x1360 px; la camara del render abarca 14 unidades de ancho
  image: { w: 2380, h: 1360, span: 14.0 }
};
SC.world.pitch.pxPerUnit = SC.world.pitch.image.w / SC.world.pitch.image.span;
// los limites donde rebotan las chapas (el borde de la mesa)
SC.world.pitch.bounds = function () {
  var p = SC.world.pitch;
  return { x: p.L / 2 + p.wall, y: p.W / 2 + p.wall };
};
// donde empiezan las 5 chapas de un equipo con esa formacion
// (games/soccer-chapa/game/src/match/lineup.js: SPOTS): side -1 a la izquierda, 1 a la derecha
SC.world.pitch.formation = function (side, formation) {
  var spots = SC.match.lineup.SPOTS[formation] || SC.match.lineup.SPOTS['1-2-2'];
  return spots.map(function (s) { return { x: s[0] * side, y: s[1] }; });
};
