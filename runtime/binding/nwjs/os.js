// os de Node: una Linux de 1 GB, con la carpeta del juego como home.
(function (N) {
  var os = {
    platform: function () {
      return 'linux';
    },
    type: function () {
      return 'Linux';
    },
    arch: function () {
      return 'x64';
    },
    homedir: function () {
      return N.GAME_DIR;
    },
    tmpdir: function () {
      return '/tmp';
    },
    hostname: function () {
      return 'quixjs';
    },
    EOL: '\n',
    cpus: function () {
      return [];
    },
    totalmem: function () {
      return 1073741824;
    },
    freemem: function () {
      return 536870912;
    },
    release: function () {
      return '';
    },
    userInfo: function () {
      return { username: 'quixjs', homedir: N.GAME_DIR };
    }
  };
  N.os = os;
})(globalThis.__quixNw);
