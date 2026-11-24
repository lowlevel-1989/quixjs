#!/usr/bin/env python3
"""tools/port_zip.py: el zip de un port para PortMaster (make portmaster
PORT=...). Lo usa cmake/portmaster.cmake.

  python3 tools/port_zip.py <port> <quixjs.aarch64> <sysroot> <jsc> <zip>

<port> es games/<nombre> (su port.toml y sus archivos), <sysroot> el Debian
de tools/sysroot.py y <jsc> la carpeta de JavaScriptCore (sus licencias).
El zip lleva el script .sh y la carpeta del port: port.json, gameinfo.xml y
port.toml (tools/portmaster.py), los archivos del port (menos el juego, lo
que escribe y lo de quien juega), quixjs.aarch64, libs.aarch64/, fonts/,
licenses/, gamedata/ vacia y VERSION.
"""
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TRIPLE = "aarch64-linux-gnu"
# las que no traen todos los CFW: el video (libvpx, libopus) y SDL2_ttf;
# SDL2, SDL2_image, freetype y libpng son las del sistema (el comprobador de
# PortMaster pide no llevarlas)
BUNDLE = ["libvpx.so.5", "libopus.so.0", "libSDL2_ttf-2.0.so.0"]
# el paquete de Debian de cada una (para su licencia), y el de la fuente
BUNDLE_PACKAGES = ["libvpx5", "libopus0", "libsdl2-ttf-2.0-0", "fonts-dejavu-core"]
THIRDPARTY = ["tomlc17", "stb:stb_vorbis", "nestegg", "minimp4", "ffmpeg"]
# lo de games/<port>/ que no va en el zip: el juego (la rom: .zip, .nw,
# .asar o www/), lo que escribe (gamedata/, cache/), lo que se crea al jugar
# (los ajustes del launcher, el mando y el teclado de Controls, el log, las
# capturas) y port.toml (lo copia tools/portmaster.py)
SKIP_DIRS = {"gamedata", "cache", "www", "screenshots"}
SKIP_FILES = {"quixjs-settings.toml", "log.txt", "port.toml"}
SKIP_EXTS = (".zip", ".nw", ".asar", ".map", ".keys")


def copy_port(src, dst):
    for base, dirs, files in os.walk(src):
        rel = os.path.relpath(base, src)
        if rel == ".":
            dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        os.makedirs(os.path.join(dst, rel), exist_ok=True)
        for name in files:
            top = rel == "."
            if top and (name in SKIP_FILES or name.endswith(SKIP_EXTS)):
                continue
            if name.endswith((".zip", ".nw", ".asar")):
                continue
            shutil.copy2(os.path.join(base, name), os.path.join(dst, rel, name))


def find_lib(sysroot, name):
    for d in (f"usr/lib/{TRIPLE}", f"lib/{TRIPLE}"):
        path = os.path.join(sysroot, d, name)
        if os.path.exists(path):
            return path
    sys.exit(f"missing {name} in the sysroot")


def main():
    if len(sys.argv) != 6:
        sys.exit(__doc__)
    port_dir, binary, sysroot, jsc, zip_path = sys.argv[1:]
    port = os.path.basename(os.path.normpath(port_dir))
    stage = zip_path + ".stage"
    shutil.rmtree(stage, ignore_errors=True)
    # el script .sh, port.json, gameinfo.xml y port.toml
    subprocess.run([sys.executable, os.path.join(ROOT, "tools/portmaster.py"),
                    os.path.join(port_dir, "port.toml"), stage], check=True)
    d = os.path.join(stage, port)
    copy_port(port_dir, d)
    for sub in ("licenses", "gamedata", "fonts", "libs.aarch64"):
        os.makedirs(os.path.join(d, sub), exist_ok=True)
    shutil.copy2(binary, os.path.join(d, "quixjs.aarch64"))
    for lib in BUNDLE:
        shutil.copy(find_lib(sysroot, lib), os.path.join(d, "libs.aarch64", lib))
    # la fuente de las pantallas de quixjs (el launcher, los avisos)
    shutil.copy(os.path.join(sysroot, "usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"), os.path.join(d, "fonts"))
    # las licencias: quixjs, lo de runtime/thirdparty, JavaScriptCore y lo de Debian
    lic = os.path.join(d, "licenses")
    shutil.copy(os.path.join(ROOT, "LICENSE"), os.path.join(lic, "quixjs.txt"))
    for item in THIRDPARTY:
        folder, _, name = item.partition(":")
        shutil.copy(os.path.join(ROOT, "runtime/thirdparty", folder, "LICENSE"), os.path.join(lic, (name or folder) + ".txt"))
    for name in os.listdir(os.path.join(jsc, "licenses")):
        shutil.copy(os.path.join(jsc, "licenses", name), lic)
    for pkg in BUNDLE_PACKAGES:
        shutil.copy(os.path.join(sysroot, "usr/share/doc", pkg, "copyright"), os.path.join(lic, pkg + ".txt"))
    with open(os.path.join(ROOT, "VERSION")) as f:
        version = f.read().strip()
    with open(os.path.join(d, "VERSION"), "w") as f:
        f.write(version + "\n")
    for name in os.listdir(stage):
        if name.endswith(".sh"):
            os.chmod(os.path.join(stage, name), 0o755)
    os.chmod(os.path.join(d, "quixjs.aarch64"), 0o755)
    if os.path.exists(zip_path):
        os.remove(zip_path)
    subprocess.run(["zip", "-q", "-r", "-9", os.path.abspath(zip_path), "."], cwd=stage, check=True)
    shutil.rmtree(stage)
    print(f"Built {zip_path} (quixjs {version}, aarch64): unzip it into the ports folder of the device")


if __name__ == "__main__":
    main()
