#include "hechizosGestor.h"
#include <algorithm>

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
    efectosZona.reserve(poolSize);
    efectosSobreLanzador.reserve(8);
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

bool HechizosGestor::perteneceAlPool(const Hechizo* spell) const {
    if (!spell) return false;
    const auto indice = static_cast<size_t>(spell->getIndicePool());
    return indice < spellPool.size() && spellPool[indice].get() == spell;
}

void HechizosGestor::registrarHechizo(
    int id, const std::string& name, ElementType element,
    float cost, float damage, float cooldown,
    float speed, float radius, glm::vec3 scale,
    float maxLifeTime, MovementPattern pattern, CastType castType, const DatosZona& zona)
{
    SpellDefinitionComponent def = {
        id, name, element,
        castType,
        pattern,
        cost, damage, cooldown
    };

    hechizhosCatalog[id] = { def, speed, radius, scale, maxLifeTime, zona };

    // Cada proyectil del catalogo tambien es una carta para la varita.
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
bool HechizosGestor::spawnSpell(int spellId, const glm::vec3& initialPosition, const glm::vec3& direction,
    int idLanzador) {
    // Compatibilidad: un disparo de 1 proyectil sin modificadores
    EstadoDisparo disparo;
    disparo.anadirProyectil(spellId);
    return spawnDisparo(disparo, initialPosition, direction, idLanzador) > 0;
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
        const CargaEfecto carga{ t.definition.id, t.definition.damage + m.danoExtra, t.definition.element };

        // Sobre uno mismo (curación…): sin proyectil, combate lo aplica al lanzador
        if (t.definition.casteo == CastType::Self) {
            efectosSobreLanzador.push_back({ carga, idLanzador });
            ++lanzados;
            continue;
        }
        // Zona directa (nova, aliento, chorro, muro…): sin proyectil, sale de la varita hacia donde apunta
        if (t.definition.casteo == CastType::AreaOfEffect) {
            if (t.zona.tieneZona()) {
                DatosZona datos = t.zona;
                datos.velocidadAvance *= m.multVelocidad;   // speed_up también acelera las zonas que avanzan
                crearZona(datos, origen, direccion, carga, idLanzador);
            }
            ++lanzados;
            continue;
        }

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
    envejecerZonas(deltaTime);   // antes de crear las de este tick
    for (auto& spell : spellPool) {
        if (!spell->IsActive()) continue;
        spell->update(deltaTime); // Mueve el hechizo y gestiona su propio tiempo de vida
        // Si se ha desactivado este tick (expiró), deja su zona (si tiene) y vuelve a la pool
        if (!spell->IsActive()) {
            crearZonaAlTerminar(spell.get(), spell->getPosition());
            devolverAlPool(spell.get());
        }
    }
}

// CAMBIO: solo si está activo (evita meter el mismo índice dos veces en la lista libre)
void HechizosGestor::destroySpell(Hechizo* spell) {
    if (perteneceAlPool(spell) && spell->IsActive()) {
        crearZonaAlTerminar(spell, spell->getPosition());   // ha golpeado a un mago
        devolverAlPool(spell);
    }
}

// NUEVO: entrada para colisiones (choque con el mundo)
void HechizosGestor::procesarChoqueSuperficie(Hechizo* spell, const glm::vec3& normal, float penetracion) {
    if (!perteneceAlPool(spell) || !spell->IsActive()) return;
    spell->alChocarSuperficie(normal, penetracion);
    // Si el hechizo ha decidido desaparecer, deja su zona a ras de la superficie y vuelve al pool
    if (!spell->IsActive()) {
        crearZonaAlTerminar(spell, spell->getPosition() + normal * penetracion);
        devolverAlPool(spell);
    }
}

int HechizosGestor::idPorNombre(const std::string& nombre) const
{
    for (const auto& [id, carta] : cartas) {
        if (carta->getNombre() == nombre) return id;
    }
    return -1;
}

// CAMBIO: antes hacía clear() y el pool quedaba inservible; ahora desactiva todos
void HechizosGestor::emptySpells() {
    for (auto& spell : spellPool) {
        if (spell->IsActive()) devolverAlPool(spell.get());
    }
    efectosZona.clear();
    efectosSobreLanzador.clear();
}

void HechizosGestor::crearZona(const DatosZona& datos, const glm::vec3& posicion, const glm::vec3& direccion,
    const CargaEfecto& carga, int idLanzador) {
    EfectoZona z;
    z.datos = datos;
    z.posicion = posicion;
    z.direccion = direccion;
    // Las zonas que avanzan lo hacen en horizontal, a la altura desde la que se lanzaron
    if (datos.velocidadAvance > 0.0f) {
        const glm::vec3 plana(direccion.x, 0.0f, direccion.z);
        if (glm::length(plana) > 0.0001f) z.direccion = glm::normalize(plana);
    }
    z.carga = carga;
    z.idLanzador = idLanzador;
    efectosZona.push_back(std::move(z));
}

// Único sitio de "qué pasa cuando un proyectil termina". Aquí irán otras reacciones
// futuras (soltar más hechizos…) sin que colisiones se entere.
void HechizosGestor::crearZonaAlTerminar(const Hechizo* spell, const glm::vec3& posicion) {
    auto it = hechizhosCatalog.find(spell->getId());
    if (it == hechizhosCatalog.end() || !it->second.zona.tieneZona()) return;
    const SpellDefinitionComponent& def = spell->getDefinition();   // daño ya con modificadores
    crearZona(it->second.zona, posicion, spell->getDirection(),
              { def.id, def.damage, def.element }, spell->getLanzador());
}

void HechizosGestor::envejecerZonas(float deltaTime) {
    efectosZona.erase(std::remove_if(efectosZona.begin(), efectosZona.end(),
        [](const EfectoZona& z) { return z.terminado(); }), efectosZona.end());
    for (EfectoZona& z : efectosZona) {
        z.tiempo += deltaTime;
        z.posicion += z.direccion * (z.datos.velocidadAvance * deltaTime);   // 0 = quieta
    }
}

const std::unordered_map<int, hechizosTemplate>& HechizosGestor::getCatalogoHechizos() const {
    return hechizhosCatalog;
}

const std::vector<std::unique_ptr<Hechizo>>& HechizosGestor::getActiveHechizos() const {
    return spellPool;
}
