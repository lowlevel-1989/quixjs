//=============================================================================
// main.js
//=============================================================================
// El main.js de RPG Maker MV (corescript, KADOKAWA, MIT, commit 182e314:
// https://github.com/rpgtkoolmv/corescript/blob/182e31449707ba7e406db0485c44c2a9d11e2dcd/js/main.js),
// el mismo que trae Firework, con un cambio del mod big-text de quixjs:
// antes de cargar los plugins, la letra de los mensajes mas grande (el juego
// dibuja a 1280x720 y en una pantalla de 720x720 se ve al 56 %) y, al final
// de la lista, QuixBigIcons (js/plugins/QuixBigIcons.js, del mod): los iconos
// de encima de los eventos mas grandes.

(function () {
  var BIG = {
    // la letra de los mensajes y las ventanas
    YEP_MessageCore: { 'Font Size': '34' },
    // la altura de linea y el ancho del texto: con retrato 460 (no 440; mas
    // pisaria el retrato), sin el 950 (no 700); corta en mas lineas
    MessageCustom2: { messageLineHeight: '44', textWidth: '460', textWidth2: '950' }
  };
  $plugins.forEach(function (plugin) {
    var change = BIG[plugin.name];
    if (!change || !plugin.parameters) return;
    for (var key in change) {
      if (key in plugin.parameters) plugin.parameters[key] = change[key];
    }
  });
  $plugins.push({ name: 'QuixBigIcons', status: true, description: '', parameters: {} });
})();

PluginManager.setup($plugins);

window.onload = function() {
    SceneManager.run(Scene_Boot);
};
