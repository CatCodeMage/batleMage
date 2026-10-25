#include "simulationData.h"

void simulationData::changeLights(std::vector<SceneLight>& newLights) {
	lights = newLights;
}

void simulationData::init(GLFWwindow* window) {
	textures.init();
	loadAssetManifest(dataPath("db/assets.json"), libMesh, libModel, textures);

	map.load(dataPath("db/map.json"), libModel);

	//lights = setLights();, en teoria mas adelante deberiamos cargar las lights por el JSON, lo dejo como aviso
	//de momento un parche en main pero tiene que cambiar.

	program = setShaders();
	 
	renderer.init(program);
	shadows.procesInit();
	keysInput.init(window);

	framesInfo.combat = generateCombat(libModel);
	framesInfo.frame = 0;
	framesInfo.lessFrames = 0;
}

void simulationData::destroy() {
	shadows.destroy();
	libMesh.destroy();
	textures.destroy();
}