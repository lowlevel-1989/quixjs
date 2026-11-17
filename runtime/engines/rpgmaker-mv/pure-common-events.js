// pure-common-events: para Game_CommonEvent.prototype.update (RPG Maker MV).
//
// Un evento comun paralelo se interpreta entero en cada imagen mientras su
// interruptor esta activo. Muchos solo miran el estado y lo apuntan (en Fear
// & Hunger, 8 activos siempre: "tiene el grupo esta habilidad?" -> un
// interruptor; unos 490 comandos por imagen) y casi siempre dan lo mismo.
//
// Un evento es "puro" si solo tiene: condiciones (111) de interruptor,
// variable, actor, oro, objeto, arma o armadura; cambios de interruptores
// (121) y de variables (122) con una constante u otra variable; llamadas
// (117) a otros eventos puros; comentarios, salir (115) y fin de rama. Sin
// esperas, sin azar, sin scripts: con el mismo estado da el mismo resultado.
//
// Con el hook, antes de repetir uno puro se leen los valores que mira y
// cambia (con las mismas funciones del juego, asi que los interruptores
// calculados de plugins como Olivia_MetaControls tambien cuentan). Si son
// los mismos que dejo su ultima ejecucion, no se repite: haria lo mismo. Si
// algo cambio, se ejecuta como siempre. Los demas eventos, como siempre.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_CommonEvent.prototype.update" = "rpgmaker-mv/pure-common-events"
//
// El archivo es una funcion que recibe la original y devuelve la nueva.
function (original) {
  var MV = 'function(){if(this._interpreter){if(!this._interpreter.isRunning()){this._interpreter.setup(this.list());}' +
    'this._interpreter.update();}}';
  if (String(original).replace(/\s+/g, '') !== MV) return original;

  var cache = new WeakMap();   // lista -> lectores (o null si no es pura)
  // evento -> { list, state }: aparte (Game_CommonEvent va en las partidas
  // dentro de $gameMap: nada de esto se guarda)
  var last = new WeakMap();

  // lo que hay que leer de cada comando; false si no es puro
  function readers(list, out, seen) {
    for (var i = 0; i < list.length; i++) {
      var c = list[i], k = c.code, p = c.parameters;
      switch (k) {
        case 0: case 108: case 408: case 411: case 412: case 115:
          break;
        case 111:
          if (!condition(p, out)) return false;
          break;
        case 121:
          for (var s = p[0]; s <= p[1]; s++) out.push(switchReader(s));
          break;
        case 122:
          if (p[3] !== 0 && p[3] !== 1) return false;
          for (var v = p[0]; v <= p[1]; v++) out.push(variableReader(v));
          if (p[3] === 1) out.push(variableReader(p[4]));
          break;
        case 117:
          var ev = $dataCommonEvents[p[0]];
          if (!ev || !ev.list) return false;
          if (seen.indexOf(p[0]) < 0) {
            seen.push(p[0]);
            if (!readers(ev.list, out, seen)) return false;
          }
          break;
        default:
          return false;
      }
    }
    return true;
  }

  function switchReader(id) { return function () { return $gameSwitches.value(id); }; }
  function variableReader(id) { return function () { return $gameVariables.value(id); }; }

  // la condicion entera (como command111), sin las ramas: su resultado
  function condition(p, out) {
    switch (p[0]) {
      case 0:
        out.push(switchReader(p[1]));
        return true;
      case 1:
        out.push(variableReader(p[1]));
        if (p[2] !== 0) out.push(variableReader(p[3]));
        return true;
      case 4:
        out.push(function () {
          var actor = $gameActors.actor(p[1]);
          if (!actor) return null;
          var n = p[3];
          switch (p[2]) {
            case 0: return $gameParty.members().contains(actor);
            case 1: return actor.name() === n;
            case 2: return actor.isClass($dataClasses[n]);
            case 3: return actor.hasSkill(n);
            case 4: return actor.hasWeapon($dataWeapons[n]);
            case 5: return actor.hasArmor($dataArmors[n]);
            case 6: return actor.isStateAffected(n);
          }
          return null;
        });
        return p[2] >= 0 && p[2] <= 6;
      case 7:
        out.push(function () { return $gameParty.gold(); });
        return true;
      case 8:
        out.push(function () { return $gameParty.hasItem($dataItems[p[1]]); });
        return true;
      case 9:
        out.push(function () { return $gameParty.hasItem($dataWeapons[p[1]], p[2]); });
        return true;
      case 10:
        out.push(function () { return $gameParty.hasItem($dataArmors[p[1]], p[2]); });
        return true;
    }
    return false;
  }

  function analyze(list) {
    if (cache.has(list)) return cache.get(list);
    var out = [];
    var r = list && readers(list, out, []) && out.length > 0 ? out : null;
    cache.set(list, r);
    return r;
  }

  function snapshot(rd) {
    var s = new Array(rd.length);
    for (var i = 0; i < rd.length; i++) s[i] = rd[i]();
    return s;
  }

  function same(a, b) {
    if (!a || a.length !== b.length) return false;
    for (var i = 0; i < a.length; i++) if (a[i] !== b[i]) return false;
    return true;
  }

  return function () {
    var it = this._interpreter;
    if (!it || it.isRunning()) return original.call(this);
    var list = this.list();
    var rd = analyze(list);
    if (!rd) return original.call(this);
    var prev = last.get(this);
    // el estado que dejo su ultima ejecucion: repetirla no cambia nada
    if (prev && prev.list === list && same(prev.state, snapshot(rd))) return;
    original.call(this);
    last.set(this, { list: list, state: it.isRunning() ? null : snapshot(rd) });
  };
}
