
#include "jsonManager.h"
#include "CombatSimulation.h"
#include "sistema_particulas/logica/ParticleJson.h"

#include <cmath>
#include <unordered_set>

using json = nlohmann::json;

const std::string SPELLS_PATH = "../db/spells.json";
const std::string PLAYERS_PATH = "../db/players.json";
const std::string ENTITIES_PATH = "../db/entities.json";

json ReplayAnimationState::record(int id, const char* entity, const char* nextAction,
	std::size_t frame, bool restart) {
	if (action != nextAction || restart) {
		action = nextAction;
		startFrame = frame;
	}
	return {
		{ "id", id },
		{ "entity", entity },
		{ "animation", action },
		{ "animationStartFrame", startFrame }
	};
}

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

static glm::vec3 replayVector(const json& values) {
	return { values.at(0).get<float>(), values.at(1).get<float>(), values.at(2).get<float>() };
}

static ModelId entityModel(const json& definition, const ModelLibrary& models) {
	const auto& asset = definition.at("asset").get_ref<const std::string&>();
	if (!models.has(asset)) throw std::runtime_error("asset desconocido: '" + asset + "'");
	return models.find(asset);
}

FramesData generateCombat(const ModelLibrary& modelLib, const std::filesystem::path& replayFile) {
	std::ifstream replayStream(replayFile), entitiesStream(ENTITIES_PATH);
	if (!replayStream) throw std::runtime_error("no se puede abrir " + replayFile.string());
	if (!entitiesStream) throw std::runtime_error("no se puede abrir " + ENTITIES_PATH);

	const json replay = json::parse(replayStream);
	const json catalog = json::parse(entitiesStream);
	const auto& frames = replay.at("frames");
	FramesData result;
	result.ticksPerSecond = replay.value("metadata", json::object())
		.value("ticksPerSecond", CombatSimulation::TICKS_PER_SECOND);
	if (!frames.is_array() || frames.empty() || result.ticksPerSecond <= 0)
		throw std::runtime_error("replay sin frames o con frecuencia invalida: " + replayFile.string());

	const auto particleStyles = replay.find("particleStyles");
	const auto particleFrames = replay.find("particleFrames");
	if ((particleStyles == replay.end()) != (particleFrames == replay.end()))
		throw std::runtime_error("El replay debe incluir particleStyles y particleFrames juntos: " + replayFile.string());
	if (particleStyles != replay.end()) {
		result.particleStyles = bm::particles::particleJson::leerEstilos(*particleStyles);
		result.particleFrames = bm::particles::particleJson::leerFrames(*particleFrames, result.particleStyles, frames.size());
	}

	result.combat.reserve(frames.size());
	for (const auto& frame : frames) {
		std::vector<Entidad> entities;
		entities.reserve(frame.size());
		std::unordered_set<EntityId> entityIds;
		for (const auto& record : frame) {
			Entidad entity;
			entity.transform.pos = replayVector(record.at(0));
			entity.transform.rot = replayVector(record.at(1));
			const json* definition = nullptr;

			// El cuarto elemento puede contener solo el ID de un hechizo.
			const json* extra = (record.size() > 3 && record[3].is_object()) ? &record[3] : nullptr;
			if (extra) {
				entity.id = extra->value("id", INVALID_ENTITY);
				if (entity.id < INVALID_ENTITY ||
					(entity.id != INVALID_ENTITY && !entityIds.insert(entity.id).second)) {
					throw std::runtime_error("id de entidad invalido o repetido en un frame del replay");
				}
			}

			if (extra && extra->contains("animation")) {
				const auto& character = *extra;
				entity.characterId = entity.id;
				entity.animation = character.at("animation").get<std::string>();
				entity.animationStartFrame = character.at("animationStartFrame").get<int>();
				if (entity.characterId == INVALID_ENTITY ||
					entity.animationStartFrame < 0 || size_t(entity.animationStartFrame) > result.combat.size()) {
					throw std::runtime_error("id o inicio de animacion invalido en el replay");
				}

				const auto& base = catalog.at(character.at("entity").get_ref<const std::string&>());
				const ModelId baseModel = entityModel(base, modelLib);
				if (modelLib.get(baseModel).skinned) {
					entity.model = baseModel;
					definition = &base;
				}
			}
			if (!definition) {
				definition = &catalog.at(record.at(2).get_ref<const std::string&>());
				entity.model = entityModel(*definition, modelLib);
			}
			entity.transform.scale = replayVector(definition->at("scale"));
			entities.push_back(std::move(entity));
		}
		result.combat.push_back(std::move(entities));
	}
	return result;
}
