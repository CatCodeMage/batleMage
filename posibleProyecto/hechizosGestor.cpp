#include "hechizosGestor.h"

// NUEVO: gira una dirección en el plano XZ (misma fórmula que Varita::aplicarDispersion, sin azar)
static glm::vec3 rotarEnXZ(const glm::vec3& d, float grados) {
    const float a = grados * 3.14159265f / 180.0f;
    const float c = std::cos(a);
    const float s = std::sin(a);
    return glm::vec3(d.x * c - d.z * s, d.y, d.x * s + d.z * c);
}

HechizosGestor::HechizosGestor(size_t poolSize) {
    // Pre-allocamos los hechizos inactivos para optimizar rendimiento
    spellPool.reserve(poolSize);   // NUEVO
    libres.reserve(poolSize);      // NUEVO
    for (size_t i = 0; i < poolSize; ++i) {
        // Creamos una instancia vacía o con valores por defecto
        auto spell = std::make_unique<Hechizo>(0, "", ElementType::Physical, 0.f, 0.f, 0.f, 0.f, 0.f);
        spell->setActive(false);
        spell->setIndicePool(static_cast<int>(i));   // NUEVO
        spellPool.push_back(std::move(spell));
    }
    // NUEVO: al revés para que el primero en salir sea el índice 0
    for (int i = static_cast<int>(poolSize) - 1; i >= 0; --i) {
        libres.push_back(i);
    }
}

// NUEVO
Hechizo* HechizosGestor::sacarDelPool() {
    if (libres.empty()) return nullptr;
    const int indice = libres.back();
    libres.pop_back();
    return spellPool[indice].get();
}

// NUEVO
void HechizosGestor::devolverAlPool(Hechizo* spell) {
    spell->reset();                              // isActive = false
    libres.push_back(spell->getIndicePool());
}

void HechizosGestor::registrarHechizo(int id, const std::string& name, ElementType element, float cost,
    float damage, float cooldown, float speed, float radius, glm::vec3 scale, float maxLifeTime) {

    // Creamos el componente de definición estática
    // CAMBIAR: DE MOMENTO PONGO QUE ES FULL PROYECTILES Y LINEAR
    SpellDefinitionComponent def = { id, name, element, CastType::Projectile, MovementPattern::Linear, cost, damage, cooldown };

    // Lo guardamos en el catálogo junto con sus datos de movimiento
    hechizhosCatalog[id] = { def, speed, radius, scale, maxLifeTime };

    // NUEVO: cada hechizo de spells.json es también una carta proyectil para la varita
    // (castDelay 0 de momento: el "cooldown" del JSON no equivale al cast delay de Noita)
    cartas[id] = std::make_unique<CartaProyectil>(id, name, cost, 0.0f);
}

// NUEVO
void HechizosGestor::registrarModificador(int id, const std::string& nombre, float mana, float castDelay,
    const ModificadoresDisparo& mods) {
    if (cartas.count(id)) {
        std::cerr << "[HechizosManager] Aviso: id de carta repetido " << id << ", se sobrescribe.\n";
    }
    cartas[id] = std::make_unique<CartaModificador>(id, nombre, mana, castDelay, mods);
}

// NUEVO
void HechizosGestor::registrarMulticast(int id, const std::string& nombre, float mana, float castDelay, int robos) {
    if (cartas.count(id)) {
        std::cerr << "[HechizosManager] Aviso: id de carta repetido " << id << ", se sobrescribe.\n";
    }
    cartas[id] = std::make_unique<CartaMulticast>(id, nombre, mana, castDelay, robos);
}

// NUEVO
const Carta* HechizosGestor::getCarta(int id) const {
    auto it = cartas.find(id);
    return (it == cartas.end()) ? nullptr : it->second.get();
}

// CAMBIO: ahora devuelve bool y usa spawnDisparo por dentro
bool HechizosGestor::spawnSpell(int spellId, const glm::vec3& initialPosition, const glm::vec3& direction) {
    // Compatibilidad: un disparo de 1 proyectil sin modificadores
    EstadoDisparo disparo;
    disparo.anadirProyectil(spellId);
    return spawnDisparo(disparo, initialPosition, direction, -1) > 0;
}

// NUEVO
int HechizosGestor::spawnDisparo(const EstadoDisparo& disparo, const glm::vec3& origen,
    const glm::vec3& direccion, int idLanzador) {
    constexpr float SEPARACION_GRADOS = 8.0f;   // abanico del multicast
    const ModificadoresDisparo& m = disparo.mods;
    const int n = disparo.numProyectiles;
    const float separacion = SEPARACION_GRADOS + m.dispersionExtra;
    int lanzados = 0;

    for (int i = 0; i < n; ++i) {
        auto it = hechizhosCatalog.find(disparo.proyectiles[i]);
        if (it == hechizhosCatalog.end()) {
            std::cerr << "[HechizosManager] Error: El hechizo con ID " << disparo.proyectiles[i]
                << " no está registrado en el catálogo.\n";
            continue;
        }
        const hechizosTemplate& t = it->second;

        Hechizo* spell = sacarDelPool();
        if (!spell) {
            std::cerr << "[HechizosManager] Advertencia: No hay hechizos disponibles en el pool (Pool exhausto).\n";
            break;
        }

        // Aplicamos la "hoja de ajustes": (base + extra) * mult
        SpellDefinitionComponent def = t.definition;
        def.damage += m.danoExtra;
        spell->setDefinition(def);
        spell->setSpeed(t.speed * m.multVelocidad);
        spell->setRadius(t.radius);
        spell->setScale(t.scale);                               // antes no se copiaba la escala
        spell->setMaxLifeTime(t.defaultLifeTime + m.lifetimeExtra);
        spell->setLanzador(idLanzador);
        spell->setComportamientos(m.etiquetas, m.rebotes);

        // Abanico centrado: 3 proyectiles -> -8º, 0º, +8º
        const float angulo = (static_cast<float>(i) - (n - 1) * 0.5f) * separacion;
        spell->cast(origen, rotarEnXZ(direccion, angulo));      // isActive = true
        ++lanzados;
    }
    return lanzados;
}

// CAMBIO: devuelve al pool los que se desactivan en este tick
void HechizosGestor::update(float deltaTime) {
    for (auto& spell : spellPool) {
        if (!spell->IsActive()) continue;
        spell->update(deltaTime); // Mueve el hechizo y gestiona su propio tiempo de vida
        // Si se ha desactivado este tick (expiró), vuelve a la pool
        if (!spell->IsActive()) devolverAlPool(spell.get());
    }
}

// CAMBIO: solo si está activo (evita meter el mismo índice dos veces en la lista libre)
void HechizosGestor::destroySpell(Hechizo* spell) {
    if (spell && spell->IsActive()) {
        devolverAlPool(spell);
    }
}

// NUEVO: entrada para colisiones (choque con el mundo)
void HechizosGestor::procesarChoqueSuperficie(Hechizo* spell, const glm::vec3& normal, float penetracion) {
    if (!spell || !spell->IsActive()) return;
    spell->alChocarSuperficie(normal, penetracion);
    // Si el hechizo ha decidido desaparecer, vuelve al pool
    if (!spell->IsActive()) devolverAlPool(spell);
}

// CAMBIO: antes hacía clear() y el pool quedaba inservible; ahora desactiva todos
void HechizosGestor::emptySpells() {
    for (auto& spell : spellPool) {
        if (spell->IsActive()) devolverAlPool(spell.get());
    }
}

const std::unordered_map<int, hechizosTemplate>& HechizosGestor::getCatalogoHechizos() const {
    return hechizhosCatalog;
}

const std::vector<std::unique_ptr<Hechizo>>& HechizosGestor::getActiveHechizos() const {
    return spellPool;
}