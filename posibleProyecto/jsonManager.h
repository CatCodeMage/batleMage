#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include "hechizosGestor.h"

ElementType parseElement(std::string element);

void readSpells(HechizosGestor spellManager);

std::vector<std::string> getPlayers();

void writeCombat();
