#pragma once
#include "global.h"
#include "hechizosGestor.h"
#include "Entidad.h"
#include "MeshLibrary.h"
#include "ModelLibrary.h"
#include "sistema_particulas/ParticleEffect.h"
#include "sistema_particulas/ParticleFrame.h"

#include <vector>

inline constexpr const char* REPLAY_FORMAT_VERSION = "1.9";   // 1.9: los hechizos llevan {"id"}

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
	bm::particles::ParticleStyleCatalog particleStyles;
	std::vector<bm::particles::ParticleFrame> particleFrames;   // Vacio solo para replays sin estas secciones.
};

ElementType parseElement(std::string element);

void readSpells(HechizosGestor& spellManager);

std::vector<std::string> getPlayers();

void writeCombat();

FramesData generateCombat(const ModelLibrary& modelLib, const std::filesystem::path& replayFile);