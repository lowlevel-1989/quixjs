# rpgmaker-mv

English | [Español](README.es.md)

Hooks for RPG Maker MV (1.6), found with the profiler (`--profile`) on a
handheld. Each one gives the same result as the original function. The
words frame, update, preset and playthrough are explained in
[runtime](../../README.md#words-used-in-the-code).

`[hooks] engine = "rpgmaker-mv"` turns on the hooks of `hooks.toml` (31
hooks on 41 paths, marked "preset" below). Most of them check that the
function is the one of RPG Maker MV or of its plugin; if not, they leave the
original. The ones marked "no check yet" do not check it yet: a game whose
plugin changes that function can turn them off with `"off"` in its `[hooks]`.
The others are turned on by each port in its own `[hooks]`.

| file                     | replaces | what it does |
|--------------------------|----------|--------------|
| `request-images-once.js` | `Game_Interpreter.requestImages` | preset: each event command list is walked once to preload its images, not every time a parallel event starts again |
| `skip-branch-table.js`   | `Game_Interpreter.prototype.skipBranch` | preset (no check yet): jumps over a false "if" block with a table built once per list, instead of line by line |
| `adjust-tone.js`         | `Bitmap.prototype.adjustTone` | preset: changes the tone of an image in C (`adjust_tone.c`), without getImageData and a JS loop |
| `blur-native.js`, `blur.c` | `Bitmap.prototype.blur` | preset: the blur of the screen snapshot behind the menu and battles in C, instead of 18 full-screen drawImage calls (330 ms to 50 ms on the RG CubeXX); exact to the math of the original |
| `tilemap-sprites-once-per-frame.js` | `Tilemap.prototype.update` | preset (no check yet): at 30 fps the map sprites are updated once per drawn frame (not twice), except those with an animation or a balloon; the tile animation keeps counting every update |
| `windowlayer-skip-empty.js` | `WindowLayer.prototype.renderWebGL` | preset (no check yet): with no window open, no filter pass (an empty full-screen render and a wait for the GPU every frame) |
| `event-idle-fast.js`     | `Game_Event.prototype.update` | preset (no check yet): a quiet event (no parallel, no movement, no chase) only updates its counters, as the whole chain of RPG Maker and plugins would |
| `tint-native.js`, `tint.c` | `Sprite.prototype._executeTint` | preset: the recolor of a sprite (battle flashes, tones) in C in one pass, instead of about eight canvas operations with blend modes; same pixels |
| `yep-enemy-name-refresh.js` | `Window_EnemyVisualSelect.prototype.updateBattlerName` | preset: YEP_BattleEngineCore is missing the braces of an if and redrew every enemy name every frame; now only when the name changes |
| `refresh-if-same-drawing.js` | `Window_BattleEnemy.prototype.refresh`, `Window_BattleStatus.prototype.refresh` | preset: the drawing calls of a window refresh are recorded; if they are the same as last time the bitmap is left alone (not drawn nor uploaded to the GPU again); if anything draws another way, the original refresh |
| `loading-notice.js` | `DataManager.loadGame`, `DataManager.setupNewGame`, `Scene_Map.prototype.onMapLoaded`, `Scene_Battle.prototype.createDisplayObjects`, `Scene_Map.prototype.start`, `Scene_Battle.prototype.start` | preset: "Loading..." at the bottom right while a save is loaded or a new game starts from the title (`quix.loading`); it goes away once the scene has started and frames are normal again. On a map change or at the start of a battle, only if the game stalls for more than 1 s (on the RX6S, several seconds before a battle); never when coming back from a menu. On the RG CubeXX, Continue is ~4 s of half-second frames |
| `yep-enemy-select-refresh.js` | `Window_EnemyVisualSelect.prototype.refresh` | preset: with Show Enemy Name off (or a hidden enemy) YEP_BattleEngineCore cleared every enemy window every frame and it was uploaded to the GPU again (~90 uploads per second in a Fear & Hunger battle); now it is cleared once |
| `pure-common-events.js`  | `Game_CommonEvent.prototype.update`, `Game_Event.prototype.updateParallel` | preset: a pure parallel process, common or map event (only conditions and switch or variable changes, no waits, no randomness), is not run again if its last run changed nothing and what it reads and writes stays the same |
| `achievements-menu.js`   | `Scene_Menu.prototype.createCommandWindow` | preset: an "Achievements" option in the game menu with the achievements of the port (`[achievements_menu] icon` for its IconSet icon) |
| `lazy-data.js`, `lazy_data.c` | `DataManager.loadDataFile` | preset: the pages of each troop and the list of each common event stay as text until they are used (fewer objects for the collector) |
| `decrypt-image-native.js`, `../rpgmaker/decrypt.c` | `Decrypter.decryptImg` | preset: quixjs reads, decrypts (key of `$dataSystem`; the decrypter in C is shared with MZ) and decodes `.rpgmvp` images in the background; the bytes do not go through JS |
| `load-audio-native.js`   | `WebAudio.prototype._load` | preset: quixjs reads, decrypts and opens the sounds in the background, with the loop comments read in C |
| `lz-string-native.js`, `lz_string.c` | `LZString.decompressFromBase64` | preset: loading a save decompresses in C; checked once against the LZString of the game |
| `../rpgmaker/gc-on-transition.js` | `Scene_Map.prototype.create`, `Scene_Battle.prototype.create` | preset (`rpgmaker/gc-on-transition`, shared with MZ): a full collection when the scene is created, with the screen black |
| `gc-when-still.js`       | `Scene_Map.prototype.update` | optional (not in the preset): the due collection runs while the player stands still; with JavaScriptCore it was a little worse than letting it collect on its own |
| `scene-release.js`       | `Scene_Base.prototype.terminate` | preset: when a scene ends, clears what kept it alive: the sprites of the last batch of PixiJS v4 and its filter stack, and globals that point to one of its sprites (YEP_BattleEngineCore: `childSprite`) |
| `battle-release.js`      | `Scene_Battle.prototype.terminate` | preset: when a battle ends, BattleManager lets go of its sprites and windows (it kept the whole battle until the next one) |
| `render-scale.js`       | `Graphics._updateRenderer` | preset (only with `[game] render_scale`): PIXI draws the game at that resolution (`"auto"`: the one it is shown at on the screen); the game logic stays at its width and height |
| `fluid-timestep-cap.js`  | `SceneManager.updateMain` | preset: at most 4 updates per frame with TDDP_FluidTimestep, so a slow frame does not spiral down |
| `interpreter-fast.js`    | `Game_Interpreter.prototype.update` | preset: runs in a row the commands that cannot change the checks done before each one (conditions, switches, variables) |
| `any-event-starting.js`  | `Game_Map.prototype.isAnyEventStarting` | preset: counts the starting events instead of walking every event of the map |
| `any-event-starting-quick.js` | `Game_Map.prototype.isAnyEventStartingQuick` | preset: the same question of YEP_StopMapMovement, answered with the count of `any-event-starting` |
| `has-skill-fast.js`      | `Game_Actor.prototype.hasSkill` | preset: answers without building the whole list of skills |
| `traits-cache.js`        | `Game_BattlerBase.prototype.traits` | preset: a table of traits per type, valid while the actor, class, states and equipment stay the same |
| `party-member-fast.js`   | `Game_Follower.prototype.actor`, `Game_Party.prototype.leader` | preset: one party member without creating arrays |
| `sprite-update-loop.js`  | `Sprite.prototype.update` | preset: a plain loop over the children instead of `forEach` with a new function |
| `sort-if-needed.js`      | `Tilemap.prototype._sortChildren` | preset: the sprites are sorted only when they are not in order |
| `button-events-keys.js`  | `Scene_Map.prototype.updateButtonEvents` | preset: YEP_ButtonCommonEvents gets its keys once, not with `for...in` every update |
| `olivia-meta-fast.js`    | `value` / `setValue` of `Game_Switches` and `Game_Variables` | preset: Olivia_MetaControls checks once per switch or variable if its name is meta |
| `events-cached.js`       | `Game_Map.prototype.events` | preset (no check yet): the same array of events while it is still valid, instead of a new one each call |
| `refresh-if-changed.js`  | `setValue` of `Game_Switches`, `Game_Variables`, `Game_SelfSwitches` | the map asks for a refresh only if the stored value changed |
| `refresh-on-net-change.js` | `Game_Map.prototype.refresh` | preset: the refresh is skipped when every switch and variable is back to its value and nothing else asked for it; with real changes, only the events whose pages depend on what changed are refreshed (only with the refresh and page functions of RPG Maker MV, or Olivia_MetaControls with `olivia-meta-fast`; a plugin that changes them gets the full refresh) |
| `find-path-index.js`    | `Game_Character.prototype.findDirectionTo` | preset: while an event looks for a path (chasing the player), the nearby events are grouped once by tile (`../lib/tile-index.js`) and `eventsXyNt` answers from there, without walking all the events of the map for every tile it tries; the same directions (only with the functions of RPG Maker MV) |
| `black-mask-once.js`    | `Spriteset_Map.prototype.createBlackMask` | Firework port (ScreenEffect): the two black bands are blurred once and reused (`cacheAsBitmap`), instead of applying the filter every frame |
| `save-picture-small.js` | `SceneManager.snapForSave` | Firework port (KJ_Menu): the picture it keeps for the save file, taken at every map change, menu and autosave, is read from the GPU at a quarter of the resolution (~450 ms to ~100 ms on the RG CubeXX); the game never shows it |
| `skip-hidden-sprites.js` | `Sprite_Character.prototype.update` | preset (no check yet): the sprites without image or out of the screen are not updated while they stay so |
| `terrax-lights-only.js`  | `Spriteset_Map.prototype.update` | TerraxLighting only walks the events whose note is a light |
| `terrax-mask-scale.js`  | `Spriteset_Map.prototype.createLightmask` | instead of `gpu-canvas`: the TerraxLighting light mask on the GPU and at the resolution the game is seen at (`render_scale`); Firework on an RG CubeXX walks at a steady 30 fps (before, almost a third of the frames went over 38 ms) |
| `snap-half.js`           | `SceneManager.snapForBackground` | the blurred snapshot behind the menu and battles is read from the GPU at half resolution and enlarged in C before the blur: a quarter of the `readPixels` (113 ms to 29 ms on the RG CubeXX, where reading from the GPU is slow); a bit blurrier than the original |
| `transfer-no-snap.js`   | `SceneManager.snapForBackground` | Firework port: going from one map to another takes no blurred picture for the background of the menus (nor the one of MK_SceneGradualOut, unless it is going to fade): they are not used, and on the RG CubeXX they were ~1 s of frozen screen on each map change |
| `tilemap-no-clear.js`   | `Spriteset_Map.prototype.createTilemap` | Firework port: the tilemap does not clear its texture (4 MB of zeros) before uploading each new tileset; it does not show, since it samples with NEAREST and each tile reads only inside its tileset |
| `tone-filter-when-needed.js` | `Spriteset_Base.prototype.updateToneChanger` | preset (no check yet): with tone 0,0,0,0 the ToneFilter of the base layer is taken out (two full-screen passes less per frame) |
| `playthrough.js`         | | not a hook: the playthrough in progress (`$gameSystem._quix`) for the cheats and the achievements |
| `natives.c`, `natives.h` | | the list of native functions (`quix.engine["rpgmaker-mv"]`) |

Measured on an RG CubeXX with Fear & Hunger: `requestImages` was 35% of the
JS time in a map with 60 parallel events, `skipBranch` walked 282,000 lines
every 5 seconds there, and `adjustTone` was 29% of loading a save.

## Adding a hook

1. Find what is slow with the profiler (`--profile`): the report says which
   function takes the time, for example
   `Game_Event.prototype.isNearThePlayer (rpg_objects.js:8590)`.
2. Get the original, as it is, with `tools/rpgmaker_mv_model.py` (the `js/`
   folder of the game and the function). It says which file it comes from
   (if a plugin changes it, the plugin's) and prints the entry for `MODEL`:

   ```
   $ python3 tools/rpgmaker_mv_model.py "mygame/www/js" Game_Event.prototype.isNearThePlayer
   Game_Event.prototype.isNearThePlayer: rpg_objects.js:8590
       'Game_Event.isNearThePlayer': function() {
           var sx = Math.abs(this.deltaXFrom($gamePlayer.x));
           var sy = Math.abs(this.deltaYFrom($gamePlayer.y));
           return sx + sy < 20;
       },
   ```

3. Write the hook in `runtime/engines/rpgmaker-mv/<name>.js`: what it
   replaces, why and what it measured; the references (where the original
   comes from, with a link pinned to a commit when there is one); the use
   line; and the function. It keeps the original when it is not the expected
   one (a plugin changed it):

   ```js
   function (original) {
     var MODEL = {
       'Game_Event.isNearThePlayer': function() {
           ...   // the output of step 2
       }
     };
     var sameCode = quix.engine.sameCode;
     if (!sameCode(original, MODEL['Game_Event.isNearThePlayer'])) return original;
     return function () {
       // the fast version, with the same result
     };
   }
   ```

4. Turn it on: in `hooks.toml` if it is for any MV game, or in the
   `[hooks]` of the game's `port.toml` if only for that game.
5. Measure it on the handhelds against the version without it (the same
   map or battle as in step 1). If it gains nothing measurable, it does not
   stay.
6. Document it: a row in the table of this README, a line in
   [References](#references) if it reproduces code from another project,
   and a line in the CHANGELOG.

## References

Some hooks reproduce or rewrite code from other projects, to give the same
result faster. Each hook has its exact references (with the lines) in its
header. Thanks to their authors.

| hook | code it reproduces or rewrites |
|------|--------------------------------|
| `interpreter-fast.js`, `pure-common-events.js`, `lazy-data.js`, `refresh-if-changed.js`, `refresh-on-net-change.js`, `tone-filter-when-needed.js`, `snap-half.js`, `event-idle-fast.js`, `any-event-starting.js`, `has-skill-fast.js`, `traits-cache.js`, `party-member-fast.js`, `sprite-update-loop.js`, `find-path-index.js`, `tint-native.js` (commit 35c3bde: the one equal to MV 1.6.0) | the core of RPG Maker MV ([corescript](https://github.com/rpgtkoolmv/corescript/tree/182e31449707ba7e406db0485c44c2a9d11e2dcd/js), KADOKAWA, MIT; commit 182e314) |
| `event-idle-fast.js` | YEP_EventChasePlayer v1.04, Yanfly Engine Plugins ([page](https://yanfly.moe/wiki/Event_Chase_Player_(YEP))) |
| `button-events-keys.js` | YEP_ButtonCommonEvents v1.02, Yanfly Engine Plugins ([yanfly.moe](https://yanfly.moe/yep/)) |
| `any-event-starting-quick.js` | YEP_StopMapMovement v1.01, Yanfly Engine Plugins ([yanfly.moe](https://yanfly.moe/yep/)) |
| `yep-enemy-name-refresh.js`, `yep-enemy-select-refresh.js` | YEP_BattleEngineCore v1.39b, Yanfly Engine Plugins ([yanfly.moe](https://yanfly.moe/yep/)) |
| `olivia-meta-fast.js`, `refresh-on-net-change.js` | Olivia_MetaControls v1.6.1, Fallen Angel Olivia ([page](https://fallenangelolivia.itch.io/meta-controls)) |
| `terrax-lights-only.js` | TerraxLighting v1.3.1, Terrax (MIT; no official page found: the copy in the game) |
| `terrax-mask-scale.js` | TerraxLighting v1.3.1, Terrax (MIT; no official page found: the copy in the game) |
| `interpreter-fast.js` | YEP_GabWindow v1.05, Yanfly Engine Plugins ([yanfly.moe](https://yanfly.moe/yep/)) |
| `fluid-timestep-cap.js` | TDDP_FluidTimestep v1.0.2, Tor Damian Design ([page](http://mvplugins.tordamian.com/?p=437)) |
