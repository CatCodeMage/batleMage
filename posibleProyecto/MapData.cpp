#include "MapData.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

static glm::vec3 aVec3(const json& v) {
    if (!v.is_array() || v.size() != 3)
        throw std::runtime_error("se esperaba [x, y, z]");
    return glm::vec3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
}

static glm::vec3 aVec3(const json& obj, const char* clave, const glm::vec3& porDefecto) {
    json::const_iterator it = obj.find(clave);
    return (it == obj.end()) ? porDefecto : aVec3(*it);
}

void loadTranformJSON(Entidad& entity, const json& vals) {
    entity.transform.pos = aVec3(vals, "position", glm::vec3(0.0f));
    entity.transform.scale = aVec3(vals, "scale", glm::vec3(1.0f));
    entity.transform.rot = aVec3(vals, "rotation", glm::vec3(0.0f));   // radianes
}

Entidad loadEntityJSON(const json& vals, const MeshLibrary& lib) {
    std::string asset = vals.at("asset").get<std::string>();

    Entidad entity;
    if (lib.has(asset)) { entity.mesh = lib.find(asset); }
    else { throw std::runtime_error("\n\n[Asset] Missing asset\n\n"); }

    loadTranformJSON(entity, vals);

    entity.kind = vals.value("kind", 0);   // opcional: si no está, 0 = liso

    return entity;
}

bool MapData::load(const std::filesystem::path& file, const MeshLibrary& lib) {
    entities.clear();

    std::ifstream in(file);
    if (!in) { throw std::runtime_error("\n\n[MapData] no se puede abrir\n\n"); }
    try {
        const json mapaJSON = json::parse(in);
         
        entities.push_back(loadEntityJSON(mapaJSON, lib));

        const json& listEntities = mapaJSON.at("entities");
        for (json::const_iterator it = listEntities.begin(); it != listEntities.end(); ++it) {
            const std::string& id = it.key();       
            const json& vals = it.value();

            entities.push_back(loadEntityJSON(vals, lib));
        }
    }
    catch (const std::exception& ex) {      // json::exception y runtime_error
        std::cerr << "[MapData] " << file << ": " << ex.what() << "\n";
        throw std::runtime_error("\n\n[MapData] error al procesar mapa\n\n");
    }
    return true;
}