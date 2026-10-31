#include "CombatSimulation.h"


// Nombres de db/entities.json: generateCombat los usa para saber que modelo dibujar
constexpr const char* REPLAY_PLAYER_ENTITY = "players";
constexpr const char* REPLAY_SPELL_ENTITY = "spell";
constexpr const char* REPLAY_FORMAT_VERSION = "1.7";   // la de combate_chorras.json

using json = nlohmann::json;

constexpr size_t SPELL_POOL_SIZE = 64;   // hechizos en vuelo a la vez, como maximo
constexpr size_t PLAYERS_IN_1V1 = 2;

// Todos los hechizos se dibujan con la misma esfera de radio 1 a escala 0,5 (entities.json, "spell").
// Chocan con ese radio, no con el "radious" de spells.json (hasta 7), para que el golpe se vea donde ocurre
constexpr float SPELL_HIT_RADIUS = 0.5f;

CombatSimulation::CombatSimulation() : m_spells(SPELL_POOL_SIZE) {}

static CylinderCollider colliderOf(const Personaje& player) {
    return { player.getPosicion(), player.getRadio(), player.getRadio() };
}

static Vector3 readPosition(const json& player) {
    const json& position = player.at("position");
    return Vector3(position.at(0).get<float>(), position.at(1).get<float>(), position.at(2).get<float>());
}

// Por la vida y no por MUERTO: yaTieneEstado no es const
static bool isAlive(const Personaje& player) {
    return player.getVida() > 0.0f;
}

// La IA guarda en la rotacion el vector hacia donde mira, no angulos.
// El replay quiere el giro en Y en radianes, con 0 mirando a +Z
static float yawOf(const Vector3& facingDirection) {
    return std::atan2(facingDirection.x, facingDirection.z);
}

// [[x, y, z], [giroX, giroY, giroZ], "entidad"]
static json replayEntity(const Vector3& position, float yaw, const char* entityName) {
    return json::array({
        json::array({ position.x, position.y, position.z }),
        json::array({ 0.0f, yaw, 0.0f }),
        entityName });
}

void CombatSimulation::setup(const std::filesystem::path& combatConfigFile) {
    readSpells(m_spells);   // antes que la IA: connectOpponents le pasa el catalogo
    createPlayers(combatConfigFile);
    connectOpponents();
    recordFrame();
}

void CombatSimulation::step() {
    for (const std::unique_ptr<Personaje>& player : m_players) {
        player->update(SECONDS_PER_TICK);   // la IA decide y mueve al jugador
    }

    m_spells.update(SECONDS_PER_TICK);      // aun no hay hechizos en vuelo: llegan en S3
    applySpellHits();
    ++m_tick;
    recordFrame();
}

void CombatSimulation::createPlayers(const std::filesystem::path& combatConfigFile) {
    std::ifstream in(combatConfigFile);
    if (!in) throw std::runtime_error("no se puede abrir " + combatConfigFile.string());
    const json config = json::parse(in);

    int nextPlayerId = 1;
    for (const json& player : config.at("players"))   // player_1, player_2 (nlohmann los ordena)
    {
        m_players.push_back(std::make_unique<Personaje>(nextPlayerId++, readPosition(player)));
        m_players.back()->setGestorHechizos(&m_spells);   // el personaje crea sus propios hechizos al lanzar

        const std::string playerType = player.value("type", "sphere");
        if (!m_players.back()->cargarDesdeJson(dataPath("db/players.json").string(), playerType))
            throw std::runtime_error(combatConfigFile.string() + ": tipo de mago desconocido \"" + playerType + "\"");
    }

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
        m_controllers.back()->cargarCatalogoDesdeGestor(m_spells);   // sin catalogo la IA no tiene que lanzar y solo huye
    }
}

size_t CombatSimulation::opponentIndexOf(size_t playerIndex) const {
    return (playerIndex + 1) % m_players.size();
}
 
void CombatSimulation::applySpellHits() {
    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (spell->IsActive()) tryHitAnyPlayer(*spell);
    }
}

// Un hechizo golpea como mucho a un mago y desaparece
bool CombatSimulation::tryHitAnyPlayer(Hechizo& spell) {
    const SphereCollider spellCollider{ spell.getPosition(), SPELL_HIT_RADIUS };
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

void CombatSimulation::recordFrame() {
    json frame = json::array();
    for (const std::unique_ptr<Personaje>& player : m_players)
        frame.push_back(replayEntity(player->getPosicion(), player->getRotacion().y, REPLAY_PLAYER_ENTITY));

    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (!spell->IsActive()) continue;
        frame.push_back(replayEntity(spell->getPosition(), yawOf(spell->getDirection()), REPLAY_SPELL_ENTITY));
    }
    m_replayFrames.push_back(std::move(frame));
}

// Como las claves de combat.json: "player_1", "player_2"; "none" si no gana nadie
std::string CombatSimulation::describeWinner() const {
    const int winnerId = getWinnerId();
    return winnerId == 0 ? "none" : "player_" + std::to_string(winnerId);
}

void CombatSimulation::writeReplay(const std::filesystem::path& replayFile) const {
    const json replay = {
        { "metadata",{
            { "version", REPLAY_FORMAT_VERSION },
            { "iterations", m_replayFrames.size() },
            { "winner", describeWinner() },
            { "ticksPerSecond", TICKS_PER_SECOND },
            { "rotationOrder", "XYZ" },
            { "rotationUnit", "radians" } } },
        { "frames", m_replayFrames } };

    std::ofstream out(replayFile);
    if (!out) throw std::runtime_error("no se puede escribir " + replayFile.string());
    out << replay.dump();   // sin sangrado: con cientos de frames, sangrado ocuparia varias veces mas
    std::cout << "[SIM] replay escrito en " << replayFile.string() << ": " << m_replayFrames.size() << " frames\n";
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