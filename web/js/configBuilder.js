function buildCombatConfig(mode) {
  const playersObj = {};
  let playerCounter = 1;

  const getSpawnPos = (index, total) => {
    const radius = 20;
    const angle = (index / total) * Math.PI * 2;
    return [
      Math.round(Math.cos(angle) * radius),
      1,
      Math.round(Math.sin(angle) * radius)
    ];
  };

  const allChars = [...State.teams.A, ...State.teams.B];

  allChars.forEach((char, index) => {
    const playerKey = `player_${playerCounter}`;

    let pos = getSpawnPos(index, allChars.length);
    if (State.spawn?.mode === 'manual' && State.spawn.positions[`${char.team}${char.idx}`]) {
        pos = State.spawn.positions[`${char.team}${char.idx}`];
    }
  
    playersObj[playerKey] = {
      position: pos,
      type: char.playerType,
      wands: char.aiWands ? [] : char.wands.map(w => ({
        slots: w.slots,
        spells: [...w.spells]
      })),
      basicSpell: char.basicSpell || null,
      aiWands: char.aiWands || false
    };

    playerCounter++;
  });

  return {
    "frames": "",
    "map": "../db/map.json",
    "metadata": {
      "combatType": State.combat.combatType,
      "mode": mode
    },
    "players": playersObj,
    "spells": "../db/spells.json"
  };
}

function cleanChar(c) {
  return {
    playerType: c.playerType,
    entity: c.entity,
    aiWands: c.aiWands || false,
    wands: c.aiWands ? [] : c.wands.map(w => ({
      slots: w.slots,
      spells: [...w.spells]
    })),
    basicSpell: c.basicSpell || null,
  };
}