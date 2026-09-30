#include "gfx/MeshFactory.h"

#include <cmath>

namespace gfx {

Mesh makeCube() {
    struct Face {
        glm::vec3 n, u, v;
    };
    const Face faces[6] = {
        {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},   {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
        {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}},  {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
        {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},  {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
    };

    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;
    for (const Face& f : faces) {
        const unsigned int base = static_cast<unsigned int>(verts.size());
        const glm::vec3 c = f.n * 0.5f;
        verts.push_back({c - f.u * 0.5f - f.v * 0.5f, f.n, {0.0f, 0.0f}});
        verts.push_back({c + f.u * 0.5f - f.v * 0.5f, f.n, {1.0f, 0.0f}});
        verts.push_back({c + f.u * 0.5f + f.v * 0.5f, f.n, {1.0f, 1.0f}});
        verts.push_back({c - f.u * 0.5f + f.v * 0.5f, f.n, {0.0f, 1.0f}});
        for (unsigned int k : {0u, 1u, 2u, 0u, 2u, 3u}) idx.push_back(base + k);
    }
    return Mesh(verts, idx);
}

Mesh makeSphere(int stacks, int slices) {
    const float pi = 3.14159265f;
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;

    for (int i = 0; i <= stacks; ++i) {
        const float theta = pi * static_cast<float>(i) / static_cast<float>(stacks);
        for (int j = 0; j <= slices; ++j) {
            const float phi = 2.0f * pi * static_cast<float>(j) / static_cast<float>(slices);
            const glm::vec3 p(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
            verts.push_back({p, p, {static_cast<float>(j) / static_cast<float>(slices), static_cast<float>(i) / static_cast<float>(stacks)}});
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            const unsigned int a = static_cast<unsigned int>(i * (slices + 1) + j);
            const unsigned int b = a + static_cast<unsigned int>(slices + 1);
            for (unsigned int k : {a, a + 1, b, a + 1, b + 1, b}) idx.push_back(k);
        }
    }
    return Mesh(verts, idx);
}

Mesh makePlane(float half, float uvRepeat) {
    const glm::vec3 n(0.0f, 1.0f, 0.0f);
    std::vector<Vertex> verts = {
        {{-half, 0.0f, -half}, n, {0.0f, 0.0f}},
        {{half, 0.0f, -half}, n, {uvRepeat, 0.0f}},
        {{half, 0.0f, half}, n, {uvRepeat, uvRepeat}},
        {{-half, 0.0f, half}, n, {0.0f, uvRepeat}},
    };
    std::vector<unsigned int> idx = {0, 2, 1, 0, 3, 2};
    return Mesh(verts, idx);
}

}  // namespace gfx
