// los hooks de Construct 2: clear-array deja los arrays igual que
// arr.length = n; effects-direct dibuja directo un objeto que "pre-dibuja"
// con un efecto que solo cambia el color, y nada mas
// (en el primer frame: los hooks se instalan despues de cargar los scripts)

function list(n) {
  var a = [];
  for (var i = 0; i < n; i++) a.push(i);
  return a;
}

// un shader: cross (mira vecinos), ext (amplia el recuadro), dest (usa lo de debajo)
function glw(flags) {
  return {
    programUsesCrossSampling: function () { return !!flags.cross; },
    programExtendsBox: function () { return !!flags.ext; },
    programUsesDest: function () { return !!flags.dest; }
  };
}
function inst(opts) {
  return {
    active_effect_types: opts.effects || [{ shaderindex: 1 }],
    opacity: opts.opacity === undefined ? 1 : opts.opacity,
    type: { plugin: { must_predraw: !!opts.predraw } }
  };
}
function draw(o, flags) {
  drawn.length = 0;
  var i = inst(o);
  new cr.layer().drawInstanceWithShadersGL(i, glw(flags || {}));
  return drawn[0] + (i.type.plugin.must_predraw === !!o.predraw ? '' : ' (flag not restored)');
}

requestAnimationFrame(function () {
  T.test('clear-array: same arrays as the original', function () {
    T.ok(String(cr.truncateArray).indexOf('POP_MAX') >= 0, 'truncateArray is the hook');
    var cases = [[0, 0], [3, 3], [5, 2], [5, 0], [40, 1], [40, 0], [2, 6]];
    cases.forEach(function (c) {
      var a = list(c[0]), b = list(c[0]);
      cr.truncateArray(a, c[1]);
      b.length = c[1];
      T.equal(JSON.stringify(a), JSON.stringify(b), c[0] + ' to ' + c[1]);
    });
    var e = list(7);
    cr.clearArray(e);
    T.equal(e.length, 0, 'clearArray empties it');
    var z = [];
    cr.clearArray(z);
    T.equal(z.length, 0, 'an empty one stays empty');
  });

  T.test('effects-direct', function () {
    T.equal(draw({ predraw: true }), 'direct', 'pre-draw plugin, colour-only effect: direct');
    T.equal(draw({}), 'direct', 'a sprite: direct, as before');
    T.equal(draw({ predraw: true, opacity: 0.5 }), 'chain', 'with opacity: the texture, as before');
    T.equal(draw({ predraw: true, effects: [{ shaderindex: 1 }, { shaderindex: 2 }] }), 'chain',
      'two effects: the texture');
    T.equal(draw({ predraw: true }, { cross: true }), 'chain', 'looks at neighbours: the texture');
    T.equal(draw({ predraw: true }, { ext: true }), 'chain', 'extends the box: the texture');
    T.equal(draw({ predraw: true }, { dest: true }), 'chain', 'uses what is below: the texture');
  });
  T.test('pixellate-1', function () {
    function layer(o) {
      var l = new cr.layer();
      l.active_effect_types = o.effects || [{ id: 'pixellate', index: 0 }];
      l.effect_params = [[o.size === undefined ? 1 : o.size]];
      l.opacity = o.opacity === undefined ? 1 : o.opacity;
      l.blend_mode = 0;
      l.forceOwnTexture = false;
      drawn.length = 0;
      var before = l.active_effect_types;
      l.drawGL({});
      return drawn[0] + (l.active_effect_types === before ? '' : ' (effects not restored)');
    }
    T.equal(layer({}), 'direct', 'Pixellate 1: direct');
    T.equal(layer({ size: 0.5 }), 'direct', 'Pixellate under 1: direct');
    T.equal(layer({ size: 2 }), 'texture', 'Pixellate 2: the texture, as before');
    T.equal(layer({ opacity: 0.5 }), 'texture', 'with opacity: the texture');
    T.equal(layer({ effects: [{ id: 'tint', index: 0 }] }), 'texture', 'another effect: the texture');
  });

  T.test('include-sol', function () {
    var A = new Type('A'), B = new Type('B'), C = new Type('C'), F = new Type('F');
    F.members = [C];   // F es una familia con C
    runtime.types_by_index = [A, B, C, F];
    function sel(t) { return t.solstack[t.cur_sol]; }
    // la hoja incluida elige A (y en un subevento, la familia F): al correr
    // deja la seleccion de A con solo algunas
    var sheet = { hasRun: false, run: function () { sel(A).select_all = false; sel(A).else_instances.push(1); },
      events: [{ solModifiers: [A], subevents: [{ solModifiers: [F] }] }] };
    var group = { conditions: [] };
    cleaned.length = 0;
    new cr.eventinclude(group, sheet).run();
    T.equal(cleaned.length, 0, 'all clean on entry: nothing to save');
    T.ok(sel(A).select_all && !sel(A).else_instances.length && A.cur_sol === 0, 'and A is left clean, as popSol would');
    // A ya tenia una seleccion al entrar: se guarda y vuelve
    sel(A).select_all = false;
    cleaned.length = 0;
    new cr.eventinclude(group, sheet).run();
    T.equal(cleaned.join(','), 'A', 'only the type not clean on entry (of the types of the sheet)');
    T.ok(sel(A).select_all === false && A.cur_sol === 0, 'and its selection comes back');
    sel(A).select_all = true;
    var picking = { conditions: [{}] };
    cleaned.length = 0;
    new cr.eventinclude(picking, sheet).run();
    T.equal(cleaned.length, 4, 'with conditions above: all the types, as the original');
    cleaned.length = 0;
    new cr.eventinclude(null, sheet).run();
    T.equal(cleaned.length, 0, 'not inside anything: nothing, as the original');
  });
  T.done();
});
