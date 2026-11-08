function buildCombatConfig(mode) {
  return {
    mode,
    combatType: State.combat.combatType,
    map: State.mapId,
    teams: {
      A: State.teams.A.map(cleanChar),
      B: State.teams.B.map(cleanChar),
    },
  };
}

function cleanChar(c) {
  return {
    playerType: c.playerType,
    entity: c.entity,
    wands: c.wands.filter(w => w.wandId),
    basicSpell: c.basicSpell || null,
  };
}