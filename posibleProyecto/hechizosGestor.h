#pragma once
#include <vector>
#include <memory>
#include <unordered_map>   // NUEVO: faltaba
#include "hechizos.h"
#include "efectosHechizo.h"

struct hechizosTemplate {
    SpellDefinitionComponent definition;
    float speed;
    float radius;
    glm::vec3 scale;
    float defaultLifeTime;
    DatosZona zona;   // fijo (JSON): no viaja con cada hechizo en vuelo
};

// en vez de crear y destruir hechizos todo el rato, creamos una piscina con una cantindad poolSize
// con esto cogemos un hechizo plantilla que hay dentro le metemos los valores del hechizo que haya en catalogo
// lo usan y una vez que acaba hacemos reset del hechizo y vuelve a la pool
// con esto hace que no tengamos que pedir memoria constantemente y haya menos fragmentación
class HechizosGestor {
public:
    // Constructor: Pre-crea un número de hechizos en memoria (Pool)
    // 'ids' es del simulador: el gestor solo lo usa para dar id a lo que crea
    HechizosGestor(size_t poolSize, GeneradorIds& ids);

    // para registrar los datos del un hechizo 
    void registrarHechizo(
        int id, const std::string& name, ElementType element,
        float cost, float damage, float cooldown,
        float speed, float radius, glm::vec3 scale,
        float maxLifeTime,
        MovementPattern pattern = MovementPattern::Linear,
        CastType castType = CastType::Projectile,
        const DatosZona& zona = {},
        float gravityMultiplier = 0.0f
    );

    // NUEVO: registrar cartas que no vienen de spells.json
    void registrarModificador(int id, const std::string& nombre, float mana, float castDelay,
        const ModificadoresDisparo& mods);
    void registrarMulticast(int id, const std::string& nombre, float mana, float castDelay, int robos);

    // NUEVO: devuelve la carta o nullptr si no existe
    const Carta* getCarta(int id) const;

    // CAMBIO: lanza un hechizo sin modificadores (compatibilidad). Devuelve si ha salido.
    bool spawnSpell(int spellId, const glm::vec3& initialPosition, const glm::vec3& direction,
        int idLanzador = -1);

    // NUEVO: lanza todos los proyectiles de un disparo aplicando sus modificadores.
    // De momento siempre se puede lanzar. Devuelve cuántos han salido.
    int spawnDisparo(const EstadoDisparo& disparo, const glm::vec3& origen, const glm::vec3& direccion,
        int idLanzador);

    // Actualiza todos los hechizos activos (llama al update de cada uno)
    void update(float deltaTime);

    // Método destruya/desactive un hechizo específico
    void destroySpell(Hechizo* spell);

    // NUEVO: colisiones avisa de que un hechizo ha chocado con el mundo (suelo, pared, roca...).
    // El hechizo decide si rebota o desaparece; si desaparece, vuelve al pool.
    void procesarChoqueSuperficie(Hechizo* spell, const glm::vec3& normal, float penetracion);

    // Obtener la lista de hechizos activos
    const std::vector<std::unique_ptr<Hechizo>>& getActiveHechizos() const;

    // Obtener catalogo de hechizos
    const std::unordered_map<int, hechizosTemplate>& getCatalogoHechizos() const;

    // Id de una carta (hechizo, modificador o multicast) por su nombre, o -1 si no existe
    int idPorNombre(const std::string & nombre) const;

    // Zonas activas: el gestor avanza 'tiempo' y las borra; colisiones marca golpes (no es const)
    std::vector<EfectoZona>& getEfectosZona() { return efectosZona; }
    // Efectos sobre el lanzador de este tick: combate los aplica y llama a limpiar
    const std::vector<EfectoSobreLanzador>& getEfectosSobreLanzador() const { return efectosSobreLanzador; }
    void limpiarEfectosSobreLanzador() { efectosSobreLanzador.clear(); }

    void emptySpells();

private:
    // NUEVO
    Hechizo* sacarDelPool();
    void devolverAlPool(Hechizo* spell);
    bool perteneceAlPool(const Hechizo* spell) const;
    void crearZona(const DatosZona& datos, const glm::vec3& posicion, const glm::vec3& direccion,
        const CargaEfecto& carga, int idLanzador);
    void crearZonaAlTerminar(const Hechizo* spell, const glm::vec3& posicion);
    void envejecerZonas(float deltaTime);

    std::vector<std::unique_ptr<Hechizo>> spellPool;

    // NUEVO: índices libres del pool (sacar/devolver es instantáneo)
    std::vector<int> libres;

    std::unordered_map<int, hechizosTemplate> hechizhosCatalog;

    // NUEVO: todas las cartas (proyectiles, modificadores, multicast). Se crean solo al cargar.
    std::unordered_map<int, std::unique_ptr<Carta>> cartas;

    // Zonas y efectos sobre el lanzador pendientes de aplicar por colisiones/combate
    std::vector<EfectoZona> efectosZona;
    std::vector<EfectoSobreLanzador> efectosSobreLanzador;
    GeneradorIds& m_ids;   // NO es suyo: pertenece al simulador
};
