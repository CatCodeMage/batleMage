#pragma once
#include "jsonManager.h"


using json = nlohmann::json;

const std::string SPELLS_PATH = "../db/spells.json";
const std::string PLAYERS_PATH = "../db/players.json";

ElementType parseElement(std::string element) {
	if (element == "fire")      return ElementType::Fire;
	if (element == "ice")       return ElementType::Ice;
	if (element == "lightning") return ElementType::Lightning;
	if (element == "arcane")    return ElementType::Arcane;
	if (element == "poison")    return ElementType::Poison;
	if (element == "water")     return ElementType::water;
	if (element == "earth")     return ElementType::earth;
	if (element == "wind")      return ElementType::wind;
	return ElementType::None;
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

			spellManager.registrarHechizo(id, name, elem, spell["cost"], spell["damage"], spell["cooldown"],
				spell["speed"], spell["radious"], 100);

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