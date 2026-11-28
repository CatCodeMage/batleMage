
#include "jsonManager.h"
#include "CombatSimulation.h"
#include "sistema_particulas/logica/ParticleJson.h"

#include <cmath>
#include <unordered_set>

using json = nlohmann::json;

const std::string SPELLS_PATH = "../db/spells.json";
const std::string SPELLS_ZONE_PATH = "../db/spells_zone.json";   // hechizos con zona (de momento aparte)
const std::string PLAYERS_PATH = "../db/players.json";
const std::string ENTITIES_PATH = "../db/entities.json";
const std::string CARDS_PATH = "../db/cards.json";


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

// "line" (y el "cone" antiguo) -> proyectil
static CastType parseCastType(const std::string& s) {
    if (s == "area") return CastType::AreaOfEffect;
    if (s == "self") return CastType::Self;
    return CastType::Projectile;
}

// "zone": { "shape": "sphere"|"cone"|"box"|"tornado", medidas…, "scale_start"?, "duration"?, "tick_interval"?, "move_speed"? }
static DatosZona parseZona(const json& z, const std::string& nombreHechizo) {
    DatosZona d;
    const std::string shape = z.value("shape", std::string("sphere"));
    if (shape == "sphere") {
        d.forma = Esfera{ z.at("radius").get<float>() };
    } else if (shape == "cone") {
        d.forma = Cono{ z.at("range").get<float>(), z.value("angle", 60.0f), z.value("height", 100.0f) };
    } else if (shape == "box") {
        const auto s = z.at("size").get<std::vector<float>>();   // [ancho, alto, largo]
        d.forma = Caja{ s.at(0), s.at(1), s.at(2) };
    } else if (shape == "tornado") {
        d.forma = Tornado{ z.at("radius_top").get<float>(), z.value("height", 4.0f) };
    } else {
        std::cerr << "[spells] " << nombreHechizo << ": forma de zona desconocida \"" << shape << "\"\n";
        return d;   // sin zona
    }
    d.escalaInicial = z.value("scale_start", 1.0f);
    d.duracion      = z.value("duration", 0.0f);
    d.intervaloDano = z.value("tick_interval", 0.0f);
    d.velocidadAvance = z.value("move_speed", 0.0f);
    return d;
}

// Lee un archivo de hechizos. 'id' continúa entre archivos para que no se repitan.
static void leerArchivoHechizos(const std::string& ruta, HechizosGestor& spellManager, int& id, bool obligatorio) {
    constexpr float DEFAULT_SPELL_COST = 10.0f;

    // La IA calcula el alcance como velocidad x tiempo de vida.
    constexpr float SPELL_LIFETIME_SECONDS = 3.0f;

    std::ifstream spells(ruta);
    if (!spells.is_open()) {
        if (obligatorio) std::cout << "No file found: " << ruta << std::endl;
        return;
    }

    json j;
    spells >> j;

    for (auto& [name, spell] : j.items()) {
        if (spellManager.idPorNombre(name) >= 0) {
            std::cerr << "[" << ruta << "] hechizo repetido \"" << name << "\", se ignora\n";
            continue;
        }

        ElementType elem = parseElement(spell["element"]);

        glm::vec3 scaleVec(
            spell["scale"][0].get<float>(),
            spell["scale"][1].get<float>(),
            spell["scale"][2].get<float>()
        );

        // Tipo de movimiento. Si no se especifica: parabolico si gravityMultiplier > 0, si no lineal.
        const float gravityMultiplier = spell.value<float>("gravityMultiplier", 0.0f);
        MovementPattern pattern = (gravityMultiplier > 0.0f)
            ? MovementPattern::Parabolic
            : MovementPattern::Linear;
        if (spell.contains("movement_pattern")) {
            const std::string movementName = spell.value("movement_pattern", std::string("linear"));
            if (movementName == "parabolic")   pattern = MovementPattern::Parabolic;
            else if (movementName == "linear") pattern = MovementPattern::Linear;
        }

        // Cómo se entrega ("line" / "area" / "self") y qué zona deja (opcional)
        CastType castType = parseCastType(spell.value("cast_type", std::string("line")));
        const DatosZona zona = spell.contains("zone") ? parseZona(spell["zone"], name) : DatosZona{};

        // Compatibilidad con spells.json: "area" sin "zone" sigue siendo proyectil, como hasta ahora
        if (castType == CastType::AreaOfEffect && !zona.tieneZona()) castType = CastType::Projectile;

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
            castType,
            zona,
            gravityMultiplier
        );

        id++;
    }
}

void readSpells(HechizosGestor& spellManager) {
    int id = 0;
    leerArchivoHechizos(SPELLS_PATH, spellManager, id, true);
    leerArchivoHechizos(SPELLS_ZONE_PATH, spellManager, id, false);   // opcional: si no existe, no pasa nada
}

static uint32_t parseEtiqueta(const std::string& tag) {
	if (tag == "bounce") return ETQ_REBOTE;
	if (tag == "homing") return ETQ_HOMING;   // reservada: aún no hace nada
	std::cerr << "[cartas.json] etiqueta desconocida: " << tag << "\n";
	return ETQ_NINGUNA;
}

void readCards(HechizosGestor& spellManager)
{
	std::ifstream file(CARDS_PATH);
	if (!file.is_open()) {
		std::cerr << "[cartas.json] no encontrado: no se cargan cartas\n";
		return;
	}
	try {
		json j;
		file >> j;
		for (auto& [name, card] : j.items()) {
			const int id = card.at("id").get<int>();
			const std::string type = card.at("type").get<std::string>();
			const float mana = card.value("mana", 0.0f);
			const float castDelay = card.value("cast_delay", 0.0f);
			
			if (type == "modifier") {
				const json mods = card.value("mods", json::object());
				ModificadoresDisparo m;
				m.danoExtra = mods.value("damage_extra", 0.0f);
				m.multVelocidad = mods.value("speed_mult", 1.0f);
				m.lifetimeExtra = mods.value("lifetime_extra", 0.0f);
				m.dispersionExtra = mods.value("spread_extra", 0.0f);
				m.rebotes = mods.value("bounces", 0);
				for (const std::string& tag : mods.value("tags", std::vector<std::string>{}))
					m.etiquetas |= parseEtiqueta(tag);
				if (m.rebotes > 0) m.etiquetas |= ETQ_REBOTE;   // "bounces" ya implica rebote
				spellManager.registrarModificador(id, name, mana, castDelay, m);
				
			}
			else if (type == "multicast") {
				spellManager.registrarMulticast(id, name, mana, castDelay, card.at("draws").get<int>());
			}
			else {
				std::cerr << "[cartas.json] tipo desconocido en " << name << ": " << type << "\n";
				
			}
			
		}
		
	}
	catch (const std::exception& e) {
		std::cerr << "[cartas.json] " << e.what() << "\n";
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
