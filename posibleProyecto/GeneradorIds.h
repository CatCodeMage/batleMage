#pragma once

// Id de INSTANCIA: identifica "esta entidad concreta" (este mago, esta bola de fuego)
// durante todo el combate. No confundir con el id de TIPO (spellId, ModelId, "fire_ball").
// Reglas: lo asigna la simulación, nunca se reutiliza, INVALID_ENTITY (0) = sin id.
using EntityId = int;
constexpr EntityId INVALID_ENTITY = 0;   // como Entidad::characterId: el replay exige id > 0

// Reparte ids de instancia únicos durante un combate.
// Dueño: el simulador (miembro por valor). Los gestores que crean entidades
// reciben una referencia. La parte gráfica NO lo conoce: lee los ids del replay.
// Solo guarda el siguiente id: no lleva registro de entidades.
class GeneradorIds {
public:
    EntityId nuevo() { return m_siguiente++; }

    // Para pausar / re-simular: guardar y restaurar el contador
    EntityId estado() const { return m_siguiente; }
    void     restaurar(EntityId siguiente) { m_siguiente = siguiente; }

private:
    EntityId m_siguiente = 1;
};
