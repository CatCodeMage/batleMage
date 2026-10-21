#include "hechizosGestor.h"

HechizosGestor::HechizosGestor(size_t poolSize) {
    // Pre-allocamos los hechizos inactivos para optimizar rendimiento
    for (size_t i = 0; i < poolSize; ++i) {
        // Creamos una instancia vacía o con valores por defecto
        auto spell = std::make_unique<Hechizo>(0, "", ElementType::Physical, 0.f, 0.f, 0.f, 0.f, 0.f);
        spell->setActive(false);
        spellPool.push_back(std::move(spell));
    }
}

void HechizosGestor::registrarHechizo(int id, const std::string& name, ElementType element, float cost, 
    float damage, float cooldown, float speed, float radius, glm::vec3 scale,float maxLifeTime) {
    
    // Creamos el componente de definición estática
    // CAMBIAR: DE MOMENTO PONGO QUE ES FULL PROYECTILES Y LINEAR
    SpellDefinitionComponent def = { id, name, element, CastType::Projectile, MovementPattern::Linear, cost, damage, cooldown };

    // Lo guardamos en el catálogo junto con sus datos de movimiento
    hechizhosCatalog[id] = { def, speed, radius, scale, maxLifeTime };
}

void HechizosGestor::spawnSpell(int spellId, const glm::vec3& initialPosition, const glm::vec3& direction) {
    // Buscamos la plantilla del hechizo en el catálogo
    auto it = hechizhosCatalog.find(spellId);
    if (it == hechizhosCatalog.end()) {
        std::cerr << "[HechizosManager] Error: El hechizo con ID " << spellId << " no está registrado en el catálogo.\n";
        return;
    }

    const hechizosTemplate& templateInfo = it->second;

    // Buscamos un hechizo inactivo en el Object Pool para reutilizarlo
    for (auto& spell : spellPool) {
        // mejor no quitar if, es para coger hechizos no activo
        if (!spell->IsActive()) {
            // Reconfiguramos las propiedades del objeto del pool usando la plantilla del catálogo
            spell->setDefinition(templateInfo.definition);
            spell->setSpeed(templateInfo.speed);
            spell->setRadius(templateInfo.radius);
            spell->setMaxLifeTime(templateInfo.defaultLifeTime);
            
            // Inicializamos la posición, dirección y activamos el proyectil
            spell->cast(initialPosition, direction);
            return;
        }
    }

    std::cerr << "[HechizosManager] Advertencia: No hay hechizos disponibles en el pool (Pool exhausto).\n";
}

void HechizosGestor::update(float deltaTime) {
    for (auto& spell : spellPool) {
        if (spell->IsActive()) {
            spell->update(deltaTime); // Mueve el hechizo y gestiona su propio tiempo de vida
        }
    }
}

void HechizosGestor::destroySpell(Hechizo* spell) {
    if (spell) {
        // Al destruirlo, queda libre para la pool
        spell->reset();
    }
}

const std::vector<std::unique_ptr<Hechizo>>& HechizosGestor::getActiveHechizos() const {
    return spellPool;
}