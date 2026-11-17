// La de YEP_BattleEngineCore tal cual (sin las llaves del if) sobre una
// ventana de mentira
function Window_EnemyVisualSelect() {}
Window_EnemyVisualSelect.prototype.updateBattlerName = function() {
    if (this._battlerName !== this._battler.name())
    this._battlerName = this._battler.name();
    this._requestRefresh = true;
};

requestAnimationFrame(function () {
  T.test('installed', function () {
    T.equal(Window_EnemyVisualSelect.prototype.updateBattlerName.__quixHook, 'rpgmaker-mv/yep-enemy-name-refresh', 'the hook');
  });
  T.test('only when the name changes', function () {
    var name = 'Maneba';
    var w = new Window_EnemyVisualSelect();
    w._battler = { name: function () { return name; } };
    w._battlerName = 'Maneba';
    w._requestRefresh = false;
    w.updateBattlerName();
    T.equal(w._requestRefresh, false, 'same name: no redraw');
    name = 'Maneba [head]';
    w.updateBattlerName();
    T.equal(w._requestRefresh, true, 'new name: redraw');
    T.equal(w._battlerName, 'Maneba [head]', 'remembered');
    w._requestRefresh = true;   // pedido por otro (el tamano): se queda
    w.updateBattlerName();
    T.equal(w._requestRefresh, true, 'a pending redraw stays');
  });
  T.done();
});
