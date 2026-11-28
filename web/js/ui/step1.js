const COMBAT_MODES = [
  { id: '1v1',           label: '1 vs 1',          desc: 'Duelo clásico' },
  { id: '2v2',           label: '2 vs 2',          desc: 'Duelo por parejas' },
  { id: '1vhorde',       label: '1 vs Horda',      desc: 'Un héroe contra N enemigos' },
  { id: 'hordevshorde',  label: 'Horda vs Horda',  desc: 'Dos grupos enfrentados' },
];

function renderStep1() {
  const root = document.getElementById('step-1');
  root.innerHTML = `
    <h2>Elige el tipo de combate</h2>
    <div class="mode-grid">
      ${COMBAT_MODES.map(m => `
        <div class="mode-card ${State.combat.combatType === m.id ? 'selected' : ''}"
             data-mode="${m.id}">
          <h3>${m.label}</h3>
          <p>${m.desc}</p>
        </div>
      `).join('')}
    </div>
    <div id="mode-extra"></div>
  `;

  root.querySelectorAll('.mode-card').forEach(card => {
    card.addEventListener('click', () => {
      State.combat.combatType = card.dataset.mode;
      renderStep1();
    });
  });

  renderModeExtra();
}

function renderModeExtra() {
  const extra = document.getElementById('mode-extra');
  const t = State.combat.combatType;
  if (!t) { extra.innerHTML = ''; return; }

  if (t === '1vhorde') {
    extra.innerHTML = `
      <label>Enemigos en la horda:
        <input type="number" id="horde-size" min="2" max="10"
               value="${State.combat.teamSizeB}" />
      </label>`;
    document.getElementById('horde-size').addEventListener('input', e => {
      State.combat.teamSizeA = 1;
      State.combat.teamSizeB = Number(e.target.value) || 2;
    });
  }

  if (t === 'hordevshorde') {
    extra.innerHTML = `
      <label>Equipo A: <input type="number" id="size-a" min="2" max="10" value="${State.combat.teamSizeA}" /></label>
      <label>Equipo B: <input type="number" id="size-b" min="2" max="10" value="${State.combat.teamSizeB}" /></label>`;
    document.getElementById('size-a').addEventListener('input', e => {
      State.combat.teamSizeA = Number(e.target.value) || 2;
    });
    document.getElementById('size-b').addEventListener('input', e => {
      State.combat.teamSizeB = Number(e.target.value) || 2;
    });
  }
}

function validateStep1() {
  if (!State.combat.combatType) { alert('Elige un tipo de combate'); return false; }

  if (State.combat.combatType === '1v1') {
    State.combat.teamSizeA = 1; State.combat.teamSizeB = 1;
  }
  if (State.combat.combatType === '2v2') {
    State.combat.teamSizeA = 2; State.combat.teamSizeB = 2;
  }
  if (State.combat.combatType === '1vhorde') {
    State.combat.teamSizeA = 1;
  }
  return true;
}