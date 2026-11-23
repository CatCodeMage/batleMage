#pragma once
#include "global.h"
#include "hechizosGestor.h"
#include "Entidad.h"
#include "MeshLibrary.h"
#include "ModelLibrary.h"

inline constexpr const char* REPLAY_FORMAT_VERSION = "1.8";

struct ReplayAnimationState {
	std::string action;
	std::size_t startFrame = 0;

	nlohmann::json record(int id, const char* entity, const char* nextAction, std::size_t frame, bool restart);
};

struct FramesData {
	std::vector<std::vector<Entidad>> combat;
	int frame = 0;
	int ticksPerSecond = 0;
	bool isPaused = false;
};

ElementType parseElement(std::string element);

void readSpells(HechizosGestor& spellManager);

std::vector<std::string> getPlayers();

void writeCombat();

FramesData generateCombat(const ModelLibrary& modelLib, const std::filesystem::path& replayFile);
