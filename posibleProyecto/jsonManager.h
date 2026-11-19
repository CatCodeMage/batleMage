#pragma once
#include "global.h"
#include "hechizosGestor.h"
#include "Entidad.h"
#include "MeshLibrary.h"
#include "ModelLibrary.h"

struct FramesData {
	std::vector<std::vector<Entidad>> combat;
	int frame = 0;
	int ticksPerSecond = 0;
};

ElementType parseElement(std::string element);

void readSpells(HechizosGestor& spellManager);

std::vector<std::string> getPlayers();

void writeCombat();

FramesData generateCombat(const ModelLibrary& modelLib, const std::filesystem::path& replayFile);
