#include "PruebaHechizos.h"
#include "lanzadorHechizos.h"
#include "Paths.h"         // dataPath
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

// =====================================================================
// CONFIGURACIÓN DE LA PRUEBA (cámbiala a tu gusto)
// =====================================================================
constexpr float DURACION_PRUEBA_S = 25.0f;   // segundos simulados
constexpr bool  MAGOS_INMORTALES = true;    // los golpes se ven y se cuentan, pero no matan
constexpr float ALTURA_SUELO = 0.0f;    // map.json: el suelo está en y = 0
constexpr float DISTANCIA_MAGOS = 10.0f;   // los hechizos alcanzan 12-30 (velocidad x 3 s): en combat.json están a ~56

// Ids de las cartas de prueba (no chocan con los de spells.json, 0..15)
constexpr int CARTA_MAS_DANO = 100;
constexpr int CARTA_REBOTE = 101;
constexpr int CARTA_ICE_LANCE_PARABOLICA = 102;
constexpr int CARTA_DOBLE = 200;
constexpr int CARTA_TRIPLE = 201;

// Igual que CombatSimulation, para que el replay sea compatible
constexpr size_t      POOL_HECHIZOS = 256;
constexpr float       SPELL_HIT_RADIUS = 0.5f;
constexpr int         HIT_EFFECT_FRAMES = static_cast<int>(PruebaHechizos::TICKS_PER_SECOND * 0.3f);
constexpr int         CAST_EFFECT_FRAMES = static_cast<int>(PruebaHechizos::TICKS_PER_SECOND * 0.3f);
constexpr const char* REPLAY_HIT_ENTITY = "hit";
constexpr const char* REPLAY_PLAYER_ENTITIES[] = { "player_red", "player_blue" };
constexpr const char* REPLAY_DEAD_ENTITIES[] = { "dead_red",   "dead_blue" };

// =====================================================================
// Ayudas
// =====================================================================
static bool isAlive(const Personaje& p) { return p.getVida() > 0.0f; }

static CylinderCollider colliderOf(const Personaje& p) {
    return { p.getPosicion(), p.getRadio(), p.getRadio() };
}

static float yawOf(const glm::vec3& dir) { return std::atan2(dir.x, dir.z); }

static float pitchOf(const glm::vec3& dir) {
    const float horiz = std::sqrt(dir.x * dir.x + dir.z * dir.z);
    if (horiz < 0.0001f && std::abs(dir.y) < 0.0001f) return 0.0f;
    return std::atan2(-dir.y, horiz);
}

// [[x, y, z], [giroX, giroY, giroZ], "entidad"]
static json replayEntity(const glm::vec3& pos, float yaw, const char* entityName) {
    return json::array({ json::array({ pos.x, pos.y, pos.z }),
                         json::array({ 0.0f, yaw, 0.0f }),
                         entityName });
}

static json replaySpellEntity(const glm::vec3& pos, float pitch, float yaw, const char* entityName) {
    return json::array({ json::array({ pos.x, pos.y, pos.z }),
                         json::array({ pitch, yaw, 0.0f }),
                         entityName });
}

PruebaHechizos::PruebaHechizos() : m_spells(POOL_HECHIZOS, m_ids) {}

int PruebaHechizos::idPorNombre(const std::string& nombre) const {
    for (const auto& [id, plantilla] : m_spells.getCatalogoHechizos())
        if (plantilla.definition.name == nombre) return id;
    std::cerr << "[PRUEBA] hechizo no encontrado en spells.json: " << nombre << "\n";
    return -1;
}

// =====================================================================
// Preparación
// =====================================================================
void PruebaHechizos::setup(const std::filesystem::path& combatConfigFile) {
    readSpells(m_spells);
    registrarCartas();
    crearMagos(combatConfigFile);
    crearVaritas();

    m_hitFrames.assign(m_players.size(), 0);
    m_animations.resize(m_players.size());
    m_impactosRecibidos.assign(m_players.size(), 0);
    m_danoRecibido.assign(m_players.size(), 0.0f);
    recordFrame();
    std::cout << "[PRUEBA] combate de prueba de hechizos: " << DURACION_PRUEBA_S << " s, magos "
        << (MAGOS_INMORTALES ? "inmortales" : "mortales") << "\n";
}

void PruebaHechizos::registrarCartas() {
    ModificadoresDisparo masDano; masDano.danoExtra = 10.0f;
    m_spells.registrarModificador(CARTA_MAS_DANO, "damage_plus", 5.0f, 0.08f, masDano);

    ModificadoresDisparo rebote;  rebote.rebotes = 2; rebote.etiquetas = ETQ_REBOTE; rebote.lifetimeExtra = 3.0f;
    m_spells.registrarModificador(CARTA_REBOTE, "bounce", 0.0f, 0.0f, rebote);

    m_spells.registrarMulticast(CARTA_DOBLE, "double_spell", 0.0f, 0.0f, 2);
    m_spells.registrarMulticast(CARTA_TRIPLE, "triple_spell", 2.0f, 0.03f, 3);

    // Variante parabolica de ice_lance exclusiva para probar la curva y el rebote en --prueba-hechizos
    m_spells.registrarHechizo(
        CARTA_ICE_LANCE_PARABOLICA,
        "ice_lance",
        ElementType::ICE,
        10.0f,
        18.0f,
        2.4f,
        5.5f,
        1.0f,
        glm::vec3(0.8f, 0.8f, 1.5f),
        3.5f,
        MovementPattern::Parabolic
    );
}

// Mismos tipos de mago y altura que el combate normal (combat.json + players.json), pero sin IA
void PruebaHechizos::crearMagos(const std::filesystem::path& combatConfigFile) {
    std::ifstream in(combatConfigFile);
    if (!in) throw std::runtime_error("no se puede abrir " + combatConfigFile.string());
    const json config = json::parse(in);

    for (const json& player : config.at("players")) {
        // Mismo tipo y altura que combat.json, pero cerca uno del otro (en el eje X) para que los hechizos lleguen
        const float altura = player.at("position").at(1).get<float>();
        const float x = (m_players.empty() ? -0.5f : 0.5f) * DISTANCIA_MAGOS;
        m_players.push_back(std::make_unique<Personaje>(m_ids.nuevo(), glm::vec3(x, altura, 0.0f), 1.0f, &m_spells));
        const std::string tipo = player.value("type", "sphere");
        if (!m_players.back()->cargarDesdeJson(dataPath("db/players.json").string(), tipo))
            throw std::runtime_error("tipo de mago desconocido \"" + tipo + "\"");
    }
    if (m_players.size() != 2)
        throw std::runtime_error("la prueba espera 2 magos y hay " + std::to_string(m_players.size()));

    // Que se miren el uno al otro (para el replay)
    for (size_t i = 0; i < 2; ++i) {
        glm::vec3 d = m_players[1 - i]->getPosicion() - m_players[i]->getPosicion();
        d.y = 0.0f;
        if (glm::length(d) > 0.0001f) d = glm::normalize(d);
        m_players[i]->setDireccion(d);
        m_players[i]->setRotacion(glm::vec3(0.0f, yawOf(d), 0.0f));
    }
}

// =====================================================================
// ESCENARIOS: aquí se decide qué lleva la varita de cada mago
// =====================================================================
void PruebaHechizos::crearVaritas() {
    m_varitas.assign(m_players.size(), VaritaPrueba{});

    const int iceLance = idPorNombre("ice_lance");
    const int fireBall = idPorNombre("fire_ball");
    const int lightningBolt = idPorNombre("lightning_bolt");
    const int earthSpike = idPorNombre("earth_spike");
    const int windBlade = idPorNombre("wind_blade");
    const int iceShard = idPorNombre("ice_shard");
    const int waterJet = idPorNombre("water_jet");
    const int daggerBarrage = idPorNombre("dagger_barrage");

    // Mago 1 (rojo, izquierda): showcase determinista de los 8 proyectiles cada 2.5 s
    VaritaPrueba& v1 = m_varitas[0];
    v1.ranuras = {
        fireBall,
        iceLance,
        lightningBolt,
        earthSpike,
        windBlade,
        iceShard,
        waterJet,
        daggerBarrage
    };
    v1.castDelay = 2.5f;
    v1.recarga = 2.5f;

    // Mago 2 (azul, derecha): quieto como blanco para apreciar claramente cada hechizo
    VaritaPrueba& v2 = m_varitas[1];
    v2.ranuras.clear();
}

// =====================================================================
// Bucle
// =====================================================================
void PruebaHechizos::step() {
    dispararVaritas();
    m_spells.update(SECONDS_PER_TICK);
    aplicarSuelo();
    aplicarImpactos();
    ++m_tick;
    recordFrame();
    countDownEffects();
}

bool PruebaHechizos::isFinished() const {
    const bool tiempoAgotado = m_tick >= static_cast<int>(DURACION_PRUEBA_S * TICKS_PER_SECOND);
    return tiempoAgotado || (!MAGOS_INMORTALES && countAlive() <= 1);
}

// Hace de varita: usa LanzadorHechizos igual que lo hará Varita::intentarDisparar
void PruebaHechizos::dispararVaritas() {
    LanzadorHechizos lanzador(m_spells);

    for (size_t i = 0; i < m_players.size(); ++i) {
        Personaje& mago = *m_players[i];
        VaritaPrueba& v = m_varitas[i];

        v.mana = std::min(v.manaMax, v.mana + v.regenMana * SECONDS_PER_TICK);
        if (v.espera > 0.0f) { v.espera -= SECONDS_PER_TICK; continue; }
        if (!isAlive(mago)) continue;

        glm::vec3 haciaRival = m_players[1 - i]->getPosicion() - mago.getPosicion();
        haciaRival.y = 0.0f;
        if (glm::length(haciaRival) < 0.0001f) continue;
        haciaRival = glm::normalize(haciaRival);

        const glm::vec3 direccion = glm::normalize(glm::vec3(haciaRival.x, v.inclinacionY, haciaRival.z));
        const glm::vec3 origen = mago.getPosicion() + haciaRival * (mago.getRadio() + SPELL_HIT_RADIUS + 0.5f);

        if (v.ranuras.empty()) continue;
        const int idxAntes = v.indice;
        const ResultadoDisparo r = lanzador.disparar(v.ranuras, v.indice, v.mana, origen, direccion, mago.getId());

        v.mana -= r.manaGastado;
        v.indice = r.nuevoIndice;
        v.espera = std::max(0.0f, v.castDelay + r.castDelay);
        if (r.hayQueRecargar) v.espera = std::max(v.espera, v.recarga + r.recarga);

        if (r.disparado) {
            v.castFrames = CAST_EFFECT_FRAMES;
            ++m_disparos;
            m_proyectiles += r.proyectilesLanzados;
            if (idxAntes >= 0 && idxAntes < static_cast<int>(v.ranuras.size())) {
                const auto it = m_spells.getCatalogoHechizos().find(v.ranuras[idxAntes]);
                if (it != m_spells.getCatalogoHechizos().end()) {
                    std::cout << "[SHOWCASE] " << it->second.definition.name << "\n";
                }
            }
            std::cout << "[PRUEBA] t=" << static_cast<float>(m_tick) / TICKS_PER_SECOND << "s jugador "
                << mago.getId() << ": " << r.proyectilesLanzados << " proyectiles, mana " << v.mana
                << (r.hayQueRecargar ? " (recarga)" : "") << "\n";
        }
    }
}

// Suelo plano: avisa a HechizosGestor como lo hará el sistema de colisiones
void PruebaHechizos::aplicarSuelo() {
    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (!spell->IsActive()) continue;
        const SphereCollider esfera{ spell->getPosition(), SPELL_HIT_RADIUS };
        const CollisionInfo contacto = m_colisiones.getGroundCollisionInfo(esfera, ALTURA_SUELO);
        if (contacto.collision) {
            const bool wasApproaching = glm::dot(spell->getVelocityVector(), contacto.normal) < 0.0f;
            const glm::vec3 hitPos = spell->getPosition() + contacto.normal * (contacto.penetration - SPELL_HIT_RADIUS);
            const std::string spellName = spell->getName();

            m_spells.procesarChoqueSuperficie(spell.get(), contacto.normal, contacto.penetration);

            if (spellName == "ice_lance" || spellName == "fire_ball" || spellName == "lightning_bolt" || spellName == "earth_spike" || spellName == "wind_blade" || spellName == "water_jet" || spellName == "dagger_barrage") {
                const float fullDuration = (spellName == "fire_ball") ? 3.18f
                    : ((spellName == "lightning_bolt") ? 2.50f
                    : ((spellName == "earth_spike") ? 1.60f
                    : ((spellName == "wind_blade") ? 1.20f
                    : ((spellName == "water_jet") ? 1.15f
                    : ((spellName == "dagger_barrage") ? 1.10f : 1.90f)))));
                if (!spell->IsActive()) {
                    recordImpactVFX(spellName, hitPos, contacto.normal, fullDuration);
                } else if (wasApproaching) {
                    recordImpactVFX(spellName + "_bounce", hitPos, contacto.normal, 0.40f);
                }
            }
        }
    }
}

void PruebaHechizos::recordImpactVFX(
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

// Golpes a magos. Usa el daño del propio hechizo (getDefinition), así se ven los modificadores
void PruebaHechizos::aplicarImpactos() {
    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (!spell->IsActive()) continue;
        const SphereCollider esfera{ spell->getPosition(), SPELL_HIT_RADIUS };

        for (size_t i = 0; i < m_players.size(); ++i) {
            Personaje& mago = *m_players[i];
            if (!isAlive(mago)) continue;
            if (mago.getId() == spell->getLanzador()) continue;   // sin fuego amigo en la prueba
            if (!m_colisiones.checkCollision(esfera, colliderOf(mago))) continue;

            const float dano = spell->getDamage();
            ++m_impactosRecibidos[i];
            m_danoRecibido[i] += dano;
            m_hitFrames[i] = HIT_EFFECT_FRAMES;
            std::cout << "[PRUEBA] jugador " << mago.getId() << " recibe " << spell->getName()
                << " (dano " << dano << ")\n";

            if (!MAGOS_INMORTALES)
                m_combat.procesarImpacto(esfera, colliderOf(mago), spell->getDefinition(), mago);

            if (spell->getName() == "ice_lance" || spell->getName() == "fire_ball" || spell->getName() == "lightning_bolt" || spell->getName() == "earth_spike" || spell->getName() == "wind_blade" || spell->getName() == "water_jet" || spell->getName() == "dagger_barrage") {
                const glm::vec3 delta = spell->getPosition() - mago.getPosicion();
                const glm::vec3 horiz(delta.x, 0.0f, delta.z);
                const glm::vec3 dir = spell->getDirection();
                const glm::vec3 hitNormal = glm::length(horiz) > 0.0001f
                    ? glm::normalize(horiz)
                    : (glm::length(dir) > 0.0001f ? -dir : glm::vec3(0.0f, 1.0f, 0.0f));
                const glm::vec3 hitPos = mago.getPosicion() + glm::vec3(0.0f, delta.y, 0.0f) + hitNormal * mago.getRadio();
                const float fullDuration = (spell->getName() == "fire_ball") ? 3.18f
                    : ((spell->getName() == "lightning_bolt") ? 2.50f
                    : ((spell->getName() == "earth_spike") ? 1.60f
                    : ((spell->getName() == "wind_blade") ? 1.20f
                    : ((spell->getName() == "water_jet") ? 1.15f
                    : ((spell->getName() == "dagger_barrage") ? 1.10f : 1.90f)))));
                recordImpactVFX(spell->getName(), hitPos, hitNormal, fullDuration);
            }

            m_spells.destroySpell(spell.get());
            break;
        }
    }
}

void PruebaHechizos::countDownEffects() {
    for (int& f : m_hitFrames) if (f > 0) --f;
    for (auto& varita : m_varitas) if (varita.castFrames > 0) --varita.castFrames;
}

size_t PruebaHechizos::countAlive() const {
    size_t n = 0;
    for (const auto& p : m_players) n += isAlive(*p) ? 1 : 0;
    return n;
}

// =====================================================================
// Replay (mismo formato que CombatSimulation)
// =====================================================================
const char* PruebaHechizos::replayEntityOfPlayer(size_t i) const {
    if (!isAlive(*m_players[i])) return REPLAY_DEAD_ENTITIES[i];
    if (m_hitFrames[i] > 0)      return REPLAY_HIT_ENTITY;
    return REPLAY_PLAYER_ENTITIES[i];
}

void PruebaHechizos::recordFrame() {
    json frame = json::array();
    for (size_t i = 0; i < m_players.size(); ++i) {
        const Personaje& p = *m_players[i];
        const char* action = "idle";
        bool restart = false;
        if (!isAlive(p)) {
            action = "death";
        } else if (m_hitFrames[i] > 0) {
            action = "hit";
            restart = m_hitFrames[i] == HIT_EFFECT_FRAMES;
        } else if (m_varitas[i].castFrames > 0) {
            action = "cast";
            restart = m_varitas[i].castFrames == CAST_EFFECT_FRAMES;
        }

        json entry = replayEntity(p.getPosicion(), p.getRotacion().y, replayEntityOfPlayer(i));
        entry.push_back(m_animations[i].record(p.getId(), REPLAY_PLAYER_ENTITIES[i],
            action, m_replayFrames.size(), restart));
        frame.push_back(std::move(entry));
    }
    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (!spell->IsActive()) continue;
        const glm::vec3 dir = spell->getDirection();
        json sample = replaySpellEntity(spell->getPosition(), pitchOf(dir), yawOf(dir), spell->getName().c_str());
        // El primer estado visible se graba despues de actualizar el tick de lanzamiento.
        const double ageTicks = std::round(static_cast<double>(spell->getCurrentLifeTime()) * TICKS_PER_SECOND);
        sample.push_back((std::max)(0.0, ageTicks - 1.0) / TICKS_PER_SECOND);
        sample.push_back(spell->getIndicePool());
        frame.push_back(std::move(sample));
    }
    m_replayFrames.push_back(std::move(frame));
}

void PruebaHechizos::recordEnding() {
    m_spells.emptySpells();
    for (auto& varita : m_varitas) varita.castFrames = 0;
    const int ticks = 2 * TICKS_PER_SECOND;
    for (int t = 0; t < ticks; ++t) { countDownEffects(); recordFrame(); }
}

std::string PruebaHechizos::describeWinner() const {
    if (MAGOS_INMORTALES || countAlive() != 1) return "none";
    for (const auto& p : m_players) if (isAlive(*p)) return "player_" + std::to_string(p->getId());
    return "none";
}

void PruebaHechizos::printResult() const {
    std::cout << "[PRUEBA] fin a los " << static_cast<float>(m_tick) / TICKS_PER_SECOND << " s: "
        << m_disparos << " disparos, " << m_proyectiles << " proyectiles\n";
    for (size_t i = 0; i < m_players.size(); ++i)
        std::cout << "         jugador " << m_players[i]->getId() << ": " << m_impactosRecibidos[i]
        << " impactos recibidos, " << m_danoRecibido[i] << " de dano\n";
}

void PruebaHechizos::writeReplay(const std::filesystem::path& replayFile) const {
    const json replay = {
        { "metadata", {
            { "version", REPLAY_FORMAT_VERSION },
            { "iterations", m_replayFrames.size() },
            { "winner", describeWinner() },
            { "ticksPerSecond", TICKS_PER_SECOND },
            { "rotationOrder", "XYZ" },
            { "rotationUnit", "radians" } } },
        { "frames", m_replayFrames },
        { "impacts", m_replayImpacts } };

    std::ofstream out(replayFile);
    if (!out) throw std::runtime_error("no se puede escribir " + replayFile.string());
    out << replay.dump();
    std::cout << "[PRUEBA] replay escrito en " << replayFile.string() << ": " << m_replayFrames.size() << " frames\n";
}
