// games/soccer-chapa/game/src/scenes/rename.js: usar una carta de cambio de
// nombre (salen en los sobres: games/soccer-chapa/game/src/gacha/pack.js), en
// tres paginas del menu (games/soccer-chapa/game/src/scenes/menu/pages.js):
//   rename       las que tienes (nombre, apellido, nombre y apellido): si
//                tienes varias iguales, amontonadas; A en una
//   rename_pick  elegir la chapa
//   rename_opts  elegir entre OPTIONS nombres al azar, cada uno en la carta
//                de la chapa. La carta se gasta al verlos: no hay vuelta
//                atras (B no vuelve) y se guarda en el perfil (renaming)
//                para seguir donde se quedo si se apaga la consola
// Nombre y apellido: primero OPTIONS nombres y despues OPTIONS apellidos, y
// es la unica que deja mantener el que tiene (una opcion mas en cada paso).
SC.scenes.rename = { OPTIONS: 5, now: null };   // now: la que se esta usando (antes de gastarla)

// las opciones de un paso: { first, last } de cada una (keep: la de mantener)
function renameOptions(r, cap) {
  var cur = SC.data.names.split(cap.name), list = [];
  var step = r.kind === 'full' ? r.step : r.kind;
  while (list.length < SC.scenes.rename.OPTIONS) {
    var n = step === 'first' ? SC.data.names.firstName() : SC.data.names.lastName();
    if (n !== cur[step] && list.indexOf(n) < 0) list.push(n);
  }
  var opts = list.map(function (n) {
    return step === 'first' ? { first: n, last: r.first ? r.last : cur.last } : { first: r.first || cur.first, last: n };
  });
  if (r.kind === 'full') opts.push({ first: r.first || cur.first, last: cur.last, keep: true });
  return opts;
}

// cuantas tiene el perfil p, de todas
SC.scenes.rename.count = function (p) {
  var r = p.renames || {};
  return (r.first || 0) + (r.last || 0) + (r.full || 0);
};

SC.scenes.rename.page = function (s, page, go) {
  var me = SC.save.profile.me(), R = SC.ui.card.rename, team = function () { return SC.save.profile.matchTeam(me); };
  me.renames = me.renames || { first: 0, last: 0, full: 0 };
  if (page === 'rename') {
    var have = R.KINDS.filter(function (kind) { return me.renames[kind] > 0; });
    return { title: 'team.rename', back: 'team', items: have.map(function (kind) {   // las que tienes, amontonadas
      return { label: 'rename.label', bg: 'card_name_' + R.RARITY[kind], stack: me.renames[kind],
               act: function () { SC.scenes.rename.now = { kind: kind, step: 'first' }; go('rename_pick')(); },
               art: function () { return R.art(kind); } };
    }) };
  }
  var r = me.renaming || SC.scenes.rename.now;            // gastada y a medias: sigue ahi
  if (page === 'rename_pick') {
    return { title: 'rename.pickcap', back: 'rename', items: [], pick: {
      list: me.cards.slice(), tag: 'rename.pickcap', next: 'rename_opts', mark: function () { return false; },
      view: function (c) { return c; },
      set: function (c) { r.capId = c.id; r.opts = null; } } };
  }
  var cap = me.cards.filter(function (c) { return c.id === r.capId; })[0];
  if (!r.opts) {                                         // al verlos, la carta se gasta
    if (r.step === 'first') me.renames[r.kind]--;
    r.opts = renameOptions(r, cap);
    me.renaming = r; SC.save.profile.save();
  }
  var step = r.kind === 'full' ? r.step : r.kind;
  return { title: 'rename.pick' + step, back: null, items: r.opts.map(function (o) {
    var full = o.first + ' ' + o.last;                   // el nombre nuevo, grande encima: se lee en una consola
    return { label: o.keep ? SC.lang.t('rename.keep') + ': ' + full : full, labelSize: 36, team: team,
             player: function () { return Object.assign({}, cap, { name: o.first + ' ' + o.last }); },
             act: function () {
               if (r.kind === 'full' && r.step === 'first') {   // y ahora el apellido
                 r.first = o.first; r.last = o.last; r.step = 'last'; r.opts = null; go('rename_opts')();
                 return;
               }
               cap.name = o.first + ' ' + o.last;
               SC.scenes.rename.now = null; me.renaming = null; SC.save.profile.save(); go(SC.scenes.rename.count(me) ? 'rename' : 'team')();   // sin mas, a Mi equipo
             } };
  }) };
};
