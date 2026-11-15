const State = {
  currentStep: 1,
  combat: {
    combatType: null,
    teamSizeA: 1,
    teamSizeB: 1,
  },
  teams: {
    A: [],
    B: [],
  },
  mapId: 'map_arena_01',
  spawn: { mode: 'auto', positions: {} },
  mode: null,
};

function resetState() {
  State.currentStep = 1;
  State.combat = { combatType: null, teamSizeA: 1, teamSizeB: 1 };
  State.players = { player_1: [], player_2: [] };
  State.mapId = '../db/map.js';
  State.mode = null;
}