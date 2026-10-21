#pragma once
#include <vector>
#include <memory>
#include "hechizos.h"

struct hechizosTemplate {
    SpellDefinitionComponent definition;
    float speed;
    float radius;
    glm::vec3 scale;
    float defaultLifeTime;
};

// en vez de crear y destruir hechizos todo el rato, creamos una piscina con una cantindad poolSize
// con esto cogemos un hechizo plantilla que hay dentro le metemos los valores del hechizo que haya en catalogo
// lo usan y una vez que acaba hacemos reset del hechizo y vuelve a la pool
// con esto hace que no tengamos que pedir memoria constantemente y haya menos fragmentación
class HechizosGestor {
public:
    // Constructor: Pre-crea un número de hechizos en memoria (Pool)
    HechizosGestor(size_t poolSize);

    // para registrar los datos del un hechizo 
    void registrarHechizo(int id, const std::string& name, ElementType element, float cost, 
    float damage, float cooldown, float speed, float radius, glm::vec3 scale,float maxLifeTime);

    // Lanza un hechizo buscando uno inactivo en la pool y aplicando las propiedades
    void spawnSpell(int spellId, const glm::vec3& initialPosition, const glm::vec3& direction);

    // Actualiza todos los hechizos activos (llama al update de cada uno)
    void update(float deltaTime);

    // Método destruya/desactive un hechizo específico
    void destroySpell(Hechizo* spell);

    // Obtener la lista de hechizos activos
    const std::vector<std::unique_ptr<Hechizo>>& getActiveHechizos() const;

    // Obtener catalogo de hechizos
    const std::unordered_map<int, hechizosTemplate>& getCatalogoHechizos() const;

private:
    std::vector<std::unique_ptr<Hechizo>> spellPool;
    
    std::unordered_map<int, hechizosTemplate> hechizhosCatalog;
};