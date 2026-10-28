#!/usr/bin/env python3
"""Convierte web/pagina.html en firmware/mascota_e6/pagina.h para el ESP32.

Uso:
  python3 tools/generar_pagina.py          # regenera pagina.h
  python3 tools/generar_pagina.py --check  # falla si pagina.h no está al día (para CI)
"""
import sys
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
ORIGEN = RAIZ / "web/pagina.html"
DESTINO = RAIZ / "firmware/mascota_e6/pagina.h"

html = ORIGEN.read_text(encoding="utf-8")
if ')pagina"' in html:
    sys.exit('El HTML no puede contener )pagina"')
contenido = (
    "// Generado desde web/pagina.html con tools/generar_pagina.py. Edita pagina.html, no este archivo.\n"
    "#pragma once\n#include <pgmspace.h>\n"
    f'const char PAGINA_HTML[] PROGMEM = R"pagina({html})pagina";\n'
)

if "--check" in sys.argv:
    actual = DESTINO.read_text(encoding="utf-8") if DESTINO.exists() else ""
    if actual != contenido:
        sys.exit("pagina.h no coincide con web/pagina.html: ejecuta python3 tools/generar_pagina.py")
    print("pagina.h al día")
else:
    DESTINO.write_text(contenido, encoding="utf-8")
    print(f"{DESTINO.relative_to(RAIZ)} generado ({len(html.encode())} bytes)")
