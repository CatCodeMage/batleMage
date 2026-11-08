function renderStep5() {
  const root = document.getElementById('step-5');
  root.innerHTML = `
    <h2>Revisión y lanzamiento</h2>
    <div id="review-summary" class="review-summary"></div>
    <div class="review-actions">
      <button id="btn-simulate" class="btn-primary">🧪 Simular</button>
      <button id="btn-play" class="btn-secondary">🎮 Jugar</button>
    </div>
    <div class="review-io">
      <button id="btn-save" class="btn-ghost">💾 Guardar preset</button>
      <button id="btn-load" class="btn-ghost">📂 Cargar preset</button>
      <input type="file" id="file-load" accept=".json" hidden />
    </div>
    <details class="review-json">
      <summary>Ver JSON generado</summary>
      <pre id="review-json-pre"></pre>
    </details>
  `;

  renderSummary();
  bindActions();
}

function renderSummary() {
  const summary = document.getElementById('review-summary');

  const combatLabels = {
    '1v1': '1 vs 1',
    '2v2': '2 vs 2',
    '1vhorde': '1 vs Horda',
    'hordevshorde': 'Horda vs Horda',
  };

  summary.innerHTML = `
    <div class="summary-block">
      <h3>Modo de combate</h3>
      <p><strong>${combatLabels[State.combat.combatType] || State.combat.combatType}</strong></p>
      <p>Equipo A: ${State.combat.teamSizeA} · Equipo B: ${State.combat.teamSizeB}</p>
    </div>

    <div class="summary-block">
      <h3>Mapa</h3>
      <p><code>${State.mapId}</code></p>
    </div>

    <div class="summary-teams">
      ${renderTeamSummary('A', State.teams.A)}
      ${renderTeamSummary('B', State.teams.B)}
    </div>
  `;
}

function renderTeamSummary(team, chars) {
  const color = team === 'A' ? 'red' : 'blue';
  return `
    <div class="summary-team team-${team}">
      <h3 style="color:${team === 'A' ? 'var(--accent)' : 'var(--blue)'}">Equipo ${team}</h3>
      ${chars.length === 0 ? '<em>Vacío</em>' : chars.map(c => {
        const activeWands = c.wands.filter(w => w.wandId);
        return `
          <div class="summary-char">
            <strong>${c.playerType}</strong>
            <small>(${c.entity})</small>
            <ul>
              ${activeWands.length === 0
                ? '<li><em>Sin varitas</em></li>'
                : activeWands.map(w => {
                    const wandDef = Data.wands[w.wandId] || {};
                    return `<li>
                      <span class="wand-name">${wandDef.name || w.wandId}</span>
                      <span class="wand-spells">${w.spells.join(', ')}</span>
                    </li>`;
                  }).join('')}
              ${c.basicSpell
                ? `<li class="basic-spell"><em>Básico:</em> ${c.basicSpell}</li>`
                : ''}
            </ul>
          </div>
        `;
      }).join('')}
    </div>
  `;
}

function bindActions() {
  document.getElementById('btn-simulate').addEventListener('click', () => {
    if (!validateAllSteps()) return;
    const cfg = buildCombatConfig('simulate');
    showJson(cfg);
    console.log('[SIMULATE]', cfg);
  });

  document.getElementById('btn-play').addEventListener('click', () => {
    if (!validateAllSteps()) return;
    const cfg = buildCombatConfig('play');
    showJson(cfg);
    console.log('[PLAY]', cfg);
    alert('Configuración de juego lista.\nMira la consola para el JSON.');
  });

  document.getElementById('btn-save').addEventListener('click', async () => {
  if (!validateAllSteps()) return;
  const cfg = buildCombatConfig('simulate');

  try {
    const res = await fetch('/save-combat', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(cfg),
    });
    const data = await res.json();

    if (!res.ok || !data.ok) throw new Error(data.error || 'Error desconocido');

  } catch (err) {
    console.error(err);
  }
});

  const fileInput = document.getElementById('file-load');
  document.getElementById('btn-load').addEventListener('click', () => fileInput.click());
  fileInput.addEventListener('change', async (e) => {
    const file = e.target.files[0];
    if (!file) return;
    try {
      const text = await file.text();
      const cfg = JSON.parse(text);
      loadPresetIntoState(cfg);
      renderStep(1);
      alert('Preset cargado');
    } catch (err) {
      alert('Error al cargar el preset: ' + err.message);
    }
  });
}

function showJson(cfg) {
  const pre = document.getElementById('review-json-pre');
  pre.textContent = JSON.stringify(cfg, null, 2);
}

function loadPresetIntoState(cfg) {
  if (!cfg || !cfg.teams) throw new Error('Formato de preset inválido');

  State.combat.combatType = cfg.combatType;
  State.combat.teamSizeA = cfg.teams.A.length;
  State.combat.teamSizeB = cfg.teams.B.length;
  State.mapId = cfg.map || 'map_arena_01';

  const rebuildTeam = (teamKey) => cfg.teams[teamKey].map(c => ({
    playerType: c.playerType,
    entity: c.entity || (teamKey === 'A' ? 'player_red' : 'player_blue'),
    wands: (c.wands || []).map(w => ({ wandId: w.wandId, spells: [...w.spells] })),
    basicSpell: c.basicSpell || null,
  }));

  State.teams.A = rebuildTeam('A');
  State.teams.B = rebuildTeam('B');
  State.mode = cfg.mode || 'simulate';
}

function validateAllSteps() {
  const validators = [
    validateStep1,
    validateStep2,
    validateStep3,
    validateStep4,
  ];
  for (const v of validators) {
    if (!v()) {
      alert('Hay errores en pasos anteriores. Vuelve a revisarlos.');
      return false;
    }
  }
  return true;
}