// ghosts.js: las naves de los demas, como fantasmas: la misma nave,
// semitransparente y de otro color. Se ven pero no chocan.
//
// Cada una se dibuja donde estaba hace DELAY ms, entre las dos posiciones
// que llegaron alrededor de ese momento, para que se mueva suave aunque
// lleguen solo unas 15 por segundo.
(function () {
  var net = window.MultiplayerNet;
  var DELAY = 120;
  var TINTS = [0xff5050, 0x50ff70, 0xffd040, 0xd060ff, 0x40e0ff, 0xff9030, 0xffffff];
  var tint = 0;

  // vectores de trabajo (no crear objetos en cada imagen)
  var pos, other, scale, rot, rotA, rotB;
  function work() {
    if (pos) return;
    pos = new THREE.Vector3();
    other = new THREE.Vector3();
    scale = new THREE.Vector3(1, 1, 1);
    rot = new THREE.Quaternion();
    rotA = new THREE.Quaternion();
    rotB = new THREE.Quaternion();
  }

  function scene(hexGL) {
    var game = hexGL.manager.get('game');
    return game && game.scene;
  }

  function make(hexGL) {
    var ship = hexGL.components.shipControls.mesh;
    var material = new THREE.MeshBasicMaterial({
      map: hexGL.track.lib.get('textures', 'ship.feisar.diffuse'),
      color: TINTS[tint++ % TINTS.length],
      transparent: true,
      opacity: 0.7,
      depthWrite: false
    });
    var ghost = new THREE.Mesh(ship.geometry, material);
    ghost.matrixAutoUpdate = false;
    ghost.visible = false;
    scene(hexGL).add(ghost);
    return ghost;
  }

  function remove(p) {
    if (p.ghost && p.ghost.parent) p.ghost.parent.remove(p.ghost);
    p.ghost = null;
  }

  // las dos muestras que rodean el momento 'at' y cuanto de una a otra
  function around(samples, at) {
    var i = 0;
    while (i < samples.length - 2 && samples[i + 1].at <= at) i++;
    var a = samples[i];
    var b = samples[Math.min(i + 1, samples.length - 1)];
    var k = 1;
    if (b.at > a.at) k = Math.max(0, Math.min(1, (at - a.at) / (b.at - a.at)));
    return { a: a.p, b: b.p, k: k };
  }

  function place(p, t) {
    if (p.samples.length === 0) return;
    var s = around(p.samples, t - DELAY);
    pos.set(s.a[0], s.a[1], s.a[2]);
    pos.lerpSelf(other.set(s.b[0], s.b[1], s.b[2]), s.k);
    rotA.set(s.a[3], s.a[4], s.a[5], s.a[6]);
    rotB.set(s.b[3], s.b[4], s.b[5], s.b[6]);
    THREE.Quaternion.slerp(rotA, rotB, rot, s.k);
    p.ghost.matrix.compose(pos, rot, scale);
    p.ghost.matrixWorldNeedsUpdate = true;
    p.ghost.visible = true;
  }

  var ghosts = {};

  ghosts.begin = function (hexGL) {
    work();
    for (var id in net.players) {
      var p = net.players[id];
      if (!p.ghost) p.ghost = make(hexGL);
      p.ghost.visible = false;
    }
  };

  // en cada imagen de la carrera
  ghosts.update = function (hexGL) {
    var t = net.now();
    for (var id in net.players) {
      var p = net.players[id];
      if (!p.ghost) p.ghost = make(hexGL);
      place(p, t);
    }
  };

  // para las pruebas (la consola de debug): cada rival, cuantas posiciones
  // llegaron, si se ve y a que distancia de la nave propia esta
  ghosts.debug = function (hexGL) {
    var me = hexGL.components.shipControls.dummy.position;
    var out = [];
    for (var id in net.players) {
      var p = net.players[id];
      var d = null;
      if (p.ghost) {
        var e = p.ghost.matrix.elements;
        d = Math.round(Math.sqrt(Math.pow(e[12] - me.x, 2) + Math.pow(e[13] - me.y, 2) +
          Math.pow(e[14] - me.z, 2)));
      }
      var seen = p.ghost ? (p.ghost.visible ? 'visible' : 'hidden') : 'none';
      out.push(id + ' samples ' + p.samples.length + ' ghost ' + seen + ' distance ' + d);
    }
    return out.join('; ');
  };

  net.onGone = remove;
  window.MultiplayerGhosts = ghosts;
})();
