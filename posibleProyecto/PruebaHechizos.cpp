#include "PruebaHechizos.h"
#include "lanzadorHechizos.h"
#include "jsonManager.h"   // readSpells
#include "Paths.h"         // dataPath
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

// =====================================================================
// CONFIGURACIÓN DE LA PRUEBA (cámbiala a tu gusto)
// =====================================================================
constexpr float DURACION_PRUEBA_S = 20.0f;   // segundos simulados
constexpr bool  MAGOS_INMORTALES = true;    // los golpes se ven y se cuentan, pero no matan
constexpr float GRAVEDAD = 5.0f;    // solo para hechizos con REBOTE
constexpr float ALTURA_SUELO = 0.0f;    // map.json: el suelo está en y = 0
constexpr float DISTANCIA_MAGOS = 10.0f;   // los hechizos alcanzan 12-30 (velocidad x 3 s): en combat.json están a ~56

// Ids de las cartas de prueba (no chocan con los de spells.json, 0..15)
constexpr int CARTA_MAS_DANO = 100;
constexpr int CARTA_REBOTE = 101;
constexpr int CARTA_DOBLE = 200;
constexpr int CARTA_TRIPLE = 201;

// Igual que CombatSimulation, para que el replay sea compatible
constexpr size_t      POOL_HECHIZOS = 256;
constexpr float       SPELL_HIT_RADIUS = 0.5f;
constexpr int         HIT_EFFECT_FRAMES = static_cast<int>(PruebaHechizos::TICKS_PER_SECOND * 0.3f);
constexpr const char* REPLAY_HIT_ENTITY = "hit";
constexpr const char* REPLAY_FORMAT_VERSION = "1.7";
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

// [[x, y, z], [giroX, giroY, giroZ], "entidad"]
static json replayEntity(const glm::vec3& pos, float yaw, const char* entityName) {
    return json::array({ json::array({ pos.x, pos.y, pos.z }),
                         json::array({ 0.0f, yaw, 0.0f }),
                         entityName });
}

PruebaHechizos::PruebaHechizos() : m_spells(POOL_HECHIZOS) {}

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
    m_impactosRecibidos.assign(m_players.size(), 0);
    m_danoRecibido.assign(m_players.size(), 0.0f);
    recordFrame();
    std::cout << "[PRUEBA] combate de prueba de hechizos: " << DURACION_PRUEBA_S << " s, magos "
        << (MAGOS_INMORTALES ? "inmortales" : "mortales") << "\n";
}

void PruebaHechizos::registrarCartas() {
    ModificadoresDisparo masDano; masDano.danoExtra = 10.0f;
    m_spells.registrarModificador(CARTA_MAS_DANO, "damage_plus", 5.0f, 0.08f, masDano);

    ModificadoresDisparo rebote;  rebote.rebotes = 4; rebote.etiquetas = ETQ_REBOTE; rebote.lifetimeExtra = 3.0f;
    m_spells.registrarModificador(CARTA_REBOTE, "bounce", 0.0f, 0.0f, rebote);

    m_spells.registrarMulticast(CARTA_DOBLE, "double_spell", 0.0f, 0.0f, 2);
    m_spells.registrarMulticast(CARTA_TRIPLE, "triple_spell", 2.0f, 0.03f, 3);
}

// Mismos tipos de mago y altura que el combate normal (combat.json + players.json), pero sin IA
void PruebaHechizos::crearMagos(const std::filesystem::path& combatConfigFile) {
    std::ifstream in(combatConfigFile);
    if (!in) throw std::runtime_error("no se puede abrir " + combatConfigFile.string());
    const json config = json::parse(in);

    int nextId = 1;
    for (const json& player : config.at("players")) {
        // Mismo tipo y altura que combat.json, pero cerca uno del otro (en el eje X) para que los hechizos lleguen
        const float altura = player.at("position").at(1).get<float>();
        const float x = (nextId == 1 ? -0.5f : 0.5f) * DISTANCIA_MAGOS;
        m_players.push_back(std::make_unique<Personaje>(nextId++, glm::vec3(x, altura, 0.0f)));

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

    // Mago 1 (rojo): [doble][+10 daño][fire_ball][lightning_bolt]
    //   -> 2 proyectiles distintos a la vez, con +10 de daño cada uno
    VaritaPrueba& v1 = m_varitas[0];
    v1.ranuras = { CARTA_DOBLE, CARTA_MAS_DANO, idPorNombre("fire_ball"), idPorNombre("lightning_bolt") };
    v1.castDelay = 0.3f;
    v1.recarga = 1.0f;

    // Mago 2 (azul): [triple][rebote x4][ice_shard x3]
    //   -> abanico de 3 que van hacia el suelo, rebotan y siguen en arco hacia el rival
    VaritaPrueba& v2 = m_varitas[1];
    const int shard = idPorNombre("ice_shard");
    v2.ranuras = { CARTA_TRIPLE, CARTA_REBOTE, shard, shard, shard };
    v2.castDelay = 0.4f;
    v2.recarga = 1.5f;
    v2.inclinacionY = -0.5f;   // hacia el suelo: rebota y hace arcos por la gravedad
}

// =====================================================================
// Bucle
// =====================================================================
void PruebaHechizos::step() {
    dispararVaritas();
    aplicarGravedad();
    m_spells.update(SECONDS_PER_TICK);
    aplicarSuelo();
    aplicarImpactos();
    ++m_tick;
    recordFrame();
    countDownHitEffects();
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

        const ResultadoDisparo r = lanzador.disparar(v.ranuras, v.indice, v.mana, origen, direccion, mago.getId());

        v.mana -= r.manaGastado;
        v.indice = r.nuevoIndice;
        v.espera = std::max(0.0f, v.castDelay + r.castDelay);
        if (r.hayQueRecargar) v.espera = std::max(v.espera, v.recarga + r.recarga);

        if (r.disparado) {
            ++m_disparos;
            m_proyectiles += r.proyectilesLanzados;
            std::cout << "[PRUEBA] t=" << static_cast<float>(m_tick) / TICKS_PER_SECOND << "s jugador "
                << mago.getId() << ": " << r.proyectilesLanzados << " proyectiles, mana " << v.mana
                << (r.hayQueRecargar ? " (recarga)" : "") << "\n";
        }
    }
}

// Gravedad solo para hechizos con REBOTE (el resto vuela recto, como siempre)
void PruebaHechizos::aplicarGravedad() {
    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (!spell->IsActive() || !(spell->getEtiquetas() & ETQ_REBOTE)) continue;
        if (spell->getSpeed() <= 0.0f) continue;
        glm::vec3 d = spell->getDirection();
        d.y -= GRAVEDAD * SECONDS_PER_TICK / spell->getSpeed();   // = restar g*dt a la velocidad vertical
        spell->setDirection(d);
    }
}

// Suelo plano: avisa a HechizosGestor como lo hará el sistema de colisiones
void PruebaHechizos::aplicarSuelo() {
    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (!spell->IsActive()) continue;
        const float fondo = spell->getPosition().y - SPELL_HIT_RADIUS;
        if (fondo <= ALTURA_SUELO)
            m_spells.procesarChoqueSuperficie(spell.get(), glm::vec3(0.0f, 1.0f, 0.0f), ALTURA_SUELO - fondo);
    }
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

            m_spells.destroySpell(spell.get());
            break;
        }
    }
}

void PruebaHechizos::countDownHitEffects() {
    for (int& f : m_hitFrames) if (f > 0) --f;
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
        frame.push_back(replayEntity(p.getPosicion(), p.getRotacion().y, replayEntityOfPlayer(i)));
    }
    for (const std::unique_ptr<Hechizo>& spell : m_spells.getActiveHechizos()) {
        if (!spell->IsActive()) continue;
        frame.push_back(replayEntity(spell->getPosition(), yawOf(spell->getDirection()), spell->getName().c_str()));
    }
    m_replayFrames.push_back(std::move(frame));
}

void PruebaHechizos::recordEnding() {
    m_spells.emptySpells();
    const int ticks = 2 * TICKS_PER_SECOND;
    for (int t = 0; t < ticks; ++t) { countDownHitEffects(); recordFrame(); }
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
        { "frames", m_replayFrames } };

    std::ofstream out(replayFile);
    if (!out) throw std::runtime_error("no se puede escribir " + replayFile.string());
    out << replay.dump();
    std::cout << "[PRUEBA] replay escrito en " << replayFile.string() << ": " << m_replayFrames.size() << " frames\n";
}