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
	skinnedRenderer.initialize();
	loadAssetManifest(dataPath("db/assets.json"), libMesh, libModel, textures, skinnedRenderer.maxBones());

	map.load(dataPath("db/map.json"), libModel);

	//lights = setLights();, en teoria mas adelante deberiamos cargar las lights por el JSON, lo dejo como aviso
	//de momento un parche en main pero tiene que cambiar.

	program = setShaders();
	 
	renderer.init(program);
	shadows.procesInit();
	keysInput.init(window);

	framesInfo = generateCombat(libModel, dataPath("db/combate_generado.json"));
	visualInterface.init(window);
}

simulationData::~simulationData() {
	visualInterface.destroy();
	glUseProgram(0);
	skinnedRenderer.destroy();
	animators.clear();
	shadows.destroy();
	if (program) glDeleteProgram(program);
	libModel.destroy();
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

const bm::anim::Animator& simulationData::poseFor(const Entidad& entity, const SkinnedModel& model) {
	if (entity.characterId == 0) return model.bindPose;

	auto it = animators.find(entity.characterId);
	if (it == animators.end() || it->second.model != entity.model) {
		it = animators.insert_or_assign(entity.characterId,
			ReplayAnimator{ entity.model, bm::anim::Animator(model.skeleton) }).first;
	}
	auto& state = it->second;

	auto clip = model.findClip(entity.animation);
	bool looping = entity.animation == "idle" || entity.animation == "move";
	if (!clip && entity.animation != "death") {
		clip = model.findClip("idle");
		looping = true;
	}
	const double seconds = double(framesInfo.frame - entity.animationStartFrame) / framesInfo.ticksPerSecond;
	if (state.clip != clip.get() || state.looping != looping) {
		state.animator.play(clip, looping, seconds);
		state.clip = clip.get();
		state.looping = looping;
	} else if (clip && seconds != state.seconds) {
		state.animator.setTime(seconds);
	}
	state.seconds = seconds;
	return state.animator;
}

void simulationData::submitEntities(const std::vector<Entidad>& entities) {
	for (const Entidad& entity : entities) {
		const Model& model = libModel.get(entity.model);
		if (model.skinned)
			submitEntidad(skinnedRenderer, *model.skinned, entity, poseFor(entity, *model.skinned));
		else
			submitEntidad(renderer, model, entity);
	}
}

void simulationData::updateCamera(const SystemInfo& sysInfo) {
	const bool isRecording = cameraPathMode == CameraPathMode::Recording;
	const bool isControllingCamera = interactionMode == InteractionMode::ControllingCamera;
	const bool isFreeToMove = isRecording && isControllingCamera;

	if (!isRecording) viewCamera.setPose(cameraPath.poseAtFrame(framesInfo.frame));
	viewCamera.update(keysInput, sysInfo, isFreeToMove);
	if (isRecording) saveShotIfRequested();
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

	submitEntities(map.entities);

	submitEntities(framesInfo.combat[framesInfo.frame]);

	renderer.submitLights(lights, sysInfo.getTimeStored());

	// Las sombras no dependen de la camara: una vez por frame, aunque haya varias vistas
	shadows.render(lights, [&]() { renderer.drawShadowGeometry(libMesh); }, &skinnedRenderer);
	prepareLightingPrograms();

	// Pintar simulacion
	const SceneViewport wholeWindow{ 0, 0, sysInfo.getWidht(), sysInfo.getHeight() };
	const SceneViewport sceneArea = arePanelsVisible
		? visualInterface.getCentralArea(sysInfo.getWidht(), sysInfo.getHeight())
		: wholeWindow;
	drawSceneViews(sceneArea);

	renderer.clearFrame();
	skinnedRenderer.clear();

	// Pintar la UI
	if (arePanelsVisible) {
		drawCombatPanel(framesInfo);
		drawCameraPathPanel(cameraPathMode, cameraPath, viewCamera, framesInfo, sceneMosaic);
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

namespace {
	constexpr float VIEW_FIELD_OF_VIEW_DEGREES = 45.0f;
	constexpr float VIEW_NEAR_PLANE = 0.1f;
	constexpr float VIEW_FAR_PLANE = 300.0f;   // con 100 se cortaria el mapa visto desde lejos


	glm::mat4 testViewFor(int viewIndex, const Camera& freeCamera) {
		if (viewIndex == 0) return freeCamera.get_view();

		constexpr float ORBIT_RADIUS = 45.0f;
		constexpr float ORBIT_HEIGHT = 25.0f;
		constexpr float DEGREES_BETWEEN_VIEWS = 45.0f;   // 8 vistas dan la vuelta entera a la arena
		const float angle = glm::radians(DEGREES_BETWEEN_VIEWS * viewIndex);
		const glm::vec3 eye(ORBIT_RADIUS * std::cos(angle), ORBIT_HEIGHT, ORBIT_RADIUS * std::sin(angle));
		return glm::lookAt(eye, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
	}
}

void simulationData::prepareLightingPrograms() {
	if (!skinnedRenderer.empty()) {
		const GLuint skinnedProgram = skinnedRenderer.lightingProgram();
		shadows.bindForLighting(skinnedProgram);
		renderer.applyLights(skinnedProgram);
	}
	shadows.bindForLighting(program);
}

void simulationData::drawSceneInViewport(const SceneViewport& viewport, const glm::mat4& view) {
	if (viewport.isVisible()) {
		glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
		const glm::mat4 projection = glm::perspective(glm::radians(VIEW_FIELD_OF_VIEW_DEGREES),
			viewport.aspectRatio(), VIEW_NEAR_PLANE, VIEW_FAR_PLANE);
		const glm::mat4 viewProjection = projection * view;

		renderer.drawLighting(libMesh, viewProjection);
		skinnedRenderer.drawLighting(viewProjection);
	}
}

void simulationData::drawSceneViews(const SceneViewport& sceneArea) {
	const std::vector<SceneViewport> views = sceneMosaic.splitArea(sceneArea);
	for (int viewIndex = 0; viewIndex < (int)views.size(); ++viewIndex)
		drawSceneInViewport(views[viewIndex], testViewFor(viewIndex, viewCamera));
}