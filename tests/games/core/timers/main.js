// setTimeout, setInterval, requestAnimationFrame y performance.now

var log = [];

T.test('performance.now', function () {
  T.equal(typeof performance.now(), 'number', 'performance.now() is a number');
});

setTimeout(function () { log.push('b'); }, 20);
setTimeout(function () { log.push('a'); }, 10);
setTimeout(function (x, y) { log.push(x + y); }, 30, 'c', 'd');
var cancelled = setTimeout(function () { log.push('never'); }, 15);
clearTimeout(cancelled);

var count = 0;
var interval = setInterval(function () {
  count++;
  if (count === 3) clearInterval(interval);
}, 5);

var frames = [];
function frame(ts) {
  frames.push(ts);
  if (frames.length < 3) requestAnimationFrame(frame);
}
requestAnimationFrame(frame);

var cancelledFrame = requestAnimationFrame(function () { log.push('frame-never'); });
cancelAnimationFrame(cancelledFrame);

setTimeout(function () {
  T.test('setTimeout', function () {
    T.deepEqual(log, ['a', 'b', 'cd'], 'order by delay, extra arguments, clearTimeout');
  });

  T.test('setInterval', function () {
    T.equal(count, 3, 'runs 3 times and clearInterval stops it');
  });

  T.test('requestAnimationFrame', function () {
    T.equal(frames.length, 3, '3 chained frames');
    T.ok(frames[1] > frames[0] && frames[2] > frames[1], 'timestamps increase');
    if (typeof quix !== 'undefined' && quix.fixedStep) {
      T.near(frames[1] - frames[0], 1000 / 60, 0.001, 'fixed step: 1000/60 ms per frame');
    }
  });

  T.done();
}, 200);
