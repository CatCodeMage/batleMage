#pragma once

// Importador de datos CPU para modelos con esqueleto.
// No realiza llamadas OpenGL ni modifica MeshLibrary / ModelLibrary.
// Requisitos: C++20, GLM y Assimp 5.x.

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace bm::anim {

constexpr std::size_t MAX_BONE_INFLUENCES = 4;

// boneIds se interpretan como índices en ImportedMesh::bones, NO como IDs
// globales del esqueleto. -1 significa que no hay influencia.
struct SkinnedVertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
    std::array<std::int32_t, MAX_BONE_INFLUENCES> boneIds{{-1, -1, -1, -1}};
    std::array<float, MAX_BONE_INFLUENCES> boneWeights{{0, 0, 0, 0}};
};

// Jerarquía de la escena. Incluye los nodos sin pesos que pueden formar parte
// de la cadena de transformación entre la raíz y un hueso.
struct ImportedNode {
    std::string name;
    std::int32_t parentIndex = -1; // -1 = raíz
    glm::mat4 bindLocal{1.0f};
};

// Binding específico de la submalla: dos submallas pueden tener offsets
// diferentes incluso cuando apuntan al mismo nodo del esqueleto.
struct ImportedBone {
    std::string name;
    std::uint32_t nodeIndex = 0; // índice en ImportedModel::nodes
    glm::mat4 offsetMatrix{1.0f}; // aiBone::mOffsetMatrix (mesh -> bone)
};

struct ImportedTexture {
    enum class Source { None, ExternalFile, EmbeddedCompressed, EmbeddedRGBA8 };
    Source source = Source::None;
    std::filesystem::path file;  // válida para ExternalFile
    std::string formatHint;      // png, jpg, etc. (si Assimp lo conoce)
    std::uint32_t width = 0;     // para EmbeddedRGBA8
    std::uint32_t height = 0;    // para EmbeddedRGBA8
    std::vector<std::uint8_t> bytes; // comprimidos o píxeles RGBA8, según source
};

struct ImportedMaterial {
    std::string name;
    glm::vec4 baseColor{1.0f};
    ImportedTexture albedo;
};

struct ImportedMesh {
    std::string name;
    std::uint32_t attachedNode = 0; // nodo al que Assimp asocia esta malla
    glm::mat4 meshBindGlobal{1.0f}; // global de attachedNode en bind pose
    std::vector<SkinnedVertex> vertices;
    std::vector<std::uint32_t> indices; // triangulados
    std::vector<ImportedBone> bones;    // paleta LOCAL de esta submalla
    ImportedMaterial material;

    bool isSkinned() const { return !bones.empty(); }
};

struct Vec3Key {
    double timeTicks = 0.0;
    glm::vec3 value{0.0f};
};

struct QuatKey {
    double timeTicks = 0.0;
    glm::quat value{1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z
};

struct ImportedChannel {
    std::uint32_t nodeIndex = 0; // nodo al que se aplica el canal
    std::vector<Vec3Key> positions;
    std::vector<QuatKey> rotations;
    std::vector<Vec3Key> scales;
};

struct ImportedClip {
    std::string name;
    double durationTicks = 0.0;
    double ticksPerSecond = 25.0; // fallback si el archivo indica 0
    std::vector<ImportedChannel> channels;
};

struct ImportedModel {
    std::filesystem::path sourceFile;
    std::vector<ImportedNode> nodes; // orden: el padre precede a los hijos
    std::vector<ImportedMesh> meshes;
    std::vector<ImportedClip> clips;
};

class AssimpModelLoader {
public:
    // Importa y copia todos los datos; la salida NO depende de que Assimp::Importer siga vivo.
    // Lanza std::runtime_error ante archivos inválidos o referencias inconsistentes.
    [[nodiscard]] static ImportedModel load(const std::filesystem::path& file);
};

} // namespace bm::anim
