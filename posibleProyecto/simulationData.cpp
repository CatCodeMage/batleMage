#include "simulationData.h"
#include <imgui.h>
#include "CombatPanel.h"

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

	visualInterface.init(window);
}

void simulationData::destroy() {
	visualInterface.destroy();
	shadows.destroy();
	libMesh.destroy();
	textures.destroy();
}

void simulationData::clearLastFrame() {
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void simulationData::updateFrame(GLFWwindow* window) {
	visualInterface.endFrame();
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void simulationData::procesMainBodyLoopRealTime(GLFWwindow* window, SystemInfo& sysInfo) {
	clearLastFrame();

	// System
	visualInterface.beginFrame();
	sysInfo.getCurrentState(window);
	float time = sysInfo.getTimeStored();

	// Camara
	viewCamera.update(keysInput, sysInfo);

	glm::mat4 vp = viewCamera.get_proj() * viewCamera.get_view();

	// Submit to queue
	submitEntidades(renderer, libModel, map.entities);

	submitEntidades(renderer, libModel, framesInfo.combat[framesInfo.frame]);

	renderer.submitLights(lights, time);

	//renderizar sombras
	shadows.render(lights, [&]() { renderer.drawShadowGeometry(libMesh);});
	shadows.bindForLighting(program);

	// Pintar
	renderer.flush(libMesh, vp);

	drawCombatPanel(framesInfo);

	updateFrame(window);
}