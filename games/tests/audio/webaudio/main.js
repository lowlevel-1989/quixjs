// WebAudio: decodeAudioData (ogg vorbis), fuentes, ganancia, paneo,
// rampas de volumen, loops y onended (como lo usa RPG Maker MV)

var ctx = new AudioContext();
var pending = 0;
function finish() { if (--pending === 0) T.done(); }

function load(url, cb) {
  pending++;
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url);
  xhr.responseType = 'arraybuffer';
  xhr.onload = function () { cb(xhr.response); };
  xhr.send();
}

T.test('API', function () {
  T.ok(typeof AudioContext === 'function', 'AudioContext');
  T.ok(ctx.destination, 'destination');
  T.equal(typeof ctx.currentTime, 'number', 'currentTime');
  var a = new Audio();
  T.equal(a.canPlayType('audio/ogg; codecs="vorbis"'), 'probably', 'ogg is supported');
  T.equal(a.canPlayType('audio/mp4'), '', 'm4a is not');
});

T.test('AudioParam ramps', function () {
  var g = ctx.createGain();
  var t = ctx.currentTime;
  g.gain.setValueAtTime(1, t);
  g.gain.linearRampToValueAtTime(0, t + 2);
  T.near(g.gain._valueAt(t + 1), 0.5, 0.001, 'half way through a linear ramp');
  T.near(g.gain._valueAt(t + 3), 0, 0.001, 'after the ramp');
  g.gain.value = 0.25;
  T.equal(g.gain.value, 0.25, 'value cancels the automation');
});

load('../../assets/tone.ogg', function (data) {
  ctx.decodeAudioData(data, function (buffer) {
    T.test('decode', function () {
      T.near(buffer.duration, 1, 0.01, 'duration 1 s');
      T.equal(buffer.sampleRate, 44100, 'sampleRate');
      T.equal(buffer.numberOfChannels, 2, 'stereo');
    });

    // fuente -> ganancia -> panner -> destination (el grafo de RPG Maker)
    var src = ctx.createBufferSource();
    var gain = ctx.createGain();
    var panner = ctx.createPanner();
    src.buffer = buffer;
    src.connect(gain);
    gain.connect(panner);
    panner.connect(ctx.destination);
    gain.gain.value = 0.5;
    panner.setPosition(0.5, 0, 0.5);
    T.test('graph', function () {
      var p = src._params();
      T.near(p.volume, 0.5, 0.001, 'volume from the gain node');
      T.near(p.pan, 0.5, 0.001, 'pan from the panner position');
    });

    var started = ctx.currentTime;
    src.onended = function () {
      T.test('onended', function () {
        if (typeof quix !== 'undefined' && quix.fixedStep) {
          T.near(ctx.currentTime - started, 1, 0.05, 'ends after the duration');
        }
        T.throws(function () { src.start(0); }, 'a source only starts once');
      });
      finish();
    };
    src.start(0);

    // en loop no termina; stop() la para
    var loop = ctx.createBufferSource();
    loop.buffer = buffer;
    loop.loop = true;
    loop.connect(ctx.destination);
    var loopEnded = false;
    loop.onended = function () { loopEnded = true; };
    loop.start(0);
    pending++;
    setTimeout(function () {
      T.equal(loopEnded, false, 'a looping source is still playing after 1.5 s');
      loop.stop();
      setTimeout(function () {
        T.equal(loopEnded, true, 'stop() ends it');
        finish();
      }, 100);
    }, 1500);
    finish();
  });
});

load('../../assets/short.ogg', function (data) {
  ctx.decodeAudioData(data).then(function (buffer) {
    T.test('mono 22 kHz', function () {
      T.near(buffer.duration, 0.5, 0.01, 'duration 0.5 s');
      T.equal(buffer.numberOfChannels, 1, 'mono');
      T.equal(buffer.sampleRate, 22050, 'sampleRate');
    });
    finish();
  });
});

load('../../assets/bad.ogg', function (data) {
  ctx.decodeAudioData(data, function () {
    T.ok(false, 'a file that is not ogg must not decode');
    finish();
  }, function (err) {
    T.ok(err instanceof Error, 'error callback for invalid data');
    finish();
  });
});
