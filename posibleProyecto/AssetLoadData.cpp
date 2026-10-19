#include "AssetLoadData.h"
#include "Paths.h"

// La implementacion de tinyobjloader se compila aqui y en ningun otro .cpp.
// fast_float desactivado: la version que trae la libreria no compila en C++20
// con MSVC (error C3615 en loop_parse_if_eight_digits).
#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using json = nlohmann::json;

constexpr int      FLOATS_PER_VERTEX = 6;     // posicion 3 + normal 3
constexpr int      MISSING_INDEX = -1;    // asi marca tinyobj un vt o vn que falta
constexpr int      BITS_PER_INDEX = 21;    // la clave del dedup guarda 3 indices en 64 bits
constexpr uint64_t MAX_INDEX_IN_KEY = (1ull << BITS_PER_INDEX) - 1;

using VertexByCornerKey = std::unordered_map<uint64_t, unsigned>;

static std::runtime_error objError(const std::filesystem::path& file, const std::string& reason) {
    return std::runtime_error(file.string() + ": " + reason);
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

// TEMPORAL hasta L2, que calculara la normal plana de la cara
static void checkCornerHasNormal(const tinyobj::index_t& corner, const std::filesystem::path& file) {
    if (corner.normal_index == MISSING_INDEX)
        throw objError(file, "una cara no tiene normal (vn)");
}

// Se suma 1 a cada indice para que 0 signifique "falta" (tinyobj usa -1)
static uint64_t makeCornerKey(const tinyobj::index_t& corner) {
    const uint64_t position = (uint64_t)(corner.vertex_index + 1);
    const uint64_t texCoord = (uint64_t)(corner.texcoord_index + 1);
    const uint64_t normal = (uint64_t)(corner.normal_index + 1);
    return (position << (2 * BITS_PER_INDEX)) | (texCoord << BITS_PER_INDEX) | normal;
}

static glm::vec3 readVec3(const std::vector<tinyobj::real_t>& floats, int index) {
    return glm::vec3(floats[3 * index + 0], floats[3 * index + 1], floats[3 * index + 2]);
}

static unsigned findOrAddVertex(const tinyobj::index_t& corner, const tinyobj::attrib_t& attrib,
    std::vector<float>& verts, VertexByCornerKey& vertexByCornerKey) {
    const uint64_t key = makeCornerKey(corner);
    VertexByCornerKey::const_iterator alreadyCreated = vertexByCornerKey.find(key);

    unsigned vertexID;
    if (alreadyCreated == vertexByCornerKey.end()) {
        vertexID = (unsigned)(verts.size() / FLOATS_PER_VERTEX);
        const glm::vec3 position = readVec3(attrib.vertices, corner.vertex_index);
        const glm::vec3 normal = readVec3(attrib.normals, corner.normal_index);
        verts.insert(verts.end(), { position.x, position.y, position.z, normal.x, normal.y, normal.z });
        vertexByCornerKey.emplace(key, vertexID); 
    }
    else
        vertexID = alreadyCreated->second;
    
    return vertexID;
}

void loadObj(const std::filesystem::path& file, std::vector<float>& verts, std::vector<unsigned>& idx) {
    tinyobj::ObjReader reader;
    parseObjFile(file, reader);
    const tinyobj::attrib_t& attrib = reader.GetAttrib();

    verts.clear();
    idx.clear();
    VertexByCornerKey vertexByCornerKey;

    // L1: todos los shapes van a un solo mesh; en L3 cada (shape, material) sera una pieza
    for (const tinyobj::shape_t& shape : reader.GetShapes()) {
        for (const tinyobj::index_t& corner : shape.mesh.indices) {
            checkCornerIsInRange(corner, attrib, file);
            checkCornerHasNormal(corner, file);
            idx.push_back(findOrAddVertex(corner, attrib, verts, vertexByCornerKey));
        }
    }

    if (idx.empty()) throw objError(file, "no tiene caras");
}

int loadAssetManifest(const std::filesystem::path& manifest, MeshLibrary& lib) {
    std::ifstream in(manifest);
    if (!in) throw std::runtime_error("no se puede abrir " + std::filesystem::absolute(manifest).string());

    json root = json::parse(in);
    int cargados = 0;
    for (json::const_iterator it = root.begin(); it != root.end(); ++it) {
        const std::string& name = it.key();
        try {
            if (lib.has(name)) throw std::runtime_error("nombre repetido");
            std::string path = it.value().at("path").get<std::string>();

            std::vector<float> verts;
            std::vector<unsigned> idx;
            loadObj(dataPath(path), verts, idx);
            std::cout << "[L1] " << name << ": " << verts.size() / FLOATS_PER_VERTEX
                << " vertices / " << idx.size() << " indices\n";   // TEMPORAL
            lib.add(name, verts, idx);
            ++cargados;
        }
        catch (const std::exception& ex) {
            throw std::runtime_error("asset '" + name + "': " + ex.what());
        }
    }
    return cargados;
}