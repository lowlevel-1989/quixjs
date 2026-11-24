// reader/screens.js: como se presenta cada pantalla en el lector, como en el
// juego. R.collect() llena R.state.lines ([{kind, runs}]: title, field, gap o
// body) y R.state.dmg (el dano de una carta, a la izquierda):
// - el panel de un recluta (al pulsarlo en la batalla);
// - la ficha de un soldado de la milicia (el cursor en su casilla, Results);
// - un jefe de Results (R.state.boss, una pagina por tarjeta);
// - una carta de Add to Militia (R.state.card, la del cursor);
// - cualquier otra pantalla: el titulo, los datos cortos en una linea y los
//   textos largos como parrafos; en una lista (el Codex), cada entrada aparte.
(function (R) {
  var SEP = { t: '   ·   ', c: '#6a6a80' };   // entre datos de una linea
  var KEY = '#8a8aa0';                             // el color de "Total attack:"...

  function width() {
    return R.W - R.PAD * 2 - (R.state.dmg !== null ? R.DMGW : 0);
  }

  // una linea de cada trozo de rs (ya partida al ancho)
  function push(kind, rs, w) {
    R.g.font = R.font(kind);
    R.wrapRuns(rs, w || width()).forEach(function (l) { R.state.lines.push({ kind: kind, runs: l }); });
  }

  // el panel de un recluta (sf_infoPanel*): el juego no lo marca como
  // ventana, asi que se busca por sus textos. El nombre, el tipo y la rareza,
  // la descripcion, el ataque total con su base, bonus y multiplicador, los
  // despliegues y los usos. Devuelve true si lo habia
  function infoPanel(all) {
    var by = {};
    all.forEach(function (t) { if (!by[t.name]) by[t.name] = t; });
    if (!by.sf_infoPanelTitle) return false;
    var runs = function (name) { return by[name] ? R.runs(by[name].raw) : []; };
    push('title', runs('sf_infoPanelTitle'));
    var head = runs('sf_infoPanelType');
    if (by.sf_infoPanelRarity) head = head.concat(head.length ? [SEP] : [], runs('sf_infoPanelRarity'));
    if (head.length) R.state.lines.push({ kind: 'field', runs: head });
    if (by.sf_infoPanelDescNEW) {
      R.state.lines.push({ kind: 'gap', runs: [] });
      push('body', runs('sf_infoPanelDescNEW'));
    }
    R.state.lines.push({ kind: 'gap', runs: [] });
    var atk = by.sf_infoPanelDmg ? [{ t: 'Total attack: ', c: KEY }].concat(runs('sf_infoPanelDmg')) : [];
    // los tres sf_IPDmg, de arriba abajo: la base, el bonus y el multiplicador
    var names = ['Base', 'Bonus', 'Multi'];
    var parts = all.filter(function (t) { return t.name === 'sf_IPDmg'; })
      .sort(function (a, b) { return a.t - b.t; }).slice(0, 3)
      .map(function (t, k) { return names[k] + ' ' + t.text; });
    if (parts.length) atk.push({ t: '   (' + parts.join(' · ') + ')', c: KEY });
    if (atk.length) R.state.lines.push({ kind: 'field', runs: atk });
    var more = by.sf_infoPanelRound ? [{ t: 'Deploys: ', c: KEY }].concat(runs('sf_infoPanelRound')) : [];
    if (by.sf_infoPanelUses) more = more.concat(more.length ? [SEP] : [], [{ t: 'Uses: ', c: KEY }], runs('sf_infoPanelUses'));
    if (more.length) R.state.lines.push({ kind: 'field', runs: more });
    return true;
  }

  // la ficha de un soldado ("MMM-ID:19\nBOOK\n\nTOTAL DMG:0", el texto que el
  // juego muestra al pasar por su casilla): su nombre (la linea que no es
  // "dato:valor") como titulo y los datos debajo
  function soldier(all) {
    var tip = all.filter(function (t) { return t.name === 'sfPieceHoverText'; })[0];
    if (!tip) return;
    var ls = tip.text.split('\n').map(function (l) { return l.trim(); }).filter(function (l) { return l; });
    ls.filter(function (l) { return l.indexOf(':') < 0; }).forEach(function (l) { push('title', [{ t: l, c: null }]); });
    var data = ls.filter(function (l) { return l.indexOf(':') >= 0; });
    if (data.length) R.state.lines.push({ kind: 'field', runs: [{ t: data.join('   ·   '), c: null }] });
  }

  // un jefe de Results: su numero por su orden ("Boss 2 of 3", el de las
  // tarjetas del juego, de izquierda a derecha) y su tarjeta
  function bossPage() {
    var b = R.state.boss;
    R.state.lines.push({ kind: 'field', runs: [{ t: 'Boss ' + (b.page + 1) + ' of ' + b.pages.length, c: KEY }] });
    emit(partsOf(b.pages[b.page].filter(function (i) { return i.name !== 'SF_bossCardNumber'; })));
  }

  // los textos de una carta: los que tienen el centro dentro de su recuadro
  // (el boundary_chooser: la carta entera, con su dano a la izquierda). El
  // dano va aparte (R.state.dmg)
  function cardTexts(all) {
    var c = R.state.card;
    var pick = all.filter(function (t) {
      return t.x >= c.x - c.w / 2 && t.x <= c.x + c.w / 2 && t.y >= c.y - c.h / 2 && t.y <= c.y + c.h / 2;
    });
    if (!pick.length) pick = all;
    return pick.filter(function (t) {
      if (R.label(t.name) !== 'Damage' || R.state.dmg !== null) return true;
      R.state.dmg = t.text;
      return false;
    });
  }

  // una lista de entradas (el Codex: varios nombres en pantalla): cada nombre
  // con lo que tiene debajo hasta el siguiente, como una carta; en cada una,
  // lo corto (tipo, dano) antes que la descripcion. Sin lista: un grupo
  function groups(pick) {
    var heads = pick.filter(function (t) { return /name|title/i.test(t.name) && t.text.length <= 28; });
    if (heads.length < 2) return [pick];
    var out = [[]].concat(heads.map(function (h) { return [h]; }));
    pick.forEach(function (t) {
      if (heads.indexOf(t) >= 0) return;
      var k = 0;
      heads.forEach(function (h, i) { if (h.t <= t.t + 12) k = i + 1; });
      out[k].push(t);
    });
    // la ultima entrada, cortada por el borde de la pantalla, sin mas que su
    // nombre: fuera
    if (out.length > 2 && out[out.length - 1].length === 1) out.pop();
    out.forEach(function (list) {
      list.sort(function (a, b) {
        var la = a.text.length > 28 ? 1 : 0, lb = b.text.length > 28 ? 1 : 0;
        return la - lb || (Math.abs(a.t - b.t) > 6 ? a.t - b.t : a.l - b.l);
      });
    });
    return out;
  }

  // un texto de varias lineas (el de una carta: nombre, tipo y descripcion)
  // se separa: sus lineas cortas del principio son el titulo y datos. Solo si
  // acaba en una descripcion larga; un bloque de lineas cortas (las
  // estadisticas de la victoria) se queda como esta
  function partsOf(pick) {
    var parts = [];
    pick.forEach(function (t) {
      var rs = R.runs(t.raw), text = R.textOf(rs);
      if (text.split('\n').filter(function (l) { return l.trim(); }).length < 2) return parts.push({ name: t.name, runs: rs, text: text.trim() });
      // por lineas, con sus colores
      var lineRuns = [[]];
      rs.forEach(function (r) {
        r.t.split('\n').forEach(function (s, k) {
          if (k) lineRuns.push([]);
          if (s) lineRuns[lineRuns.length - 1].push({ t: s, c: r.c });
        });
      });
      lineRuns = lineRuns.filter(function (l) { return R.textOf(l).trim(); });
      var lastText = lineRuns.length ? R.textOf(lineRuns[lineRuns.length - 1]).trim() : '';
      if (lastText.length <= 28) return parts.push({ name: t.name, runs: rs, text: text.trim(), body: true });
      var k = 0;
      while (k < lineRuns.length - 1 && R.textOf(lineRuns[k]).trim().length <= 28) {
        parts.push({ name: k ? '' : t.name, runs: lineRuns[k], text: R.textOf(lineRuns[k]).trim() });
        k++;
      }
      var rest = [];
      lineRuns.slice(k).forEach(function (l, n) { rest = rest.concat(n ? [{ t: ' ', c: null }] : [], l); });
      parts.push({ name: t.name, runs: rest, text: R.textOf(rest), body: true });
    });
    return parts;
  }

  // las partes de un grupo, a lineas: su titulo (el primer texto corto sin
  // etiqueta), sus datos cortos en una linea y sus parrafos
  function emit(parts) {
    var S = R.state, fields = [], titled = false, start = S.lines.length;
    function flush() {
      if (!fields.length) return;
      var rs = [];
      fields.forEach(function (f, n) { rs = rs.concat(n ? [SEP] : [], f); });
      push('field', rs);
      fields = [];
    }
    parts.forEach(function (t) {
      var lab = t.body ? '' : R.label(t.name), short = !t.body && t.text.length <= 28;
      if (short && !lab && !titled && S.lines.length === start && !fields.length) {
        titled = true;
        push('title', t.runs);
        return;
      }
      if (short) {
        // un numero suelto sin etiqueta (el de cada casilla de la milicia)
        // no dice nada sin su casilla
        if (!lab && /^\d+$/.test(t.text)) return;
        // con su etiqueta, salvo si ya trae una ("DMG: 0")
        fields.push(lab && t.text.indexOf(':') < 0 ? [{ t: lab + ': ', c: KEY }].concat(t.runs) : t.runs);
        return;
      }
      flush();
      if (S.lines.length > start) S.lines.push({ kind: 'gap', runs: [] });
      push('body', t.runs);
    });
    flush();
  }

  R.collect = function () {
    var S = R.state, rt = R.runtime();
    if (!rt) return;
    var all = R.texts(rt);
    S.lines = [];
    S.dmg = null;
    S.scroll = 0;
    if (S.hover) soldier(all);
    else if (S.boss) bossPage();
    else if (S.card || !infoPanel(all)) {
      var pick = S.card ? cardTexts(all) : all;
      // las letras sueltas (el indice A-Z del Codex) no se leen
      pick = pick.filter(function (t) { return !/^[a-z]$/i.test(t.text); });
      groups(pick).forEach(function (list) {
        if (!list.length) return;
        if (S.lines.length) S.lines.push({ kind: 'gap', runs: [] });
        emit(partsOf(list));
      });
    }
    if (!S.lines.length) S.lines = [{ kind: 'body', runs: [{ t: '(no text on screen)', c: null }] }];
  };
})(window.QuixReader = window.QuixReader || {});
