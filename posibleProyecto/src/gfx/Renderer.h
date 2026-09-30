#pragma once
#include <vector>

#include <glm/glm.hpp>

#include "gfx/AutoCamera.h"
#include "gfx/Mesh.h"
#include "gfx/Shader.h"
#include "gfx/Texture.h"
#include "sim/Battle.h"

namespace gfx {

// Dibuja el estado de una Battle. Solo LEE la simulacion: no la modifica nunca.
// Iluminacion Phong (luz direccional + luces puntuales que emiten los proyectiles),
// suelo texturizado, sombras planas proyectadas, escudos translucidos y barras de vida.
class Renderer {
public:
    Renderer();   // requiere un contexto OpenGL activo

    void render(const sim::Battle& battle, const AutoCamera& camera, int width, int height);

private:
    struct Material {
        glm::vec3 color{1.0f};
        glm::vec3 emissive{0.0f};
        float alpha = 1.0f;
        float specular = 0.3f;
        float shininess = 32.0f;
        bool unlit = false;
        bool textured = false;
    };

    // Una pieza a dibujar: la escena se "recolecta" primero y luego se dibuja en varias pasadas
    // (opaca, sombras...) sin duplicar la logica de construccion de los modelos.
    struct Part {
        const Mesh* mesh;
        glm::mat4 model;
        Material material;
        bool castsShadow;
    };

    void collectParts(const sim::Battle& battle, std::vector<Part>& out) const;
    void addFighter(const sim::Battle& battle, int i, std::vector<Part>& out) const;
    void addProjectiles(const sim::Battle& battle, std::vector<Part>& out) const;
    void addPillars(const sim::Battle& battle, std::vector<Part>& out) const;

    void setLights(const sim::Battle& battle) const;
    void draw(const Mesh& mesh, const glm::mat4& model, const Material& m) const;
    void drawShields(const sim::Battle& battle) const;
    void drawBars(const sim::Battle& battle, const AutoCamera& camera) const;

    Shader shader_;
    Mesh cube_;
    Mesh sphere_;
    Mesh plane_;
    Texture floorTex_;
    glm::vec3 lightDir_;   // direccion HACIA la luz
};

}  // namespace gfx
