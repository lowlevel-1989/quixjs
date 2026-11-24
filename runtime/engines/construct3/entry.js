// entry.js de construct3: antes de los scripts del juego ([engine] name =
// "construct3"). No es un hook: lo que hace no es cambiar una funcion del
// juego sino dejar un valor que su main.js mira al cargarse.
//
// Construct 3 no arranca si su supportcheck.js no lo deja, y ese solo falla
// en quixjs por WebAssembly: lo usan los plugins que traen archivos .wasm, no
// el motor. Lo que mira ademas (WebGL, Intl.Segmenter, JavaScript moderno) lo
// tiene quixjs. El nombre cambia con la version de Construct 3:
// C3_IsSupported (las de 2025) o C3_Is_Supported (las de antes).
window.C3_IsSupported = true;
window.C3_Is_Supported = true;
