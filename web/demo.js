// demo.js — Simula en el navegador la API del ESP32 para el sitio de GitHub Pages.
// Reproduce la misma lógica del firmware: modo aleatorio con "bolsa barajada"
// (cada ciclo muestra todas las poses sin repetir), pose fija e intervalo.
// En la demo el refresco de la pantalla (~27 s en el panel real) se simula en 4 s.
(function () {
  const REFRESCO_MS = 4000;
  const FIJA_S = 86400;
  let datos = null, st = null, bolsa = [], pos = 0, ultimoMs = 0, refrescoHasta = 0, pendiente = -1;

  const guardar = () => {
    try { localStorage.setItem('mascota-demo', JSON.stringify({ modo: st.modo, pose: st.pose, intervalo: st.intervalo })); } catch (e) {}
  };
  const leer = () => {
    try { return JSON.parse(localStorage.getItem('mascota-demo')) || {}; } catch (e) { return {}; }
  };

  function siguienteAleatoria() {
    const n = st.nombres.length;
    if (pos >= bolsa.length) {
      bolsa = [...Array(n).keys()];
      for (let i = n - 1; i > 0; i--) {                       // Fisher-Yates
        const j = Math.floor(Math.random() * (i + 1));
        [bolsa[i], bolsa[j]] = [bolsa[j], bolsa[i]];
      }
      if (bolsa[0] === st.actual && n > 1) {                  // nunca dos veces seguidas
        const j = 1 + Math.floor(Math.random() * (n - 1));
        [bolsa[0], bolsa[j]] = [bolsa[j], bolsa[0]];
      }
      pos = 0;
    }
    return bolsa[pos++];
  }

  function mostrar(i) { pendiente = i; refrescoHasta = Date.now() + REFRESCO_MS; }

  function tick() {
    const ahora = Date.now();
    if (refrescoHasta && ahora >= refrescoHasta) {
      st.actual = pendiente; refrescoHasta = 0; ultimoMs = ahora;
    }
    const espera = st.modo === 'aleatorio' ? st.intervalo : FIJA_S;
    if (!refrescoHasta && st.actual >= 0 && ahora - ultimoMs >= espera * 1000) {
      mostrar(st.modo === 'aleatorio' ? siguienteAleatoria() : st.pose);
    }
    st.refrescando = !!refrescoHasta;
    st.proximo = st.refrescando ? 0 : Math.max(0, Math.round(espera - (ahora - ultimoMs) / 1000));
  }

  async function iniciar() {
    if (st) return;
    datos = await (await fetch('poses.json')).json();
    const g = leer();
    st = {
      modo: g.modo === 'fija' ? 'fija' : 'aleatorio',
      pose: Number.isInteger(g.pose) ? Math.min(Math.max(g.pose, 0), datos.nombres.length - 1) : 0,
      intervalo: Math.min(Math.max(g.intervalo || 180, 180), 86400),
      intervaloMin: 180, intervaloMax: 86400,
      actual: -1, refrescando: true, proximo: 0, errorPantalla: false,
      nombres: datos.nombres,
    };
    mostrar(st.modo === 'aleatorio' ? siguienteAleatoria() : st.pose);
    avisoDemo(datos.repo);
  }

  function avisoDemo(repo) {
    const m = document.querySelector('main'), d = document.createElement('div');
    d.className = 'card';
    d.style.cssText = 'margin-top:12px;font-size:.9rem;color:var(--muted)';
    d.innerHTML = 'Demostración en el navegador: en el ESP32 esta misma página controla la pantalla e-paper real. ' +
      'Aquí el refresco de la pantalla se simula en 4 s (en el panel tarda ~27 s).' +
      (repo ? ` <a href="https://github.com/${repo}" style="color:var(--accent)">Ver el código en GitHub</a>.` : '');
    m.insertBefore(d, m.children[2]);
  }

  window.MASCOTA_DEMO = {
    img: i => `poses/${String(i + 1).padStart(2, '0')}.png`,
    async api(url, d) {
      await iniciar();
      tick();
      if (url.endsWith('api/config') && d) {
        const antesModo = st.modo, antesPose = st.pose;
        if ('modo' in d) st.modo = d.modo === 'fija' ? 'fija' : 'aleatorio';
        if ('pose' in d) st.pose = Math.min(Math.max(parseInt(d.pose, 10) || 0, 0), st.nombres.length - 1);
        if ('intervalo' in d) st.intervalo = Math.min(Math.max(parseInt(d.intervalo, 10) || 180, 180), 86400);
        if (st.modo !== antesModo || (st.modo === 'fija' && st.pose !== antesPose)) {
          mostrar(st.modo === 'aleatorio' ? siguienteAleatoria() : st.pose);
        }
        guardar();
        tick();
      } else if (url.endsWith('api/otra')) {
        mostrar(st.modo === 'aleatorio' ? siguienteAleatoria() : st.pose);
        tick();
      }
      return JSON.parse(JSON.stringify(st));
    },
  };
})();
