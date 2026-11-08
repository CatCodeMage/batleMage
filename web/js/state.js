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
  mapId: '/db/map.json',
  mode: null,
};

function resetState() {
  State.currentStep = 1;
  State.combat = { combatType: null, teamSizeA: 1, teamSizeB: 1 };
  State.teams = { A: [], B: [] };
  State.mapId = 'map_arena_01';
  State.mode = null;
}