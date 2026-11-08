function renderStep2() {
  const root = document.getElementById('step-2');
  const players = Object.entries(Data.players);

  root.innerHTML = `
    <h2>Asigna personajes a los equipos</h2>
    <div class="teams">
      <div class="team team-a" id="team-a"></div>
      <div class="team team-b" id="team-b"></div>
    </div>
    <h3>Personajes disponibles</h3>
    <div class="player-pool">
      ${players.map(([id, p]) => `
        <div class="player-card" data-player="${id}">
          <strong>${id}</strong>
          <small>HP ${p.stats.maxHealth} · MP ${p.stats.maxMana} · SPD ${p.stats.moveSpeed}</small>
        </div>
      `).join('')}
    </div>
  `;

  renderTeamSlots('A', State.combat.teamSizeA);
  renderTeamSlots('B', State.combat.teamSizeB);

  root.querySelectorAll('.player-card').forEach(card => {
    card.addEventListener('click', () => {
      const id = card.dataset.player;
      assignToFirstEmptySlot(id);
      renderStep2();
    });
  });
}

function renderTeamSlots(team, size) {
  const container = document.getElementById(`team-${team.toLowerCase()}`);
  const assigned = State.teams[team];

  container.innerHTML = `<h3>Equipo ${team}</h3>`;

  for (let i = 0; i < size; i++) {
    const p = assigned[i];
    container.innerHTML += `
      <div class="slot ${p ? 'filled' : 'empty'}" data-team="${team}" data-idx="${i}">
        ${p ? `
          <span>${p.playerType}</span>
          <button class="btn-remove" data-team="${team}" data-idx="${i}">✕</button>
        ` : `<em>Vacío</em>`}
      </div>`;
  }

  container.querySelectorAll('.btn-remove').forEach(btn => {
    btn.addEventListener('click', () => {
      State.teams[btn.dataset.team].splice(Number(btn.dataset.idx), 1);
      renderStep2();
    });
  });
}

function assignToFirstEmptySlot(playerType) {
  for (const team of ['A', 'B']) {
    const size = team === 'A' ? State.combat.teamSizeA : State.combat.teamSizeB;
    if (State.teams[team].length < size) {
      State.teams[team].push({
        playerType,
        entity: team === 'A' ? 'player_red' : 'player_blue',
        wands: [
          { wandId: null, spells: [] },
          { wandId: null, spells: [] },
          { wandId: null, spells: [] },
          { wandId: null, spells: [] },
        ],
        basicSpell: null,
      });
      return;
    }
  }
  alert('Ambos equipos están llenos');
}

function validateStep2() {
  const okA = State.teams.A.length === State.combat.teamSizeA;
  const okB = State.teams.B.length === State.combat.teamSizeB;
  if (!okA || !okB) { alert('Completa todos los slots de ambos equipos'); return false; }
  return true;
}