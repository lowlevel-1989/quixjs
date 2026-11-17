# rpgmaker-mv

English | [Español](README.es.md)

Hooks for RPG Maker MV (1.6), found with the profiler (`--profile`) on a
handheld. Each one gives the same result as the original function. The
words frame, update, preset and playthrough are explained in
[runtime](../../README.md#words-used-in-the-code).

`[hooks] engine = "rpgmaker-mv"` turns on the hooks of `hooks.toml` (30
hooks on 35 paths, marked "preset" below). Most of them check that the
function is the one of RPG Maker MV or of its plugin; if not, they leave the
original. The ones marked "no check yet" do not check it yet: a game whose
plugin changes that function can turn them off with `"off"` in its `[hooks]`.
The others are turned on by each port in its own `[hooks]`.

| file                     | replaces | what it does |
|--------------------------|----------|--------------|
| `request-images-once.js` | `Game_Interpreter.requestImages` | preset: each event command list is walked once to preload its images, not every time a parallel event starts again |
| `skip-branch-table.js`   | `Game_Interpreter.prototype.skipBranch` | preset (no check yet): jumps over a false "if" block with a table built once per list, instead of line by line |
| `adjust-tone.js`         | `Bitmap.prototype.adjustTone` | preset: changes the tone of an image in C (`adjust_tone.c`), without getImageData and a JS loop |
| `tilemap-sprites-once-per-frame.js` | `Tilemap.prototype.update` | preset (no check yet): at 30 fps the map sprites are updated once per drawn frame (not twice), except those with an animation or a balloon; the tile animation keeps counting every update |
| `windowlayer-skip-empty.js` | `WindowLayer.prototype.renderWebGL` | preset (no check yet): with no window open, no filter pass (an empty full-screen render and a wait for the GPU every frame) |
| `event-idle-fast.js`     | `Game_Event.prototype.update` | preset (no check yet): a quiet event (no parallel, no movement, no chase) only updates its counters, as the whole chain of RPG Maker and plugins would |
| `tint-native.js`, `tint.c` | `Sprite.prototype._executeTint` | preset: the recolor of a sprite (battle flashes, tones) in C in one pass, instead of about eight canvas operations with blend modes; same pixels |
| `yep-enemy-name-refresh.js` | `Window_EnemyVisualSelect.prototype.updateBattlerName` | preset: YEP_BattleEngineCore is missing the braces of an if and redrew every enemy name every frame; now only when the name changes |
| `pure-common-events.js`  | `Game_CommonEvent.prototype.update` | preset: a pure parallel common event (only conditions and switch or variable changes, no waits, no randomness) is not run again while what it reads and writes stays the same |
| `achievements-menu.js`   | `Scene_Menu.prototype.createCommandWindow` | preset: an "Achievements" option in the game menu with the achievements of the port (`[achievements_menu] icon` for its IconSet icon) |
| `lazy-data.js`, `lazy_data.c` | `DataManager.loadDataFile` | preset: the pages of each troop and the list of each common event stay as text until they are used (fewer objects for the collector) |
| `decrypt-image-native.js`, `decrypt.c` | `Decrypter.decryptImg` | preset: quixjs reads, decrypts (key of `$dataSystem`) and decodes `.rpgmvp` images in the background; the bytes do not go through JS |
| `load-audio-native.js`   | `WebAudio.prototype._load` | preset: quixjs reads, decrypts and opens the sounds in the background, with the loop comments read in C |
| `lz-string-native.js`, `lz_string.c` | `LZString.decompressFromBase64` | preset: loading a save decompresses in C; checked once against the LZString of the game |
| `gc-on-transition.js`    | `Scene_Map.prototype.create`, `Scene_Battle.prototype.create` | preset: a full collection when the scene is created, with the screen black |
| `gc-when-still.js`       | `Scene_Map.prototype.update` | preset: the due collection runs while the player stands still, not while walking |
| `fluid-timestep-cap.js`  | `SceneManager.updateMain` | preset: at most 4 updates per frame with TDDP_FluidTimestep, so a slow frame does not spiral down |
| `interpreter-fast.js`    | `Game_Interpreter.prototype.update` | preset: runs in a row the commands that cannot change the checks done before each one (conditions, switches, variables) |
| `any-event-starting.js`  | `Game_Map.prototype.isAnyEventStarting` | preset: counts the starting events instead of walking every event of the map |
| `has-skill-fast.js`      | `Game_Actor.prototype.hasSkill` | preset: answers without building the whole list of skills |
| `traits-cache.js`        | `Game_BattlerBase.prototype.traits` | preset: a table of traits per type, valid while the actor, class, states and equipment stay the same |
| `party-member-fast.js`   | `Game_Follower.prototype.actor`, `Game_Party.prototype.leader` | preset: one party member without creating arrays |
| `sprite-update-loop.js`  | `Sprite.prototype.update` | preset: a plain loop over the children instead of `forEach` with a new function |
| `sort-if-needed.js`      | `Tilemap.prototype._sortChildren` | preset: the sprites are sorted only when they are not in order |
| `button-events-keys.js`  | `Scene_Map.prototype.updateButtonEvents` | preset: YEP_ButtonCommonEvents gets its keys once, not with `for...in` every update |
| `olivia-meta-fast.js`    | `value` / `setValue` of `Game_Switches` and `Game_Variables` | preset: Olivia_MetaControls checks once per switch or variable if its name is meta |
| `events-cached.js`       | `Game_Map.prototype.events` | preset (no check yet): the same array of events while it is still valid, instead of a new one each call |
| `refresh-if-changed.js`  | `setValue` of `Game_Switches`, `Game_Variables`, `Game_SelfSwitches` | the map asks for a refresh only if the stored value changed |
| `refresh-on-net-change.js` | `Game_Map.prototype.refresh` | preset (no check yet): the refresh is skipped when every switch and variable is back to its value and nothing else asked for it |
| `skip-hidden-sprites.js` | `Sprite_Character.prototype.update` | preset (no check yet): the sprites without image or out of the screen are not updated while they stay so |
| `terrax-lights-only.js`  | `Spriteset_Map.prototype.update` | TerraxLighting only walks the events whose note is a light |
| `tone-filter-when-needed.js` | `Spriteset_Base.prototype.updateToneChanger` | preset (no check yet): with tone 0,0,0,0 the ToneFilter of the base layer is taken out (two full-screen passes less per frame) |
| `playthrough.js`         | | not a hook: the playthrough in progress (`$gameSystem._quix`) for the cheats and the achievements |
| `natives.c`, `natives.h` | | the list of native functions (`__quixEngine["rpgmaker-mv"]`) |

Measured on an RG CubeXX with Fear & Hunger: `requestImages` was 35% of the
JS time in a map with 60 parallel events, `skipBranch` walked 282,000 lines
every 5 seconds there, and `adjustTone` was 29% of loading a save.
