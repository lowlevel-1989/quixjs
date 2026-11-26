// games/soccer-chapa/game/src/cup/net.js: el torneo por la red local
// (quix.lan: las consolas con el juego abierto en la misma red, y por
// ZeroTier si su red deja pasar los avisos). Todo va en JSON, con su tipo (t):
//   room   el anfitrion: su sala (SC.cup.room.summary) sin los participantes (un
//          mensaje es de 4 KB como mucho: con 16 no cabe), al cambiar y si no
//          cada IDLE segundos (para que se sepa que sigue ahi)
//   parts  el anfitrion, de PARTS en PARTS: los participantes
//   cards  el anfitrion, de uno en uno: las cartas de un participante
//          parts y cards casi no cambian: van todos de golpe cuando cambian o
//          aparece una consola, y si no, uno cada SLOW segundos (por si se
//          perdio alguno)
//   join   una consola que quiere entrar: su equipo y sus cartas
//   full   el anfitrion, a la que no cabe
//   leave  una consola que se va
//   start  una consola pide empezar un partido (r, k) suyo
//   result una consola manda el resultado de un partido que jugo
//   state  la consola que lleva un partido: donde esta todo
//          (games/soccer-chapa/game/src/cup/remote.js), solo a las que lo
//          juegan o lo miran
//   watch  una consola que mira un partido, cada segundo
//   flick, call, ready   una consola, en su turno: su golpe, su aviso de tiro, listo
//   pz     una consola: lo que hace en la pausa (games/soccer-chapa/game/src/cup/pause.js)
// Un mensaje se puede perder: la sala y el estado se mandan enteros cada vez.
SC.cup.net = { TICK: 0.25, IDLE: 1, SLOW: 1, ROOM_LIFE: 4000, PARTS: 4,
              ok: false, id: 'here', rooms: {}, cards: {}, states: {}, inbox: [], t: 0, rr: 0 };

// abre la red (si no se puede, el torneo es solo en esta consola)
SC.cup.net.open = function () {
  var N = SC.cup.net;
  if (!N.ok && window.quix && quix.lan) N.ok = !!quix.lan.open();
  if (N.ok) N.id = quix.lan.id;
  return N.ok;
};

SC.cup.net.close = function () {
  var N = SC.cup.net;
  if (N.ok) quix.lan.close();
  N.ok = false; N.id = 'here'; N.rooms = {}; N.inbox = []; N.cards = {}; N.states = {}; N.rs = N.ps = N.pp = null;
};

SC.cup.net.send = function (msg, to) {
  if (!SC.cup.net.ok) return;
  var text = JSON.stringify(msg);
  if (to) quix.lan.send(text, to); else quix.lan.send(text);
};

// cada frame: lee lo que llego. Las salas y las cartas se guardan; lo demas
// queda en inbox hasta que alguien lo tome (SC.cup.net.take)
SC.cup.net.poll = function () {
  var N = SC.cup.net;
  if (!N.ok) return;
  N.peers = quix.lan.peers();
  quix.lan.receive().forEach(function (r) {
    var msg; try { msg = JSON.parse(r.data); } catch (e) { return; }
    msg.from = r.from;
    if (msg.t === 'room') {                                 // la sala, con los participantes que ya llegaron
      var old = N.rooms[r.from], parts = old && old.room.key === msg.room.key ? old.parts : [];
      N.rooms[r.from] = { room: msg.room, parts: parts, at: Date.now() };
      msg.room.parts = parts.slice(0, msg.room.n);
    } else if (msg.t === 'parts') {
      var rm = N.rooms[r.from];
      if (rm) { msg.list.forEach(function (p, i) { rm.parts[msg.at + i] = p; }); rm.room.parts = rm.parts.slice(0, rm.room.n); }
    }
    else if (msg.t === 'cards') N.cards[msg.part] = msg.cards;
    else if (msg.t === 'state') N.states[msg.id] = msg;          // solo el ultimo de cada partido
    else N.inbox.push(msg);
  });
  if (N.inbox.length > 200) N.inbox.splice(0, N.inbox.length - 200);
  Object.keys(N.rooms).forEach(function (k) { if (Date.now() - N.rooms[k].at > N.ROOM_LIFE) delete N.rooms[k]; });
};

// los mensajes de ese tipo que llegaron (y los quita)
SC.cup.net.take = function (type) {
  var N = SC.cup.net, out = [];
  N.inbox = N.inbox.filter(function (m) { if (m.t === type) { out.push(m); return false; } return true; });
  return out;
};

// el anfitrion, cada frame: anuncia su sala y reparte las cartas
SC.cup.net.host = function (room, dt) {
  var N = SC.cup.net;
  N.t += dt;
  if (N.t < N.TICK) return;
  N.t = 0; N.rr++;
  var sum = SC.cup.room.summary(room), parts = sum.parts, i;
  delete sum.parts; sum.n = parts.length;
  var rs = JSON.stringify(sum), ps = JSON.stringify(parts) + room.parts.map(function (p) { return p.cards ? p.cards.length : 0; }),
      peers = (N.peers || []).map(function (x) { return x.id; }).sort().join();
  var fresh = ps !== N.ps || peers !== N.pp;
  N.idle = (N.idle || 0) + N.TICK;
  if (rs !== N.rs || fresh || N.idle >= N.IDLE) { N.rs = rs; N.idle = 0; N.send({ t: 'room', room: sum }); }
  var slow = N.rr % Math.round(N.SLOW / N.TICK) === 0;
  if (fresh) {                                           // todo de golpe
    N.ps = ps; N.pp = peers;
    for (i = 0; i < parts.length; i += N.PARTS) N.send({ t: 'parts', at: i, list: parts.slice(i, i + N.PARTS) });
    room.parts.forEach(cards);
  } else if (slow && parts.length) {                     // uno cada SLOW segundos
    var at = (N.rr * N.PARTS) % (Math.ceil(parts.length / N.PARTS) * N.PARTS);
    N.send({ t: 'parts', at: at, list: parts.slice(at, at + N.PARTS) });
    cards(room.parts[N.rr % room.parts.length]);
  }
  function cards(p) { if (p) { N.cards[p.id] = p.cards; N.send({ t: 'cards', part: p.id, cards: p.cards }); } }
};

// las salas que se ven ahora: [{ host, room }]
SC.cup.net.list = function () {
  var N = SC.cup.net;
  return Object.keys(N.rooms).map(function (k) { return { host: k, room: N.rooms[k].room }; });
};
