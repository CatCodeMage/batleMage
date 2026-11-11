#include "AssimpModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace bm::anim {
namespace {

constexpr double DEFAULT_TICKS_PER_SECOND = 25.0;
constexpr float MIN_WEIGHT = 1e-8f;

using NodeMap = std::unordered_map<std::string, std::int32_t>;
using MeshAttachment = std::pair<std::uint32_t, std::uint32_t>; // meshIndex, nodeIndex

[[nodiscard]] glm::vec3 toGlm(const aiVector3D& v) {
    return {v.x, v.y, v.z};
}

[[nodiscard]] glm::quat toGlm(const aiQuaternion& q) {
    return glm::normalize(glm::quat(q.w, q.x, q.y, q.z));
}

// aiMatrix4x4 nombra filas a/b/c/d; GLM recibe columnas.
[[nodiscard]] glm::mat4 toGlm(const aiMatrix4x4& m) {
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4);
}

[[nodiscard]] std::runtime_error importError(const std::filesystem::path& file,
                                              const std::string& message) {
    return std::runtime_error("AssimpModelLoader [" + file.string() + "]: " + message);
}

struct HierarchyData {
    NodeMap nodeByName;
    std::vector<MeshAttachment> attachments;
    std::vector<glm::mat4> globals;
};

void readHierarchy(const aiNode* node, std::int32_t parentIndex,
                   const glm::mat4& parentGlobal, ImportedModel& output,
                   HierarchyData& hierarchy) {
    const auto index = static_cast<std::uint32_t>(output.nodes.size());
    const std::string name = node->mName.C_Str();
    const glm::mat4 local = toGlm(node->mTransformation);
    const glm::mat4 global = parentGlobal * local;

    output.nodes.push_back({name, parentIndex, local});
    hierarchy.globals.push_back(global);

    // Si un nombre está repetido, lo marcamos como ambiguo. Los canales y
    // huesos que lo referencien serán rechazados explícitamente.
    const auto [it, inserted] = hierarchy.nodeByName.emplace(name, static_cast<std::int32_t>(index));
    if (!inserted) it->second = -1;

    for (std::uint32_t i = 0; i < node->mNumMeshes; ++i) {
        hierarchy.attachments.emplace_back(node->mMeshes[i], index);
    }
    for (std::uint32_t i = 0; i < node->mNumChildren; ++i) {
        readHierarchy(node->mChildren[i], static_cast<std::int32_t>(index),
                      global, output, hierarchy);
    }
}

[[nodiscard]] std::uint32_t findNodeIndex(const std::string& name,
                                           const NodeMap& nodeByName,
                                           const std::filesystem::path& file) {
    const auto it = nodeByName.find(name);
    if (it == nodeByName.end()) {
        throw importError(file, "el hueso/canal '" + name + "' no tiene nodo en la jerarquía");
    }
    if (it->second < 0) {
        throw importError(file, "nombre de nodo ambiguo '" + name + "' (nodos duplicados)");
    }
    return static_cast<std::uint32_t>(it->second);
}

[[nodiscard]] ImportedTexture readAlbedo(const aiScene& scene, const aiMaterial& material,
                                          const std::filesystem::path& file) {
    ImportedTexture result;
    aiString texturePath;
    unsigned int uvChannel = 0;

    aiReturn status = material.GetTexture(aiTextureType_BASE_COLOR, 0,
                                          &texturePath, nullptr, &uvChannel);
    if (status != aiReturn_SUCCESS) {
        status = material.GetTexture(aiTextureType_DIFFUSE, 0,
                                     &texturePath, nullptr, &uvChannel);
    }
    if (status != aiReturn_SUCCESS) return result;
    if (uvChannel != 0) {
        throw importError(file, "una textura utiliza UV" + std::to_string(uvChannel) +
                                "; esta primera versión soporta únicamente UV0");
    }

    const aiTexture* embedded = scene.GetEmbeddedTexture(texturePath.C_Str());
    if (!embedded) {
        result.source = ImportedTexture::Source::ExternalFile;
        result.file = (file.parent_path() / std::filesystem::path(texturePath.C_Str())).lexically_normal();
        return result;
    }

    result.formatHint = embedded->achFormatHint;
    if (!embedded->pcData) throw importError(file, "textura embebida sin datos");

    if (embedded->mHeight == 0) {
        // Imagen comprimida (PNG, JPG...): mWidth es el número de bytes.
        result.source = ImportedTexture::Source::EmbeddedCompressed;
        const auto* begin = reinterpret_cast<const std::uint8_t*>(embedded->pcData);
        result.bytes.assign(begin, begin + embedded->mWidth);
    } else {
        // Imagen no comprimida: aiTexel usa campos B/G/R/A, pero exponemos RGBA8.
        result.source = ImportedTexture::Source::EmbeddedRGBA8;
        result.width = embedded->mWidth;
        result.height = embedded->mHeight;
        const auto pixels = static_cast<std::size_t>(result.width) * result.height;
        result.bytes.resize(pixels * 4);
        for (std::size_t i = 0; i < pixels; ++i) {
            const aiTexel& p = embedded->pcData[i];
            result.bytes[i * 4 + 0] = p.r;
            result.bytes[i * 4 + 1] = p.g;
            result.bytes[i * 4 + 2] = p.b;
            result.bytes[i * 4 + 3] = p.a;
        }
    }
    return result;
}

[[nodiscard]] ImportedMaterial readMaterial(const aiScene& scene, unsigned materialIndex,
                                             const std::filesystem::path& file) {
    ImportedMaterial result;
    if (materialIndex >= scene.mNumMaterials) return result;

    const aiMaterial& material = *scene.mMaterials[materialIndex];
    aiString materialName;
    if (material.Get(AI_MATKEY_NAME, materialName) == aiReturn_SUCCESS) {
        result.name = materialName.C_Str();
    }

    aiColor4D color;
    if (material.Get(AI_MATKEY_BASE_COLOR, color) == aiReturn_SUCCESS ||
        material.Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS) {
        result.baseColor = glm::vec4(color.r, color.g, color.b, color.a);
    }
    result.albedo = readAlbedo(scene, material, file);
    return result;
}

struct Influence {
    std::int32_t boneId;
    float weight;
};

void assignWeights(ImportedMesh& output, const aiMesh& source,
                   const NodeMap& nodeByName, const std::filesystem::path& file) {
    std::vector<std::vector<Influence>> weights(source.mNumVertices);

    for (std::uint32_t i = 0; i < source.mNumBones; ++i) {
        const aiBone& bone = *source.mBones[i];
        const std::string boneName = bone.mName.C_Str();
        const std::uint32_t nodeIndex = findNodeIndex(boneName, nodeByName, file);
        const auto id = static_cast<std::int32_t>(output.bones.size());
        output.bones.push_back({boneName, nodeIndex, toGlm(bone.mOffsetMatrix)});

        for (std::uint32_t j = 0; j < bone.mNumWeights; ++j) {
            const aiVertexWeight& weight = bone.mWeights[j];
            if (weight.mVertexId >= source.mNumVertices) {
                throw importError(file, "vértice fuera de rango en hueso '" + boneName + "'");
            }
            if (!std::isfinite(weight.mWeight) || weight.mWeight < 0.0f) {
                throw importError(file, "peso inválido en hueso '" + boneName + "'");
            }
            if (weight.mWeight > MIN_WEIGHT) {
                weights[weight.mVertexId].push_back({id, weight.mWeight});
            }
        }
    }

    // Vértices de una malla deformable que no tienen pesos: se asocian al
    // nodo de la propia malla (influencia rígida) para evitar un resultado nulo.
    std::int32_t fallbackBone = -1;
    for (std::size_t vertexId = 0; vertexId < weights.size(); ++vertexId) {
        auto& influences = weights[vertexId];
        if (influences.empty() && source.mNumBones > 0) {
            if (fallbackBone == -1) {
                fallbackBone = static_cast<std::int32_t>(output.bones.size());
                output.bones.push_back({"__mesh_fallback_" + output.name,
                                        output.attachedNode, glm::mat4(1.0f)});
            }
            influences.push_back({fallbackBone, 1.0f});
        }

        // Nos quedamos con las cuatro contribuciones más altas y renormalizamos.
        std::sort(influences.begin(), influences.end(), [](const Influence& a, const Influence& b) {
            return a.weight > b.weight;
        });
        if (influences.size() > MAX_BONE_INFLUENCES) influences.resize(MAX_BONE_INFLUENCES);

        float sum = 0.0f;
        for (const auto& influence : influences) sum += influence.weight;
        if (sum <= MIN_WEIGHT) continue;
        for (std::size_t i = 0; i < influences.size(); ++i) {
            output.vertices[vertexId].boneIds[i] = influences[i].boneId;
            output.vertices[vertexId].boneWeights[i] = influences[i].weight / sum;
        }
    }
}

[[nodiscard]] ImportedMesh readMesh(const aiScene& scene, const aiMesh& source,
                                     std::uint32_t attachedNode, const glm::mat4& meshBindGlobal,
                                     const NodeMap& nodeByName, const std::filesystem::path& file,
                                     std::uint32_t meshIndex) {
    ImportedMesh output;
    output.name = source.mName.length ? source.mName.C_Str() : "mesh_" + std::to_string(meshIndex);
    output.attachedNode = attachedNode;
    output.meshBindGlobal = meshBindGlobal;
    output.material = readMaterial(scene, source.mMaterialIndex, file);

    if (!source.HasPositions()) throw importError(file, "malla sin posiciones: " + output.name);
    output.vertices.resize(source.mNumVertices);
    for (std::uint32_t i = 0; i < source.mNumVertices; ++i) {
        SkinnedVertex& vertex = output.vertices[i];
        vertex.position = toGlm(source.mVertices[i]);
        if (source.HasNormals()) vertex.normal = toGlm(source.mNormals[i]);
        if (source.HasTextureCoords(0)) {
            vertex.uv = glm::vec2(source.mTextureCoords[0][i].x, source.mTextureCoords[0][i].y);
        }
    }

    output.indices.reserve(static_cast<std::size_t>(source.mNumFaces) * 3);
    for (std::uint32_t faceId = 0; faceId < source.mNumFaces; ++faceId) {
        const aiFace& face = source.mFaces[faceId];
        if (face.mNumIndices != 3) {
            throw importError(file, "se esperaba una malla triangulada: " + output.name);
        }
        for (std::uint32_t j = 0; j < 3; ++j) {
            if (face.mIndices[j] >= source.mNumVertices) {
                throw importError(file, "índice de vértice fuera de rango en " + output.name);
            }
            output.indices.push_back(face.mIndices[j]);
        }
    }
    assignWeights(output, source, nodeByName, file);
    return output;
}

[[nodiscard]] ImportedClip readClip(const aiAnimation& source, const NodeMap& nodeByName,
                                     const std::filesystem::path& file, unsigned clipIndex) {
    ImportedClip clip;
    clip.name = source.mName.length ? source.mName.C_Str() : "animation_" + std::to_string(clipIndex);
    clip.durationTicks = source.mDuration;
    clip.ticksPerSecond = source.mTicksPerSecond > 0.0
        ? source.mTicksPerSecond : DEFAULT_TICKS_PER_SECOND;

    clip.channels.reserve(source.mNumChannels);
    for (std::uint32_t i = 0; i < source.mNumChannels; ++i) {
        const aiNodeAnim& src = *source.mChannels[i];
        ImportedChannel channel;
        channel.nodeIndex = findNodeIndex(src.mNodeName.C_Str(), nodeByName, file);

        channel.positions.reserve(src.mNumPositionKeys);
        for (std::uint32_t k = 0; k < src.mNumPositionKeys; ++k) {
            const aiVectorKey& key = src.mPositionKeys[k];
            channel.positions.push_back({key.mTime, toGlm(key.mValue)});
        }
        channel.rotations.reserve(src.mNumRotationKeys);
        for (std::uint32_t k = 0; k < src.mNumRotationKeys; ++k) {
            const aiQuatKey& key = src.mRotationKeys[k];
            channel.rotations.push_back({key.mTime, toGlm(key.mValue)});
        }
        channel.scales.reserve(src.mNumScalingKeys);
        for (std::uint32_t k = 0; k < src.mNumScalingKeys; ++k) {
            const aiVectorKey& key = src.mScalingKeys[k];
            channel.scales.push_back({key.mTime, toGlm(key.mValue)});
        }
        clip.channels.push_back(std::move(channel));
    }
    return clip;
}

} // namespace

ImportedModel AssimpModelLoader::load(const std::filesystem::path& file) {
    if (!std::filesystem::exists(file)) {
        throw importError(file, "el archivo no existe");
    }

    Assimp::Importer importer;
    // No usar aiProcess_PreTransformVertices: destruiría la jerarquía.
    constexpr unsigned int flags = aiProcess_Triangulate |
                                   aiProcess_JoinIdenticalVertices |
                                   aiProcess_GenSmoothNormals |
                                   aiProcess_FlipUVs |
                                   aiProcess_ValidateDataStructure;

    const aiScene* scene = importer.ReadFile(file.string(), flags);
    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        throw importError(file, importer.GetErrorString());
    }

    ImportedModel output;
    output.sourceFile = file;
    HierarchyData hierarchy;
    readHierarchy(scene->mRootNode, -1, glm::mat4(1.0f), output, hierarchy);

    output.meshes.reserve(hierarchy.attachments.size());
    for (const auto& [meshIndex, nodeIndex] : hierarchy.attachments) {
        if (meshIndex >= scene->mNumMeshes) {
            throw importError(file, "la jerarquía referencia una malla inexistente");
        }
        output.meshes.push_back(readMesh(*scene, *scene->mMeshes[meshIndex], nodeIndex,
                                         hierarchy.globals[nodeIndex], hierarchy.nodeByName,
                                         file, meshIndex));
    }
    if (output.meshes.empty()) throw importError(file, "el modelo no contiene mallas");

    output.clips.reserve(scene->mNumAnimations);
    for (std::uint32_t i = 0; i < scene->mNumAnimations; ++i) {
        output.clips.push_back(readClip(*scene->mAnimations[i], hierarchy.nodeByName, file, i));
    }
    return output; // todos los datos son propios; importer se destruye de forma segura
}

} // namespace bm::anim
