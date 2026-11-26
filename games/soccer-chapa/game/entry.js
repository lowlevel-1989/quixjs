// games/soccer-chapa/game/entry.js: el arranque. Crea la ventana de PixiJS a
// pantalla completa (innerWidth x innerHeight, [game] fit = "screen"), carga
// las imagenes de assets/ y los sonidos, lee los ajustes (el idioma) y el
// perfil guardados (games/soccer-chapa/game/src/save/) y abre el menu principal
// (games/soccer-chapa/game/src/scenes/menu/menu.js).
(function () {
  var app = new PIXI.Application(window.innerWidth, window.innerHeight,
                                 { backgroundColor: SC.match.camera.FELT, antialias: false });
  document.body.appendChild(app.view);
  window.SC_APP = app;   // para la consola de depuracion
  SC.world.weather.roll();
  var files = { pitch: 'assets/pitch.jpg', mask: 'assets/mask.png', shade: 'assets/shade.png',
                gloss: 'assets/gloss.png', shadow: 'assets/shadow.png',
                metal: 'assets/metal.png', snow: 'assets/snow.png' };
  var ui = ['scoreboard', 'chip', 'turn', 'meter', 'meter_fill', 'needle', 'mark',
            'btn_a', 'btn_b', 'btn_x', 'btn_y', 'btn_lr', 'btn_ud', 'btn_r2', 'btn_l2', 'btn_move', 'btn_l1r1', 'btn_start', 'goal_band', 'goal_line', 'wind_panel',
            'wind_arrow', 'dot', 'tip', 'halo', 'name_tag', 'name_line',
            'zone_arrow', 'zone_line', 'drop', 'flake', 'card_ball', 'card_holo', 'card_glow', 'card_rays',
            'card_shine', 'impact_streak', 'logo', 'coin_flip', 'crown', 'card_back', 'card_back_logo', 'cap_shadow', 'pack_body', 'pack_top', 'pack_label'];
  ui.forEach(function (n) { files['ui/' + n + '.png'] = 'assets/ui/' + n + '.png'; });
  SC.gacha.positions.ORDER.concat(['ball', 'goal', 'kit', 'name']).forEach(function (p) {   // las 16 cartas y las de pelota, porteria y franela
    SC.gacha.rarity.ORDER.forEach(function (r) { files['ui/card_' + p + '_' + r + '.png'] = 'assets/ui/card_' + p + '_' + r + '.png'; });
  });
  for (var m = 0; m < SC.gacha.rarity.MODELS; m++) files['ui/card_mythic_' + m + '.png'] = 'assets/ui/card_mythic_' + m + '.png';
  Object.keys(SC.data.balls).forEach(function (b) {
    ['', '_roll', '_gloss'].forEach(function (k) { files['balls/' + b + k] = 'assets/balls/' + b + k + '.png'; });
  });
  Object.keys(SC.data.goals.list).forEach(function (g) {
    ['_r', '_l'].forEach(function (k) { files['goals/' + g + k] = 'assets/goals/' + g + k + '.png'; });
  });
  Object.keys(files).forEach(function (k) { PIXI.loader.add(k, files[k]); });
  PIXI.loader.load(function (loader, res) { SC.audio.sfx.load(function () { start(loader, res); }); });
  function start(loader, res) {
    SC.assets = {};
    Object.keys(files).forEach(function (k) { SC.assets[k] = res[k].texture; });
    SC.save.settings.load(); SC.save.profile.load();
    SC.scenes.menu.open(app);
  }
})();
