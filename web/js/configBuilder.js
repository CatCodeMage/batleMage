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

    const wandsCalculation = char.aiWands ? [] : char.wands.map((w, wIdx) => {
      let totalMana = 0;
      let totalCastDelay = 0.15;
      let totalRecharge = 0.80;
      let totalDispersion = 2.0;

      w.spells.forEach(spellId => {
        const sp = Data.spells[spellId];

        if (sp) {
          totalMana += (sp.manaCost || 0);
          totalCastDelay += (sp.castDelay || 0);
          totalRecharge += (sp.rechargeTime || 0);
          totalDispersion += (sp.dispersion || 0);
        }
      });

      return {
        nombre: `Varita ${wIdx + 1}`,
        castDelay: totalCastDelay,
        rechargeTime: totalRecharge,
        manaMax: totalMana,
        manaActual: totalMana,
        recargaMana: 15.0,
        capacidad: w.slots,
        dispersion: totalDispersion,
        ranuras: [...w.spells],
        indiceActual: 0,
        tiempoEspera: 0
      };
    })
  
    playersObj[playerKey] = {
      position: pos,
      type: char.playerType,
      wands: wandsCalculation,
      basicSpell: char.basicSpell || null,
      aiWands: char.aiWands || false
    };

    playerCounter++;
  });



  return {
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