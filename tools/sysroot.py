#!/usr/bin/env python3
"""tools/sysroot.py: el sistema contra el que se compila para las portatiles
(PortMaster): unos paquetes de Debian buster (glibc 2.28, SDL 2.0.9: lo
bastante viejo para cualquier CFW), desempaquetados en una carpeta. Sin root
ni contenedor. Lo usa cmake/toolchains/portmaster.cmake, una vez.

  python3 tools/sysroot.py <carpeta> <arquitectura de Debian> <paquetes...>

Cada paquete se comprueba con la sha256 del indice de Debian. Los enlaces
absolutos (libm.so -> /lib/...) apuntarian a este PC: se hacen relativos.
"""
import hashlib
import io
import lzma
import os
import shutil
import sys
import tarfile
import urllib.request

ARCHIVE = "http://archive.debian.org/debian"
SUITE = "buster"


def fetch(url):
    with urllib.request.urlopen(url) as r:
        return r.read()


def packages(arch):
    """el indice: {paquete: (ruta, sha256)}"""
    text = lzma.decompress(fetch(f"{ARCHIVE}/dists/{SUITE}/main/binary-{arch}/Packages.xz")).decode()
    index = {}
    for block in text.split("\n\n"):
        fields = dict(line.split(": ", 1) for line in block.splitlines() if ": " in line and not line.startswith(" "))
        if "Package" in fields:
            index[fields["Package"]] = (fields["Filename"], fields["SHA256"])
    return index


def unpack_deb(data, out):
    """un .deb es un ar con data.tar.*: se saca data.tar en out"""
    pos = 8  # "!<arch>\n"
    while pos < len(data):
        name = data[pos:pos + 16].decode().strip().rstrip("/")
        size = int(data[pos + 48:pos + 58])
        body = data[pos + 60:pos + 60 + size]
        if name.startswith("data.tar"):
            with tarfile.open(fileobj=io.BytesIO(body)) as t:
                t.extractall(out, filter="tar")
        pos += 60 + size + (size % 2)


def relative_links(root):
    for base, dirs, files in os.walk(root):
        for name in dirs + files:
            path = os.path.join(base, name)
            target = os.path.islink(path) and os.readlink(path)
            if target and target.startswith("/"):
                os.remove(path)
                os.symlink(os.path.relpath(root + target, base), path)


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    out, arch, wanted = sys.argv[1], sys.argv[2], sys.argv[3:]
    if os.path.exists(os.path.join(out, ".ready")):
        return
    print(f"Downloading Debian {SUITE} for {arch} into {out} (only the first time)")
    tmp = out + ".tmp"
    shutil.rmtree(tmp, ignore_errors=True)
    os.makedirs(tmp)
    index = packages(arch)
    for name in wanted:
        if name not in index:
            sys.exit(f"no package {name} for {arch}")
        path, sha = index[name]
        data = fetch(f"{ARCHIVE}/{path}")
        if hashlib.sha256(data).hexdigest() != sha:
            sys.exit(f"{name}: the sha256 does not match the index of Debian")
        unpack_deb(data, tmp)
    relative_links(tmp)
    open(os.path.join(tmp, ".ready"), "w").close()
    shutil.rmtree(out, ignore_errors=True)
    os.rename(tmp, out)


if __name__ == "__main__":
    main()
