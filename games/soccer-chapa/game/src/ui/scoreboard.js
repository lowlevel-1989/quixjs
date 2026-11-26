// games/soccer-chapa/game/src/ui/scoreboard.js: el marcador, arriba a la
// izquierda: los dos colores de cada equipo, sus 3 letras, el resultado y
// una barra bajo el equipo que tiene el turno.
SC.ui.scoreboard = {};

function abbr(name) {
  return name.replace(/[^A-Za-z0-9ÁÉÍÓÚÑáéíóúñ]/g, '').slice(0, 3).toUpperCase();
}

SC.ui.scoreboard.create = function (parent, teams) {
  var u = SC.ui.u, sb = { box: new PIXI.Container(), teams: teams };
  sb.box.x = 12 * u; sb.box.y = 8 * u;
  parent.addChild(sb.box);
  sb.box.addChild(SC.ui.sprite('scoreboard'));
  var ox = 10 * u, oy = 9 * u;        // el borde de la sombra de la placa
  teams.forEach(function (t, i) {
    var x = i === 0 ? ox + 5 * u : ox + 286 * u;
    var top = SC.ui.sprite('chip', t.color1);
    var bottom = SC.ui.sprite('chip', t.color2);
    top.x = bottom.x = x; top.y = oy + 4 * u; bottom.y = oy + 21 * u;
    top.width = bottom.width = 9 * u;
    sb.box.addChild(top, bottom);
    var name = SC.ui.text('Black', 19);
    name.text = abbr(t.name);
    name.x = i === 0 ? ox + 75 * u : ox + 225 * u; name.y = oy + 21 * u;
    sb.box.addChild(name);
  });
  sb.goals = [132, 168].map(function (x) {
    var s = SC.ui.text('Black', 23, 0x0e1216);
    s.x = ox + x * u; s.y = oy + 22 * u;
    sb.box.addChild(s);
    return s;
  });
  sb.bar = SC.ui.sprite('turn'); sb.bar.y = oy + 44 * u;
  sb.box.addChild(sb.bar);
  sb.barX = [ox + 35 * u, ox + 185 * u];
  return sb;
};

SC.ui.scoreboard.update = function (sb, m) {
  sb.goals[0].text = m.score[0];
  sb.goals[1].text = m.score[1];
  sb.bar.visible = m.phase !== 'goal';
  sb.bar.x = sb.barX[m.turn];
  sb.bar.tint = SC.ui.light(sb.teams[m.turn].color1);
};
