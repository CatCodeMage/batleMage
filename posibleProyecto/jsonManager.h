#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include "hechizosGestor.h"
#include "Entidad.h"

ElementType parseElement(std::string element);

void readSpells(HechizosGestor spellManager);

std::vector<std::string> getPlayers();

void writeCombat();

std::vector<std::vector <Entidad>> generateCombat();