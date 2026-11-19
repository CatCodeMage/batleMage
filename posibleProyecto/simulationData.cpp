#include "simulationData.h"
#include <imgui.h>
#include "CombatPanel.h"
#include "CameraPathPanel.h"

void simulationData::changeLights(std::vector<SceneLight>& newLights) {
	lights = newLights;
}

void simulationData::init(GLFWwindow* window) {
	constexpr int FIRST_FRAME = 0;
	cameraPath.saveShot(FIRST_FRAME, viewCamera.getPose());   // toma por defecto: la camara inicial

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

void simulationData::clearLastFrame(GLFWwindow* window, SystemInfo& sysInfo) {
	sysInfo.getCurrentState(window);
	glViewport(0, 0, sysInfo.getWidht(), sysInfo.getHeight());   // la ventana se puede redimensionar
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void simulationData::updateFrame(GLFWwindow* window) {
	visualInterface.endFrame();
	glfwSwapBuffers(window);
	keysInput.clearPressedKeys();   // las pulsaciones solo valen el frame en que llegan
	glfwPollEvents();
}

void simulationData::updateCamera(const SystemInfo& sysInfo) {
	const bool isRecording = cameraPathMode == CameraPathMode::Recording;
	const bool isControllingCamera = interactionMode == InteractionMode::ControllingCamera;
	const bool isFreeToMove = isRecording && isControllingCamera;

	if (!isRecording) viewCamera.setPose(cameraPath.poseAtFrame(framesInfo.frame));
	viewCamera.update(keysInput, sysInfo, isFreeToMove);
	if (isRecording) saveShotIfRequested();

	viewProjection = viewCamera.get_proj() * viewCamera.get_view();
}

void simulationData::saveShotIfRequested() {
	const bool hasPressedSave = keysInput.wasKeyPressed(GLFW_KEY_SPACE);
	const bool isTypingInPanel = visualInterface.isUsingKeyboard();   // p. ej. escribiendo el frame exacto
	if (!hasPressedSave || isTypingInPanel) return;

	cameraPath.saveShot(framesInfo.frame, viewCamera.getPose());
	std::cout << "Toma guardada en el frame " << framesInfo.frame
		<< " (" << cameraPath.getShots().size() << " tomas)\n";
}

void simulationData::procesMainBodyLoopRealTime(GLFWwindow* window, SystemInfo& sysInfo) {
	clearLastFrame(window, sysInfo);

	// UI
	const bool arePanelsVisible = interactionMode == InteractionMode::Editing;
	visualInterface.beginFrame(arePanelsVisible);

	// System
	visualInterface.setMouseEnabled(arePanelsVisible);
	updateInteractionMode(window);

	// Camara
	updateCamera(sysInfo);

	// Submit to queue
	submitEntidades(renderer, libModel, map.entities);

	submitEntidades(renderer, libModel, framesInfo.combat[framesInfo.frame]);

	renderer.submitLights(lights, sysInfo.getTimeStored());

	//renderizar sombras
	shadows.render(lights, [&]() { renderer.drawShadowGeometry(libMesh);});
	shadows.bindForLighting(program);

	// Pintar simulacion 
	renderer.flush(libMesh, viewProjection);

	// Pintar la UI
	if (arePanelsVisible) {
		drawCombatPanel(framesInfo);
		drawCameraPathPanel(cameraPathMode, cameraPath, viewCamera, framesInfo);
	}
	else visualInterface.drawCameraModeHint();

	// Mostrar el resultado
	updateFrame(window);
}

void simulationData::updateInteractionMode(GLFWwindow* window) {
	const bool isEditing = interactionMode == InteractionMode::Editing;
	const bool hasClickedScene = isEditing && visualInterface.wasSceneClicked();
	const bool hasPressedEscape = !isEditing && keysInput.wasKeyPressed(GLFW_KEY_ESCAPE);
	const bool hasLostFocus = !isEditing && !glfwGetWindowAttrib(window, GLFW_FOCUSED);   // Alt+Tab: no volver con el cursor bloqueado

	if (hasClickedScene) {
		interactionMode = InteractionMode::ControllingCamera;
		keysInput.captureCursor(window);
	}
	else if (hasPressedEscape || hasLostFocus) {
		interactionMode = InteractionMode::Editing;
		keysInput.releaseCursor(window);
	}
}