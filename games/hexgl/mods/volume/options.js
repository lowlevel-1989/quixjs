// options.js: Music y Effects en el menu del port, antes de Credits. Es el
// hook de PortMenu.items: recibe la lista de opciones y agrega las dos.
function (original) {
  var levels = window.VolumeLevels;

  function option(label, name) {
    return {
      label: label + ': ' + levels.get(name),
      change: function (d) {
        levels.set(name, levels.get(name) + d);
      }
    };
  }

  return function (list) {
    list = original.call(this, list);
    list.push(option('Music', 'music'), option('Effects', 'effects'));
    return list;
  };
}
