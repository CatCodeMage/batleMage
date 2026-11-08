function renderStep3() {
  const root = document.getElementById('step-3');
  root.innerHTML = `
    <h2>Configura varitas y hechizos</h2>
    <p class="step-hint">
      Cada personaje puede llevar hasta <strong>4 varitas</strong>.
      Cada varita puede contener cualquier hechizo, hasta su <em>capacidad</em>.
    </p>
    <div id="wand-list"></div>
  `;
  renderAllWands();
}

function renderAllWands() {
  const container = document.getElementById('wand-list');
  container.innerHTML = '';

  for (const team of ['A', 'B']) {
    if (State.teams[team].length === 0) continue;

    const teamHeader = document.createElement('h2');
    teamHeader.className = `team-header team-${team.toLowerCase()}`;
    teamHeader.textContent = `Equipo ${team}`;
    container.appendChild(teamHeader);

    State.teams[team].forEach((char, idx) => {
      container.appendChild(buildCharacterWands(team, idx, char));
    });
  }
}

/**
 * Construye el bloque completo de un personaje: 4 slots de varita + hechizo básico.
 */
function buildCharacterWands(team, charIdx, char) {
  const div = document.createElement('div');
  div.className = `character-wands team-${team.toLowerCase()}`;
  div.dataset.team = team;
  div.dataset.charIdx = charIdx;

  const header = document.createElement('h3');
  header.textContent = `${char.playerType} (Equipo ${team})`;
  div.appendChild(header);

  for (let slot = 0; slot < 4; slot++) {
    if (!char.wands[slot]) char.wands[slot] = { wandId: null, spells: [] };
    div.appendChild(buildWandSlot(char, slot));
  }

  div.appendChild(buildBasicSpellSelector(char));

  return div;
}

function buildWandSlot(char, slotIdx) {
  const slot = char.wands[slotIdx];
  const wrapper = document.createElement('div');
  wrapper.className = 'wand-slot';
  wrapper.dataset.slot = slotIdx;

  const options = Object.entries(Data.wands).map(([id, w]) =>
    `<option value="${id}" ${slot.wandId === id ? 'selected' : ''}>${w.name}</option>`
  ).join('');

  wrapper.innerHTML = `
    <div class="wand-slot-header">
      <strong>Varita ${slotIdx + 1}</strong>
      <select class="wand-select">
        <option value="">— Sin varita —</option>
        ${options}
      </select>
      <button type="button" class="btn-clear-wand" title="Vaciar hechizos">Eliminar</button>
    </div>
    <div class="wand-body"></div>
  `;

  wrapper.querySelector('.wand-select').addEventListener('change', (e) => {
    const wandId = e.target.value || null;
    slot.wandId = wandId;
    if (wandId && slot.spells.length > Data.wands[wandId].capacity) {
      slot.spells = slot.spells.slice(0, Data.wands[wandId].capacity);
    }
    refreshWandBody(wrapper, slot);
  });

  wrapper.querySelector('.btn-clear-wand').addEventListener('click', () => {
    slot.spells = [];
    refreshWandBody(wrapper, slot);
  });

  refreshWandBody(wrapper, slot);
  return wrapper;
}

function refreshWandBody(wrapper, slot) {
  const body = wrapper.querySelector('.wand-body');

  if (!slot.wandId) {
    body.innerHTML = '<em class="empty-msg">Selecciona una varita</em>';
    return;
  }

  const wand = Data.wands[slot.wandId];
  const used = slot.spells.length;
  const cap = wand.capacity;
  const full = used >= cap;

  const statsHtml = `
    <div class="wand-stats">
      <span class="stat mana" title="Maná propio de la varita">
        <strong>${wand.mana}</strong> Maná
      </span>
      <span class="stat capacity ${full ? 'full' : ''}" title="Hechizos en cola / capacidad">
        <strong>${used}/${cap}</strong>
      </span>
      <span class="stat cast" title="Retardo entre hechizos">
        Tiempo entre hechizos <strong>${wand.castDelay}s</strong>
      </span>
      <span class="stat recharge" title="Recarga al vaciar la cola">
        Tiempo de recarga <strong>${wand.rechargeTime}s</strong>
      </span>
    </div>
  `;

  const spellsHtml = Object.entries(Data.spells).map(([sid, sp]) => {
    const inQueue = slot.spells.includes(sid);
    const disabled = !inQueue && full;
    return `
      <button type="button"
              class="spell-chip element-${sp.element} ${inQueue ? 'selected' : ''} ${disabled ? 'disabled' : ''}"
              data-spell="${sid}"
              ${disabled ? 'disabled' : ''}
              title="Daño ${sp.damage} · CD ${sp.cooldown}s · ${sp.element}">
        ${sid}
        <small>(${sp.damage} · ${sp.element})</small>
      </button>
    `;
  }).join('');

  body.innerHTML = `
    ${statsHtml}
    <div class="spell-pool">${spellsHtml}</div>
  `;

  body.querySelectorAll('.spell-chip').forEach(chip => {
    chip.addEventListener('click', () => {
      const sid = chip.dataset.spell;
      if (slot.spells.includes(sid)) {
        slot.spells = slot.spells.filter(s => s !== sid);
      } else {
        if (slot.spells.length >= wand.capacity) {
          return;
        }
        slot.spells.push(sid);
      }
      refreshWandBody(wrapper, slot);
    });
  });
}

function buildBasicSpellSelector(char) {
  const div = document.createElement('div');
  div.className = 'basic-spell-row';

  const options = Object.entries(Data.spells).map(([sid, sp]) =>
    `<option value="${sid}" ${char.basicSpell === sid ? 'selected' : ''}>
      ${sid} (${sp.element}, dmg ${sp.damage})
    </option>`
  ).join('');

  div.innerHTML = `
    <label>
      <strong>Hechizo básico:</strong>
      <select class="basic-spell-select">
        <option value="">— Ninguno —</option>
        ${options}
      </select>
    </label>
  `;

  div.querySelector('.basic-spell-select').addEventListener('change', (e) => {
    char.basicSpell = e.target.value || null;
  });

  return div;
}

function validateStep3() {
  for (const team of ['A', 'B']) {
    for (const char of State.teams[team]) {
      const activeWands = char.wands.filter(w => w.wandId);

      if (activeWands.length === 0) {
        alert(`${char.playerType} (Equipo ${team}) no tiene ninguna varita configurada.`);
        return false;
      }

      for (const w of activeWands) {
        if (w.spells.length === 0) {
          const wandDef = Data.wands[w.wandId];
          alert(`La varita "${wandDef?.name || w.wandId}" de ${char.playerType} está vacía.`);
          return false;
        }
        if (w.spells.length > Data.wands[w.wandId].capacity) {
          alert(`La varita "${wandDef?.name || w.wandId}" de ${char.playerType} excede su capacidad.`);
          return false;
        }
      }
    }
  }
  return true;
}