// olivia-meta-fast: para el plugin Olivia_MetaControls de RPG Maker MV, en
// value() y setValue() de Game_Switches y Game_Variables.
//
// Olivia_MetaControls permite interruptores y variables "meta" (compartidos
// entre partidas): los marca con <Global> o <Local> en su nombre. Para ello
// cambia value() y setValue(), y en cada lectura o escritura de cualquier
// interruptor o variable pasa dos expresiones regulares por su nombre (y
// descifra sus propios nombres: el plugin esta ofuscado). Una lectura costaba
// ~10 us, y los eventos leen miles por segundo; en Fear & Hunger solo 1 de
// 3601 interruptores es meta.
//
// Con el hook se mira una vez por interruptor o variable si su nombre es
// meta (la misma expresion regular que el plugin). Si lo es, se usa la de
// Olivia; si no, directamente la value() o setValue() que Olivia guardo de
// antes (la que ella misma llamaria: en setValue() Olivia la llama siempre y
// despues, solo si es meta, guarda el valor compartido). Si los nombres cambian (otro $dataSystem) se vuelve a
// mirar. Sin Olivia_MetaControls, la original. Si el hook se puso antes que
// el plugin (y Olivia guardo esa copia como la de antes), se usa la de
// debajo: nunca se llama a si mismo.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_Switches.prototype.value" = "rpgmaker-mv/olivia-meta-fast"
//   "Game_Variables.prototype.value" = "rpgmaker-mv/olivia-meta-fast"
//   "Game_Switches.prototype.setValue" = "rpgmaker-mv/olivia-meta-fast"
//   "Game_Variables.prototype.setValue" = "rpgmaker-mv/olivia-meta-fast"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var META = /<(?:Global|Global Meta)>|<(?:Local|Local Meta|Meta)>/i;
  var state = null;   // se mira la primera vez (ya con los plugins)
  var kinds = new WeakMap();   // lista de nombres -> Uint8Array (1 meta, 2 normal)

  function prepare(self) {
    var M = typeof Olivia === 'object' && Olivia && Olivia.MetaControls;
    if (!M) return false;
    var src = String(original);
    var vars = self instanceof Game_Variables;
    var kind = vars ? 'Variables' : 'Switches';
    // value() o setValue() de Olivia (las suyas miran <Global>)
    if (src.indexOf('<(?:Global|Global Meta)>') < 0) return false;
    var set = src.indexOf('___Game_' + kind + '_setValue___') >= 0;
    var prev = M[set ? '___Game_' + kind + '_setValue___' : '___Game_' + kind + '_value___'];
    // si Olivia guardo una copia de este hook (se puso antes que ella), la de debajo
    while (typeof prev === 'function' && prev.__quixOriginal) prev = prev.__quixOriginal;
    if (typeof prev !== 'function' || prev === original) return false;
    return { prev: prev, vars: vars, set: set };
  }

  function isMeta(names, id) {
    var k = kinds.get(names);
    if (!k || k.length !== names.length) {
      k = new Uint8Array(names.length);
      kinds.set(names, k);
    }
    var kind = k[id];
    if (kind === 0) kind = k[id] = META.test(names[id]) ? 1 : 2;
    return kind === 1;
  }

  // value(id) y setValue(id, value): con call, sin arguments (no crea un
  // objeto en cada llamada)
  var hooked = function (id, value) {
    if (state === null) state = prepare(this);
    if (!state) return original.call(this, id, value);
    var names = state.vars ? $dataSystem.variables : $dataSystem.switches;
    if (state.set) {
      // Olivia: la de antes siempre; lo meta solo dentro del rango
      if (!(id > 0 && id < names.length)) return state.prev.call(this, id, value);
      if (typeof names[id] !== 'string' || isMeta(names, id)) return original.call(this, id, value);
      return state.prev.call(this, id, value);
    }
    if (typeof (names && names[id]) !== 'string') return original.call(this, id, value);   // como siempre (puede fallar igual)
    return isMeta(names, id) ? original.call(this, id, value) : state.prev.call(this, id);
  };
  hooked.__quixOriginal = original;
  return hooked;
}
