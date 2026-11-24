// same-code: agrega quix.engine.sameCode(fn, model), "es fn la funcion
// original?", para los hooks de motor.
//
// El flujo:
//   1. Al arrancar, runtime/engines/api.c crea la variable global quix.engine y,
//      justo despues, ejecuta este archivo una vez (tools/embed_js.py lo
//      mete en el binario al compilar).
//   2. Este archivo agrega quix.engine.sameCode (abajo, al final:
//      "})(quix.engine);" es la llamada).
//   3. Al cargar el juego, runtime/game/hooks/hooks.c instala cada hook, y el hook la usa:
//        var sameCode = quix.engine.sameCode;
//        if (!sameCode(original, MODEL['Game_Event.isStarting'])) return original;
//
// Para que sirve: un hook cambia una funcion del juego (de RPG Maker o de un
// plugin) por una mas rapida que da el mismo resultado. Eso solo es seguro si
// la funcion del juego es la que el hook conoce: si otro plugin la cambio, el
// hook no sabe que hace la nueva y no debe tocarla. Cada hook guarda la
// original, tal cual, en su MODEL (copiada del juego con
// tools/rpgmaker_mv_model.py), y pregunta si la del juego es igual.
//
// Como lo sabe: compara el codigo de las dos (String(fn) da su texto) sin
// espacios, tabuladores ni saltos de linea. Da igual el sangrado; cualquier
// cambio en el codigo hace que no sea igual. Los comentarios de dentro de la
// funcion si cuentan: el modelo se copia tal cual, con ellos (quitar uno haria
// que el hook no se pusiera):
//
//   sameCode(function () { return this._starting; },
//            function() {
//                return this._starting;
//            })                                         -> true
//   sameCode(function () { return this._starting || this._locked; },
//            function () { return this._starting; })  -> false
//
// Se usa al instalar el hook (o la primera vez), no en cada imagen.

// Todo va dentro de una funcion que se llama en el acto (al final), para que
// text y models no sean variables globales: solo las ve sameCode.
(function (quixEngine) {
  // el codigo de una funcion, sin espacios
  function text(f) {
    return String(f).replace(/\s+/g, '');
  }

  // el texto de cada modelo, calculado una vez (modelo -> texto)
  var models = new WeakMap();

  function sameCode(fn, model) {
    if (typeof fn !== 'function') return false;
    var m = models.get(model);
    if (m === undefined) models.set(model, (m = text(model)));
    return text(fn) === m;
  }

  quixEngine.sameCode = sameCode;
})(quix.engine);
