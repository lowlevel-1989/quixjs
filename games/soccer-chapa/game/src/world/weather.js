// games/soccer-chapa/game/src/world/weather.js: el clima del campo y como cambia
// el movimiento. friction: lo que frena el fieltro (unidades/s cada
// segundo); bounce: lo que rebota una chapa contra otra o contra el borde.
// El fieltro frena mucho: el golpe sale seco y rapido pero no llega lejos
// (asi el pase hay que medirlo). En seco, un golpe al 100 % lleva la bola
// unas 3.5 unidades (media cancha: mide 7; games/soccer-chapa/game/src/match/rules.js: MAX_SPEED).
SC.world.weather = {
  kinds: {
    dry:  { friction: 4.2, bounce: 0.80, ballFriction: 4.4 },
    rain: { friction: 3.15, bounce: 0.70, ballFriction: 3.3 },
    snow: { friction: 5.2, bounce: 0.60, ballFriction: 5.8 }     // la nieve frena mas y rebota menos
  },
  current: 'dry'
};
SC.world.weather.get = function () { return SC.world.weather.kinds[SC.world.weather.current]; };

// el terreno de un partido: seco, lluvia o nieve. Con seed (un texto: en el
// torneo, la sala y el partido) sale siempre el mismo, asi todas las consolas
// que juegan o miran ese partido ven el mismo campo
SC.world.weather.roll = function (seed) {
  var x = Math.random();
  if (seed) {
    var h = 2166136261;
    for (var i = 0; i < seed.length; i++) h = Math.imul(h ^ seed.charCodeAt(i), 16777619);
    x = (h >>> 0) / 4294967296;
  }
  SC.world.weather.current = x < 0.5 ? 'dry' : x < 0.8 ? 'rain' : 'snow';
};
