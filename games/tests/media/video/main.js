// <video>: WebM VP9 + Opus (clip.webm: 64x48, 1 s, 10 imagenes, un tono) y
// MP4 H.264 + AAC (clip.mp4: lo mismo, con imagenes B; big.mp4: 1920x1080,
// 5 imagenes, sin audio). Con video se reproducen; sin video se saltan
// (loadeddata y ended enseguida), que es lo que necesita un juego para
// seguir.

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
  T.equal(v.canPlayType('video/mp4'), supported ? 'probably' : '', 'mp4 too');
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
  playVideo('clip.mp4', function (v) {
    T.test('an MP4 plays to the end', function () {
      if (!supported) {
        T.ok(true, 'built without video: skipped');
        return;
      }
      T.equal(v.videoWidth, 64, 'videoWidth');
      T.equal(v.videoHeight, 48, 'videoHeight');
      // la de la pista de video: con imagenes B lleva un poco mas (1.1)
      T.near(v.duration, 1.0, 0.15, 'duration');
      T.equal(v.webkitDecodedFrameCount, 10, 'every frame decoded (B frames too)');
      T.ok(v._audio, 'with its AAC audio');
      T.ok(v.ended && v.paused, 'ended and paused');
    });
    playVideo('big.mp4', function (b) {
      T.test('a 1080p MP4 is shown at half size', function () {
        if (!supported) {
          T.ok(true, 'built without video: skipped');
          return;
        }
        T.equal(b.videoWidth, 960, 'videoWidth');
        T.equal(b.videoHeight, 540, 'videoHeight');
        T.equal(b.webkitDecodedFrameCount, 5, 'every frame decoded');
      });
      // como YSP_VideoPlayer al soltar un video: removeAttribute('src') y load()
      var r = document.createElement('video');
      r.onloadeddata = function () {
        r.play();
        r.pause();
        r.removeAttribute('src');
        r.load();
        T.test('removeAttribute src closes the video', function () {
          T.equal(r.src, '', 'no src');
          T.ok(r._qid < 0, 'closed');
          T.ok(r.paused, 'paused');
        });
        playVideo('missing.webm', function (m) {
          T.test('a video that cannot play is skipped', function () {
            T.ok(m.ended, 'ended right away');
            T.deepEqual(log, ['clip.webm:loadeddata', 'clip.webm:ended', 'clip.mp4:loadeddata', 'clip.mp4:ended',
                              'big.mp4:loadeddata', 'big.mp4:ended', 'missing.webm:loadeddata', 'missing.webm:ended'],
                        'events in order');
          });
          T.done();
        });
      };
      r.src = 'clip.mp4';
    });
  });
});
