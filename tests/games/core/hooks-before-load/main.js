// El arranque de RPG Maker MZ (js/main.js): agrega los <script> de sus
// librerias y, en window load, llama a effekseer.initRuntime y espera a
// onload para seguir. effekseer lo crea uno de esos <script>.
var started = false, hookAtLoad = null;
var s = document.createElement('script');
s.src = 'js/effekseer.js';
document.body.appendChild(s);
window.addEventListener('load', function () {
  hookAtLoad = effekseer.initRuntime.__quixHook || null;
  effekseer.initRuntime('js/libs/effekseer.wasm', function () { started = true; }, function () {});
});

setTimeout(function () {
  T.test('a hook on a function of a startup <script>', function () {
    T.equal(hookAtLoad, 'rpgmaker-mz/load/effekseer-wasm-off', 'already on at window load');
    T.equal(typeof WebAssembly, 'undefined', 'quixjs has no WebAssembly');
    T.ok(started, 'without WebAssembly, onload goes on');
  });
  T.done();
}, 100);
