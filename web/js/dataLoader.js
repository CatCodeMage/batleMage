const Data = {
  players: null,
  spells: null,
  map: null,
  entities: null,
  assets: null,
  hitbox: null,
  wands: null,
};

async function loadAllData() {
    const files = {
    players:  './db/players.json',
    spells:   './db/spells.json',
    map:      './db/map.json',
    entities: './db/entities.json',
    assets:   './db/assets.json',
    hitbox:   './db/hitbox.json',
    wands:    './db/wands.json',
  };    

const entries = await Promise.all(
    Object.entries(files).map(async ([key, url]) => {
        const res = await fetch(url);

        if (!res.ok) {
            throw new Error(`No s'ha pogut carregar ${url}`);
        }

        return [key, await res.json()];
    })
);

for (const [key, val] of entries) {
    Data[key] = val;
}

return Data;
}