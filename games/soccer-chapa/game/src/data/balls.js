// games/soccer-chapa/game/src/data/balls.js: las bolas. Cada equipo elige la
// suya (la de casa se usa en su campo); salen del gacha como las franelas. Sus
// imagenes: games/soccer-chapa/game/assets/balls/<id>.png, <id>_roll.png y
// <id>_gloss.png (games/soccer-chapa/game/src/match/ball.js); gloss: cuanto
// brilla (1 si no lo dice).
SC.data.balls = {
  steel:  { rarity: 'start' },    // la de pinball
  onyx:   { rarity: 'common' },
  pearl:  { rarity: 'common' },
  gold:   { rarity: 'rare' },
  marble: { rarity: 'rare' },
  glass:  { rarity: 'epic' },
  soccer: { rarity: 'common', gloss: 0.5 }   // el balon clasico, de cuero
};
