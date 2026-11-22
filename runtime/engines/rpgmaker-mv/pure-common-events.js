// pure-common-events: para Game_CommonEvent.prototype.update y
// Game_Event.prototype.updateParallel (RPG Maker MV).
//
// Un evento comun paralelo se interpreta entero en cada imagen mientras su
// interruptor esta activo, y un evento del mapa con una pagina en proceso
// paralelo igual. Muchos solo miran el estado y lo apuntan (en Fear &
// Hunger, 8 eventos comunes activos siempre: "tiene el grupo esta
// habilidad?" -> un interruptor; en Level8_A, un evento que copia en 16
// variables la posicion de 8 eventos) y casi siempre dan lo mismo.
//
// Una lista es "pura" si solo tiene: condiciones (111) de interruptor,
// variable, actor, oro, objeto, arma o armadura; cambios de interruptores
// (121) y de variables (122) con una constante, otra variable o un dato del
// juego (posicion de un personaje, objetos del grupo...); llamadas (117) a
// otros eventos puros; comentarios, salir (115) y fin de rama. Sin esperas,
// sin azar, sin scripts: con el mismo estado da el mismo resultado.
//
// Con el hook, al ejecutar una pura se leen los valores que mira y cambia
// antes y despues (con las mismas funciones del juego, asi que los
// interruptores calculados de plugins como Olivia_MetaControls tambien
// cuentan). Si la ejecucion no cambio nada y los valores siguen siendo esos,
// la siguiente no se hace: tampoco cambiaria nada. Si algo cambio (o la
// ultima ejecucion cambio algo), se ejecuta como siempre. Lo demas, como
// siempre.
//
// Referencias (el codigo que reproduce o reescribe; gracias a sus autores):
//   nucleo de RPG Maker MV (corescript, KADOKAWA, MIT), commit 182e314:
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_CommonEvent.js#L39-L47 (update)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Event.js#L321-L329 (updateParallel)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Interpreter.js#L437-L578 (command111)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Interpreter.js#L677-L718 (command121, command122)
//   https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/rpg_objects/Game_Interpreter.js#L720-L808 (gameDataOperand)
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "Game_CommonEvent.prototype.update" = "rpgmaker-mv/pure-common-events"
//   "Game_Event.prototype.updateParallel" = "rpgmaker-mv/pure-common-events"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // MODEL: las funciones que espera el hook, tal cual (las de RPG Maker MV o
  // las del plugin; ver Referencias arriba), copiadas del juego con
  // tools/rpgmaker_mv_model.py. No se ejecutan: son el modelo con el que se
  // compara.
  var MODEL = {
    'Game_CommonEvent.update': function() {
        if (this._interpreter) {
            if (!this._interpreter.isRunning()) {
                this._interpreter.setup(this.list());
            }
            this._interpreter.update();
        }
    },
    'Game_Event.updateParallel': function() {
        if (this._interpreter) {
            if (!this._interpreter.isRunning()) {
                this._interpreter.setup(this.list(), this._eventId);
            }
            this._interpreter.update();
        }
    },
    'Game_Interpreter.command122': function() {
        var value = 0;
        switch (this._params[3]) { // Operand
            case 0: // Constant
                value = this._params[4];
                break;
            case 1: // Variable
                value = $gameVariables.value(this._params[4]);
                break;
            case 2: // Random
                value = this._params[5] - this._params[4] + 1;
                for (var i = this._params[0]; i <= this._params[1]; i++) {
                    this.operateVariable(i, this._params[2], this._params[4] + Math.randomInt(value));
                }
                return true;
                break;
            case 3: // Game Data
                value = this.gameDataOperand(this._params[4], this._params[5], this._params[6]);
                break;
            case 4: // Script
                value = eval(this._params[4]);
                break;
        }
        for (var i = this._params[0]; i <= this._params[1]; i++) {
            this.operateVariable(i, this._params[2], value);
        }
        return true;
    }
  };
  // sameCode (runtime/engines/same-code.js): la funcion del juego es igual que
  // el modelo, sin contar espacios? Si un plugin la cambio, el hook deja la
  // suya y no hace nada
  var sameCode = quix.engine.sameCode;
  // los dos son iguales salvo el numero de evento que recibe setup
  var isEvent = sameCode(original, MODEL['Game_Event.updateParallel']);
  if (!isEvent && !sameCode(original, MODEL['Game_CommonEvent.update'])) return original;

  var cache = new WeakMap(); // lista -> lectores (o null si no es pura)
  // evento -> { list, state }: aparte (Game_CommonEvent y Game_Event van en
  // las partidas dentro de $gameMap: nada de esto se guarda)
  var last = new WeakMap();
  // 122 con un dato del juego: solo si command122 es el de RPG Maker MV (el
  // valor sale de gameDataOperand, que el lector llama igual)
  var gameData = null;

  function gameDataOk() {
    if (gameData === null) {
      var I = typeof Game_Interpreter === 'function' && Game_Interpreter.prototype;
      gameData = !!(
        I &&
        typeof I.gameDataOperand === 'function' &&
        sameCode(I.command122, MODEL['Game_Interpreter.command122'])
      );
    }
    return gameData;
  }

  // lo que hay que leer de cada comando; false si no es puro. Cada valor una
  // sola vez: keys guarda los que ya estan en out (en SKILLSdemonSeedAVAILABLE
  // de Fear & Hunger, el interruptor 491 salia 33 veces y la variable 34
  // unas 20: se comprueban en cada actualizacion)
  function add(out, keys, key, fn) {
    if (keys[key]) return;
    keys[key] = true;
    out.push(fn);
  }

  function readers(list, out, seen, keys) {
    for (var i = 0; i < list.length; i++) {
      var c = list[i],
        k = c.code,
        p = c.parameters;
      switch (k) {
        case 0:
        case 108:
        case 408:
        case 411:
        case 412:
        case 115:
          break;
        case 111:
          if (!condition(p, out, keys)) return false;
          break;
        case 121:
          for (var s = p[0]; s <= p[1]; s++) add(out, keys, 's' + s, switchReader(s));
          break;
        case 122:
          if (p[3] === 3) {
            if (!gameDataOk()) return false;
            add(out, keys, 'd' + p[4] + ',' + p[5] + ',' + p[6], gameDataReader(p));
            add(out, keys, 'm', mapReader);
          } else if (p[3] !== 0 && p[3] !== 1) {
            return false;
          }
          for (var v = p[0]; v <= p[1]; v++) add(out, keys, 'v' + v, variableReader(v));
          if (p[3] === 1) add(out, keys, 'v' + p[4], variableReader(p[4]));
          break;
        case 117:
          var ev = $dataCommonEvents[p[0]];
          if (!ev || !ev.list) return false;
          if (seen.indexOf(p[0]) < 0) {
            seen.push(p[0]);
            if (!readers(ev.list, out, seen, keys)) return false;
          }
          break;
        default:
          return false;
      }
    }
    return true;
  }

  function switchReader(id) {
    return function () {
      return $gameSwitches.value(id);
    };
  }
  function variableReader(id) {
    return function () {
      return $gameVariables.value(id);
    };
  }
  // el dato como lo pide command122, con el interprete del evento (para un
  // personaje mira su mapa y su evento)
  function gameDataReader(p) {
    return function (it) {
      return it.gameDataOperand(p[4], p[5], p[6]);
    };
  }
  // el interprete de un evento comun guarda el mapa de su ultima ejecucion:
  // si cambia de mapa, se ejecuta otra vez
  function mapReader() {
    return $gameMap.mapId();
  }

  // la condicion entera (como command111), sin las ramas: su resultado
  function condition(p, out, keys) {
    switch (p[0]) {
      case 0:
        add(out, keys, 's' + p[1], switchReader(p[1]));
        return true;
      case 1:
        add(out, keys, 'v' + p[1], variableReader(p[1]));
        if (p[2] !== 0) add(out, keys, 'v' + p[3], variableReader(p[3]));
        return true;
      case 4:
        if (!(p[2] >= 0 && p[2] <= 6)) return false;
        add(out, keys, 'a' + p[1] + ',' + p[2] + ',' + p[3], function () {
          var actor = $gameActors.actor(p[1]);
          if (!actor) return null;
          var n = p[3];
          switch (p[2]) {
            case 0:
              return $gameParty.members().contains(actor);
            case 1:
              return actor.name() === n;
            case 2:
              return actor.isClass($dataClasses[n]);
            case 3:
              return actor.hasSkill(n);
            case 4:
              return actor.hasWeapon($dataWeapons[n]);
            case 5:
              return actor.hasArmor($dataArmors[n]);
            case 6:
              return actor.isStateAffected(n);
          }
          return null;
        });
        return true;
      case 7:
        add(out, keys, 'g', function () {
          return $gameParty.gold();
        });
        return true;
      case 8:
        add(out, keys, 'i' + p[1], function () {
          return $gameParty.hasItem($dataItems[p[1]]);
        });
        return true;
      case 9:
        add(out, keys, 'w' + p[1] + ',' + p[2], function () {
          return $gameParty.hasItem($dataWeapons[p[1]], p[2]);
        });
        return true;
      case 10:
        add(out, keys, 'r' + p[1] + ',' + p[2], function () {
          return $gameParty.hasItem($dataArmors[p[1]], p[2]);
        });
        return true;
    }
    return false;
  }

  function analyze(list) {
    if (cache.has(list)) return cache.get(list);
    var out = [];
    // sin lectores (solo comentarios, por ejemplo): no hace nada
    var r = list && readers(list, out, [], {}) ? out : null;
    cache.set(list, r);
    return r;
  }

  function snapshot(rd, it) {
    var s = new Array(rd.length);
    for (var i = 0; i < rd.length; i++) s[i] = rd[i](it);
    return s;
  }

  // lo mismo que same(state, snapshot(rd, it)), sin crear el array y
  // parando en la primera diferencia (se mira en cada actualizacion de cada
  // evento en paralelo)
  function unchanged(state, rd, it) {
    if (!state || state.length !== rd.length) return false;
    for (var i = 0; i < rd.length; i++) if (rd[i](it) !== state[i]) return false;
    return true;
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
    // la ultima ejecucion no cambio nada y todo sigue igual: esta tampoco
    var prev = last.get(this);
    if (prev && prev.list === list && unchanged(prev.state, rd, it)) return;
    // lo mismo que la original (es el modelo), mirando antes y despues
    if (isEvent) it.setup(list, this._eventId);
    else it.setup(list);
    var before = snapshot(rd, it);
    it.update();
    var quiet = !it.isRunning() && same(before, snapshot(rd, it));
    last.set(this, { list: list, state: quiet ? before : null });
  };
}
