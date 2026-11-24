// reader/text.js: el formato del texto del lector. El BBCode del juego en
// trozos con su color, las etiquetas de los datos ("Damage", "Uses"...) por el
// nombre de su objeto, y el texto partido en lineas que caben en un ancho
// (con la letra que tenga el canvas del lector, view.js).
(function (R) {
  // sin las etiquetas BBCode ([color=...]...[/color]) ni corchetes sueltos
  R.clean = function (t) {
    return String(t).replace(/\[[^\]]*\]/g, '').replace(/[\[\]]/g, '').replace(/\r/g, '').trim();
  };

  // la etiqueta de un dato, por el nombre de su objeto (sf_infoPanelDmg...)
  var LABELS = [[/dmg|damage/i, 'Damage'], [/uses/i, 'Uses'], [/rarity/i, 'Rarity'],
                [/type/i, 'Type'], [/round/i, 'Round'], [/reroll/i, 'Rerolls']];
  R.label = function (name) {
    for (var i = 0; i < LABELS.length; i++) if (LABELS[i][0].test(name)) return LABELS[i][1];
    return '';
  };

  // el texto con sus colores: el BBCode del juego ([color=#a99c8b]...[/color])
  // en trozos {t, c}; las demas etiquetas ([tag=unit]...) se quitan. c null:
  // el color de la linea
  R.runs = function (raw) {
    var out = [], color = [], re = /\[(\/?)([a-z]+)(?:=([^\]]*))?\]/gi, last = 0, m;
    raw = String(raw).replace(/\r/g, '');
    function add(s) {
      s = s.replace(/[\[\]]/g, '');
      if (s) out.push({ t: s, c: color.length ? color[color.length - 1] : null });
    }
    while ((m = re.exec(raw))) {
      add(raw.slice(last, m.index));
      last = re.lastIndex;
      if (m[2].toLowerCase() !== 'color') continue;
      if (m[1]) color.pop();
      else color.push(m[3]);
    }
    add(raw.slice(last));
    return out;
  };

  // el texto de unos trozos, sin colores
  R.textOf = function (rs) {
    return rs.map(function (r) { return r.t; }).join('');
  };

  // los trozos partidos en lineas que caben en el ancho w (con la letra que
  // tenga R.g); cada linea, sus trozos. Los saltos de linea del texto,
  // tambien. Una palabra puede tener varios trozos ("GRENADE" y su "S"
  // pegada, de otro color) y no se parte
  R.wrapRuns = function (rs, w) {
    var g = R.g, words = [], cur = [];
    function end() {
      if (cur.length) words.push(cur);
      cur = [];
    }
    rs.forEach(function (r) {
      r.t.split(/(\n|\s+)/).forEach(function (piece) {
        if (!piece) return;
        if (piece === '\n') {
          end();
          words.push('\n');
        } else if (/^\s+$/.test(piece)) {
          end();
        } else {
          cur.push({ t: piece, c: r.c });
        }
      });
    });
    end();
    var lines = [], line = [], width = 0, space = g.measureText(' ').width;
    words.forEach(function (word) {
      if (word === '\n') {
        lines.push(line);
        line = [];
        width = 0;
        return;
      }
      var ww = word.reduce(function (s, p) { return s + g.measureText(p.t).width; }, 0);
      if (line.length && width + space + ww > w) {
        lines.push(line);
        line = [];
        width = 0;
      }
      if (line.length) {
        line.push({ t: ' ', c: null });
        width += space;
      }
      line = line.concat(word);
      width += ww;
    });
    if (line.length) lines.push(line);
    return lines;
  };
})(window.QuixReader = window.QuixReader || {});
