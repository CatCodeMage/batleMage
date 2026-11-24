#include "CombatSimulation.h"


// Nombres de db/entities.json: generateCombat los usa para saber que modelo dibujar
constexpr const char* REPLAY_HIT_ENTITY = "hit";
constexpr const char* REPLAY_FORMAT_VERSION = "1.8";   // los proyectiles incluyen su edad visual

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


CombatSimulation::CombatSimulation()
    : m_spells(SPELL_POOL_SIZE)
{
}


// Por la vida y no por MUERTO: yaTieneEstado no es const
static bool isAlive(const Personaje& player)
{
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

    m_hitFrames.assign(
        m_players.size(),
        0
    );

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
        recordFrame();
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


void CombatSimulation::createPlayers(
    const std::filesystem::path& combatConfigFile)
{
    std::ifstream in(
        combatConfigFile
    );

    if (!in)
    {
        throw std::runtime_error(
            "no se puede abrir " +
            combatConfigFile.string()
        );
    }

    const json config =
        json::parse(in);

    int nextPlayerId = 1;

    for (
        const json& player :
        config.at("players"))
    {
        m_players.push_back(
            std::make_unique<Personaje>(
                nextPlayerId++,
                readPosition(player)
            )
        );

        // el personaje crea sus propios hechizos al lanzar
        m_players.back()->setGestorHechizos(
            &m_spells
        );

        const std::string playerType =
            player.value(
                "type",
                "sphere"
            );

        if (!m_players.back()->cargarDesdeJson(
            dataPath(
                "db/players.json"
            ).string(),
            playerType))
        {
            throw std::runtime_error(
                combatConfigFile.string() +
                ": tipo de mago desconocido \"" +
                playerType +
                "\""
            );
        }
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

            if (spellName == "ice_lance" || spellName == "fire_ball")
            {
                const float fullDuration = (spellName == "fire_ball") ? 3.18f : 1.9f;
                if (!spell->IsActive())
                {
                    recordImpactVFX(spellName, hitPos, info.normal, fullDuration);
                }
                else if (wasApproaching)
                {
                    recordImpactVFX(spellName + "_bounce", hitPos, info.normal, 0.45f);
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

            if (spell.getName() == "ice_lance" || spell.getName() == "fire_ball")
            {
                const glm::vec3 delta = spell.getPosition() - player.getPosicion();
                const glm::vec3 horiz(delta.x, 0.0f, delta.z);
                const glm::vec3 dir = spell.getDirection();
                const glm::vec3 hitNormal = glm::length(horiz) > 0.0001f
                    ? glm::normalize(horiz)
                    : (glm::length(dir) > 0.0001f ? -dir : glm::vec3(0.0f, 1.0f, 0.0f));
                const glm::vec3 hitPos = player.getPosicion() + glm::vec3(0.0f, delta.y, 0.0f) + hitNormal * player.getRadio();
                const float fullDuration = (spell.getName() == "fire_ball") ? 3.18f : 1.9f;
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


void CombatSimulation::recordFrame()
{
    json frame =
        json::array();

    for (
        size_t playerIndex = 0;
        playerIndex < m_players.size();
        ++playerIndex)
    {
        const Personaje& player =
            *m_players[playerIndex];

        frame.push_back(
            replayEntity(
                player.getPosicion(),
                player.getRotacion().y,
                replayEntityOfPlayer(
                    playerIndex
                )
            )
        );
    }

    for (
        const std::unique_ptr<Hechizo>& spell :
        m_spells.getActiveHechizos())
    {
        if (!spell->IsActive())
        {
            continue;
        }

        const Vector3 dir = spell->getDirection();
        json sample = replaySpellEntity(spell->getPosition(), pitchOf(dir), yawOf(dir), spell->getName().c_str());
        // El primer estado visible se graba despues de actualizar el tick de lanzamiento.
        const double ageTicks = std::round(static_cast<double>(spell->getCurrentLifeTime()) * TICKS_PER_SECOND);
        sample.push_back((std::max)(0.0, ageTicks - 1.0) / TICKS_PER_SECOND);
        sample.push_back(spell->getIndicePool());
        frame.push_back(std::move(sample));
    }

    m_replayFrames.push_back(
        std::move(frame)
    );
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
        {
            "metadata",
            {
                {
                    "version",
                    REPLAY_FORMAT_VERSION
                },

                {
                    "iterations",
                    m_replayFrames.size()
                },

                {
                    "winner",
                    describeWinner()
                },

                {
                    "ticksPerSecond",
                    TICKS_PER_SECOND
                },

                {
                    "rotationOrder",
                    "XYZ"
                },

                {
                    "rotationUnit",
                    "radians"
                }
            }
        },

        {
            "frames",
            m_replayFrames
        },

        {
            "impacts",
            m_replayImpacts
        }
    };

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
