#include "SkinnedMesh.h"

#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace bm::anim {
namespace {

static_assert(std::is_standard_layout_v<SkinnedVertex>);
static_assert(sizeof(std::uint32_t) == sizeof(GLuint));
static_assert(MAX_BONE_INFLUENCES == 4);

void validateMesh(const ImportedMesh& mesh) {
    if (mesh.indices.size() % 3 != 0 ||
        mesh.indices.size() > static_cast<std::size_t>((std::numeric_limits<GLsizei>::max)()) ||
        mesh.vertices.size() > static_cast<std::size_t>((std::numeric_limits<GLsizeiptr>::max)()) / sizeof(SkinnedVertex) ||
        mesh.indices.size() > static_cast<std::size_t>((std::numeric_limits<GLsizeiptr>::max)()) / sizeof(std::uint32_t)) {
        throw std::invalid_argument("SkinnedMesh: tamano o triangulacion invalidos");
    }
    for (std::uint32_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) throw std::invalid_argument("SkinnedMesh: indice fuera de rango");
    }
    for (const SkinnedVertex& vertex : mesh.vertices) {
        for (float weight : vertex.boneWeights) {
            if (!std::isfinite(weight) || weight < 0.0f) {
                throw std::invalid_argument("SkinnedMesh: peso de hueso invalido");
            }
        }
    }
}

void uploadBuffer(GLenum target, GLsizeiptr bytes, const void* data) {
    glBufferData(target, bytes, data, GL_STATIC_DRAW);
    GLint64 allocated = 0;
    glGetBufferParameteri64v(target, GL_BUFFER_SIZE, &allocated);
    if (allocated != bytes) throw std::runtime_error("SkinnedMesh: no se pudo reservar el buffer");
}

} // namespace

SkinnedMesh::SkinnedMesh(const ImportedMesh& mesh) {
    create(mesh);
}

SkinnedMesh::~SkinnedMesh() {
    destroy();
}

SkinnedMesh::SkinnedMesh(SkinnedMesh&& other) noexcept {
    *this = std::move(other);
}

SkinnedMesh& SkinnedMesh::operator=(SkinnedMesh&& other) noexcept {
    if (this == &other) return *this;
    destroy();
    m_vao = std::exchange(other.m_vao, 0);
    m_vbo = std::exchange(other.m_vbo, 0);
    m_ebo = std::exchange(other.m_ebo, 0);
    m_indexCount = std::exchange(other.m_indexCount, 0);
    m_boneCount = std::exchange(other.m_boneCount, 0);
    m_baseColor = other.m_baseColor;
    return *this;
}

void SkinnedMesh::create(const ImportedMesh& mesh) {
    validateMesh(mesh);
    GLint maxAttributes = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttributes);
    if (maxAttributes < 5) throw std::runtime_error("SkinnedMesh: se necesitan cinco atributos de vertice");

    GLint previousVao = 0, previousArrayBuffer = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
    // El temporal libera cualquier recurso parcial si falla la reserva.
    SkinnedMesh next;
    try {
        glGenVertexArrays(1, &next.m_vao);
        glGenBuffers(1, &next.m_vbo);
        glGenBuffers(1, &next.m_ebo);
        if (!next.m_vao || !next.m_vbo || !next.m_ebo) {
            throw std::runtime_error("SkinnedMesh: no se pudieron crear los objetos OpenGL");
        }
        glBindVertexArray(next.m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, next.m_vbo);
        uploadBuffer(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(SkinnedVertex)), mesh.vertices.data());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, next.m_ebo);
        uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)), mesh.indices.data());

        const GLsizei stride = sizeof(SkinnedVertex);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(SkinnedVertex, position)));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(SkinnedVertex, normal)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(SkinnedVertex, uv)));
        glVertexAttribIPointer(3, 4, GL_INT, stride, reinterpret_cast<const void*>(offsetof(SkinnedVertex, boneIds)));
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(SkinnedVertex, boneWeights)));
        for (GLuint location = 0; location < 5; ++location) glEnableVertexAttribArray(location);
        next.m_indexCount = static_cast<GLsizei>(mesh.indices.size());
        next.m_boneCount = mesh.bones.size();
        next.m_baseColor = mesh.material.baseColor;
    } catch (...) {
        glBindVertexArray(static_cast<GLuint>(previousVao));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
        throw;
    }
    glBindVertexArray(static_cast<GLuint>(previousVao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
    *this = std::move(next);
}

void SkinnedMesh::draw() const {
    if (!m_vao || m_indexCount == 0) return;
    GLint previousVao = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(static_cast<GLuint>(previousVao));
}

void SkinnedMesh::destroy() {
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vao = m_vbo = m_ebo = 0;
    m_indexCount = 0;
    m_boneCount = 0;
    m_baseColor = glm::vec4(1.0f);
}

} // namespace bm::anim
