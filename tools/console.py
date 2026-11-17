#!/usr/bin/env python3
"""tools/console.py: consola para la consola de debug de quixjs. Solo usa la
libreria estandar.

Heredada de tamatama (https://github.com/lowlevel-1989/tamatama,
tools/console.py), igual que el servidor (runtime/core/debug_server.c).

Terminal 1:  ./quixjs --debug -c juego/config.toml
Terminal 2:  python3 tools/console.py

Tambien un comando suelto (para automatizar pruebas):

  python3 tools/console.py press Enter
  python3 tools/console.py shot titulo.png
  python3 tools/console.py eval "SceneManager._scene.constructor.name"
  python3 tools/console.py -w 1.5 press ArrowDown   # espera 1.5 s despues

Habla con quixjs.sock (lo crea quixjs en la carpeta donde se ejecuta) con los
comandos de texto de runtime/core/debug_server.c. Cada respuesta termina en una
linea "ok" o "err MENSAJE".
"""
import argparse
import os
import socket
import sys
import time

try:
    import readline
except ImportError:
    readline = None

HISTORY_FILE = os.path.expanduser("~/.quixjs_console_history")
COMMANDS = ["help", "info", "press", "down", "up", "shot", "eval", "pause", "resume", "run", "quit"]
KEYS = (["Enter", "Escape", "Space", "Tab", "Backspace", "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight",
         "ShiftLeft", "ControlLeft", "AltLeft", "PageUp", "PageDown", "Home", "End", "Insert", "Delete"] +
        ["Key" + chr(c) for c in range(ord("A"), ord("Z") + 1)] +
        ["Digit%d" % i for i in range(10)] + ["F%d" % i for i in range(1, 13)])


class Console:
    def __init__(self, path):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(path)
        self.buf = b""

    def command(self, line):
        """Envia un comando y devuelve (ok, lineas de la respuesta)."""
        self.sock.sendall(line.encode() + b"\n")
        out = []
        while True:
            while b"\n" not in self.buf:
                data = self.sock.recv(65536)
                if not data:
                    raise ConnectionError("quixjs closed the connection")
                self.buf += data
            raw, self.buf = self.buf.split(b"\n", 1)
            text = raw.decode(errors="replace")
            if text == "ok":
                return True, out
            if text.startswith("err "):
                out.append(text[4:])
                return False, out
            out.append(text)


def completer(text, state):
    buf = readline.get_line_buffer() if readline else ""
    words = buf.split()
    if len(words) <= 1 and not buf.endswith(" "):
        options = [c for c in COMMANDS if c.startswith(text)]
    elif words[0] in ("press", "down", "up"):
        options = [k for k in KEYS if k.startswith(text)]
    else:
        options = []
    return options[state] if state < len(options) else None


def interactive(console):
    if readline:
        try:
            readline.read_history_file(HISTORY_FILE)
        except OSError:
            pass
        readline.set_completer(completer)
        readline.parse_and_bind("tab: complete")
    print("quixjs debug console. help for the commands, Ctrl+D to leave.")
    try:
        while True:
            try:
                line = input("quixjs> ").strip()
            except EOFError:
                print()
                break
            if not line:
                continue
            ok, out = console.command(line)
            for l in out:
                print(l if ok else "error: " + l)
            if line in ("quit", "exit"):
                break
    finally:
        if readline:
            try:
                readline.write_history_file(HISTORY_FILE)
            except OSError:
                pass


def main():
    ap = argparse.ArgumentParser(description="quixjs debug console")
    ap.add_argument("-s", "--socket", default="quixjs.sock", help="socket path (default: quixjs.sock)")
    ap.add_argument("-w", "--wait", type=float, default=0, help="seconds to wait after the command")
    ap.add_argument("command", nargs=argparse.REMAINDER, help="one command to send (none: interactive)")
    args = ap.parse_args()

    try:
        console = Console(args.socket)
    except OSError as e:
        print("cannot connect to %s: %s (is quixjs running with --debug?)" % (args.socket, e), file=sys.stderr)
        return 1

    if not args.command:
        interactive(console)
        return 0

    ok, out = console.command(" ".join(args.command))
    for l in out:
        print(l if ok else "error: " + l, file=sys.stdout if ok else sys.stderr)
    if args.wait > 0:
        time.sleep(args.wait)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
