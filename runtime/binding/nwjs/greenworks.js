// greenworks: la libreria de Steam para nw.js, sin Steam. Algunos juegos de
// nw.js (Gunfield, de Construct 2) dan sus logros con ella; su greenworks.js
// carga un .node de Steam que en quixjs no existe. require("./greenworks")
// devuelve este en su lugar (require.js).
//
// Los logros que el juego activa quedan en quix.steam.achievements[id] = 1,
// para que los [[achievements]] del toml del port los lean como simbolo:
//   [symbols] win_nova = "path:quix.steam.achievements.WIN_CHARACTER_0"
// Y cada id, una vez, en steam-achievements.txt, en la carpeta de las
// partidas (la unica donde el juego puede escribir: runtime/core/sandbox.c): quien
// hace el port juega y copia los ids a sus [[achievements]].
(function (N) {
  var steam = N.g.quix.steam;   // lo crea runtime/binding/quix/quix.c
  var NOTES = N.SAVES_DIR ? N.SAVES_DIR + '/steam-achievements.txt' : null;

  function done(cb, v) {
    if (typeof cb === 'function') {
      setTimeout(function () {
        cb(v);
      }, 0);
    }
  }

  function noteId(id) {
    if (!NOTES) return;
    try {
      var seen = N.fsn.readText(NOTES) || '';
      if (seen.split('\n').indexOf(id) >= 0) return;
      N.fsn.mkdir(N.SAVES_DIR, true);
      N.fsn.write(NOTES, id + '\n', true);
    } catch (e) {}
  }

  function yes() {
    return true;
  }
  function no() {
    return false;
  }
  function nothing() {}

  N.greenworks = {
    init: yes,
    initAPI: yes,
    // "Steam abierto": los plugins de RPG Maker (Cyclone-Steam de Look Outside,
    // OrangeGreenworks) solo activan logros si lo esta. Lo demas que piden
    // contesta como un Steam sin DLC, sin nube y sin amigos
    isSteamRunning: yes,
    getSteamId: function () {
      // screenName: [game] player del toml ("Master" sin el).
      // staticAccountId (64 bits, como texto) y level: el plugin de Steam de
      // Construct 3 los lee al cargar
      return { screenName: steam.player || 'Master', accountId: 0, steamId: '0', staticAccountId: '0', level: 0, isValid: 1 };
    },
    getCurrentGameLanguage: function () {
      return 'english';
    },
    getCurrentUILanguage: function () {
      return 'english';
    },
    activateAchievement: function (id, ok) {
      id = String(id);
      if (!steam.achievements[id]) {
        console.log('[greenworks] achievement: ' + id);
        noteId(id);
      }
      steam.achievements[id] = 1;
      done(ok);
    },
    getAchievement: function (id, ok) {
      done(ok, !!steam.achievements[id]);
    },
    clearAchievement: function (id, ok) {
      delete steam.achievements[id];
      done(ok);
    },
    getAchievementNames: function () {
      return Object.keys(steam.achievements);
    },
    getNumberOfAchievements: function () {
      return Object.keys(steam.achievements).length;
    },
    // estadisticas: en memoria mientras se juega
    getStatInt: function (name) {
      return steam.stats[name] | 0;
    },
    getStatFloat: function (name) {
      return +steam.stats[name] || 0;
    },
    setStat: function (name, value) {
      steam.stats[name] = value;
      return true;
    },
    storeStats: function (ok) {
      done(ok);
    },
    isSubscribedApp: yes,
    getDLCCount: function () {
      return 0;
    },
    isDLCInstalled: no,
    installDLC: nothing,
    uninstallDLC: nothing,
    isCloudEnabled: no,
    isCloudEnabledForUser: no,
    getFriendCount: function () {
      return 0;
    },
    FriendFlags: {},
    isGameOverlayEnabled: no,
    activateGameOverlay: nothing,
    activateGameOverlayToWebPage: nothing,
    on: nothing,
    once: nothing,
    removeAllListeners: nothing
  };
})(globalThis.__quixNw);
