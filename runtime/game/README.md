# game

English | [Español](README.es.md)

What quixjs adds on top of the game, without touching its files. Each folder
has its README.

| folder          | what it does |
|-----------------|--------------|
| `hooks/`        | `[hooks]` of the toml of the game: change how a function of the game runs (`gpu-canvas`, `skip`, `once-per-frame`, engine hooks, `engine = "rpgmaker-mv"`) ([README](hooks/README.md)) |
| `symbols/`      | `[symbols]`: names for the state of the game, read by the achievements and written by the cheats ([README](symbols/README.md)) |
| `achievements/` | achievements for ports, defined in the toml of the game ([README](achievements/README.md)) |
| `cheats/`       | cheats for ports (`[[cheat]]`), turned on in the launcher; a playthrough with cheats gets no achievements ([README](cheats/README.md)) |
