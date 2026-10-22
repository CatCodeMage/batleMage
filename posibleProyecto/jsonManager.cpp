#pragma once
#include "jsonManager.h"


using json = nlohmann::json;

const std::string SPELLS_PATH = "../db/spells.json";
const std::string PLAYERS_PATH = "../db/players.json";
const std::string COMBAT_PATH = "../db/combate_chorras.json";

ElementType parseElement(std::string element) {
	if (element == "fire")      return ElementType::Fire;
	if (element == "ice")       return ElementType::Ice;
	if (element == "physical") return ElementType::Physical;
	return ElementType::Fire;
}

void readSpells(HechizosGestor& spellManager) {
	std::ifstream spells(SPELLS_PATH);

	if (spells.is_open())
	{
		json j;
		spells >> j;

		int id = 0;

		for (auto& [name, spell] : j.items()) {
			ElementType elem = parseElement(spell["element"]);

			std::cout << id << " " << name << " " << spell["element"] << " " << spell["cost"] << " " << spell["damage"]
				<< " " << spell["cooldown"] << " " << spell["speed"] << " " << spell["radious"] << std::endl;

			glm::vec3 scaleVec(
				spell["scale"][0].get<float>(),
				spell["scale"][1].get<float>(),
				spell["scale"][2].get<float>()
			);

			spellManager.registrarHechizo(
				id,
				name,
				elem,
				spell["cost"].get<float>(),
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

std::vector<std::vector <Entidad>> generateCombat() {
	try {
		std::ifstream simulationFile(COMBAT_PATH);

		if (!simulationFile.is_open())
		{
			return {};
		}

		json simulationData;

		simulationFile >> simulationData;

		json jsonCombat = simulationData["frames"];

		std::cout << jsonCombat;
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
		return {};
	}

}