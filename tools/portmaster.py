#!/usr/bin/env python3
"""tools/portmaster.py: los archivos que pide PortMaster, desde el port.toml
de un port (games/<port>/port.toml): [game] (lo comun) y [portmaster] (lo
de PortMaster). Lo usa make PLATFORM=portmaster.

  python3 tools/portmaster.py games/fearandhunger/port.toml <carpeta>

Escribe en <carpeta>:
  <script>             el launcher, desde portmaster/portmaster.sh ([portmaster] script)
  <port>/port.json     lo que lee PortMaster para listar e instalar el port
  <port>/gameinfo.xml  lo que lee EmulationStation para enseñarlo
  <port>/port.toml     el mismo port.toml: el launcher ejecuta quixjs -c port.toml

PortMaster pide JSON y XML; en el repositorio solo está el .toml.
"""
import json
import os
import shutil
import sys
import tomllib
from xml.sax.saxutils import escape


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    toml_path, out = sys.argv[1], sys.argv[2]
    port = os.path.basename(os.path.dirname(os.path.abspath(toml_path)))
    with open(toml_path, "rb") as f:
        data = tomllib.load(f)
    if "portmaster" not in data:
        sys.exit(toml_path + ": no [portmaster] section")
    # [portmaster] manda sobre [game] en lo que tengan los dos
    p = dict(data.get("game", {}))
    p.update(data["portmaster"])
    title, script = p["title"], p["script"]
    os.makedirs(os.path.join(out, port), exist_ok=True)

    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    with open(os.path.join(root, "portmaster", "portmaster.sh")) as f:
        game_dir = data.get("game", {}).get("game_dir", "gamedata")
        sh = (f.read().replace("@PORT@", port).replace("@TITLE@", title)
              .replace("@PORT_VERSION@", p.get("version", "?"))
              .replace("@GAME_DIR_PARENT@", os.path.dirname(game_dir) or game_dir)
              # MV: la carpeta www/; MZ (sin www/): lo de la carpeta del juego
              .replace("@GAME_WHAT@", "the folder " + os.path.basename(game_dir) + "/"
                       if os.path.dirname(game_dir) else "the contents of the game folder (the one with index.html)")
              .replace("@GAME_DIR@", game_dir))
    with open(os.path.join(out, script), "w") as f:
        f.write(sh)
    os.chmod(os.path.join(out, script), 0o755)

    # la captura del juego (screenshot.jpg o .png junto al port.toml): la
    # imagen del port en el catalogo de PortMaster y en EmulationStation
    shot = None
    for name in ("screenshot.jpg", "screenshot.png"):
        if os.path.exists(os.path.join(os.path.dirname(toml_path), name)):
            shot = name
            shutil.copy(os.path.join(os.path.dirname(toml_path), name), os.path.join(out, port, name))
            break

    # la portada (cover.png o .jpg junto al port.toml): la de PortMaster
    covers = []
    for name in ("cover.png", "cover.jpg"):
        if os.path.exists(os.path.join(os.path.dirname(toml_path), name)):
            covers.append(name)
            shutil.copy(os.path.join(os.path.dirname(toml_path), name), os.path.join(out, port, name))
            break

    store = p.get("store")
    meta = {
        "version": 4,
        "name": port + ".zip",
        "items": [script, port],
        "items_opt": [],
        "attr": {
            "title": title,
            "porter": p.get("porter", []),
            "desc": p.get("desc", ""),
            "desc_md": None,
            "inst": p.get("inst", ""),
            "inst_md": None,
            "genres": p.get("genres", []),
            "image": {"screenshot": shot, "covers": covers, "thumbnail": None, "video": None} if shot else {},
            "rtr": False,
            "exp": p.get("experimental", False),
            # null y no []: los PortMaster anteriores a 2025.06 (el que trae
            # ROCKNIX 20250517) fallan al instalar con una lista
            # (runtime_nicename hace runtime.startswith); los nuevos pasan
            # null a [] (harbourmaster/info.py)
            "runtime": None,
            "store": [{"name": store["name"], "gameurl": store["url"]}] if store else [],
            "availability": p.get("availability", "full"),
            "reqs": [],
            # las arquitecturas que lleva el zip (PM_ARCHES de portmaster.mk)
            "arch": os.environ.get("PM_ARCHES", "aarch64").split(),
            "min_glibc": "2.28",
        },
    }
    with open(os.path.join(out, port, "port.json"), "w") as f:
        json.dump(meta, f, indent=2, ensure_ascii=False)
        f.write("\n")

    genre = (p.get("genres") or [""])[0]
    xml = (
        '<?xml version="1.0" encoding="utf-8"?>\n<gameList>\n  <game>\n'
        f"    <path>./{escape(script)}</path>\n"
        f"    <name>{escape(title)}</name>\n"
        f"    <desc>{escape(p.get('desc', ''))}</desc>\n"
        f"    <developer>{escape(p.get('developer', ''))}</developer>\n"
        + (f"    <publisher>{escape(p['publisher'])}</publisher>\n" if p.get("publisher") else "")
        + (f"    <releasedate>{escape(p['releasedate'])}</releasedate>\n" if p.get("releasedate") else "")
        +
        f"    <genre>{escape(genre.upper() if len(genre) <= 3 else genre.capitalize())}</genre>\n"
        + (f"    <image>./{escape(port)}/{shot}</image>\n" if shot else "")
        +
        "  </game>\n</gameList>\n"
    )
    with open(os.path.join(out, port, "gameinfo.xml"), "w") as f:
        f.write(xml)

    shutil.copy(toml_path, os.path.join(out, port, "port.toml"))


if __name__ == "__main__":
    main()
