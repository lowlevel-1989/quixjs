// games/soccer-chapa/game/src/match/ai/levels.js: los niveles de la maquina.
// No juegan igual de bien ni de la misma forma: cada uno prueba golpes
// distintos con la fisica del juego (games/soccer-chapa/game/src/match/ai/sim.js) y los valora a su
// manera (games/soccer-chapa/game/src/match/ai/choose.js). Ninguno es imposible: todos fallan algo
// al ejecutar (err).
//   caps     cuantas chapas (las mas cercanas al balon) mira para golpear
//   mates    si piensa en pasar (si no, solo tira o avanza)
//   powers   las fuerzas que prueba
//   spin     el efecto que prueba: 'none', 'goal' (solo al tirar) o 'all'
//   wind     cuanto del viento tiene en cuenta (0 lo ignora, 1 entero)
//   setup    cuanto valora que el pase le deje tiro (jugadas de dos golpes)
//   err      lo que falla al ejecutar: [angulo en radianes, fuerza]
//   call     como avisa el tiro a gol: 'careless' (a veces no avisa o
//            avisa tiros malos) o 'sure' (solo si su prueba dice gol)
//   keeper   como coloca a su portero: 'random', 'ball' (hacia el balon) o
//            'line' (en la linea del tiro), con un error (keeperErr)
//   think    lo que tarda en decidir (segundos)
SC.match.ai.ORDER = ['apprentice', 'amateur', 'veteran', 'legend'];

SC.match.ai.LEVELS = {
  apprentice: {
    caps: 2, mates: true, powers: [0.55, 0.8, 1], spin: 'none', wind: 0, setup: 0,
    err: [0.07, 0.12], call: 'careless', keeper: 'random', keeperErr: 0.5, think: 1.2
  },
  amateur: {
    caps: 2, mates: true, powers: [0.4, 0.55, 0.7, 0.85, 1], spin: 'none', wind: 0.5, setup: 0,
    err: [0.05, 0.08], call: 'sure', keeper: 'ball', keeperErr: 0.35, think: 1.0
  },
  veteran: {
    caps: 3, mates: true, powers: [0.35, 0.5, 0.65, 0.8, 0.9, 1], spin: 'goal', wind: 1, setup: 1,
    err: [0.03, 0.05], call: 'sure', keeper: 'line', keeperErr: 0.25, think: 0.9
  },
  legend: {
    caps: 3, mates: true, powers: [0.3, 0.42, 0.55, 0.68, 0.8, 0.9, 1], spin: 'all', wind: 1, setup: 1.6,
    err: [0.015, 0.03], call: 'sure', keeper: 'line', keeperErr: 0.12, think: 0.8
  }
};

// el nivel de un equipo (team.level; si no tiene, aficionado)
SC.match.ai.level = function (team) { return SC.match.ai.LEVELS[team.level] || SC.match.ai.LEVELS.amateur; };
