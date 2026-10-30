#include "jsonManager.h"


using json = nlohmann::json;

const std::string SPELLS_PATH = "../db/spells.json";
const std::string PLAYERS_PATH = "../db/players.json";
const std::string COMBAT_PATH = "../db/combate_generado.json";
const std::string ENTITIES_PATH = "../db/entities.json";

ElementType parseElement(std::string element) {
	if (element == "fire")      return ElementType::Fire;
	if (element == "ice")       return ElementType::ICE;
	if (element == "physical") return ElementType::Physical;
	return ElementType::Fire;
}

void readSpells(HechizosGestor& spellManager) {
	constexpr float DEFAULT_SPELL_COST = 10.0f;

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
				100
			);

			id++;
		}
	}
	else
	{
		std::cout << "No file found" << std::endl;;
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

Entidad generateEntity(const std::vector<float>& position, const std::vector<float>& rotation,
	const EntityInfo eInfo, const ModelLibrary& lib) {

	if (!lib.has(eInfo.asset)) throw std::runtime_error("asset desconocido: '" + eInfo.asset + "'");

	Entidad entity;
	entity.model = lib.find(eInfo.asset);
	entity.transform.scale = glm::vec3(eInfo.scale[0], eInfo.scale[1], eInfo.scale[2]);
	entity.transform.pos = glm::vec3(position[0], position[1], position[2]);
	entity.transform.rot = glm::vec3(rotation[0], rotation[1], rotation[2]);

	entity.kind = 0;   // opcional: si no está, 0 = liso

	return entity;
}

EntityInfo getEntityInfo(const std::string entityName) {
	try {
		std::ifstream entityFile(ENTITIES_PATH);

		if (!entityFile.is_open())
		{
			return EntityInfo{};
		}

		json entityData;
		entityFile >> entityData;

		return EntityInfo{ entityData.at(entityName).at("asset"), entityData.at(entityName).at("scale").get<std::vector<float>>()};
	}
	catch (const std::exception& e) {
		std::cout << e.what();
		return EntityInfo{};
	}
}

std::vector<std::vector <Entidad>> generateCombat(const ModelLibrary& modelLib) {
	try {
		std::ifstream simulationFile(COMBAT_PATH);

		if (!simulationFile.is_open())
		{
			return {};
		}

		json simulationData;

		std::vector<std::vector <Entidad>> combatSequence;

		simulationFile >> simulationData;

		json jsonCombat = simulationData["frames"];

		for (auto& frame : jsonCombat) {

			std::vector<Entidad> entities;
			
			for (auto& entity : frame)
			{
				EntityInfo eInfo = getEntityInfo(entity[2]);

				entities.push_back(generateEntity(entity[0].get<std::vector<float>>(), 
					entity[1].get<std::vector<float>>(), eInfo, modelLib));
			}

			combatSequence.push_back(entities);
		}

		return combatSequence;
		
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
		return {};
	}

}