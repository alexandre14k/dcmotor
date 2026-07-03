#!/usr/bin/env python3
import subprocess
import os
import shutil
import sys

BUILD_DIR = "out"
APP_TITLE = "dcmotor"

def init_xmake():
    if shutil.which("xmake"):
        print("xmake already installed")
        return
    subprocess.run(
        "curl -fsSL https://xmake.io/shget.text | bash",
        shell=True, check=True
    )

def build(mode="release"):
    # Configure once with -o, it gets saved in .xmake
    subprocess.run(["xmake", "f", "-o", BUILD_DIR, "-m", mode], check=True)
    # Build without -o, it remembers the directory
    subprocess.run(["xmake"], check=True)

    # Clean up dummy build folder if xmake falls back to defaults
    if os.path.exists("build"):
        shutil.rmtree("build")

def clean():
    subprocess.run(["xmake", "clean", "-a"], check=True)
    for d in [BUILD_DIR, "build", ".xmake"]:
        if os.path.exists(d):
            shutil.rmtree(d)

def run():
    subprocess.run(["xmake", "run", APP_TITLE], check=True)

def debug():
    build("debug")
    subprocess.run(["xmake", "run", "-d", APP_TITLE], check=True)

def install_specs():
    if not os.path.exists("spec.md"):
        print("spec.md missing")
        return
    with open("spec.md", "r") as f:
        for line in f:
            pkg = line.strip()
            if pkg and not pkg.startswith("#"):
                subprocess.run(["xmake", "require", pkg], check=True)

def clear_screen():
    if sys.platform == "win32":
        os.system("cls")
    else:
        os.system("clear")

def main():
    while True:
        cmd = input(
            "[b]uild [c]lean [r]un [d]ebug "
            "[s]pecs [k]lear [i]nit e[x]it > "
        ).strip().lower()
        if cmd == 'b':
            build()
        elif cmd == 'c':
            clean()
        elif cmd == 'r':
            run()
        elif cmd == 'd':
            debug()
        elif cmd == 's':
            install_specs()
        elif cmd == 'k':
            clear_screen()
        elif cmd == 'i':
            init_xmake()
        elif cmd == 'x':
            break

if __name__ == "__main__":
    main()