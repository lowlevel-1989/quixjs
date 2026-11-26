// games/soccer-chapa/game/src/ui/net-pause.js: la pausa de un partido por red
// en pantalla (games/soccer-chapa/game/src/cup/pause.js): el campo oscuro, lo
// que queda, quien pauso y los botones de quien puede hacer algo:
//   quien pauso     A sigue, X pide 30 s mas, B deja el partido
//   el rival        si le piden mas: A acepta, B rechaza (si no, se amplia sola)
SC.ui.netPause = {
  ROWS: {
    mine: [['a', 'npause.go'], ['x', 'npause.more'], ['b', 'npause.quit']],
    asking: [['b', 'npause.quit']],
    answer: [['a', 'npause.yes'], ['b', 'npause.no']]
  },
  KEYS: { mine: { a: 'go', x: 'more', b: 'quit' }, asking: { b: 'quit' }, answer: { a: 'yes', b: 'no' } }
};

SC.ui.netPause.create = function (ui, w, h) {
  var u = SC.ui.u, R = SC.ui.netPause.ROWS, v = { box: new PIXI.Container() };
  var dim = new PIXI.Graphics(); dim.beginFill(0x070a14, 0.7); dim.drawRect(0, 0, w, h); dim.endFill();
  v.title = SC.ui.text('Black Italic', 34, 0xffd45a); v.title.x = w / 2; v.title.y = h * 0.3;
  v.time = SC.ui.text('Black Italic', 72); v.time.x = w / 2; v.time.y = h * 0.3 + 70 * u;
  v.sub = SC.ui.text('Bold', 20, 0xdfe6e2); v.sub.x = w / 2; v.sub.y = h * 0.3 + 140 * u;
  v.box.addChild(dim, v.title, v.time, v.sub);
  v.buttons = SC.ui.buttons.create(v.box, w, h, Object.keys(R).map(function (k) { return R[k]; }));
  v.box.visible = false;
  ui.addChild(v.box);
  return v;
};

// cada frame: lo pone al dia; mine(t): el equipo t se juega aqui. Devuelve
// lo que se pulso ({ t, a }: el equipo y 'go', 'more', 'quit', 'yes' o 'no')
SC.ui.netPause.update = function (v, m, teams, mine) {
  var z = m.pz;
  v.box.visible = !!z;
  if (!z) return null;
  var s = Math.max(0, Math.ceil(z.left)), wait = Math.max(0, Math.ceil(z.askLeft));
  v.title.text = SC.lang.t('npause.title');
  v.time.text = Math.floor(s / 60) + ':' + ('0' + s % 60).slice(-2);
  var role = mine(z.by) ? (z.ask ? 'asking' : 'mine') : mine(1 - z.by) && z.ask ? 'answer' : '';
  v.sub.text = !z.ask ? SC.lang.t('npause.by', teams[z.by].name)
    : role === 'asking' ? SC.lang.t('npause.asking', wait) : SC.lang.t('npause.asked', teams[z.by].name, wait);
  SC.ui.buttons.show(v.buttons, SC.ui.netPause.ROWS[role] || []);
  var keys = SC.ui.netPause.KEYS[role] || {}, hit = SC.ui.pad.hit;
  for (var b in keys) if (hit[b]) return { t: role === 'answer' ? 1 - z.by : z.by, a: keys[b] };
  return null;
};
