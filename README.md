# QuixJS

>  🚧 **Experimental Concept**  
>
> QuixJS is currently in **early development** and should be considered an **experimental prototype**. The core idea is being explored and subject to major changes. It is **not yet ready for production use**.

---

**QuixJS** is a lightweight embedded runtime for executing JavaScript-based games (built with Vanilla JS, PixiJS, Phaser, and others) on handheld devices without requiring X11. It uses **QuickJS** as the scripting engine and **SDL2** for rendering, input, and audio—featuring native C reimplementations of critical parts of popular JS game engines for optimal performance.

> ⚠️ This project is under active development. The goal is to deliver smooth 2D game experiences on low-powered devices with minimal overhead and no reliance on traditional graphical environments.

---

## Execution Flow

QuixJS is configured through a `config.toml` file, which defines the game directory, resolution, and main script to execute.

Example `config.toml`:

```toml
[game]
cache_dir = "cache/hello"
game_dir  = "games/hello"
width     = 640
height    = 480

[scripts]
entry = "main.js"
libs  = ["libs/pixi.js"]
```

At runtime, QuixJS follows this process:

1. **Cache check**: Determines if the JS code has already been compiled to bytecode and cached in `cache_dir`.
2. **Compilation**: If not cached, the JS files are compiled to QuickJS bytecode for performance.
3. **Execution**: Runs the compiled bytecode within a runtime environment that includes native C bindings and engine-specific reimplementations.
4. **Rendering & input**: Uses SDL2 for 2D rendering, input handling, and audio output—eliminating the need for a browser or heavyweight APIs.

---

## 🔍 Key Features

- ⚡ Modern, lightweight JavaScript runtime powered by QuickJS
- 🧩 Supports games written in Vanilla JS, PixiJS, Phaser, and other 2D engines
- 🔧 Native C reimplementations of core game engine features (e.g., parts of PixiJS)
- 🎨 Native 2D rendering via SDL2—**no X11 required**
- 🎮 Built-in support for input handling and audio on handheld devices
- 🔄 Smart caching system to avoid unnecessary recompilation

---

## 📁 Typical Game Structure

```bash
games/hello/
├── libs/             # External JS libraries (PixiJS, Phaser, etc.)
│   └── pixi.js
├── main.js           # Main entry point for the game
├── assets/           # Game assets (images, sounds, etc.)
├── config.toml       # Game-specific configuration file
└── README.md
```

---

## 🚀 Usage & Options

```bash
Usage: quixjs [--config <path>]
Options:
  -c, --config <path>  Path to the config.toml file (default: config.toml)
  -f, --force          Force recompilation, ignoring cached bytecode
  -b, --backend <type> Select backend: sdl or dummy (default: sdl)
  -h, --help           Show this help message
```

Example:

```bash
./quixjs --config games/hello/config.toml --force
```
