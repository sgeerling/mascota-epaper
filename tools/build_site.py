#!/usr/bin/env python3
"""Genera el sitio web de la mascota (interfaz + imágenes de todas las poses).

Pasos:
  1. Descarga Adafruit GFX (versión fija) si no está en build/.
  2. Compila tools/render/render.cpp con el mismo dibujo que usa el ESP32.
  3. Dibuja las poses y las guarda como PNG en site/poses/.
  4. Copia la interfaz (web/pagina.html) como site/index.html en modo demo.

Uso:
  python3 tools/build_site.py            # genera site/
  python3 tools/build_site.py --readme   # además actualiza docs/poses.png
Requiere: git, g++ y Pillow (pip install pillow).
"""
import argparse
import json
import re
import os
import shutil
import subprocess
import sys
from pathlib import Path

from PIL import Image

RAIZ = Path(__file__).resolve().parent.parent
BUILD = RAIZ / "build"
SITE = RAIZ / "site"
GFX_VERSION = "1.12.6"
GFX_DIR = BUILD / f"Adafruit-GFX-Library-{GFX_VERSION}"

# Colores aproximados del panel Spectra 6 (índice del controlador -> RGB)
PALETA = {0: (30, 30, 30), 1: (250, 250, 245), 2: (240, 200, 20),
          3: (190, 40, 40), 5: (40, 60, 150), 6: (40, 130, 70)}


def correr(cmd, **kw):
    print("+", " ".join(str(c) for c in cmd))
    subprocess.run(cmd, check=True, **kw)


def compilar_render():
    BUILD.mkdir(exist_ok=True)
    if not (GFX_DIR / "Adafruit_GFX.cpp").exists():
        correr(["git", "clone", "--quiet", "--depth", "1", "--branch", GFX_VERSION,
                "https://github.com/adafruit/Adafruit-GFX-Library.git", str(GFX_DIR)])
    exe = BUILD / "render"
    correr(["g++", "-std=c++17", "-O2", "-DARDUINO=10819",
            "-I", RAIZ / "tools/render/shim", "-I", GFX_DIR, "-I", RAIZ / "firmware/mascota_e6",
            RAIZ / "tools/render/render.cpp", GFX_DIR / "Adafruit_GFX.cpp", "-o", exe])
    return exe


def a_png(raw: Path, tam: int) -> Image.Image:
    im = Image.frombytes("P", (tam, tam), raw.read_bytes())
    pal = []
    for i in range(256):
        pal += PALETA.get(i, (255, 0, 255))
    im.putpalette(pal)
    return im


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--readme", action="store_true", help="actualiza docs/poses.png")
    ap.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", ""),
                    help="usuario/repositorio para el enlace de la demo")
    args = ap.parse_args()

    exe = compilar_render()
    raw_dir = BUILD / "raw"
    shutil.rmtree(raw_dir, ignore_errors=True)
    raw_dir.mkdir(parents=True)
    correr([exe, raw_dir])

    datos = json.loads((raw_dir / "poses.json").read_text(encoding="utf-8"))
    tam, nombres = datos["tamano"], datos["nombres"]

    shutil.rmtree(SITE, ignore_errors=True)
    (SITE / "poses").mkdir(parents=True)
    imagenes = []
    for i in range(len(nombres)):
        im = a_png(raw_dir / f"pose{i + 1:02d}.raw", tam)
        im.save(SITE / "poses" / f"{i + 1:02d}.png", optimize=True)
        imagenes.append(im)

    datos["repo"] = args.repo
    (SITE / "poses.json").write_text(json.dumps(datos, ensure_ascii=False), encoding="utf-8")

    html = (RAIZ / "web/pagina.html").read_text(encoding="utf-8")
    marca = "<script>\nconst $ = id"
    if marca not in html:
        sys.exit("No encontré el script principal en web/pagina.html")
    html = html.replace(marca, '<script src="demo.js"></script>\n' + marca, 1)
    (SITE / "index.html").write_text(html, encoding="utf-8")
    shutil.copy(RAIZ / "web/demo.js", SITE / "demo.js")
    (SITE / ".nojekyll").write_text("")

    # Pronóstico del tiempo: la página usa las mismas poses. Revisa que sus nombres
    # coincidan con poses.h y la envuelve en un documento HTML completo.
    pron = (RAIZ / "web/pronostico.html").read_text(encoding="utf-8")
    bloque = re.search(r"const POSES = \[(.*?)\n\];", pron, re.S)
    en_pagina = re.findall(r'\{ nombre: "([^"]+)"', bloque.group(1) if bloque else "")
    if en_pagina != nombres:
        sys.exit("Los nombres de las poses en web/pronostico.html no coinciden con poses.h:\n"
                 f"  poses.h:        {nombres}\n  pronostico.html: {en_pagina}")
    (SITE / "pronostico.html").write_text(
        '<!doctype html>\n<html lang="es">\n<head>\n<meta charset="utf-8">\n'
        '<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">\n'
        "<style>:root{padding-top:env(safe-area-inset-top,0px);padding-bottom:env(safe-area-inset-bottom,0px)}"
        "body{margin:0}img{max-width:100%}[hidden]{display:none!important}</style>\n"
        "</head>\n<body>\n" + pron + "\n</body>\n</html>\n", encoding="utf-8")

    if args.readme:
        cols, m = 6, 4
        filas = (len(imagenes) + cols - 1) // cols
        hoja = Image.new("RGB", (cols * (tam + m) + m, filas * (tam + m) + m), (210, 210, 205))
        for i, im in enumerate(imagenes):
            hoja.paste(im.convert("RGB"), (m + (i % cols) * (tam + m), m + (i // cols) * (tam + m)))
        (RAIZ / "docs").mkdir(exist_ok=True)
        hoja.save(RAIZ / "docs/poses.png", optimize=True)
        print("docs/poses.png actualizado")

    print(f"Sitio listo en {SITE.relative_to(RAIZ)}/ con {len(nombres)} poses.")
    print("Para verlo:  python3 -m http.server -d site 8000   y abre http://localhost:8000")


if __name__ == "__main__":
    main()
