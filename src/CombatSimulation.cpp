#include "CombatSimulation.h"
#include "jsonManager.h"   // readSpells

using json = nlohmann::json;

constexpr size_t SPELL_POOL_SIZE = 64;   // hechizos en vuelo a la vez, como maximo
constexpr size_t PLAYERS_IN_1V1 = 2;
constexpr const char* BASIC_SPELL_NAME = "fire_ball";   // hasta que la IA elija hechizo, todos lanzan este
constexpr float SPELL_SPAWN_DISTANCE = 1.0f;            // la bola sale delante del mago, no dentro

// Cuerpo del mago para las colisiones: provisional, hasta tener el modelo
constexpr float PLAYER_COLLIDER_RADIUS = 0.5f;
constexpr float PLAYER_COLLIDER_HALF_HEIGHT = 1.0f;
constexpr float SPELL_SPAWN_MARGIN = 0.1f;

CombatSimulation::CombatSimulation() : m_spells(SPELL_POOL_SIZE) {}

static Vector3 readPosition(const json& player) {
    const json& position = player.at("position");
    return Vector3(position.at(0).get<float>(), position.at(1).get<float>(), position.at(2).get<float>());
}

// Por la vida y no por MUERTO: yaTieneEstado no es const
static bool isAlive(const Personaje& player) {
    return player.getVida() > 0.0f;
}

static CylinderCollider colliderOf(const Personaje& player) {
    return { player.getPosicion(), PLAYER_COLLIDER_RADIUS, PLAYER_COLLIDER_HALF_HEIGHT };
}

void CombatSimulation::setup(const std::filesystem::path& combatConfigFile) {
    createPlayers(combatConfigFile);
    connectOpponents();
    readSpells(m_spells);
    m_basicSpellId = findSpellIdByName(BASIC_SPELL_NAME);
    m_wasCasting.assign(m_players.size(), false);
    std::cout << "[SIM] hechizos en el catalogo: " << m_spells.getCatalogoHechizos().size() << "\n";
}

void CombatSimulation::step() {
    for (const std::unique_ptr<Personaje>& player : m_players) {
        player->update(SECONDS_PER_TICK);   // la IA decide y mueve al jugador
    }

    spawnSpellsForNewCasts();

    m_spells.update(SECONDS_PER_TICK);      // aun no hay hechizos en vuelo: llegan en S3
    applySpellHits();
    ++m_tick;
}

void CombatSimulation::createPlayers(const std::filesystem::path& combatConfigFile) {
    std::ifstream in(combatConfigFile);
    if (!in) throw std::runtime_error("no se puede abrir " + combatConfigFile.string());
    const json config = json::parse(in);

    int nextPlayerId = 1;
    for (const json& player : config.at("players"))   // player_1, player_2 (nlohmann los ordena)
        m_players.push_back(std::make_unique<Personaje>(nextPlayerId++, readPosition(player)));

    if (m_players.size() != PLAYERS_IN_1V1)
        throw std::runtime_error(combatConfigFile.string() + ": se esperaban 2 jugadores y hay " +
            std::to_string(m_players.size()));
}

// 1v1: la IA de cada jugador persigue al otro
void CombatSimulation::connectOpponents() {
    for (size_t playerIndex = 0; playerIndex < m_players.size(); ++playerIndex) {
        Personaje* opponent = m_players[opponentIndexOf(playerIndex)].get();
        m_controllers.push_back(std::make_unique<ControladorMaquinaEstados>(opponent));
        m_players[playerIndex]->setControlador(m_controllers.back().get());
    }
}

size_t CombatSimulation::opponentIndexOf(size_t playerIndex) const {
    return (playerIndex + 1) % m_players.size();
}

// La IA no avisa cuando lanza: deja CASTEANDO puesto 0,3 s.
// Un lanzamiento nuevo es el tick en que CASTEANDO aparece.
void CombatSimulation::spawnSpellsForNewCasts() {
    for (size_t playerIndex = 0; playerIndex < m_players.size(); ++playerIndex) {
        const bool isCasting = m_players[playerIndex]->yaTieneEstado(EstadoPersonaje::CASTEANDO);
        const bool startedCasting = isCasting && !m_wasCasting[playerIndex];
        if (startedCasting) spawnBasicSpell(playerIndex);
        m_wasCasting[playerIndex] = isCasting;
    }
}

void CombatSimulation::spawnBasicSpell(size_t casterIndex) {
    const Personaje& caster = *m_players[casterIndex];
    const Vector3 casterPosition = caster.getPosicion();
    const Vector3 targetPosition = m_players[opponentIndexOf(casterIndex)]->getPosicion();
    const Vector3 direction = glm::normalize(targetPosition - casterPosition);

    // Hechizo no guarda quien lo lanzo: si nace fuera del mago y va recto alejandose, no puede darle
    const float spellRadius = m_spells.getCatalogoHechizos().at(m_basicSpellId).radius;
    const float spawnDistance = spellRadius + PLAYER_COLLIDER_RADIUS + SPELL_SPAWN_MARGIN;
    m_spells.spawnSpell(m_basicSpellId, casterPosition + direction * spawnDistance, direction);
    std::cout << "[SIM] tick " << m_tick << ": jugador " << caster.getId() << " lanza " << BASIC_SPELL_NAME << "\n";
}

// Por nombre: readSpells numera en orden alfabetico (nlohmann ordena las claves), fire_ball es el 4
int CombatSimulation::findSpellIdByName(const std::string& spellName) const {
    for (const auto& [spellId, spellTemplate] : m_spells.getCatalogoHechizos())
        if (spellTemplate.definition.name == spellName) return spellId;
    throw std::runtime_error("db/spells.json no tiene el hechizo \"" + spellName + "\"");
}

void CombatSimulation::applySpellHits() {
    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (spell->IsActive()) tryHitAnyPlayer(*spell);
    }
}

// Un hechizo golpea como mucho a un mago y desaparece
bool CombatSimulation::tryHitAnyPlayer(Hechizo& spell) {
    const SphereCollider spellCollider{ spell.getPosition(), spell.getRadius() };
    const SpellDefinitionComponent& definition = m_spells.getCatalogoHechizos().at(spell.getId()).definition;

    for (const std::unique_ptr<Personaje>& player : m_players) {
        if (!isAlive(*player)) continue;
        if (m_combat.procesarImpacto(spellCollider, colliderOf(*player), definition, *player)) {
            m_spells.destroySpell(&spell);
            return true;
        }
    }
    return false;
}

size_t CombatSimulation::countAlivePlayers() const {
    size_t alivePlayers = 0;
    for (const std::unique_ptr<Personaje>& player : m_players)
        alivePlayers += (size_t) isAlive(*player);
    return alivePlayers;
}

bool CombatSimulation::isFinished() const {
    const bool atMostOneStanding = countAlivePlayers() <= 1;
    const bool outOfTime = m_tick >= MAX_COMBAT_SECONDS * TICKS_PER_SECOND;
    return atMostOneStanding || outOfTime;
}

int CombatSimulation::getWinnerId() const {
    if (countAlivePlayers() != 1) return 0;
    for (const std::unique_ptr<Personaje>& player : m_players)
        if (isAlive(*player)) return player->getId();
    return 0;
}

void CombatSimulation::printResult() const {
    std::cout << "[SIM] fin en el tick " << m_tick << " (" << (float)m_tick / TICKS_PER_SECOND << " s): ";
    const int winnerId = getWinnerId();
    if (winnerId == 0) std::cout << "sin ganador\n";
    else               std::cout << "gana el jugador " << winnerId << "\n";
}

static const char* describeAIState(EstadoIA state) {
    switch (state) {
    case EstadoIA::ESPERANDO:    return "esperando";
    case EstadoIA::PERSIGUIENDO: return "persiguiendo";
    case EstadoIA::ATACANDO:     return "atacando";
    case EstadoIA::HUYENDO:      return "huyendo";
    }
    return "desconocido";
}

// getActiveHechizos devuelve toda la pool, tambien los inactivos: hay que filtrar
void CombatSimulation::printSpellsInFlight() const {
    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (!spell->IsActive()) continue;
        const glm::vec3 position = spell->getPosition();
        std::cout << "      " << spell->getName()
            << " en (" << position.x << ", " << position.y << ", " << position.z << ")\n";
    }
}

void CombatSimulation::printState() const {
    std::cout << "[SIM] tick " << m_tick << " (" << (float)m_tick / TICKS_PER_SECOND << " s)\n";
    for (size_t playerIndex = 0; playerIndex < m_players.size(); ++playerIndex) {
        const Personaje& player = *m_players[playerIndex];
        const Vector3 position = player.getPosicion();
        std::cout << "      jugador " << player.getId()
            << " en (" << position.x << ", " << position.y << ", " << position.z << ")"
            << " vida " << player.getVida() << " mana " << player.getMana()
            << " IA " << describeAIState(m_controllers[playerIndex]->getEstadoIA()) << "\n";
    }
    printSpellsInFlight();
}