// screens.js: las pantallas del mod multiplayer en el menu del port
// (PortMenu, en entry.js) y su carrera.
//
//   'lobby'      la sala: quien esta y si ya cargo la pista; A sale para
//                todos, B vuelve al menu
//   'mp-result'  los tiempos de todos (llegan mientras los demas terminan)
//
// La carrera es la de siempre (PortMenu.race), pero la pausa no la para
// (los demas siguen) y Select no la reinicia.
(function () {
  var net = window.MultiplayerNet;
  var ghosts = window.MultiplayerGhosts;
  var M = window.PortMenu;
  var lobby = { players: [], start: false };

  function ship(hexGL) {
    return hexGL.components.shipControls;
  }

  var race = {
    title: 'Multiplayer',
    pauseItems: ['Continue', 'Leave race'],
    pauseStops: false,
    restart: false,
    pause: function (choice) {
      if (choice !== 'Leave race') return;
      M.stopRace();
      net.raceEnd(null, false, false);
      M.go('lobby');
    },
    begin: function (hexGL) {
      net.raceBegin();
      ghosts.begin(hexGL);
    },
    frame: function (hexGL) {
      if (!net.open || !net.racing) return;
      net.poll();
      var pos = new THREE.Vector3();
      var rot = new THREE.Quaternion();
      var scale = new THREE.Vector3();
      ship(hexGL).mesh.matrixWorld.decompose(pos, rot, scale);
      net.sendShip(pos, rot);
      ghosts.update(hexGL);
    },
    end: function (time, finished) {
      net.raceEnd(time, finished, ship(M.hexGL()).shieldDamage === 0);
      return 'mp-result';
    }
  };

  function startForAll() {
    net.start();
    M.race(race);
  }

  function leave() {
    net.disconnect();
    M.go('menu');
  }

  // una fila: el nombre a la izquierda, lo demas a la derecha
  function row(i, name, right, color) {
    var y = M.H * (0.24 + i * 0.075);
    M.text(name, M.W * 0.1, y, 20, color, 'left');
    M.text(right, M.W * 0.9, y, 20, color, 'right');
  }

  function darken() {
    M.g.fillStyle = M.DIM;
    M.g.fillRect(0, 0, M.W, M.H);
  }

  function drawLobby() {
    var W = M.W, H = M.H;
    M.background();
    darken();
    M.text('Multiplayer', W / 2, H * 0.1, 40, M.WHITE);
    if (!net.open) {
      M.text('No network', W / 2, H * 0.45, 28, M.WHITE);
      M.text('B: back', W / 2, H * 0.9, 20, M.WHITE);
      return;
    }
    var ready = M.ready();
    var rows = [{ name: net.me() + '  (you)', ready: ready }].concat(lobby.players);
    for (var i = 0; i < rows.length && i < 8; i++) {
      var color = i === 0 || rows[i].ready ? M.WHITE : M.BLUE;
      row(i, rows[i].name, rows[i].ready ? 'ready' : 'loading', color);
    }
    if (!lobby.players.length) {
      M.text('Waiting for other handhelds on this network...', W / 2, H * 0.78, 18, M.BLUE);
    }
    if (!ready) {
      M.text('Loading ' + Math.round(M.loaded() * 100) + '%', W / 2, H * 0.84, 20, M.WHITE);
    }
    var keys = (ready ? 'A: start the race for everyone     ' : '') + 'B: leave';
    M.text(keys, W / 2, H * 0.92, 20, M.WHITE);
  }

  function resultTime(r) {
    if (r.time == null) return 'racing...';
    var t = r.finished ? M.timeString(r.time) : 'destroyed';
    // con el truco No damage (en su handheld) los choques no le quitaban escudo
    if (r.noDamage) t += '  (no damage)';
    return t;
  }

  function drawResults() {
    var W = M.W, H = M.H;
    M.g.clearRect(0, 0, W, H);
    darken();
    M.text('Results', W / 2, H * 0.12, 40, M.WHITE);
    var list = net.results();
    for (var i = 0; i < list.length && i < 8; i++) {
      var r = list[i];
      row(i + 0.3, (i + 1) + '.  ' + r.name, resultTime(r), r.me ? M.WHITE : M.BLUE);
    }
    M.text('A: back to the room     B: leave', W / 2, H * 0.92, 20, M.WHITE);
  }

  M.screen('lobby', {
    draw: drawLobby,
    key: function () {
      return JSON.stringify(lobby.players) + net.open;
    },
    frame: function () {
      lobby = net.lobby(M.ready());
      if (lobby.start) M.race(race);   // otro handheld empezo
    },
    press: function (what) {
      if (what === 'ok' && M.ready() && net.open) startForAll();
      else if (what === 'back') leave();
    }
  });

  M.screen('mp-result', {
    draw: drawResults,
    key: function () {
      return JSON.stringify(net.results());
    },
    press: function (what) {
      if (what === 'ok') M.go('lobby');
      else if (what === 'back') leave();
    }
  });

  // lo que usa option.js
  window.MultiplayerScreens = {
    open: function () {
      net.connect();
      M.go('lobby');
      M.load();   // la pista se carga mientras se espera en la sala
    }
  };
})();
