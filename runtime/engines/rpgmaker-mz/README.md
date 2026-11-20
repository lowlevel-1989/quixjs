# rpgmaker-mz

English | [Español](README.es.md)

Hooks for RPG Maker MZ (1.8): what an MZ game needs to start in quixjs, and
what the profiler (`--profile`) found on a handheld (tested with Look
Outside, MZ 1.8.1). Each one checks that the function is the one of RPG
Maker MZ (or of the PixiJS v5 it ships) with `__quixEngine.sameCode`; if
not, it keeps the original.

`[hooks] engine = "rpgmaker-mz"` installs the hooks of `hooks.toml`.

| file                     | replaces | what it does |
|--------------------------|----------|--------------|
| `effekseer-no-wasm.js`   | `effekseer.initRuntime` | preset: without WebAssembly (quixjs has none) Effekseer did not start nor report it and the game stayed black; now it goes on without Effekseer: Effekseer animations are not shown, the others are |
| `color-filter-identity-off.js` | `ColorFilter.prototype.initialize` | preset: a neutral ColorFilter (hue 0, tone 0,0,0,0, blend with alpha 0, brightness 255) has `enabled` false and PIXI does not draw it: on the map there are three (the scene's and the two of Spriteset_Base), each one a separate texture and a full screen pass per frame. It is checked when drawing: with a fade, a flash or a tone, it is back |
| `menu-blur-once.js`      | `Scene_MenuBase.prototype.createBackground` | preset: the blurred background of the menus (BlurFilter, 8 full screen passes per frame) is drawn once into a Bitmap, with the same filter; a submenu with the same snapshot reuses it |
| `no-vao-bind.js`         | `PIXI.systems.GeometrySystem.prototype.bind` | preset: without OES_vertex_array_object (the WebGL of quixjs has none) PixiJS v5 went through `initGeometryVao` on every bind (signature, strides, a `join`); now only the first time of each geometry with each program. The same WebGL calls |
| `touch-ui-off.js`        | `ConfigManager.readFlag`, `ConfigManager.save` | preset: without a touch screen (`navigator.maxTouchPoints` 0), the Touch UI option starts off (no buttons on screen); the one of a `config.rmmzsave` from the PC does not count (Look Outside ships one with `touchUI` true), the one the player saves in Options does |
| `decrypt-image-native.js` | `Bitmap.prototype._startDecrypting` | preset: quixjs reads, decrypts and decodes the encrypted images (`.png_`) in the background, with the decrypter shared with RPG Maker MV (`rpgmaker/decrypt.c`: the format is the same, a 16 byte header and XOR with `encryptionKey`); no XHR, `slice` or Blob in the JS. The image is loaded by its path (`img/enemies/...`), so `[game] image_scale` sees it |
| `battle-images-release.js` | `SceneManager.onSceneTerminate` | preset: MZ only empties the image cache on a map transfer, so every battle added its enemies, backgrounds and faces until the next transfer; now the images that entered the cache during a battle are destroyed when it ends (as `ImageManager.clear` does on a transfer), and the map's stay |
| `rpgmaker/gc-on-transition.js` | `Scene_Map.prototype.create`, `Scene_Battle.prototype.create` | preset, shared by MV and MZ (`runtime/engines/rpgmaker`): a full GC when the map or the battle is created, with the screen black, that frees what the previous scene left |

The rest MZ needs is in the nw.js mode (`[engine] nwjs = true`,
`runtime/binding/nwjs.js`): `location` is `file:` as in nw.js (MZ then
loads audio with XHR, no `fetch`) and there is an empty `indexedDB` (MZ asks
for it at start, although under nw.js it saves with `fs`).

The tests are `games/tests/core/hooks-mz` (the hooks over the real code of
RPG Maker MZ and PixiJS, with what surrounds it mocked) and
`games/tests/core/native-loading-mz` (a `.png_` image loaded with the hook
and with the original: the same pixels).

## Measured

On the Powkiddy RGB30 (RK3566, libmali, 30 fps, performance governor), Look
Outside: title, new game, walking in the bedroom and the menu open, twice
with the drawing hooks off (`"off"`) and twice with them; the JS of each
frame (`--fps`):

| drawing hooks | map walking, JS ms | menu, JS ms (fps) |
|---------------|--------------------|-------------------|
| all off       | 19.0 | 20.3 (28.7) |
| all           | 15.0 | 13.0 (30) |
| all but `color-filter-identity-off` | 18.1 | 14.6 (30) |
| all but `no-vao-bind` | 16.3 | 14.6 (30) |
| all but `menu-blur-once` | 15.0 | 18.2 (30) |

Each one saves on its own: the ColorFilters ~3 ms on the map, the blur ~5
ms in the menu, the bind without VAO ~1.3 ms on the map and ~1.5 ms in the
menu. RAM is the same (RSS peak ~430 MB in both).

What is seen is the same: with the game paused the scene was drawn with and
without the hook and the pixels compared. The map without the neutral
ColorFilters: identical. The menu background blurred once against the
BlurFilter of every frame: 5% of the bytes change by 1 or 2 (3 of 255 at
most), the rounding of going through a Bitmap.

`decrypt-image-native`, in four battles in a row (troops 189, 441, 465 and
200, the ones with the biggest enemies) from the bedroom, two runs with and
two without: the RSS peak goes from ~578 to ~571 MB; the load time of each
battle (1.7 to 2.3 s) and the JS per frame are the same. What it gives is
that the image is loaded by its path: `[game] image_scale` sees it. With
`image_scale = "auto"` (88% on 720x720) only on `img/enemies`, 10 images
are reduced in those battles and the RSS peak drops ~3 MB more (within the
noise), and the reduced pixel art shows (doubled pixel columns): Look
Outside does not use it.

RAM in battle, the same four battles, two runs per case (RSS of the
process, MB):

| | RSS peak | MemAvailable min | RSS in battles 189/441/465/200 | load of each battle |
|---|---|---|---|---|
| without both | 560 / 575 | 141 / 145 MB | 460 / 476 / 496 / 551 | 2.4 / 1.8 / 1.9 / 1.8 s |
| `battle-images-release` | 545 / 546 | 134 / 145 MB | 458 / 463 / 483 / 516 | 2.4 / 2.2 / 2.2 / 2.3 s |
| both | 529 / 531 | 172 / 173 MB | 435 / 432 / 442 / 515 | 2.5 / 2.4 / 2.4 / 2.4 s |

Without the hooks the cache went from 34 to 87 MB over the four battles and
did not go down back on the map; with them it is back to 34 MB after each
one. The last battle (200: its backgrounds and faces) is still the biggest.
In exchange each battle takes ~0.5 s more to load (it reads its backgrounds
and faces again, and the GC), with the screen black; the JS per frame and
what is seen are the same. `[engine] low_memory_mb = 160` changed nothing:
with the hooks free RAM did not go below 160 MB.

## Credits

| hooks | code from |
|-------|-----------|
| `color-filter-identity-off.js`, `menu-blur-once.js`, `touch-ui-off.js`, `decrypt-image-native.js`, `battle-images-release.js` | RPG Maker MZ 1.8.1 (`rmmz_core.js`, `rmmz_scenes.js`, `rmmz_managers.js`; KADOKAWA, MIT): the `MODEL`s are its code, as is |
| `no-vao-bind.js`, `color-filter-identity-off.js` | [PixiJS](https://github.com/pixijs/pixijs/blob/9e2bab08bad6b0e8dddac05d65fd290eca7193b1/packages/core/src/geometry/GeometrySystem.ts#L161) v5.3.12 (MIT; commit 9e2bab0): `GeometrySystem.bind` and `initGeometryVao` in the `MODEL`; `Container.renderAdvanced` skips the filters with `enabled` false |
| `effekseer-no-wasm.js` | [Effekseer for WebGL](https://github.com/effekseer/EffekseerForWebGL/blob/7f19e5d4ce6194de12711fcea6db89e2a6957815/src/js/effekseer.src.js#L1018) 1.70b (MIT; commit 7f19e5d) |
