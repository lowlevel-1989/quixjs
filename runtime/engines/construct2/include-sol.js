// include-sol: para cr.eventinclude.prototype.run (Construct 2).
//
// Una hoja de eventos incluida dentro de un grupo empieza con la seleccion
// de instancias limpia: Construct 2 hace pushCleanSol y popSol de TODOS los
// tipos de objeto del juego (types_by_index) en cada inclusion y en cada
// imagen. Gunfield tiene ~530 tipos y 23 hojas incluidas en un grupo: unas
// 24500 operaciones por imagen, aunque cada hoja use unos pocos tipos.
//
// Con el hook, si ningun evento por encima de la inclusion tiene condiciones
// (un grupo sin condiciones no elige instancias), solo se limpian los tipos
// que esa hoja elige o cambia: los solModifiers de todos sus eventos y
// subeventos (Construct 2 ya pone ahi los contenedores) y de las hojas que
// incluye, mas las familias de cada uno y los miembros de cada familia. Los
// demas ya estan limpios al entrar (Construct 2 limpia la seleccion despues
// de cada evento de primer nivel, y nadie por encima eligio nada), asi que
// quedan como los habria dejado la original. Los disparadores que llama la
// hoja (funciones, "al crearse"...) limpian los suyos. Con condiciones por
// encima, la original. El conjunto se calcula una vez por inclusion.
// Comprobado en Gunfield: 2,6 millones de lecturas de seleccion en una
// partida; las de tipos fuera del conjunto (los que la hoja solo usa en
// expresiones) encontraron siempre la seleccion limpia.
//
// Uso (config.toml o port.toml):
//   [hooks]
//   "cr.eventinclude.prototype.run" = "construct2/include-sol"
//
// El archivo es una funcion que recibe la original y devuelve la nueva. La
// llama runtime/hooks/hooks.c (hooks_install) al instalar el hook, cuando el
// juego ya cargo sus scripts. Ver runtime/engines/README.md.
function (original) {
  // solo la de Construct 2: limpia types_by_index y corre include_sheet
  var text = String(original).replace(/\s+/g, '');
  if (text.indexOf('pushCleanSol(this.runtime.types_by_index)') < 0 ||
      text.indexOf('include_sheet.run(true)') < 0) {
    return original;
  }

  function addType(set, list, t) {
    if (!t || set.has(t)) return;
    set.add(t);
    list.push(t);
  }

  // los tipos que puede tocar una hoja (y lo que incluye)
  function typesOf(sheet, set, list, seen) {
    if (!sheet || seen.has(sheet)) return;
    seen.add(sheet);
    (function walk(events) {
      for (var i = 0; events && i < events.length; i++) {
        var ev = events[i];
        if (ev.include_sheet) typesOf(ev.include_sheet, set, list, seen);
        var mods = [ev.solModifiers, ev.solModifiersIncludingParents];
        for (var m = 0; m < mods.length; m++) {
          for (var j = 0; mods[m] && j < mods[m].length; j++) addType(set, list, mods[m][j]);
        }
        walk(ev.subevents);
      }
    })(sheet.events);
  }

  function closure(list, set) {
    // familias de cada tipo y miembros de cada familia (de mas no hace dano)
    for (var i = 0; i < list.length; i++) {
      var t = list[i];
      var fam = t.families || [];
      for (var f = 0; f < fam.length; f++) addType(set, list, fam[f]);
      var mem = t.members || [];
      for (var k = 0; k < mem.length; k++) addType(set, list, mem[k]);
      var box = t.is_contained ? t.container || [] : [];
      for (var c = 0; c < box.length; c++) addType(set, list, box[c]);
    }
    return list;
  }

  // ningun evento por encima elige instancias: sin condiciones
  function noPicksAbove(inc) {
    for (var p = inc.parent; p; p = p.parent) {
      if (p.conditions && p.conditions.length) return false;
    }
    return true;
  }

  // la seleccion actual del tipo es "todas" (como la deja pushCleanSol)
  function clean(t) {
    var sol = t.solstack[t.cur_sol];
    return sol.select_all && !sol.else_instances.length;
  }

  var cache = new WeakMap();   // inclusion -> sus tipos, o null (la original)

  return function () {
    if (!this.parent || !this.include_sheet) return original.apply(this, arguments);
    var types = cache.get(this);
    if (types === undefined) {
      types = null;
      if (noPicksAbove(this)) {
        var set = new Set(), list = [];
        typesOf(this.include_sheet, set, list, new Set());
        types = closure(list, set);
      }
      cache.set(this, types);
    }
    if (!types) return original.apply(this, arguments);
    // solo hace falta guardar la seleccion de los tipos que no esta limpia:
    // guardar una limpia y despues volver a ella es lo mismo que dejarla
    // limpia al acabar
    var dirty = [];
    for (var i = 0; i < types.length; i++) if (!clean(types[i])) dirty.push(types[i]);
    var runtime = this.runtime;
    if (dirty.length) runtime.pushCleanSol(dirty);
    if (!this.include_sheet.hasRun) this.include_sheet.run(true);
    for (var j = 0; j < types.length; j++) {
      var sol = types[j].solstack[types[j].cur_sol];
      if (!sol.select_all) sol.select_all = true;
      if (sol.else_instances.length) sol.else_instances.length = 0;
    }
    if (dirty.length) runtime.popSol(dirty);
  };
}
