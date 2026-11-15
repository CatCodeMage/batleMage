const MIN_WAND_SLOTS = 4;
const MAX_WAND_SLOTS = 10;
const MAX_WANDS_PER_CHAR = 8;

function renderStep3() {
  const root = document.getElementById('step-3');
  root.innerHTML = `
    <h2>Configura varitas y hechizos</h2>
    <p class="step-hint">
      Añade varitas a cada personaje. Cada varita empieza con 4 espacios
      (hasta 10). Arrastra los hechizos del pool a los espacios para
      reordenarlos libremente.
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
      container.appendChild(buildCharacterBlock(team, idx, char));
    });
  }
}

/**
 * Construye el bloque completo de un personaje: 4 slots de varita + hechizo básico.
 */
function buildCharacterBlock(team, charIdx, char) {
  
  if (!Array.isArray(char.wands)) char.wands = [];
  if (typeof char.aiWands !== 'boolean') char.aiWands = false;
  
  const div = document.createElement('div');
  div.className = `character-wands team-${team.toLowerCase()}`;
  div.dataset.team = team;
  div.dataset.charIdx = charIdx;

  const header = document.createElement('div');
  header.className = 'char-header'
  header.innerHTML = `
    <h3>${char.playerType} (Equipo ${team})</h3>
    <label class="ai-toggle">
      <input type="checkbox" ${char.aiWands ? 'checked' : ''} />
      Dejar que la IA configure las varitas
    </label>
  `;
  header.querySelector('input').addEventListener('change', (e) => {
    char.aiWands = e.target.checked;
    renderAllWands();
  })
  div.appendChild(header);

  if (char.aiWands) {
    const notice = document.createElement('div');
    notice.className = 'ai-notice';
    notice.innerHTML = `
      <p>La IA elegirá las varitas y los hechizos de <strong>${char.playerType}</strong> automáticamente.</p>
      <p><small>Podrás revisar la configuración final en el paso 5.</small></p>
    `;
    div.appendChild(notice);
    return div;
  }

 const wandConfig = document.createElement('div');
  wandConfig.className = 'wand-config';
  char.wands.forEach((wand, wIdx) => {
    wandConfig.appendChild(buildWand(wand, wIdx, char));
  });
  div.appendChild(wandConfig);

  const addWandBtn = document.createElement('button');
  addWandBtn.className = 'btn-add-wand';
  addWandBtn.textContent = "Añdir Varita";
  addWandBtn.disabled = char.wands.length >= MAX_WANDS_PER_CHAR;
  addWandBtn.addEventListener('click', () => {
    char.wands.push({slots: MIN_WAND_SLOTS, spells: []});
    renderAllWands();
  })

  div.appendChild(addWandBtn);
  div.appendChild(buildSpellPool(char));
  div.appendChild(buildBasicSpellSelector(char));
  setUpDragAndDrop(div, char);

  return div;
}

function buildWand(wand, wandIdx, char) {
  const wrapper = document.createElement('div');
  wrapper.className = 'wand';
  wrapper.dataset.wandIdx = wandIdx;

  const header = document.createElement('div');
  header.className = 'wand-header';
  header.innerHTML = `
    <strong>Varita ${wandIdx + 1}</strong>
    <span class="wand-slot-counter">
      <button class="btn-slot-dec" title="Reducir espacios">−</button>
      <span class="slot-count">${wand.slots}</span>
      <button class="btn-slot-inc" title="Ampliar espacios">+</button>
      espacios
    </span>
    <button class="btn-remove-wand" title="Eliminar varita">Eliminar varita</button>
  `;
  wrapper.appendChild(header);

  header.querySelector('.btn-slot-dec').addEventListener('click', () => {
    if (wand.slots > MIN_WAND_SLOTS) {
      wand.slots--;
      if (wand.spells.length > wand.slots) {
        wand.spells = wand.spells.slice(0, wand.slots);        
      }
    }
    renderAllWands();
  });

  header.querySelector('.btn-slot-inc').addEventListener('click', () => {
    if (wand.slots < MAX_WAND_SLOTS) {
      wand.slots++;
      renderAllWands();
    }
  });

  header.querySelector('.btn-remove-wand').addEventListener('click', () => {
    if (confirm(`¿Eliminar varita ${wandIdx + 1}?`)) {
      char.wands.splice(wandIdx, 1);
      renderAllWands();
    }
  });

  const wandSlots = document.createElement('div');
  wandSlots.className = 'wand-slots';
  for (let i = 0; i < wand.slots; i++) {
    const slot = document.createElement('div');
    slot.className = 'wand-slot-item';
    slot.dataset.wandIdx = wandIdx;
    slot.dataset.slotIdx = i;

    const spellId = wand.spells[i];
    if (spellId) {
      const sp = Data.spells[spellId];
      slot.classList.add('filled', `element-${sp.element}`);
      slot.draggable = true;
      slot.innerHTML = `
        <span class="spell-name">${spellId}</span>
        <button class="btn-remove-spell" title="Quitar">✕</button>
      `;
      slot.querySelector('.btn-remove-spell').addEventListener('click', (e) => {
        e.stopPropagation();
        wand.spells.splice(i, 1);
        renderAllWands();
      })
    } else {
      slot.classList.add('empty');
      slot.innerHTML = `<span class="slot-number">${i}</span>`;
    }

    wandSlots.appendChild(slot);
    
  }

  wrapper.appendChild(wandSlots);

  return wrapper;
}

function buildSpellPool(char) {
  const pool = document.createElement('div');
  pool.className = 'spell-pool';
  pool.innerHTML = `<h4>Pool de hechizos (arrastra a una varita)</h4>
    <div class="spell-pool-list"></div>`;
  const list = pool.querySelector('.spell-pool-list');

  Object.entries(Data.spells).forEach(([sid, sp]) => {
    const chip = document.createElement('div');
    chip.className = `spell-chip element-${sp.element}`;
    chip.draggable = true;
    chip.dataset.spell = sid;
    chip.dataset.origin = 'pool';
    chip.innerHTML = `
      ${sid}
      <small>(${sp.damage} · ${sp.element})</small>
    `;
    list.appendChild(chip);
  });

  return pool;
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

function setUpDragAndDrop(blockEl, char) {
  let dragData = null;

  blockEl.addEventListener('dragstart', (e) => {
    const chip = e.target.closest('.spell-chip');
    const slot = e.target.closest('.wand-slot-item.filled');
    if (chip) {
      dragData = {origin: 'pool', spell: chip.dataset.spell};
      e.dataTransfer.effectAllowed = 'copy';
      e.dataTransfer.setData('text/plain', chip.dataset.spell);
    } else if (slot) {
      const wIdx = Number(slot.dataset.wandIdx);
      const sIdx = Number(slot.dataset.slotIdx);

      dragData = {
        origin: 'slot',
        spell: char.wands[wIdx].spells[sIdx],
        wandIdx: wIdx,
        slotIdx: sIdx,
      };

      e.dataTrasnfer.effectAllowed = 'move';
      e.dataTrasnfer.setData('text/plain', dragData.spell);
    }
  });
  
  blockEl.addEventListener('dragover', (e) => {
    const slot = e.target.closest('.wand-slot-item');
    if (!slot) return;

    e.preventDefault();
    e.dataTransfer.dropEffect = dragData?.origin === 'slot' ? 'move' : 'copy';
    slot.classList.add('drag-over');
  });

  blockEl.addEventListener('dragleave', (e) => {
    const slot = e.target.closest('.wand-slot-item');
    if (slot) slot.classList.remove('drag-over');
  });

  blockEl.addEventListener('drop',  (e) => {
    const slot = e.target.closest('.wand-slot-item');
    if (!slot || !dragData) return;
    e.preventDefault();
    slot.classList.remove('drag-over');

    const targetWand = Number(slot.dataset.wandIdx);
    const targetSlot = Number(slot.dataset.slotIdx);
    const wand = char.wands[targetWand];

    if (dragData.origin === 'pool') {
      insertSpell(char, wand, dragData.spell, targetSlot);
    } else {
      moveSpell(char, dragData.wandIdx, dragData.slotIdx, targetWand, targetSlot);
    }

    dragData = null;
    renderAllWands();
  });

  blockEl.addEventListener('dragend', () => {
    dragData = null;
    blockEl.querySelectorAll('.drag-over').forEach(el => el.classList.remove('drag-over'));
  });
}

function insertSpell(char, wand, spellId, targetIdx) {
  if (wand.spells.length >= wand.slots) {
    alert('Esta varita está llena. Amplía los espacios o elimina un hechizo.');
    return;
  }

  if (targetIdx < wand.spells.length) {
    wand.spells.splice(targetIdx, 0, spellId);
  } else {
    wand.spells.push(spellId);
  }
}

function moveSpell(char, fromWand, fromSlot, toWand, toSlot) {
  const srcWand = char.wands[fromWand];
  const dstWand = char.wands[toWand];
  const spell = srcWand.spells[fromSlot];
  if (!spell) return;

  srcWand.spells.splice(fromSlot, 1);

  if (fromWand === toWand && fromSlot < toSlot) {
    toSlot--;
  }
  if (toSlot >= dstWand.spells.length) {
    dstWand.spells.push(spell);
  } else {
    dstWand.spells.splice(toSlot, 0, spell);
  }
}

function validateStep3() {
  for (const team of ['A', 'B']) {
    for (const char of State.teams[team]) {
      if (char.aiWands) continue;   // la IA s'encarrega, no validem

      if (char.wands.length === 0) {
        alert(`${char.playerType} (Equipo ${team}) no tiene ninguna varita.`);
        return false;
      }
      const nonEmpty = char.wands.filter(w => w.spells.length > 0);
      if (nonEmpty.length === 0) {
        alert(`${char.playerType} (Equipo ${team}) no tiene hechizos en ninguna varita.`);
        return false;
      }
      for (const w of char.wands) {
        if (w.spells.length > w.slots) {
          alert(`Una varita de ${char.playerType} excede sus espacios.`);
          return false;
        }
      }
    }
  }
  return true;
}