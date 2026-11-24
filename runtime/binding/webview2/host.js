// El programa de Windows que envuelve un juego exportado con WebView2
// (Construct 3, "Windows WebView2"): el juego le habla por
// chrome.webview.postMessage y espera sus respuestas por el evento message.
// Aqui no hay programa: se responde a lo que el juego necesita para seguir.
//   wrapper-init       la respuesta, sin extensiones (sin Steam: el juego no
//                      manda mensajes a extensiones que no estan)
//   set-fullscreen, set-window-size, set-window-position, ready: nada (la
//                      ventana la lleva quixjs)
(function (g) {
  var listeners = [];

  function reply(data) {
    setTimeout(function () {
      for (var i = 0; i < listeners.length; i++) listeners[i]({ data: data, additionalObjects: [] });
    }, 0);
  }

  g.chrome = g.chrome || {};
  g.chrome.webview = {
    hostObjects: {},
    addEventListener: function (type, fn) {
      if (type === 'message' && typeof fn === 'function') listeners.push(fn);
    },
    removeEventListener: function (type, fn) {
      var i = listeners.indexOf(fn);
      if (i >= 0) listeners.splice(i, 1);
    },
    postMessage: function (text) {
      var m;
      try {
        m = typeof text === 'string' ? JSON.parse(text) : text;
      } catch (e) {
        return;
      }
      if (m && m.type === 'wrapper-init') reply({ type: 'wrapper-init-response', registeredComponentIds: [] });
    }
  };
})(globalThis);
