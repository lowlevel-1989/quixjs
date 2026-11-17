// Un plugin que se carga tarde (como los <script> de los plugins de RPG
// Maker) y cambia una funcion que ya tenia hook. Aqui, Olivia_MetaControls y
// el hook rpgmaker-mv/olivia-meta-fast: el orden del juego de verdad.
var $dataSystem = { switches: [null, 'normal', 'shared <Global>', 'other'] };
function Game_Switches() { this._data = []; }
Game_Switches.prototype.value = function (switchId) { return !!this._data[switchId]; };
function Game_Variables() {}
var Olivia, metaWrites = [];
Game_Switches.prototype.setValue = function (switchId, value) { this._data[switchId] = value; };
setTimeout(function () {
  Olivia = { MetaControls: {} };
  Olivia.MetaControls.___Game_Switches_setValue___ = Game_Switches.prototype.setValue;
  Game_Switches.prototype.setValue = function (id, value) {
    Olivia['MetaControls']['___Game_Switches_setValue___'].call(this, id, value);
    if (id > 0 && id < $dataSystem['switches'].length) {
      if ($dataSystem.switches[id].match(/<(?:Global|Global Meta)>/i)) metaWrites.push('g' + id);
      else if ($dataSystem.switches[id].match(/<(?:Local|Local Meta|Meta)>/i)) metaWrites.push('l' + id);
    }
  };
  Olivia.MetaControls.___Game_Switches_value___ = Game_Switches.prototype.value;
  Game_Switches.prototype.value = function (id) {
    if ($dataSystem.switches[id].match(/<(?:Global|Global Meta)>/i)) return 'global';
    else if ($dataSystem.switches[id].match(/<(?:Local|Local Meta|Meta)>/i)) return 'local';
    else return Olivia.MetaControls.___Game_Switches_value___.call(this, id);
  };
}, 50);

var frames = 0;
requestAnimationFrame(function wait() {
  if (++frames < 90) return requestAnimationFrame(wait);   // hooks_check: cada 30 imagenes
  T.test('a plugin loaded late', function () {
    T.equal(Game_Switches.prototype.value.__quixHook, 'rpgmaker-mv/olivia-meta-fast', 'the hook is back on top of the plugin');
    T.ok(!!Olivia.MetaControls.___Game_Switches_value___.__quixHook, 'the plugin saved the hooked copy (the real order)');
    var s = new Game_Switches();
    s._data[1] = true;
    T.equal(s.value(1), true, 'normal switch on');
    T.equal(s.value(3), false, 'normal switch off');
    T.equal(s.value(2), 'global', 'meta switch through the plugin');
    T.equal(Game_Switches.prototype.setValue.__quixHook, 'rpgmaker-mv/olivia-meta-fast', 'setValue hook back on top');
    s.setValue(1, false); s.setValue(2, true); s.setValue(3, true); s.setValue(9, true);
    T.equal(JSON.stringify(s._data.slice(1)), JSON.stringify([false, true, true, null, null, null, null, null, true]), 'every write stored');
    T.equal(metaWrites.join(','), 'g2', 'only the meta switch went through the plugin meta part');
  });
  T.done();
});
