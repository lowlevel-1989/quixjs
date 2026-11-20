# Tests

English | [Español](tests.es.md)

The tests are games: each folder in `games/tests` has a `config.toml` and a
`main.js`, and most of them an `index.html`. They run without a window (dummy
backend) and with a fixed clock, so they always give the same result and take a few
seconds. WebGL works without a window too (an offscreen GL context).

```
make
make test                    # all of them
tests/run.sh pixi_v4         # only those with "pixi_v4" in the path
VERBOSE=1 tests/run.sh core  # with the full output of each one
tests/transfer.sh            # Transfer Save between two quixjs processes
```

```
ok    audio/webaudio
ok    bench/canvas-lights
...
46 passed, 0 failed
```

The output of each test is saved in `cache/tests-logs`. GitHub runs `make test` on
every push (`.github/workflows/tests.yml`), with libvpx and libopus installed so
`media/video` runs with video support.

## tests/run.sh

| variable  | what it does |
|-----------|--------------|
| `BACKEND` | `sdl`: each test in a window, with the real clock (and `--fps`) |
| `HOLD`    | with a window: milliseconds it stays open at the end (`--hold`) |
| `FPS`     | `1`: shows the frames per second of each test |
| `LOGDIR`  | folder of the logs (default `cache/tests-logs`) |
| `VERBOSE` | `1`: the full output of each test |
| `QUIXJS`  | the binary (default `./quixjs`) |

The first argument is a filter: only the tests with that text in their path run.
A test can have an `args` file with its own arguments for quixjs (`core/gamepad-frame`
has `-b sdl`); with `-b sdl` and without `BACKEND` it runs without a window
(`SDL_VIDEODRIVER=offscreen`).

```
BACKEND=sdl HOLD=2000 tests/run.sh pixi_v4_webgl
```

## tests/transfer.sh

It starts two quixjs processes on this computer, without a window and with `--launcher`:
one in Receive Save and the other in Send Save, both driven with `--script`. With
`QUIXJS_TRANSFER_HOSTS=127.0.0.1` they only talk through that address. It checks
that the saves arrive intact, that a file that is not a save does not travel, and
that the old save goes to the backup. It also checks that the achievements file
travels with the saves (the old one goes to the backup too) and that the receiver
reads it again. It is not part of `make test` (it needs UDP
on 127.0.0.1). The protocol: [runtime/transfer](../runtime/transfer/README.md).

## What they test

| folder           | what it tests |
|------------------|---------------|
| `core/timers`    | setTimeout (order, arguments, clear), setInterval, requestAnimationFrame, fixed clock |
| `core/events`    | addEventListener, once, handleEvent, on<type>, Event, KeyboardEvent, keyboard with `quix.pressKey` |
| `core/dom`       | window, document, createElement, appendChild, getElementById (also with `el.id = ...`), instanceof, localStorage, gamepads |
| `core/gamepad-frame` | with the sdl backend (`args`) and a virtual SDL gamepad (`quix.pressPad`): the gamepads are read once per frame and every read of the frame sees the same; a press shorter than a frame is seen in that frame and not in the next |
| `core/lan` | `quix.lan` with one device: open, its id and name, nobody else, send to nobody, close |
| `core/vibration` | `navigator.vibrate` (ms and patterns), `gamepad.vibrationActuator` (`playEffect` resolves `complete`, or `preempted` when cut; `reset`) and the presets of `lib/rumble` |
| `core/pad-keys` | `[controls.keys]` with the buttons of `controller.keys`: the button presses its key (keydown and keyup with `code` and `keyCode`); a button without an action gives none |
| `core/nwjs-fs-text` | nw.js `fs`: a text written and read back with `utf8` stays whole, also with zeros inside (the saves of RPG Maker MZ) |
| `core/xhr`       | XMLHttpRequest: json, text, arraybuffer, 404, synchronous |
| `core/mods`      | `[game] mods`: a file of an active mod replaces the one of the game; with two, the last one wins; an inactive mod is not used; the hooks of an active mod go on top of the hook of the game; `navigator.vibrate` |
| `core/scripts`   | `<script>` added with appendChild (as RPG Maker loads its plugins), DOMContentLoaded, window.onload |
| `core/legacy`    | old JS that many plugins use: `RegExp.$1..$9`, `lastMatch`... |
| `core/engine-hooks` | the engine hooks of `runtime/engines/rpgmaker-mv`: each one gives the same result as the RPG Maker MV function it replaces |
| `core/hooks-late-plugin` | a plugin loaded late replaces a function with a hook: quixjs puts the hook back on top, and the hook does not call itself |
| `core/achievements` | `[symbols]` and `[[achievements]]`: conditions, `prev()`, symbols not created yet, `quix.achievementUnlocked` |
| `core/cheats`    | a cheat that is on: it sets its symbol (again if the game changes it), the playthrough is marked and gets no achievements |
| `core/cheats-free` | a game without achievements: the cheat applies with no known engine, once its object exists, and again when the game changes it |
| `core/cheats-off` | no cheats on: a marked save gets no achievements and its symbols go back to their value; a clean one gets them |
| `core/achievements-menu` | the hook `rpgmaker-mv/achievements-menu`: the Achievements option in the menu and its list |
| `core/hooks-engine` | `[hooks] engine = "rpgmaker-mv"`: the engine hooks without listing them; the game's own (`"off"` or another) win |
| `core/hooks-mz` | `[hooks] engine = "rpgmaker-mz"`: the RPG Maker MZ hooks over its real code (neutral ColorFilter, the menu blur once, Touch UI, `GeometrySystem.bind` without VAO with the same WebGL calls) |
| `core/native-loading-mz` | `rpgmaker-mz/decrypt-image-native`: an encrypted RPG Maker MZ image (`.png_`) read and decrypted in C, by its path; the same pixels as with the original (XHR and Blob) |
| `core/native-loading` | `decrypt-image-native` and `load-audio-native`: encrypted `.rpgmvp` and `.rpgmvo` read and decrypted in the background |
| `core/pure-common-events` | `pure-common-events`: a pure parallel process (common or map event) is not run again if its last run changed nothing and what it reads stays the same; a run that changes something (another branch, adding to a variable) is always done |
| `core/refresh-partial` | `refresh-on-net-change`: with real changes only the events that depend on them are refreshed; the pages, the same as with a full refresh (500 random changes) |
| `core/tint-native` | `tint-native`: the same pixels as `Sprite.prototype._executeTint` |
| `core/render-scale` | `render-scale`: with `render_scale = 0.5` PIXI draws at half the resolution (canvas 100x50) and the drawing lands in place |
| `core/save-picture-small` | `save-picture-small`: the save picture at a quarter of the size, with the colors of the scene in place |
| `core/yep-enemy-name` | `yep-enemy-name-refresh`: the enemy name is redrawn only when it changes |
| `canvas/style`   | fillStyle and the other properties: parsing, serialization, invalid values, save/restore |
| `canvas/rect`    | fillRect, clearRect, strokeRect, globalAlpha, translate, scale, rotate, setTransform |
| `canvas/path`    | arc, polygons, evenodd, curves, stroke, isPointInPath, clip |
| `canvas/image`   | Image (load, error, data URI), drawImage, ImageData, createPattern, gradients, composite |
| `canvas/text`    | measureText, fillText, textAlign, textBaseline, strokeText, fonts |
| `canvas/gpu`     | the hooks `gpu-canvas`, `once-per-frame` and `skip`: a light mask drawn on the GPU and by software, the copy to a WebGL texture, the way back to the CPU, resize, a repaint equal to the previous one |
| `webgl/basic`    | WebGL 1 without libraries: context, shaders, buffers, textures, framebuffers, readPixels, queries |
| `audio/webaudio` | WebAudio: decodeAudioData (ogg vorbis), sources, gain, pan, volume ramps, loops, onended |
| `media/video`    | `<video>`: canPlayType, a WebM (`clip.webm`, made with ffmpeg from `testsrc` and a `sine` tone) and an MP4 with AAC audio and B frames (`clip.mp4`) played to the end, a 1080p MP4 (`big.mp4`) at half size, `removeAttribute('src')` closes it, a missing file skipped; without video support it checks that it is skipped |
| `pixi_v4/boot`   | Application, CanvasRenderer, ticker, backgroundColor |
| `pixi_v4/sprite` | textures, position, scale, anchor, rotation, alpha, visible |
| `pixi_v4/container` | children, pivot, rotation, bounds |
| `pixi_v4/graphics` | rect, circle, polygon, line, fill alpha |
| `pixi_v4/tint`   | tint (CanvasTinter with multiply), blend mode ADD |
| `pixi_v4/tiling` | TilingSprite, tilePosition, RenderTexture |
| `pixi_v4/loader` | PIXI.loader with json and images |
| `pixi_v4/text`   | PIXI.Text |
| `pixi_v4/keyboard` | moving a sprite with the arrows |
| `pixi_v4_webgl/*` | the same nine `pixi_v4` tests with the WebGL renderer (`lib/webgl.js`) |
| `bench/canvas-lights` | benchmark: a light mask like the lighting plugins of RPG Maker MV (radial gradients in `lighter` mode); prints ms per mask |

The images, sounds and files they use are in `games/tests/assets` (made for the
tests: `quad.png` is 4x4 with a red, a green, a blue and a white block;
`quad.rpgmvp` and `quad.png_` are that PNG encrypted as in RPG Maker MV and
MZ, with the key of `core/native-loading`). PixiJS is
`games/tests/lib/pixi_v4/pixi.js`.

## Write a test

A test uses `games/tests/lib/assert.js` and ends with `T.done()`:

```js
var canvas = document.createElement('canvas');
var ctx = canvas.getContext('2d');

T.test('fillRect', function () {
  ctx.fillStyle = 'red';
  ctx.fillRect(0, 0, 10, 10);
  T.pixel(ctx, 5, 5, [255, 0, 0, 255], 0, 'red inside');
});

T.done();
```

| function                          | what it checks |
|-----------------------------------|----------------|
| `T.test(name, fn)`                | groups checks; an exception inside is a failure |
| `T.ok(cond, msg)`                 | cond is true |
| `T.equal(got, expected, msg)`     | `===` |
| `T.deepEqual(got, expected, msg)` | same JSON |
| `T.near(got, expected, eps, msg)` | difference <= eps |
| `T.throws(fn, msg)`               | fn throws an exception |
| `T.pixel(ctx, x, y, rgba, tol, msg)` | color of a pixel, with a tolerance per channel |
| `T.waitFor(cond, fn, frames, msg)` | waits (frame by frame) until cond() is true |
| `T.done()`                        | prints the result and calls `quix.exit(0 or 1)` |

The `config.toml` of a test:

```toml
[game]
cache_dir = "cache/tests/canvas/rect"
game_dir  = "games/tests/canvas/rect"
width     = 320
height    = 240

[scripts]
libs     = []                       # ["../../lib/pixi_v4/pixi.js"] for PIXI
includes = ["../../lib/assert.js"]
entry    = "main.js"
```

The same test can be opened in a browser (from a local server, for example
`python3 -m http.server` in the repository folder) to compare: the result is
printed in the console. Checks that only make sense in quixjs go inside
`if (typeof quix !== 'undefined')`.

## Exit code

`tests/run.sh` only looks at the exit code of quixjs: `0` passed, `1` a check failed
or there was an exception, `3` the test did not call `T.done()` within 600 frames.

## --script

An input script drives a game (or the menu of `--launcher`) without touching it:
one action per line, at a given frame, with `#` comments.

```
120  press Enter        keydown in this frame, keyup in the next one
200  down  ArrowDown    hold a key
230  up    ArrowDown    release it
300  shot  title.png    screenshot of this frame (PNG)
400  log   some text    writes the text in the output
900  exit  0            ends with that code
```

The key names are `KeyboardEvent.code` (`Enter`, `KeyZ`, `ArrowDown`...) and they
arrive as keys of the real keyboard. With `--launcher` the frames count from the
first frame of the menu, and the ones of the game come after them.

```
./quixjs -c mygame.toml --script play.keys
```

## Debug console

Inherited from tamatama (https://github.com/lowlevel-1989/tamatama). With
`--debug` quixjs listens on a Unix socket (`quixjs.sock` in the current folder, or
`--debug=PATH`) and runs text commands between frames, when no JS is running.
`tools/console.py` is the client: interactive, or one command per call.

```
Terminal 1:  ./quixjs --debug -c mygame.toml
Terminal 2:  python3 tools/console.py
             python3 tools/console.py press Enter
             python3 tools/console.py eval "SceneManager._scene.constructor.name"
```

| command              | what it does |
|----------------------|--------------|
| `help`               | the list of commands |
| `info`               | frame, clock, backend, paused |
| `press KEY`          | press and release a key (`KeyboardEvent.code`) |
| `down KEY` / `up KEY` | hold / release a key |
| `shot FILE.png`      | screenshot of what is on screen |
| `eval CODE`          | run JavaScript in the game and print the result (JSON when possible) |
| `pause`              | stop the game (the window keeps responding) |
| `resume`             | continue |
| `run N`              | advance N frames and pause |
| `ach`                | the achievements of the port and the values of their symbols |
| `ach reset`          | forget the unlocked achievements |
| `fps [N]`            | the frame rate cap; `fps N` changes it now (10 to 60) |
| `prof start` / `prof stop` | sampling profiler: where the time goes (`prof [N]` shows the top N) |
| `prof save [FILE]`   | write the report to FILE (`quixjs-profile.txt`) |
| `gc`                 | run the garbage collector: how long it takes and what it frees |
| `mem`                | size of the JS heap and number of objects |
| `mem release`        | free the CPU pixels of images already on the GPU, as quixjs does with low RAM |
| `types`              | JS objects by type; twice in a row, what was created in between (the garbage the game makes) |
| `pacing`             | intervals between frames really presented, since the last `pacing` |
| `quit`               | exit quixjs |

Each answer ends with a line `ok` or `err MESSAGE`. More about the client:
[tools/README.md](../tools/README.md).
