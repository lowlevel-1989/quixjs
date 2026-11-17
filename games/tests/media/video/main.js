// <video>: WebM VP9 + Opus (clip.webm: 64x48, 1 s, 10 imagenes, un tono).
// Con video (libvpx y libopus) se reproduce; sin video se salta (loadeddata
// y ended enseguida), que es lo que necesita un juego para seguir.

var supported = !!(window.__quixVideo && __quixVideo.supported);
var log = [];

function playVideo(src, done) {
  var v = document.createElement('video');
  v.onloadeddata = function () { log.push(src + ':loadeddata'); v.play(); };
  v.addEventListener('ended', function () { log.push(src + ':ended'); done(v); });
  v.src = src;
  v.load();
  return v;
}

T.test('canPlayType', function () {
  var v = document.createElement('video');
  T.equal(v.canPlayType('video/webm'), supported ? 'probably' : '', 'webm only with video support');
  T.equal(v.canPlayType('video/mp4'), '', 'no mp4');
});

playVideo('clip.webm', function (v) {
  T.test('a WebM plays to the end', function () {
    if (!supported) {
      T.ok(true, 'built without video: skipped');
      return;
    }
    T.equal(v.videoWidth, 64, 'videoWidth');
    T.equal(v.videoHeight, 48, 'videoHeight');
    T.near(v.duration, 1.0, 0.1, 'duration');
    T.equal(v.webkitDecodedFrameCount, 10, 'every frame decoded');
    T.ok(v.ended && v.paused, 'ended and paused');
  });
  playVideo('missing.webm', function (m) {
    T.test('a video that cannot play is skipped', function () {
      T.ok(m.ended, 'ended right away');
      T.deepEqual(log, ['clip.webm:loadeddata', 'clip.webm:ended', 'missing.webm:loadeddata', 'missing.webm:ended'],
                  'events in order');
    });
    T.done();
  });
});
