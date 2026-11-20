// <video> con un archivo que quixjs no reproduce (MP4): los eventos de uno que
// cargo (canplay, que espera PIXI.VideoBaseTexture) y el final
var a = document.createElement('video'), gotA = [];
['loadeddata', 'canplay', 'canplaythrough', 'ended'].forEach(function (t) {
  a.addEventListener(t, function () { gotA.push(t); });
});
a.src = 'movies/Opening.mp4';   // solo src, sin load() (YSP_VideoPlayer)

var b = document.createElement('video'), loadsB = 0;
b.addEventListener('loadeddata', function () { loadsB++; });
b.src = 'movies/Other.mp4';
b.load();                       // src y load() (RPG Maker MV)

setTimeout(function () {
  T.test('video: only src', function () {
    T.deepEqual(gotA, ['loadeddata', 'canplay', 'canplaythrough', 'ended'], 'loads, can play, and ends');
    T.equal(a.readyState, 4, 'readyState 4');
    T.equal(a.ended, true, 'ended');
  });
  T.test('video: src and load()', function () {
    T.equal(loadsB, 1, 'loadeddata once');
  });
  T.done();
}, 100);
