// playthrough: la partida en curso de RPG Maker MV, para los trucos y los
// logros (no es un hook: lo usa runtime/cheats, y de ahi los logros).
//
// En RPG Maker MV la partida en curso es $gameSystem: es nueva al empezar una
// partida y la que se guarda y se carga con ella. Al arrancar, el juego ya
// crea uno (para el titulo) que no es una partida, y algunos plugins hacen
// otro para su escena de antes del titulo (HIME_PreTitleEvents en Fear &
// Hunger): solo cuenta el $gameSystem con el que se juega en el mapa
// (Scene_Map exacto: la escena de antes del titulo es una subclase). Lo que se apunta en
// $gameSystem._quix viaja con la partida:
//   cheats  true: en esta partida se usaron trucos (sin logros para siempre)
//   orig    el valor de cada simbolo antes de que un truco lo cambiara (para
//           devolverlo si el truco se desactiva)
({
  live: null,   // el $gameSystem con el que ya se ha jugado en el mapa
  detect: function () {
    return (typeof Utils === 'function' || typeof Utils === 'object') && Utils !== null &&
           Utils.RPGMAKER_NAME === 'MV';
  },
  current: function () {
    if (typeof $gameSystem !== 'object' || !$gameSystem) return null;
    var scene = typeof SceneManager === 'object' || typeof SceneManager === 'function' ? SceneManager._scene : null;
    if (scene && typeof Scene_Map === 'function' && scene.constructor === Scene_Map) this.live = $gameSystem;
    return this.live === $gameSystem ? $gameSystem : null;
  },
  get: function (key) {
    var s = this.current();
    return s && s._quix ? s._quix[key] : undefined;
  },
  set: function (key, value) {
    var s = this.current();
    if (!s) return false;
    if (!s._quix) s._quix = {};
    s._quix[key] = value;
    return true;
  }
})
