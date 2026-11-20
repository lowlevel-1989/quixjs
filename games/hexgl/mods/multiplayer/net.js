// net.js: la red del mod multiplayer, con quix.lan (runtime/lan de quixjs:
// sin abrir puertos ni escribir direcciones).
//
// Mensajes (JSON):
//   {t: "lobby", ready}     en la sala, cada medio segundo
//                           (ready: la pista esta cargada)
//   {t: "start"}            alguien empezo: todos salen a la vez
//   {t: "pos", p: [x, y, z, qx, qy, qz, qw]}
//                           la nave, unas 15 veces por segundo
//   {t: "done", time, finished, noDamage}
//                           llego a la meta (o la destruyeron);
//                           noDamage: con el truco No damage
(function () {
  var SEND_MS = 66;    // ~15 posiciones por segundo
  var LOBBY_MS = 500;
  var GONE_MS = 3000;  // sin noticias: se fue

  var net = {
    open: false,
    players: {},       // id -> {name, ready, seen, samples, time, ...}
    racing: false,
    myResult: null
  };
  var startHeard = 0;
  var lastLobby = 0;
  var lastSend = 0;

  function now() {
    return performance.now();
  }

  function send(msg) {
    quix.lan.send(JSON.stringify(msg));
  }

  // UDP puede perder un paquete: lo importante se manda varias veces
  function sendThrice(msg) {
    for (var i = 0; i < 3; i++) send(msg);
  }

  function player(id) {
    if (!net.players[id]) {
      net.players[id] = {
        name: id,
        ready: false,
        seen: now(),
        samples: [],
        time: null
      };
    }
    return net.players[id];
  }

  function receive(p, m, t) {
    if (m.t === 'lobby') {
      p.ready = !!m.ready;
    } else if (m.t === 'start') {
      startHeard = t;
    } else if (m.t === 'pos' && m.p && m.p.length === 7) {
      p.samples.push({ at: t, p: m.p });
      if (p.samples.length > 20) p.samples.shift();
    } else if (m.t === 'done') {
      p.time = m.time;
      p.finished = !!m.finished;
      p.noDamage = !!m.noDamage;
    }
  }

  // lo que llego por la red, y quien sigue ahi
  net.poll = function () {
    var t = now();
    var peers = quix.lan.peers();
    for (var i = 0; i < peers.length; i++) {
      var p = player(peers[i].id);
      p.name = peers[i].name;
      p.seen = t;
    }
    var msgs = quix.lan.receive();
    for (var j = 0; j < msgs.length; j++) {
      var m;
      try {
        m = JSON.parse(msgs[j].data);
      } catch (e) {
        continue;
      }
      var q = player(msgs[j].from);
      q.seen = t;
      receive(q, m, t);
    }
    for (var id in net.players) {
      if (t - net.players[id].seen > GONE_MS) {
        if (net.onGone) net.onGone(net.players[id]);
        delete net.players[id];
      }
    }
  };

  net.available = function () {
    return typeof quix === 'object' && !!quix.lan;
  };

  net.connect = function () {
    net.open = quix.lan.open();
    net.players = {};
    startHeard = 0;
    return net.open;
  };

  net.disconnect = function () {
    for (var id in net.players) {
      if (net.onGone) net.onGone(net.players[id]);
    }
    net.players = {};
    if (net.open) quix.lan.close();
    net.open = false;
    net.racing = false;
  };

  net.me = function () {
    return quix.lan.name || '';
  };

  // en la sala, en cada imagen: quien esta y si alguien empezo
  net.lobby = function (ready) {
    if (!net.open) return { players: [], start: false };
    net.poll();
    var t = now();
    if (t - lastLobby > LOBBY_MS) {
      send({ t: 'lobby', ready: ready });
      lastLobby = t;
    }
    var list = [];
    for (var id in net.players) {
      list.push({ name: net.players[id].name, ready: net.players[id].ready });
    }
    // un start viejo (llego en otra pantalla) ya no vale
    var start = ready && startHeard > 0 && t - startHeard < 1000;
    startHeard = 0;
    return { players: list, start: start };
  };

  net.start = function () {
    sendThrice({ t: 'start' });
  };

  net.raceBegin = function () {
    net.racing = true;
    net.myResult = null;
    for (var id in net.players) {
      var p = net.players[id];
      p.samples = [];
      p.time = null;
      p.finished = false;
    }
  };

  // la nave propia, de vez en cuando
  net.sendShip = function (pos, rot) {
    var t = now();
    if (t - lastSend < SEND_MS) return;
    lastSend = t;
    send({ t: 'pos', p: [pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, rot.w] });
  };

  net.raceEnd = function (time, finished, noDamage) {
    net.racing = false;
    net.myResult = { time: time, finished: finished, noDamage: noDamage };
    sendThrice({ t: 'done', time: time, finished: finished, noDamage: noDamage });
  };

  // los tiempos: el propio y los que han llegado, el mejor primero
  net.results = function () {
    if (net.open) net.poll();
    var list = [];
    var mine = net.myResult;
    if (mine) {
      list.push({
        name: net.me(),
        time: mine.time,
        finished: mine.finished,
        noDamage: mine.noDamage,
        me: true
      });
    }
    for (var id in net.players) {
      var p = net.players[id];
      list.push({
        name: p.name,
        time: p.time,
        finished: p.finished,
        noDamage: p.noDamage,
        me: false
      });
    }
    function key(r) {
      return r.finished && r.time != null ? r.time : Infinity;
    }
    list.sort(function (a, b) {
      return key(a) - key(b);
    });
    return list;
  };

  net.now = now;
  window.MultiplayerNet = net;
})();
