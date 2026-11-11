#include "lanzadorHechizos.h"
#include "hechizosGestor.h"   // aquí sí hace falta: usamos getCarta y spawnDisparo

EstadoDisparo LanzadorHechizos::leerCartas(const std::vector<int>& ranuras, int& indice,
    float manaDisponible) const {
    EstadoDisparo estado;
    LectorCartas lector;
    const int total = static_cast<int>(ranuras.size());

    // Lectura secuencial izquierda -> derecha, sin wrap: al llegar al final se corta
    while (lector.robosPendientes > 0 && indice < total) {
        const int idCarta = ranuras[indice++];
        if (idCarta < 0) continue;                                       // ranura vacía (-1)

        const Carta* carta = m_gestor.getCarta(idCarta);
        if (!carta) continue;                                            // id desconocido
        if (estado.mana + carta->getMana() > manaDisponible) continue;   // sin maná: se salta

        estado.mana += carta->getMana();
        estado.castDelay += carta->getCastDelay();
        estado.recarga += carta->getRecarga();
        carta->aplicar(estado, lector);                                  // cada carta hace lo suyo
    }
    return estado;
}

ResultadoDisparo LanzadorHechizos::disparar(const std::vector<int>& ranuras, int indiceInicial,
    float manaDisponible, const glm::vec3& origen,
    const glm::vec3& direccion, int idLanzador) {
    int indice = indiceInicial;
    const EstadoDisparo estado = leerCartas(ranuras, indice, manaDisponible);

    ResultadoDisparo r;
    if (estado.numProyectiles > 0) {
        r.proyectilesLanzados = m_gestor.spawnDisparo(estado, origen, direccion, idLanzador);
    }
    r.disparado = r.proyectilesLanzados > 0;
    r.manaGastado = estado.mana;      // como en Noita: se gasta aunque no salga nada
    r.castDelay = estado.castDelay;
    r.recarga = estado.recarga;
    r.hayQueRecargar = indice >= static_cast<int>(ranuras.size());
    r.nuevoIndice = r.hayQueRecargar ? 0 : indice;
    return r;
}