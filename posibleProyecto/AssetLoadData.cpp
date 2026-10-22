#include "AssetLoadData.h"
#include "Paths.h"

// La implementacion de tinyobjloader se compila aqui y en ningun otro .cpp.
// fast_float desactivado: la version que trae la libreria no compila en C++20
// con MSVC (error C3615 en loop_parse_if_eight_digits).
#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

constexpr int NO_MATERIAL = -1;

// Constantes y definiciones para calcular las normales cuando no estan en .obj
constexpr int       CORNERS_PER_TRIANGLE = 3;
constexpr float     MIN_CROSS_LENGTH = 1e-12f;              // por debajo, el triangulo no tiene area
const     glm::vec3 DEGENERATE_NORMAL = glm::vec3(0, 1, 0);  // para no dividir por cero al normalizar

using TriangleCorners = std::array<tinyobj::index_t, CORNERS_PER_TRIANGLE>;

// Constantes y definiciones necesarias para leer todos los .obj
using json = nlohmann::json;

constexpr int      MISSING_INDEX = -1;    // asi marca tinyobj un vt o vn que falta
constexpr int      BITS_PER_INDEX = 21;    // la clave del dedup guarda 3 indices en 64 bits
constexpr uint64_t MAX_INDEX_IN_KEY = (1ull << BITS_PER_INDEX) - 1;

using VertexByCornerKey = std::unordered_map<uint64_t, unsigned>;

const glm::vec2 MISSING_UV = glm::vec2(0.0f);   // sin vt: la textura blanca da igual donde se muestree

// La v del OBJ crece hacia arriba y las filas de una imagen hacia abajo: se invierte aqui, una sola vez
static glm::vec2 readTexCoord(const std::vector<tinyobj::real_t>& texcoords, int index) {
    if (index == MISSING_INDEX) return MISSING_UV;
    return glm::vec2(texcoords[2 * index + 0], 1.0f - texcoords[2 * index + 1]);
}

static std::runtime_error objError(const std::filesystem::path& file, const std::string& reason) {
    return std::runtime_error(file.string() + ": " + reason);
}



static glm::vec3 readVec3(const std::vector<tinyobj::real_t>& floats, int index) {
    return glm::vec3(floats[3 * index + 0], floats[3 * index + 1], floats[3 * index + 2]);
}

static void parseObjFile(const std::filesystem::path& file, tinyobj::ObjReader& reader) {
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;    // las caras de 4 o mas lados llegan ya partidas en triangulos

    if (!reader.ParseFromFile(file.string(), config))
        throw objError(file, "no se puede leer: " + reader.Error());

    if (!reader.Warning().empty())
        std::cerr << "[OBJ] " << file.string() << ": " << reader.Warning();
}

static void checkIndex(int index, size_t floatCount, int floatsPerElement,
    const std::string& elementName, const std::filesystem::path& file) {
    const size_t elementCount = floatCount / floatsPerElement;
    const bool isInRange = index >= 0 && (size_t)index < elementCount;
    if (!isInRange)
        throw objError(file, "indice de " + elementName + " fuera de rango: " +
            std::to_string(index + 1) + " (hay " + std::to_string(elementCount) + ")");

    const bool fitsInCornerKey = (uint64_t)index + 1 <= MAX_INDEX_IN_KEY;
    if (!fitsInCornerKey)
        throw objError(file, "demasiados elementos de tipo " + elementName + " para la clave del dedup");
}

// tinyobj no comprueba que un indice positivo exista: sin esto leeriamos fuera del vector
static void checkCornerIsInRange(const tinyobj::index_t& corner, const tinyobj::attrib_t& attrib,
    const std::filesystem::path& file) {
    checkIndex(corner.vertex_index, attrib.vertices.size(), 3, "vertice (v)", file);

    const bool hasTexCoord = corner.texcoord_index != MISSING_INDEX;
    if (hasTexCoord) checkIndex(corner.texcoord_index, attrib.texcoords.size(), 2, "uv (vt)", file);

    const bool hasNormal = corner.normal_index != MISSING_INDEX;
    if (hasNormal) checkIndex(corner.normal_index, attrib.normals.size(), 3, "normal (vn)", file);
}

static bool triangleHasAllNormals(const TriangleCorners& triangle) {
    for (const tinyobj::index_t& corner : triangle)
        if (corner.normal_index == MISSING_INDEX) return false;
    return true;
}

// El orden antihorario de las esquinas (el del OBJ) hace que la normal apunte hacia fuera
static glm::vec3 computeFaceNormal(const TriangleCorners& triangle, const std::vector<tinyobj::real_t>& positions) {
    const glm::vec3 a = readVec3(positions, triangle[0].vertex_index);
    const glm::vec3 b = readVec3(positions, triangle[1].vertex_index);
    const glm::vec3 c = readVec3(positions, triangle[2].vertex_index);
    const glm::vec3 perpendicular = glm::cross(b - a, c - a);

    const bool isDegenerate = glm::length(perpendicular) < MIN_CROSS_LENGTH;
    if (isDegenerate) return DEGENERATE_NORMAL;
    return glm::normalize(perpendicular);
}

static void assignFlatNormal(TriangleCorners& triangle, const std::vector<tinyobj::real_t>& positions,
    std::vector<tinyobj::real_t>& normals) {
    const glm::vec3 faceNormal = computeFaceNormal(triangle, positions);
    const int faceNormalIndex = (int)(normals.size() / 3);
    normals.insert(normals.end(), { faceNormal.x, faceNormal.y, faceNormal.z });
    for (tinyobj::index_t& corner : triangle) corner.normal_index = faceNormalIndex;
}

// Se suma 1 a cada indice para que 0 signifique "falta" (tinyobj usa -1)
static uint64_t makeCornerKey(const tinyobj::index_t& corner) {
    const uint64_t position = (uint64_t)(corner.vertex_index + 1);
    const uint64_t texCoord = (uint64_t)(corner.texcoord_index + 1);
    const uint64_t normal = (uint64_t)(corner.normal_index + 1);
    return (position << (2 * BITS_PER_INDEX)) | (texCoord << BITS_PER_INDEX) | normal;
}

static unsigned findOrAddVertex(const tinyobj::index_t& corner, const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::real_t>& normals, std::vector<float>& verts, VertexByCornerKey& vertexByCornerKey) {
    const uint64_t key = makeCornerKey(corner);
    VertexByCornerKey::const_iterator alreadyCreated = vertexByCornerKey.find(key);
    if (alreadyCreated != vertexByCornerKey.end()) return alreadyCreated->second;

    const unsigned  newVertex = (unsigned)(verts.size() / FLOATS_PER_VERTEX);
    const glm::vec3 position = readVec3(attrib.vertices, corner.vertex_index);
    const glm::vec3 normal = readVec3(normals, corner.normal_index);
    const glm::vec2 uv = readTexCoord(attrib.texcoords, corner.texcoord_index);
    verts.insert(verts.end(), { position.x, position.y, position.z, normal.x, normal.y, normal.z, uv.x, uv.y });
    vertexByCornerKey.emplace(key, newVertex);
    return newVertex;
}

struct PieceBuilder {
    ObjPiece          piece;
    VertexByCornerKey vertexByCornerKey;   
};

using PieceBuilderByMaterial = std::map<int, PieceBuilder>;

static TriangleCorners readTriangle(const std::vector<tinyobj::index_t>& corners, size_t triangleIndex) {
    const size_t first = triangleIndex * CORNERS_PER_TRIANGLE;
    return { corners[first], corners[first + 1], corners[first + 2] };
}

static void addTriangle(TriangleCorners triangle, const tinyobj::attrib_t& attrib,
    std::vector<tinyobj::real_t>& normals, PieceBuilder& builder, const std::filesystem::path& file) {
    for (const tinyobj::index_t& corner : triangle) checkCornerIsInRange(corner, attrib, file);

    if (!triangleHasAllNormals(triangle)) assignFlatNormal(triangle, attrib.vertices, normals);

    for (const tinyobj::index_t& corner : triangle)
        builder.piece.idx.push_back(findOrAddVertex(corner, attrib, normals,
            builder.piece.verts, builder.vertexByCornerKey));
}

static void splitShapeByMaterial(const tinyobj::shape_t& shape, const tinyobj::attrib_t& attrib,
    std::vector<tinyobj::real_t>& normals, const std::filesystem::path& file, std::vector<ObjPiece>& pieces) {
    PieceBuilderByMaterial builderByMaterial;
    const size_t triangleCount = shape.mesh.indices.size() / CORNERS_PER_TRIANGLE;
    for (size_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex) {
        const int materialId = shape.mesh.material_ids[triangleIndex];
        addTriangle(readTriangle(shape.mesh.indices, triangleIndex), attrib, normals,
            builderByMaterial[materialId], file);
    }

    for (std::pair<const int, PieceBuilder>& entry : builderByMaterial) {
        entry.second.piece.shapeName = shape.name;
        entry.second.piece.materialId = entry.first;
        pieces.push_back(std::move(entry.second.piece));
    }
}

// Las rutas del .mtl son relativas a su carpeta, que es la del .obj (mtl_search_path vacio).
// Si Blender escribio una ruta absoluta, el operador / se queda con ella tal cual.
static ObjMaterial describeMaterial(int materialId, const std::vector<tinyobj::material_t>& materials,
    const std::filesystem::path& objFolder) {
    const bool hasMaterial = materialId != NO_MATERIAL;
    if (!hasMaterial) return ObjMaterial();

    const tinyobj::material_t& source = materials[materialId];
    ObjMaterial material;
    material.name = source.name;
    material.color = glm::vec4(source.diffuse[0], source.diffuse[1], source.diffuse[2], 1.0f);

    const bool hasAlbedoTexture = !source.diffuse_texname.empty();
    if (hasAlbedoTexture) material.albedoTexture = objFolder / source.diffuse_texname;
    return material;
}

std::vector<ObjPiece> loadObj(const std::filesystem::path& file) {
    tinyobj::ObjReader reader;
    parseObjFile(file, reader);
    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    std::vector<tinyobj::real_t> normals = attrib.normals;   // copia: aqui se anaden las normales planas

    std::vector<ObjPiece> pieces;
    for (const tinyobj::shape_t& shape : reader.GetShapes())
        splitShapeByMaterial(shape, attrib, normals, file, pieces);

    if (pieces.empty()) throw objError(file, "no tiene caras");

    for (ObjPiece& piece : pieces)
        piece.material = describeMaterial(piece.materialId, reader.GetMaterials(), file.parent_path());

    return pieces;
}

static GLuint loadAlbedoTexture(const ObjMaterial& material, TextureLibrary& textures) {
    const bool hasAlbedoTexture = !material.albedoTexture.empty();
    return hasAlbedoTexture ? textures.load(material.albedoTexture) : textures.white();
}

static Model registerPieces(const std::string& assetName, const std::vector<ObjPiece>& pieces, MeshLibrary& meshes, TextureLibrary& textures) {
    Model model;
    for (size_t pieceIndex = 0; pieceIndex < pieces.size(); ++pieceIndex) {
        const ObjPiece& piece = pieces[pieceIndex];
        const std::string meshName = assetName + "#" + std::to_string(pieceIndex);
        const MeshId mesh = meshes.add(meshName, piece.verts, piece.idx);
        model.parts.push_back({ mesh, piece.material.color, loadAlbedoTexture(piece.material, textures) });
    }
    return model;
}

int loadAssetManifest(const std::filesystem::path& manifest, MeshLibrary& meshes, ModelLibrary& models,
    TextureLibrary& textures) {

    std::ifstream in(manifest);
    if (!in) throw std::runtime_error("no se puede abrir " + std::filesystem::absolute(manifest).string());

    json root = json::parse(in);
    int cargados = 0;
    for (json::const_iterator it = root.begin(); it != root.end(); ++it) {
        const std::string& name = it.key();
        try {
            if (models.has(name)) throw std::runtime_error("nombre repetido");
            std::string path = it.value().at("path").get<std::string>();

            const std::vector<ObjPiece> pieces = loadObj(dataPath(path));
            models.add(name, registerPieces(name, pieces, meshes, textures));
            ++cargados;
        }
        catch (const std::exception& ex) {
            throw std::runtime_error("asset '" + name + "': " + ex.what());
        }
    }
    return cargados;
}

ObjPiece::ObjPiece() {
    this->materialId = NO_MATERIAL;
}