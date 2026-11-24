# Finding where a game saves

English | [Español](trace-saves.es.md)

When the player uses a cheat, the playthrough is marked and gives no more
achievements, even if it is saved and loaded another day (see
[the cheats](../runtime/game/cheats/README.md)). For that quixjs has to follow the
playthrough: know when a new one starts, put the mark inside the save of the
game and get it back when loading. quixjs does not know by itself how each game
does that; three hooks that call `quix.playthrough` tell it:

| hook | when it is called | what it does with the mark |
|------|-------------------|----------------------------|
| start | the player starts a new playthrough (New Game, a new run) | `quix.playthrough.start()`: a clean playthrough, no mark |
| save | the game builds what it is going to write in the save | adds what `quix.playthrough.save()` returns (the signed mark) to what is saved |
| load | the game has just read a save | takes from it what save put and gives it to `quix.playthrough.load(data)` |

Example in RPG Maker: when saving, the save object has
`contents.quix = { data: '{"cheats":true}', sig: "…" }`; when loading, quixjs
checks the signature and the playthrough is still marked. If it is missing or
the signature does not match, it counts as a playthrough with cheats. Without
any hook, cheats are not applied.

RPG Maker MV and MZ have them in their engine hooks, and Construct 3 has save
and load. For another engine, or to start a playthrough in Construct 3, you
have to find which function of the game each one goes on. `--trace-saves` is
for that.

## What --trace-saves does

```
./build/quixjs -c games/mygame/port.toml --trace-saves > trace.log
```

It writes in the log:

- **watching**: at start, the functions of the game whose name looks like a
  playthrough one (`save`, `loadFrom`, `newGame`, `continue`, `toJson`...).
  They are the candidates for a hook.
- **read / write**: each time the game reads or writes a save (a file of
  `[saves] files`, the `localStorage` or IndexedDB), with:
  - `inside`: the candidates still running at that moment, from the inside
    out: the chain that asked for the read or write (if it is asynchronous,
    until its promise is done).
  - `code of`: the beginning of the code of the innermost one of that chain,
    to see what it calls.
  - `called just before`: the candidates called and finished in the 3 seconds
    before, from the most recent to the oldest. The ones called many times
    are only counted.
  - below, the JavaScript stack: which code asked for it (file, line and
    column). If the game saves asynchronously, the stack no longer reaches
    the game; that is what `inside` and `called just before` are for.
- **screen**: each screen change, with the function and the name of the
  screen (`SceneManager.goto` in RPG Maker, `_StartRunning` in Construct 3,
  `doChangeLayout` in Construct 2, `SceneStack.push` in GDevelop). It is for
  the start hook.
- **quix.playthrough**: each `start`, `save` and `load`, to check that the
  hooks are in the right place.

Each combination of operation and stack is written once. Without
`--trace-saves` nothing is wrapped and it costs nothing.

## How to use it

1. Open the game with `--trace-saves` and, in another terminal, watch the log:
   ```
   tail -f trace.log | grep -A8 "trace-saves\] \(read\|write\)"
   ```
2. Start a new playthrough, save, go back to the title and load.
3. For each read or write of the save, look at `inside` and `code of`: the
   innermost function of the chain and what it calls. The hook goes on the
   function that builds (save) or gets (load) the data of the playthrough,
   not on the one that only asks for it.
4. Put the hooks in `[hooks]` of the `port.toml` (files in `hooks/` of the
   port) and try again: the `quix.playthrough` lines have to show up.

## Example: Million Monster Militia (Construct 3)

A port without any playthrough hook, opened with `--trace-saves`.

### 1. Where it saves

In the game, Continue run is pressed and this shows up:

```
[trace-saves] read IndexedDB c3-savegames-lg3lvcac89t/keyvaluepairs "mysave"
    inside: C3.Runtime.prototype._DoLoadFromSlot
    inside: C3.Runtime.prototype._HandleSaveOrLoad
    code of C3.Runtime.prototype._DoLoadFromSlot: async _DoLoadFromSlot(slotName){try{const loadJson=await this._GetSavegamesStorage().getItem(slotName);if(!loadJson)throw new Error("empty slot");console.log("[Construct] Loaded state from storage ("+loadJson.length+" chars)");await this._DoLoadFromJsonString(loadJson);...
    called just before: C3.Runtime.prototype._GetSavegamesStorage
    called just before: C3.Runtime.prototype.LoadFromSlot
```

- `read IndexedDB c3-savegames…/"mysave"`: the save is in IndexedDB, in the
  slot `"mysave"` (the name was chosen by the author in the events).
- `inside`: the functions still running while it is read, from the inside
  out. `_HandleSaveOrLoad` called `_DoLoadFromSlot`, and that one waits for
  the read. It is the chain that asked for the save.
- `called just before`: the ones that were called and already finished.
  `LoadFromSlot` is the "Load game" action of the events.

### 2. Where the load hook goes

The `code of` line is the code of the innermost function of the chain,
`_DoLoadFromSlot`. quixjs takes it from the function itself with
`Function.prototype.toString` (JavaScript gives the text of any function),
with the spaces joined and cut at 300 characters.

Only the innermost one is shown: the last one called that has not finished
yet. When the save is read, it is the one waiting for that read, so its code
shows what happens right before (the read) and right after (where the data
goes, which is where the hook goes). The outer ones, like
`_HandleSaveOrLoad`, only call the next one and say nothing new; the ones in
`called just before` already finished and are not part of this read.

In `_DoLoadFromSlot`:

```js
const loadJson = await this._GetSavegamesStorage().getItem(slotName);  // reads the text
...
await this._DoLoadFromJsonString(loadJson);                            // turns it into the playthrough
```

The load hook goes on the function that gets the data already read:
`C3.Runtime.prototype._DoLoadFromJsonString`. There the hook takes the mark out
of the text and gives it to `quix.playthrough.load()`.

### 3. Where the save hook goes

Saving in the game shows the same the other way round:

```
[trace-saves] write IndexedDB c3-savegames-lg3lvcac89t/keyvaluepairs "mysave"
    inside: C3.Runtime.prototype._DoSaveToSlot
    inside: C3.Runtime.prototype._HandleSaveOrLoad
    code of C3.Runtime.prototype._DoSaveToSlot: async _DoSaveToSlot(slotName){const saveJson=await this._SaveToJsonString();try{await this._GetSavegamesStorage().setItem(slotName,saveJson);...
    called just before: C3.Runtime.prototype._GetSavegamesStorage
    called just before: C3.TimelineManager.prototype._SaveTimelines
    (and 13 functions called many times, left out)
```

```js
const saveJson = await this._SaveToJsonString();                 // builds the text
await this._GetSavegamesStorage().setItem(slotName, saveJson);   // writes it
```

The save hook goes on the function that builds the text:
`C3.Runtime.prototype._SaveToJsonString`. There the hook adds the mark before
it is written. The functions "called many times" are the `_SaveToJson` of each
object of the game: work inside, not the chain.

Since `_SaveToJsonString` and `_DoLoadFromJsonString` belong to the Construct 3
runtime and not to the game, these two hooks work for any game of the engine
and are in `runtime/engines/construct3/hooks/playthrough-save.js` and
`playthrough-load.js`.

### 4. Where the start hook goes

Starting a playthrough almost never reads or writes anything, but it does open
a screen. The trace writes each screen change with its name (the `screen`
lines). In the game a new run is chosen and this shows up:

```
[trace-saves] screen: C3.Runtime.prototype._DoChangeLayout "NewRunSelectTree"
[trace-saves] screen: C3.Layout.prototype._StartRunning "NewRunSelectTree"
```

- `"NewRunSelectTree"` is the screen where a new run starts (with Custom game
  it is `"CustomGame"`). Continue run opens another screen, because it loads
  the save, and the load hook already covers that.
- `C3.Layout.prototype._StartRunning` is the function that starts each
  screen; inside it, `this` is the screen and `this.GetName()` its name.

The start hook goes on that function and only calls `quix.playthrough.start()`
when the screen is one of those two. The screen names belong to this game, so
the hook goes in the port:

```toml
[hooks]
engine = "construct3"
"C3.Layout.prototype._StartRunning" = "hooks/playthrough-new"
```

```js
// games/millionmonstermilitia/hooks/playthrough-new.js
function(original) {
  return function () {
    var name = this.GetName();
    if ((name === 'NewRunSelectTree' || name === 'CustomGame') && window.quix && quix.playthrough)
      quix.playthrough.start();
    return original.apply(this, arguments);
  };
}
```

### 5. Checking it

With the three hooks in place, the trace writes each call:

```
[trace-saves] quix.playthrough.start(): a clean playthrough
[trace-saves] quix.playthrough.save(): a clean playthrough
[trace-saves] quix.playthrough.load(): signed data: a clean playthrough
```

## Example: Fear & Hunger (RPG Maker MV)

When saving in slot 1:

```
[trace-saves] write .../gamedata/save/file1.rpgsave
    called just before: StorageManager.saveToLocalFile
    called just before: StorageManager.save
    called just before: DataManager.makeSaveContents
    called just before: DataManager.saveGameWithoutRescue
    called just before: DataManager.saveGame
```

Here the stack has names: RPG Maker is not minified. `makeSaveContents` builds
the object that is saved (the save hook puts `contents.quix` there) and
`extractSaveContents` is its pair when loading. A new playthrough goes through
`DataManager.setupNewGame`, which calls `createGameObjects`. Since it is the
same in every RPG Maker game, the three hooks are in the engine
(`runtime/engines/rpgmaker/hooks/playthrough-*.js`) and a port has nothing to
do.

## Games made of runs: only start

In a roguelite the playthrough is a run, and many do not save a run halfway:
what they write is the overall progress (unlocks, counters). There the
playthrough starts and ends in the same session, so the start hook is enough;
save and load are not needed. The trace shows it: the writes have nothing to
do with the run (`Save/GameOverCount.txt`, `Save0.save` with the unlocks) and
the `screen` lines show when one starts.

- Gunfield (Construct 2): a run starts when going from the `Base` screen to
  `Game`; `Game` restarts on each level, so the hook checks where it comes
  from (`games/gunfield/hooks/playthrough-new.js`, on
  `cr.runtime.prototype.doChangeLayout`).
- Super Smiling Dungeon (GDevelop): a run starts when going down to
  `CARD-FLOOR1-CLIC` from the hero choice or from `GAME OVER`; the rooms go
  back to that scene within the same run
  (`games/supersmilingdungeon/hooks/playthrough-new.js`, on
  `gdjs.SceneStack.prototype.push` and `replace`).

## Games without an engine: the profiler

A game with its own code (no RPG Maker, Construct or GDevelop) may have no
functions with save or screen names, and then the trace has no candidates.
The profiler helps there ([benchmark](benchmark.md#--profile)): it measures
which functions run at a moment of the game, whatever their names.

```
./build/quixjs -c games/mygame/port.toml --debug --trace-saves
python3 tools/console.py      # in another terminal
> prof start                  # right before pressing New Game
> prof stop                   # once it is playing
> prof save start.txt
```

In `start.txt`, the `self / total` section lists the JS functions that ran in
that time with their file and line; the ones of the game that do not show up
in the rest of the playthrough (`newGame`, `initLevel`, `reset`...) are the
candidates for the start hook. The same goes for saving and loading:
`prof start` before saving, and the function that writes shows up next to the
`write` line of the trace.

## What it does not find

- A game that saves from its events (GDevelop, Construct) without a named
  function: the stack points to the code of the events (`code1.js:386`), which
  has to be read.
- Starting a playthrough in a game that does not change screens when it
  starts: it has to be found with the profiler.
- Saves in `localStorage` with a name that is not recognized: they show up in
  the log, but so can settings of the game (the window, the volume).
