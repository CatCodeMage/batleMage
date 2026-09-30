#pragma once
#include <glm/glm.hpp>

#include "sim/Battle.h"

namespace gfx {

// Camara automatica ("director"): encuadra a los dos combatientes sin intervencion humana.
//  - Se coloca de lado respecto al eje que une a los combatientes y ajusta la distancia a la separacion.
//  - Oscila suavemente para dar dinamismo.
//  - Al terminar el combate, se acerca al ganador.
// Se actualiza con el paso FIJO de la simulacion, asi que la grabacion es reproducible.
class AutoCamera {
public:
    void reset() { initialized_ = false; }
    void update(const sim::Battle& battle, float dt);

    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;
    const glm::vec3& position() const { return pos_; }
    const glm::vec3& target() const { return target_; }

private:
    glm::vec3 pos_{0.0f, 6.0f, 14.0f};
    glm::vec3 target_{0.0f, 1.2f, 0.0f};
    bool initialized_ = false;
};

}  // namespace gfx
