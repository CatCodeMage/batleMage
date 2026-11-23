function getAvailableMaps() {
  if (Data.maps && typeof Data.maps === 'object') {
    return Object.entries(Data.maps).map(([id, def]) => ({ id, def }));
  }
  return [{ id: 'map_arena_01', def: Data.map }];
}

function renderStep4() {
  const root = document.getElementById('step-4');
  const maps = getAvailableMaps();

  if (!State.mapId || !maps.find(m => m.id === State.mapId)) {
    State.mapId = maps[0].id;
  }

  root.innerHTML = `
    <h2>Elige el mapa</h2>
    <div class="map-grid">
      ${maps.map(m => renderMapCard(m, m.id === State.mapId)).join('')}
    </div>
    <div id="map-details" class="map-details"></div>
  `;

  root.querySelectorAll('.map-card').forEach(card => {
    card.addEventListener('click', () => {
      State.mapId = card.dataset.mapId;
      renderStep4();
    });
  });

  const selected = maps.find(m => m.id === State.mapId);
  renderMapDetails(selected);

  renderSpawnOptions();
}

function renderMapCard(mapEntry, isSelected) {
  const def = mapEntry.def || {};
  const scale = def.scale || [100, 1, 100];
  const entityCount = def.entities ? Object.keys(def.entities).length : 0;
  const assetName = def.asset || 'desconocido';

  return `
    <div class="map-card ${isSelected ? 'selected' : ''}" data-map-id="${mapEntry.id}">
      <div class="map-thumb" data-asset="${assetName}">
        <span class="map-thumb-label">${assetName}</span>
      </div>
      <h3>${mapEntry.id}</h3>
      <ul class="map-meta">
        <li><strong>Tamaño:</strong> ${scale[0]} × ${scale[2]}</li>
        <li><strong>Entidades:</strong> ${entityCount}</li>
        <li><strong>Suelo:</strong> ${assetName}</li>
      </ul>
    </div>
  `;
}

function renderMapDetails(mapEntry) {
  const details = document.getElementById('map-details');
  if (!mapEntry || !mapEntry.def) {
    details.innerHTML = '<em>Sin detalles disponibles</em>';
    return;
  }

  const def = mapEntry.def;
  const entities = def.entities || {};
  const groups = {
    arena:    [],
    columnas: [],
    rocas:    [],
    arbustos: [],
    árboles:  [],
    otros:    [],
  };

  for (const [name, ent] of Object.entries(entities)) {
    if (name.includes('arena_floor')) groups.arena.push({ name, ent });
    else if (name.includes('column')) groups.columnas.push({ name, ent });
    else if (name.includes('rock')) groups.rocas.push({ name, ent });
    else if (name.includes('bush')) groups.arbustos.push({ name, ent });
    else if (name.includes('tree') || name.includes('forest')) groups.árboles.push({ name, ent });
    else groups.otros.push({ name, ent });
  }

  details.innerHTML = `
    <h3>Detalles de <code>${mapEntry.id}</code></h3>
    ${Object.entries(groups).filter(([, arr]) => arr.length).map(([label, arr]) => `
      <details>
        <summary>${label} (${arr.length})</summary>
        <ul>
          ${arr.map(({ name, ent }) => `
            <li>
              <code>${name}</code> — asset: ${ent.asset}
              · pos: [${(ent.position || []).join(', ')}]
              · rot: [${(ent.rotation || []).join(', ')}]
              · scale: [${(ent.scale || []).join(', ')}]
            </li>
          `).join('')}
        </ul>
      </details>
    `).join('')}
  `;
}

function renderSpawnOptions() {
  const root = document.getElementById('step-4');
  const container = document.createElement('div');
  container.className = 'spawn-options';
  container.innerHTML = `
    <h3>Distribución inicial <small>(opcional)</small></h3>
    <label>
      <input type="radio" name="spawn-mode" value="auto"
        ${(State.spawn?.mode ?? 'auto') === 'auto' ? 'checked' : ''} />
      Automática (círculo alrededor del centro)
    </label>
    <label>
      <input type="radio" name="spawn-mode" value="manual"
        ${State.spawn?.mode === 'manual' ? 'checked' : ''} />
      Manual (editar coordenadas)
    </label>
    <div id="spawn-manual" class="spawn-manual"></div>
  `;
  root.appendChild(container);

  container.querySelectorAll('input[name=spawn-mode]').forEach(r => {
    r.addEventListener('change', () => {
      State.spawn = State.spawn || {};
      State.spawn.mode = r.value;
      renderStep4();
    });
  });

  if (State.spawn?.mode === 'manual') {
    renderManualSpawnEditor(document.getElementById('spawn-manual'));
  } else if (!State.spawn) {
    State.spawn = { mode: 'auto' };
  }
}

function renderManualSpawnEditor(container) {
  const allChars = [
    ...State.teams.A.map((c, i) => ({ ...c, team: 'A', idx: i })),
    ...State.teams.B.map((c, i) => ({ ...c, team: 'B', idx: i })),
  ];

  State.spawn.positions = State.spawn.positions || {};

  container.innerHTML = allChars.map(c => {
    const key = `${c.team}${c.idx}`;
    const pos = State.spawn.positions[key] || [0, 0, 0];
    return `
      <div class="spawn-row" data-key="${key}">
        <strong>${c.playerType} (${c.team})</strong>
        <label>X <input type="number" step="0.5" data-axis="0" value="${pos[0]}" /></label>
        <label>Y <input type="number" step="0.5" data-axis="1" value="${pos[1]}" /></label>
        <label>Z <input type="number" step="0.5" data-axis="2" value="${pos[2]}" /></label>
      </div>
    `;
  }).join('');

  container.querySelectorAll('.spawn-row').forEach(row => {
    const key = row.dataset.key;
    row.querySelectorAll('input').forEach(input => {
      input.addEventListener('input', () => {
        State.spawn.positions[key] = State.spawn.positions[key] || [0, 0, 0];
        State.spawn.positions[key][Number(input.dataset.axis)] = Number(input.value) || 0;
      });
    });
  });
}

function validateStep4() {
  if (!State.mapId) {
    alert('Selecciona un mapa');
    return false;
  }
  const maps = getAvailableMaps();
  if (!maps.find(m => m.id === State.mapId)) {
    alert('El mapa seleccionado ya no está disponible');
    return false;
  }
  return true;
}