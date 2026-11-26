#include "CombatSimulation.h"
#include "sistema_particulas/logica/ParticleJson.h"


// Nombres de db/entities.json: generateCombat los usa para saber que modelo dibujar
constexpr const char* REPLAY_HIT_ENTITY = "hit";

// constantes para contar los frames en que esta en estado "hit"
constexpr float HIT_EFFECT_SECONDS = 0.3f;
constexpr int HIT_EFFECT_FRAMES =
static_cast<int>(
    CombatSimulation::TICKS_PER_SECOND *
    HIT_EFFECT_SECONDS
    );

using json = nlohmann::json;

constexpr size_t SPELL_POOL_SIZE = 64;   // hechizos en vuelo a la vez, como maximo
constexpr size_t PLAYERS_IN_1V1 = 2;

// Todos los hechizos se dibujan con la misma esfera de radio 1 a escala 0,5
// (entities.json, "spell").
// Chocan con ese radio, no con el "radious" de spells.json (hasta 7),
// para que el golpe se vea donde ocurre
constexpr float SPELL_HIT_RADIUS = 0.5f;

constexpr const char* REPLAY_PLAYER_ENTITIES[] = {
    "player_red",
    "player_blue"
};

constexpr const char* REPLAY_DEAD_ENTITIES[] = {
    "dead_red",
    "dead_blue"
};


CombatSimulation::CombatSimulation() : m_spells(SPELL_POOL_SIZE, m_ids) {}

static bool isAlive(const Personaje& player) {
    return player.getVida() > 0.0f;
}


const char* CombatSimulation::replayEntityOfPlayer(
    size_t playerIndex) const
{
    if (!isAlive(*m_players[playerIndex]))
        return REPLAY_DEAD_ENTITIES[playerIndex];

    if (m_hitFrames[playerIndex] > 0)
        return REPLAY_HIT_ENTITY;

    return REPLAY_PLAYER_ENTITIES[playerIndex];
}


static CylinderCollider colliderOf(
    const Personaje& player)
{
    return {
        player.getPosicion(),
        player.getRadio(),
        player.getRadio()
    };
}


static Vector3 readPosition(
    const json& player)
{
    const json& position =
        player.at("position");

    return Vector3(
        position.at(0).get<float>(),
        position.at(1).get<float>(),
        position.at(2).get<float>()
    );
}


// La IA guarda en la rotacion el vector hacia donde mira, no angulos.
// El replay quiere el giro en Y en radianes, con 0 mirando a +Z
static float yawOf(
    const Vector3& facingDirection)
{
    return std::atan2(
        facingDirection.x,
        facingDirection.z
    );
}

static float pitchOf(
    const Vector3& direction)
{
    const float horiz = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    if (horiz < 0.0001f && std::abs(direction.y) < 0.0001f)
        return 0.0f;
    return std::atan2(-direction.y, horiz);
}


// [[x, y, z], [giroX, giroY, giroZ], "entidad"]
static json replayEntity(
    const Vector3& position,
    float yaw,
    const char* entityName)
{
    return json::array({
        json::array({
            position.x,
            position.y,
            position.z
        }),

        json::array({
            0.0f,
            yaw,
            0.0f
        }),

        entityName
        });
}

static json replaySpellEntity(
    const Vector3& position,
    float pitch,
    float yaw,
    const char* entityName)
{
    return json::array({
        json::array({
            position.x,
            position.y,
            position.z
        }),

        json::array({
            pitch,
            yaw,
            0.0f
        }),

        entityName
        });
}


void CombatSimulation::setup(
    const std::filesystem::path& combatConfigFile)
{
    // antes que la IA:
    // connectOpponents le pasa el catalogo
    readSpells(m_spells);

    createPlayers(combatConfigFile);
    connectOpponents();
    m_hitFrames.assign(m_players.size(), 0);
    m_playerReplayStates.resize(m_players.size());
    recordFrame();
}


void CombatSimulation::recordEnding()
{
    constexpr float ENDING_SECONDS = 4.0f;

    const int endingTicks =
        static_cast<int>(
            ENDING_SECONDS *
            TICKS_PER_SECOND
            );

    // Para que no se vean una vez haya muerto un personaje
    m_spells.emptySpells();

    for (
        int endingTick = 0;
        endingTick < endingTicks;
        ++endingTick)
    {
        countDownHitEffects();
        recordFrame(true);
    }
}


void CombatSimulation::step()
{
    for (
        const std::unique_ptr<Personaje>& player :
        m_players)
    {
        // la IA decide y mueve al jugador
        player->update(
            SECONDS_PER_TICK
        );
    }

    // mueve los hechizos en vuelo
    m_spells.update(
        SECONDS_PER_TICK
    );

    applySpellHits();

    ++m_tick;

    recordFrame();

    countDownHitEffects();
}

std::vector<Varita> CombatSimulation::createWands() {
    std::vector<Varita> aiWands;
    int numWands = 1 + rand() % 4;

    const auto& catalog = m_spells.getCatalogoHechizos();
    if (catalog.empty()) {
        return aiWands;
    }
    for (int i = 0; i < numWands; i++)
    {
        int capacity = 4 + rand() % 7;
        float totalMana = 0.0f;
        float totalCastDelay = 0.20f;
        float totalRecharge = 0.80f;
        float totalDispersion = 2.0f;
        std::vector<int> ranuras;

        for (int j = 0; j < capacity; j++)
        {
            int spellId = rand() % catalog.size();
            ranuras.push_back(spellId);

            const auto& def = catalog.at(spellId).definition;
            totalMana += def.cost;
        }

        Varita v("Varita " + std::to_string(i + 1), totalCastDelay, totalRecharge, totalMana, 15.0f, capacity, totalDispersion);

        aiWands.push_back(v);
    }

    return aiWands;

}

void CombatSimulation::createPlayers(const std::filesystem::path& combatConfigFile) {
    std::ifstream in(combatConfigFile);
    if (!in) throw std::runtime_error("no se puede abrir " + combatConfigFile.string());
    const json config = json::parse(in);

    std::unordered_map<std::string, int> nameToId;
    for (const auto& [id, t] : m_spells.getCatalogoHechizos()) {
        nameToId[t.definition.name] = t.definition.id;
    }

    for (const json& player : config.at("players"))   // player_1, player_2 (nlohmann los ordena)
    {
        std::vector<Varita> playerWands;
        if (player.at("aiWands")) {
            playerWands = createWands(); // Generara varetes de forma random, caldria modificar-ho en un futur
        }
        else {
            //esperar a que player estigui completa
            for (const auto& wand : player.at("wands"))
            {
                Varita v(wand.value("nombre", "Varita"),
                    wand.value("castDelay", 0.2f),
                    wand.value("rechargeTime", 0.8f),
                    wand.value("manaMax", 100.0f),
                    wand.value("recargaMana", 15.0f),
                    wand.value("capacidad", 4),
                    wand.value("dispersion", 2.0f));

                std::vector<int> ranuras;
                for (const auto& spellName : wand.at("ranuras"))
                {
                    std::string name = spellName.get<std::string>();
                    ranuras.push_back(nameToId[name]);
                }

                playerWands.push_back(v);
            }
        }
        //m_players.back()->setGestorHechizos(&m_spells);   // el personaje crea sus propios hechizos al lanzar
        m_players.push_back(std::make_unique<Personaje>(m_ids.nuevo(), readPosition(player), 1.0f, &m_spells, playerWands));
        // TODO: Afegir més obj, de moment no es toca
        const std::string playerType = player.value("type", "sphere"); 
        if (!m_players.back()->cargarDesdeJson(dataPath("db/players.json").string(), playerType))
            throw std::runtime_error(combatConfigFile.string() + ": tipo de mago desconocido \"" + playerType + "\"");
    }

    if (m_players.size() != PLAYERS_IN_1V1)
    {
        throw std::runtime_error(
            combatConfigFile.string() +
            ": se esperaban 2 jugadores y hay " +
            std::to_string(
                m_players.size()
            )
        );
    }
}


// 1v1: la IA de cada jugador persigue al otro
void CombatSimulation::connectOpponents()
{
    for (
        size_t playerIndex = 0;
        playerIndex < m_players.size();
        ++playerIndex)
    {
        Personaje* opponent =
            m_players[
                opponentIndexOf(
                    playerIndex
                )
            ].get();

        m_controllers.push_back(
            std::make_unique<
            ControladorMaquinaEstados
            >(opponent)
        );

        m_players[playerIndex]->
            setControlador(
                m_controllers.back().get()
            );

        // sin catalogo la IA no tiene que lanzar
        // y solo huye
        m_controllers.back()->
            cargarCatalogoDesdeGestor(
                m_spells
            );
    }
}


size_t CombatSimulation::opponentIndexOf(
    size_t playerIndex) const
{
    return (
        playerIndex + 1
        ) % m_players.size();
}


void CombatSimulation::applySpellHits()
{
    constexpr float GROUND_HEIGHT =
        0.0f;

    for (
        const std::unique_ptr<Hechizo>& spell :
        m_spells.getActiveHechizos())
    {
        if (!spell->IsActive())
        {
            continue;
        }

        // Primero comprobar impacto contra personajes
        if (tryHitAnyPlayer(*spell))
        {
            continue;
        }

        const SphereCollider spellCollider{
            spell->getPosition(),
            SPELL_HIT_RADIUS
        };
        const CollisionInfo info =
            m_collisionManager.getGroundCollisionInfo(spellCollider, GROUND_HEIGHT);
        if (info.collision)
        {
            const bool wasApproaching = glm::dot(spell->getVelocityVector(), info.normal) < 0.0f;
            const glm::vec3 hitPos = spell->getPosition() + info.normal * (info.penetration - SPELL_HIT_RADIUS);
            const std::string spellName = spell->getName();

            m_spells.procesarChoqueSuperficie(spell.get(), info.normal, info.penetration);

            if (spellName == "ice_lance" || spellName == "fire_ball" || spellName == "lightning_bolt" || spellName == "earth_spike" || spellName == "wind_blade" || spellName == "water_jet" || spellName == "dagger_barrage")
            {
                const float fullDuration = (spellName == "fire_ball") ? 3.18f
                    : ((spellName == "lightning_bolt") ? 2.50f
                    : ((spellName == "earth_spike") ? 1.60f
                    : ((spellName == "wind_blade") ? 1.20f
                    : ((spellName == "water_jet") ? 1.15f
                    : ((spellName == "dagger_barrage") ? 1.10f : 1.90f)))));
                if (!spell->IsActive())
                {
                    recordImpactVFX(spellName, hitPos, info.normal, fullDuration);
                }
                else if (wasApproaching)
                {
                    recordImpactVFX(spellName + "_bounce", hitPos, info.normal, 0.40f);
                }
            }
        }
    }
}


void CombatSimulation::recordImpactVFX(
    const std::string& type,
    const glm::vec3& position,
    const glm::vec3& normal,
    float duration)
{
    m_replayImpacts.push_back({
        { "type", type },
        { "position", json::array({ position.x, position.y, position.z }) },
        { "normal", json::array({ normal.x, normal.y, normal.z }) },
        { "startTick", m_tick },
        { "duration", duration }
    });
}


// Un hechizo golpea como mucho a un mago y desaparece
bool CombatSimulation::tryHitAnyPlayer(
    Hechizo& spell)
{
    const SphereCollider spellCollider{
        spell.getPosition(),
        SPELL_HIT_RADIUS
    };

    const SpellDefinitionComponent& definition = spell.getDefinition();

    for (
        size_t playerIndex = 0;
        playerIndex < m_players.size();
        ++playerIndex)
    {
        Personaje& player =
            *m_players[playerIndex];

        if (!isAlive(player) || player.getId() == spell.getLanzador())
        {
            continue;
        }

        if (
            m_combat.procesarImpacto(
                spellCollider,
                colliderOf(player),
                definition,
                player))
        {
            // el golpeado se dibuja un momento
            // con el modelo "hit"
            m_hitFrames[playerIndex] =
                HIT_EFFECT_FRAMES;

            if (spell.getName() == "ice_lance" || spell.getName() == "fire_ball" || spell.getName() == "lightning_bolt" || spell.getName() == "earth_spike" || spell.getName() == "wind_blade" || spell.getName() == "water_jet" || spell.getName() == "dagger_barrage")
            {
                const glm::vec3 delta = spell.getPosition() - player.getPosicion();
                const glm::vec3 horiz(delta.x, 0.0f, delta.z);
                const glm::vec3 dir = spell.getDirection();
                const glm::vec3 hitNormal = glm::length(horiz) > 0.0001f
                    ? glm::normalize(horiz)
                    : (glm::length(dir) > 0.0001f ? -dir : glm::vec3(0.0f, 1.0f, 0.0f));
                const glm::vec3 hitPos = player.getPosicion() + glm::vec3(0.0f, delta.y, 0.0f) + hitNormal * player.getRadio();
                const float fullDuration = (spell.getName() == "fire_ball") ? 3.18f
                    : ((spell.getName() == "lightning_bolt") ? 2.50f
                    : ((spell.getName() == "earth_spike") ? 1.60f
                    : ((spell.getName() == "wind_blade") ? 1.20f
                    : ((spell.getName() == "water_jet") ? 1.15f
                    : ((spell.getName() == "dagger_barrage") ? 1.10f : 1.90f)))));
                recordImpactVFX(spell.getName(), hitPos, hitNormal, fullDuration);
            }

            m_spells.destroySpell(
                &spell
            );

            return true;
        }
    }

    return false;
}


size_t CombatSimulation::countAlivePlayers() const
{
    size_t alivePlayers = 0;

    for (
        const std::unique_ptr<Personaje>& player :
        m_players)
    {
        alivePlayers +=
            (size_t)isAlive(*player);
    }

    return alivePlayers;
}


bool CombatSimulation::isFinished() const
{
    const bool atMostOneStanding =
        countAlivePlayers() <= 1;

    const bool outOfTime =
        m_tick >=
        MAX_COMBAT_SECONDS *
        TICKS_PER_SECOND;

    return
        atMostOneStanding ||
        outOfTime;
}


int CombatSimulation::getWinnerId() const
{
    if (countAlivePlayers() != 1)
    {
        return 0;
    }

    for (
        const std::unique_ptr<Personaje>& player :
        m_players)
    {
        if (isAlive(*player))
        {
            return player->getId();
        }
    }

    return 0;
}


void CombatSimulation::printResult() const
{
    std::cout
        << "[SIM] fin en el tick "
        << m_tick
        << " ("
        << (float)m_tick /
        TICKS_PER_SECOND
        << " s): ";

    const int winnerId =
        getWinnerId();

    if (winnerId == 0)
    {
        std::cout
            << "sin ganador\n";
    }
    else
    {
        std::cout
            << "gana el jugador "
            << winnerId
            << "\n";
    }
}

void CombatSimulation::recordFrame(bool ending) {
    json frame = json::array();
    const size_t frameIndex = m_replayFrames.size();

    for (size_t playerIndex = 0; playerIndex < m_players.size(); ++playerIndex) {
        const Personaje& player = *m_players[playerIndex];
        auto& state = m_playerReplayStates[playerIndex];
        const glm::vec3 position = player.getPosicion();
        const char* action = "idle";
        if (!isAlive(player)) action = "death";
        else if (m_hitFrames[playerIndex] > 0) action = "hit";
        else if (!ending && player.yaTieneEstado(EstadoPersonaje::CASTEANDO) &&
                 !player.yaTieneEstado(EstadoPersonaje::CONGELADO)) action = "cast";
        else if (frameIndex > 0 && position != state.position) action = "move";

        state.position = position;

        // Se conserva la entidad visual antigua para los personajes OBJ.
        json entry = replayEntity(position, player.getRotacion().y, replayEntityOfPlayer(playerIndex));
        entry.push_back(state.animation.record(player.getId(), REPLAY_PLAYER_ENTITIES[playerIndex],
            action, frameIndex, m_hitFrames[playerIndex] == HIT_EFFECT_FRAMES));
        frame.push_back(std::move(entry));
    }

    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (!spell->IsActive()) continue;
        json entry = replayEntity(spell->getPosition(), yawOf(spell->getDirection()), spell->getName().c_str());
        entry.push_back(json{ { "id", spell->getIdEntidad() } });
        frame.push_back(std::move(entry));
    }

    // El indice del replay tambien avanza durante recordEnding(), aunque m_tick no cambie.
    m_particleFrames.emplace_back();
    try {
        m_replayFrames.push_back(std::move(frame));
        m_particleFrames.back() = m_particulas.registrarFrame(frameIndex);
    } catch (...) {
        if (m_replayFrames.size() > frameIndex) m_replayFrames.erase(frameIndex);
        m_particleFrames.pop_back();
        throw;
    }
}


void CombatSimulation::countDownHitEffects()
{
    for (
        int& framesLeft :
        m_hitFrames)
    {
        if (framesLeft > 0)
        {
            --framesLeft;
        }
    }
}


// Como las claves de combat.json:
// "player_1", "player_2";
// "none" si no gana nadie
std::string CombatSimulation::describeWinner() const
{
    const int winnerId =
        getWinnerId();

    return winnerId == 0
        ? "none"
        : "player_" +
        std::to_string(
            winnerId
        );
}


void CombatSimulation::writeReplay(
    const std::filesystem::path& replayFile) const
{
    const json replay = {
        { "metadata",{
            { "version", REPLAY_FORMAT_VERSION },
            { "iterations", m_replayFrames.size() },
            { "winner", describeWinner() },
            { "ticksPerSecond", TICKS_PER_SECOND },
            { "rotationOrder", "XYZ" },
            { "rotationUnit", "radians" } } },
        { "frames", m_replayFrames },
        { "particleStyles", bm::particles::particleJson::escribirEstilos(m_particulas.estilos()) },
        { "particleFrames", bm::particles::particleJson::escribirFrames(m_particleFrames) }, { "impacts", m_replayImpacts } };

    std::ofstream out(
        replayFile
    );

    if (!out)
    {
        throw std::runtime_error(
            "no se puede escribir " +
            replayFile.string()
        );
    }

    // sin sangrado:
    // con cientos de frames, sangrado
    // ocuparia varias veces mas
    out << replay.dump();

    std::cout
        << "[SIM] replay escrito en "
        << replayFile.string()
        << ": "
        << m_replayFrames.size()
        << " frames\n";
}


static const char* describeAIState(
    EstadoIA state)
{
    switch (state)
    {
    case EstadoIA::ESPERANDO:
        return "esperando";

    case EstadoIA::PERSIGUIENDO:
        return "persiguiendo";

    case EstadoIA::ATACANDO:
        return "atacando";

    case EstadoIA::HUYENDO:
        return "huyendo";
    }

    return "desconocido";
}


// getActiveHechizos devuelve toda la pool,
// tambien los inactivos:
// hay que filtrar
void CombatSimulation::printSpellsInFlight() const
{
    for (
        const std::unique_ptr<Hechizo>& spell :
        m_spells.getActiveHechizos())
    {
        if (!spell->IsActive())
        {
            continue;
        }

        const glm::vec3 position =
            spell->getPosition();

        std::cout
            << "      "
            << spell->getName()
            << " en ("
            << position.x
            << ", "
            << position.y
            << ", "
            << position.z
            << ")\n";
    }
}


void CombatSimulation::printState() const
{
    std::cout
        << "[SIM] tick "
        << m_tick
        << " ("
        << (float)m_tick /
        TICKS_PER_SECOND
        << " s)\n";

    for (
        size_t playerIndex = 0;
        playerIndex < m_players.size();
        ++playerIndex)
    {
        const Personaje& player =
            *m_players[playerIndex];

        const Vector3 position =
            player.getPosicion();

        std::cout
            << "      jugador "
            << player.getId()
            << " en ("
            << position.x
            << ", "
            << position.y
            << ", "
            << position.z
            << ")"
            << " vida "
            << player.getVida()
            << " mana "
            << player.getMana()
            << " IA "
            << describeAIState(
                m_controllers[playerIndex]->
                getEstadoIA()
            )
            << "\n";
    }

    printSpellsInFlight();
}
