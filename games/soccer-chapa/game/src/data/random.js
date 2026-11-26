// games/soccer-chapa/game/src/data/random.js: el azar sobre listas.
SC.data.random = {};

// un elemento al azar
SC.data.random.pick = function (list) { return list[Math.floor(Math.random() * list.length)]; };

// la lista desordenada (la misma lista)
SC.data.random.shuffle = function (list) { return list.sort(function () { return Math.random() - 0.5; }); };
