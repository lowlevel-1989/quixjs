// games/abbaye/hooks/savestate.js: para Abbaye.Game.prototype.create. Los
// savestates de L'Abbaye des Morts (quix.state, runtime/binding/quix/state.c).
//
// Se guarda al entrar en una sala: Select + X espera ("Waiting for a safe
// moment") hasta que Jean cambia de sala y se guarda en ese instante. Una sala
// recien montada es lo que el juego sabe rehacer: al entrar (o al reaparecer
// Jean) cada enemigo vuelve a su sitio (switchOn llama a reset), asi que basta
// con la partida, que leyendo el codigo es:
//
//   Jean          la sala, su posicion y direccion, corazones, cruces y el
//                 punto de control (chepoint*)
//   las banderas  gameFlags (campana, interruptores, palanca, CROSS_USED...)
//   la escalera   stairIndex (bloques y escalones salen de ahi)
//   los objetos   ActionEntity.used (switchOn rehace el resto)
//   las puertas   Door.isClosed y activated
//   Satan         si existe
//
// Cargar, en cualquier momento seguro: los datos y la sala con setRoomByIndex,
// como cuando Jean reaparece (afterKillHero), con Jean en la entrada guardada.
function (original) {
  function list(grp) { return grp ? grp.children : []; }

  return function () {
    var r = original.apply(this, arguments);
    var g = this;
    if (!window.quix || !quix.state) return r;

    // cuando entro Jean en la sala
    var entered = Date.now(), setRoom = g.setRoom;
    g.setRoom = function () {
      var changed = setRoom.apply(this, arguments);
      if (changed) entered = Date.now();
      return changed;
    };

    quix.state.register({
      // Jean vivo, sin pausa ni pergamino, sin la escalera construyendose ni
      // Satan muriendo; para guardar, ademas, recien entrado en la sala
      can: function (load) {
        var h = g.hero, s = g.satan;
        var safe = Abbaye.game.state.current === 'Game' && h && !h.isDying && !h.isBlocked &&
                   !g.isOnDelay && !g.game.paused && !g.getGlobalFlag('PAUSE_ALL') &&
                   !g.getGlobalFlag('SHOW_LAST_SCROLL') && !(s && s.exists && s.isBlocked);
        return !!safe && (load || Date.now() - entered < 400);
      },
      save: function () {
        var h = g.hero, flags = {};
        for (var k in g.gameFlags) flags[k] = g.gameFlags[k];
        return JSON.stringify({
          room: g.currentRoomIndex, flags: flags, stair: g.stairIndex,
          hero: [h.x, h.y, h.scale.x, h.numHearts, h.numCross, h.chepointRoom, h.chepointX, h.chepointY],
          items: g.roomsData.map(function (room) {
            return room ? list(room.items).map(function (e) { return !!e.used; }) : [];
          }),
          doors: g.roomsData.map(function (room) {
            return room ? list(room.doors).map(function (e) { return [e.isClosed, e.activated]; }) : [];
          }),
          satan: g.satan ? g.satan.exists : true
        });
      },
      load: function (text) {
        var s = JSON.parse(text), h = g.hero;
        g.gameFlags = [];
        for (var k in s.flags) g.gameFlags[k] = s.flags[k];
        g.stairIndex = s.stair;
        g.roomsData.forEach(function (room, i) {
          list(room && room.items).forEach(function (e, n) {
            if ('used' in e && s.items[i]) e.used = s.items[i][n];
          });
          list(room && room.doors).forEach(function (e, n) {
            var d = s.doors[i] && s.doors[i][n];
            if (d) { e.isClosed = d[0]; e.activated = d[1]; }
          });
        });
        if (g.satan && g.satan.exists !== s.satan) {
          if (s.satan) g.satan.revive(); else g.satan.kill();
        }
        // el suelo de Satan: el juego lo pone al acabar la escalera y lo quita al matarlo
        g.map.setCollisionByIndex(185, !!s.flags.CROSS_USED && s.satan);
        var v = s.hero;
        h.numHearts = v[3];
        h.numCross = v[4];
        h.setCheckPoint(v[5], v[6], v[7]);
        // la sala, como al reaparecer, y Jean como resetHero, en la entrada guardada
        g.setRoomByIndex(s.room);
        h.resetHero();
        h.x = h.body.x = v[0];
        h.y = h.body.y = v[1];
        h.scale.x = v[2];
        g.panel.setNumLives(h.numHearts);
        g.panel.setNumCrosses(h.numCross);
      }
    });
    return r;
  };
}
