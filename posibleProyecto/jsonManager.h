#pragma once
#include "global.h"
#include "hechizosGestor.h"
#include "Entidad.h"
#include "MeshLibrary.h"
#include "ModelLibrary.h"

ElementType parseElement(std::string element);

void readSpells(HechizosGestor& spellManager);

std::vector<std::string> getPlayers();

void writeCombat();

std::vector<std::vector<Entidad>> generateCombat(const ModelLibrary& modelLib,
    std::vector<ModelAnimation>& animations,
    std::vector<ImpactVFX>* impacts = nullptr);
