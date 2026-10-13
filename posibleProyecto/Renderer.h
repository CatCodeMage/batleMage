#pragma once
#include "Mesh.h"
#include "MeshLibrary.h"

struct DrawItem { MeshId mesh; glm::mat4 model; glm::vec4 color; int kind; };

class Renderer {
public:
    Renderer();
    void init(GLuint program);
    void submit(MeshId id, const glm::mat4& model, const glm::vec4& color, int kind = 0);
    void flush(const MeshLibrary& lib, const glm::mat4& viewProj);
private:
    std::vector<DrawItem> w_items;
    GLint w_locModel, w_locViewProj, w_locColor, w_locKind;
};