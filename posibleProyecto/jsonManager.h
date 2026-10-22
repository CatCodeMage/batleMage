#pragma once
#include "global.h"
#include "hechizosGestor.h"
#include "Entidad.h"
#include "MeshLibrary.h"
#include "ModelLibrary.h"

struct EntityInfo {
	std::string asset;
	std::vector<float> scale;
};

ElementType parseElement(std::string element);

void readSpells(HechizosGestor spellManager);

std::vector<std::string> getPlayers();

void writeCombat();

Entidad generateEntity(const std::vector<float>& position, const std::vector<float>& rotation,
	const EntityInfo eInfo, const ModelLibrary& lib);

EntityInfo getEntityInfo(const std::string entityName);

std::vector<std::vector <Entidad>> generateCombat(const ModelLibrary& modelLib);
