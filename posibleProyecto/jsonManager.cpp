
#include "jsonManager.h"
#include <cmath>

using json = nlohmann::json;

const std::string SPELLS_PATH = "../db/spells.json";
const std::string PLAYERS_PATH = "../db/players.json";
const std::string COMBAT_PATH = "../db/combate_generado.json";
const std::string ENTITIES_PATH = "../db/entities.json";

ElementType parseElement(std::string element) {
    if (element == "fire")      return ElementType::Fire;
    if (element == "ice")       return ElementType::ICE;
    if (element == "lightning") return ElementType::Lightning;
    if (element == "wind")      return ElementType::Wind;
    if (element == "water")     return ElementType::Water;
    if (element == "physical")  return ElementType::Physical;
    return ElementType::Fire;
}

void readSpells(HechizosGestor& spellManager) {
    constexpr float DEFAULT_SPELL_COST = 10.0f;

    // La IA calcula el alcance como velocidad x tiempo de vida.
    constexpr float SPELL_LIFETIME_SECONDS = 3.0f;

    std::ifstream spells(SPELLS_PATH);

    if (spells.is_open())
    {
        json j;
        spells >> j;

        int id = 0;

        for (auto& [name, spell] : j.items()) {
            ElementType elem = parseElement(spell["element"]);

            glm::vec3 scaleVec(
                spell["scale"][0].get<float>(),
                spell["scale"][1].get<float>(),
                spell["scale"][2].get<float>()
            );

            const float gravityMultiplier = spell.value<float>("gravityMultiplier", 0.0f);

            // NUEVO: leer el tipo de movimiento del JSON.
            // Si no se especifica, sera parabolico cuando gravityMultiplier > 0 y lineal en caso contrario.
            MovementPattern pattern = (gravityMultiplier > 0.0f)
                ? MovementPattern::Parabolic
                : MovementPattern::Linear;

            if (spell.contains("movement_pattern")) {
                std::string movementName = spell.value("movement_pattern", std::string("linear"));
                if (movementName == "parabolic") {
                    pattern = MovementPattern::Parabolic;
                } else if (movementName == "linear") {
                    pattern = MovementPattern::Linear;
                }
            }

            spellManager.registrarHechizo(
                id,
                name,
                elem,
                spell.value<float>("cost", DEFAULT_SPELL_COST),
                spell["damage"].get<float>(),
                spell["cooldown"].get<float>(),
                spell["speed"].get<float>(),
                spell["radious"].get<float>(),
                scaleVec,
                SPELL_LIFETIME_SECONDS,
                pattern,
                gravityMultiplier
            );

            id++;
        }
    }
    else
    {
        std::cout << "No file found" << std::endl;
    }
}

std::vector<std::string> getPlayers() {
    try {
        std::vector<std::string> playerOptions;

        std::ifstream players(PLAYERS_PATH);

        if (!players.is_open())
        {
            return {};
        }

        json j;
        players >> j;

        for (auto& [key, value] : j.items()) {
            playerOptions.push_back(key);
        }

        return playerOptions;
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;

        return {};
    }
}

struct EntityInfo {
    ModelId model;
    glm::vec3 scale;
    AnimationId animation = NO_ANIMATION;
};

static glm::vec3 readVec3(const json& value) {
    return glm::vec3(value.at(0).get<float>(), value.at(1).get<float>(), value.at(2).get<float>());
}

static ModelId requireModel(const ModelLibrary& models, const std::string& entityName, const std::string& asset) {
    if (!models.has(asset))
        throw std::runtime_error("entidad '" + entityName + "': asset desconocido '" + asset + "'");
    return models.find(asset);
}

static std::unordered_map<std::string, EntityInfo> loadEntityInfo(const ModelLibrary& models,
    std::vector<ModelAnimation>& animations) {
    std::ifstream file(ENTITIES_PATH);
    if (!file) throw std::runtime_error("no se puede abrir " + ENTITIES_PATH);
    const json definitions = json::parse(file);

    std::unordered_map<std::string, EntityInfo> entities;
    for (const auto& [name, definition] : definitions.items()) {
        EntityInfo info{ requireModel(models, name, definition.at("asset").get<std::string>()),
            readVec3(definition.at("scale")) };

        const auto config = definition.find("animation");
        if (config != definition.end()) {
            ModelAnimation animation;
            animation.fps = config->at("fps").get<double>();
            animation.spinRadiansPerSecond = glm::radians(config->value("spinDegreesPerSecond", 0.0));
            animation.loop = config->value("loop", true);
            const json& assets = config->at("assets");
            if (!std::isfinite(animation.fps) || animation.fps <= 0.0 || !assets.is_array() || assets.empty())
                throw std::runtime_error("entidad '" + name + "': animation requiere fps positivo y una lista de assets");
            animation.frames.reserve(assets.size());
            for (const json& asset : assets)
                animation.frames.push_back(requireModel(models, name, asset.get<std::string>()));

            info.animation = static_cast<AnimationId>(animations.size());
            animations.push_back(std::move(animation));
        }
        entities.emplace(name, info);
    }
    return entities;
}

std::vector<std::vector<Entidad>> generateCombat(const ModelLibrary& modelLib,
    std::vector<ModelAnimation>& animations,
    std::vector<ImpactVFX>* impacts) {
    std::ifstream simulationFile(COMBAT_PATH);
    if (!simulationFile) throw std::runtime_error("no se puede abrir " + COMBAT_PATH);
    const json simulationData = json::parse(simulationFile);
    const json& frames = simulationData.at("frames");
    if (!frames.is_array() || frames.empty()) throw std::runtime_error("replay sin frames: " + COMBAT_PATH);

    std::vector<ModelAnimation> configuredAnimations;
    const auto definitions = loadEntityInfo(modelLib, configuredAnimations);
    std::vector<std::vector<Entidad>> combatSequence;
    combatSequence.reserve(frames.size());
    for (const json& frame : frames) {
        auto& entities = combatSequence.emplace_back();
        entities.reserve(frame.size());
        for (const json& sample : frame) {
            const std::string name = sample.at(2).get<std::string>();
            const auto info = definitions.find(name);
            if (info == definitions.end()) throw std::runtime_error("entidad desconocida en replay: '" + name + "'");

            Entidad& entity = entities.emplace_back();
            entity.model = info->second.model;
            entity.animation = info->second.animation;
            entity.isIceLance = (name == "ice_lance");
            entity.isFireBall = (name == "fire_ball");
            entity.isLightningBolt = (name == "lightning_bolt");
            entity.isEarthSpike = (name == "earth_spike");
            entity.isWindBlade = (name == "wind_blade");
            entity.isWaterBolt = (name == "water_jet");
            entity.isDaggerBarrage = (name == "dagger_barrage");
            if (sample.size() >= 4)
                entity.animationAge = sample.at(3).get<double>();
            if (sample.size() >= 5 && sample.at(4).is_number_integer())
                entity.poolIndex = sample.at(4).get<int>();
            entity.transform.scale = info->second.scale;
            entity.transform.pos = readVec3(sample.at(0));
            entity.transform.rot = readVec3(sample.at(1));
        }
    }

    if (impacts != nullptr) {
        impacts->clear();
        const auto itImpacts = simulationData.find("impacts");
        if (itImpacts != simulationData.end() && itImpacts->is_array()) {
            impacts->reserve(itImpacts->size());
            for (const json& item : *itImpacts) {
                ImpactVFX vfx;
                vfx.type = item.value("type", std::string("ice_lance"));
                vfx.position = readVec3(item.at("position"));
                vfx.normal = readVec3(item.at("normal"));
                if (glm::length(vfx.normal) > 0.0001f)
                    vfx.normal = glm::normalize(vfx.normal);
                else
                    vfx.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vfx.startTick = item.value("startTick", 0);
                vfx.duration = item.value("duration", 1.9f);
                impacts->push_back(std::move(vfx));
            }
        }
    }

    animations = std::move(configuredAnimations);
    return combatSequence;
}
