// games/soccer-chapa/game/src/ui/card/rename.js: las cartas de cambio de
// nombre, como las de la franela (el fondo, games/soccer-chapa/game/tools/ui/items.py:
// name): una firma grande y debajo lo que cambia de una chapa: el nombre,
// el apellido o los dos.
SC.ui.card.rename = {
  KINDS: ['first', 'last', 'full'],
  RARITY: { first: 'rare', last: 'rare', full: 'epic' }
};

// el color de la tinta de cada rareza (el de la firma de
// games/soccer-chapa/game/tools/ui/items.py: name)
SC.ui.card.rename.INK = { common: 0x223ca0, rare: 0xf0f4fa, epic: 0xffeca0, legendary: 0xffffff };

// el dibujo: lo que cambia, debajo de la firma (que va en el fondo), limpio:
// sin placa ni borde, del color de la tinta
SC.ui.card.rename.art = function (kind) {
  var box = new PIXI.Container(), ink = SC.ui.card.rename.INK[SC.ui.card.rename.RARITY[kind]];
  var t = new PIXI.Text(SC.lang.t('rename.' + kind).toUpperCase(), {
    fontFamily: 'Montserrat Black', fontSize: 26, fill: ink, align: 'center', lineHeight: 30, letterSpacing: 2, padding: 8
  });
  t.anchor.set(0.5); t.y = 140;
  box.addChild(t);
  return box;
};

// la carta entera, como las de Franela y bola (games/soccer-chapa/game/src/ui/menu/hand.js: card)
SC.ui.card.rename.card = function (kind) {
  return SC.ui.menu.hand.card({ label: 'rename.label', bg: 'card_name_' + SC.ui.card.rename.RARITY[kind],
                                art: function () { return SC.ui.card.rename.art(kind); } });
};
