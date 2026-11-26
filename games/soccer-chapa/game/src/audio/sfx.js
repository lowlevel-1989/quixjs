// games/soccer-chapa/game/src/audio/sfx.js: los sonidos (WebAudio;
// games/soccer-chapa/game/assets/sfx/*.ogg de
// games/soccer-chapa/game/tools/sound/sfx.py). Se cargan al empezar;
// SC.audio.sfx.play los toca con su volumen, su tono y su lado (izquierda o
// derecha segun donde pasa en el campo). Los bucles (el publico, el balon
// rodando) suenan siempre y el juego les cambia el volumen. Dos salidas con su
// volumen de Opciones (games/soccer-chapa/game/src/save/settings.js): el
// ambiente (amb: todo el publico, AMBIENCE) y los efectos (out: lo demas,
// tambien el balon rodando).
SC.audio.sfx = {
  NAMES: ['flick', 'flick1', 'flick2', 'flick3', 'cap', 'flip', 'tear', 'tick', 'ball', 'clink1', 'clink2', 'clink3', 'tap1', 'tap2', 'tap3', 'thump', 'roll', 'pass', 'go', 'goal', 'crowd', 'ooh', 'cheer',
          'whistle', 'whistle_end', 'clap', 'rise', 'chant'],
  GAIN: 1.6, VARY: 0.04, buffers: {}, loops: {},  // GAIN: el volumen a 10; VARY: lo que cambia cada vez
  // el publico: va por la salida del Ambiente (sus opciones); lo demas, por la de Efectos
  AMBIENCE: ['crowd', 'chant', 'cheer', 'ooh', 'rise', 'clap']
};

SC.audio.sfx.load = function (done) {
  var AC = window.AudioContext || window.webkitAudioContext, left = SC.audio.sfx.NAMES.length;
  if (!AC) { done(); return; }
  var ctx = SC.audio.sfx.ctx = new AC();
  SC.audio.sfx.out = ctx.createGain(); SC.audio.sfx.out.connect(ctx.destination);
  SC.audio.sfx.amb = ctx.createGain(); SC.audio.sfx.amb.connect(ctx.destination);
  SC.audio.sfx.volumes();
  SC.audio.sfx.NAMES.forEach(function (n) {
    var req = new XMLHttpRequest();
    req.open('GET', 'assets/sfx/' + n + '.ogg'); req.responseType = 'arraybuffer';
    function one() { if (--left === 0) done(); }
    req.onload = function () {
      ctx.decodeAudioData(req.response, function (b) { SC.audio.sfx.buffers[n] = b; one(); }, one);
    };
    req.onerror = one;
    req.send();
  });
};

// el volumen de las dos salidas, de los ajustes (0 a 10)
SC.audio.sfx.volumes = function () {
  if (!SC.audio.sfx.out) return;
  SC.audio.sfx.out.gain.value = SC.save.settings.sfx / 10 * SC.audio.sfx.GAIN;
  SC.audio.sfx.amb.gain.value = SC.save.settings.ambience / 10 * SC.audio.sfx.GAIN;
};

// toca name: vol (0 a 1), rate (el tono: 1 normal), pan (-1 izquierda, 1
// derecha) y cut (Hz: sin agudos por encima)
SC.audio.sfx.play = function (name, vol, rate, pan, cut) {
  var ctx = SC.audio.sfx.ctx, b = SC.audio.sfx.buffers[name];
  if (!ctx || !b) return;
  var src = ctx.createBufferSource(), g = ctx.createGain(), node = g;
  src.buffer = b;                              // cada vez un poco distinto (tono y volumen): que no se repita igual
  src.playbackRate.value = (rate || 1) * (1 + (Math.random() - 0.5) * SC.audio.sfx.VARY);
  g.gain.value = (vol === undefined ? 1 : vol) * (1 - Math.random() * SC.audio.sfx.VARY * 2);
  if (cut) {                                   // cut: los agudos por encima de cut Hz, apagados (un golpe flojo)
    var lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = cut;
    src.connect(lp); lp.connect(g);
  } else src.connect(g);
  if (pan && ctx.createStereoPanner) { node = ctx.createStereoPanner(); node.pan.value = pan; g.connect(node); }
  node.connect(sfxOut(name));
  src.start(0);
};

// la salida de un sonido: la del Ambiente (el publico) o la de Efectos
function sfxOut(name) {
  return SC.audio.sfx.AMBIENCE.indexOf(name) >= 0 ? SC.audio.sfx.amb : SC.audio.sfx.out;
}

// un bucle que suena siempre; SC.audio.sfx.level le cambia el volumen
SC.audio.sfx.loop = function (name) {
  var ctx = SC.audio.sfx.ctx, b = SC.audio.sfx.buffers[name];
  if (!ctx || !b || SC.audio.sfx.loops[name]) return;
  var src = ctx.createBufferSource(), g = ctx.createGain();
  src.buffer = b; src.loop = true; g.gain.value = 0;
  src.connect(g); g.connect(sfxOut(name)); src.start(0);
  SC.audio.sfx.loops[name] = { src: src, gain: g, v: 0 };
};

// el volumen de un bucle, suavizado para que no de saltos
SC.audio.sfx.level = function (name, vol, dt) {
  var l = SC.audio.sfx.loops[name];
  if (!l) return;
  l.v += (vol - l.v) * Math.min(1, dt * 8);
  l.gain.gain.value = l.v;
};

SC.audio.sfx.stop = function (name) {
  var l = SC.audio.sfx.loops[name];
  if (l) { l.src.stop(0); delete SC.audio.sfx.loops[name]; }
};
