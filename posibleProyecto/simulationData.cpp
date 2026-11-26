#include "simulationData.h"
#include <imgui.h>
#include "CombatPanel.h"
#include "CameraPathPanel.h"

#include <utility>

void simulationData::changeLights(std::vector<SceneLight>& newLights) {
	lights = newLights;
}

std::vector<SceneLight> simulationData::setLights() {
	std::vector<SceneLight> sceneLights;
	sceneLights = map.m_lights;

	// se deja asi por si hay que cargar luces dinamicas

	return sceneLights;
}

void simulationData::init(GLFWwindow* window) {
	constexpr int FIRST_FRAME = 0;
	cameraPath.saveShot(FIRST_FRAME, viewCamera.getPose());   // toma por defecto: la camara inicial

	textures.init();
	skinnedRenderer.initialize();
	loadAssetManifest(dataPath("db/assets.json"), libMesh, libModel, textures, skinnedRenderer.maxBones());

	map.load(dataPath("db/map.json"), libModel);

	lights = setLights(); // en teoria mas adelante deberiamos cargar las lights por el JSON, lo dejo como aviso
	//de momento un parche en main pero tiene que cambiar.

	program = setShaders();
	 
	renderer.init(program);
	shadows.procesInit();
	keysInput.init(window);

	framesInfo = generateCombat(libModel, dataPath("db/combate_generado.json"));

	if (!framesInfo.particleFrames.empty()) {
		particleModule.initialize(std::move(framesInfo.particleStyles));
	}
	particleModule.setFrames(std::move(framesInfo.particleFrames), framesInfo.ticksPerSecond);

	visualInterface.init(window);
}

simulationData::~simulationData() {
	visualInterface.destroy();
	glUseProgram(0);
	particleModule.shutdown();
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

	submitEntities(map.entities);

	submitEntities(framesInfo.combat[framesInfo.frame]);

	renderer.submitLights(lights, sysInfo.getTimeStored());

	//renderizar sombras
	shadows.render(lights, [&]() { renderer.drawShadowGeometry(libMesh); }, &skinnedRenderer);
	if (!skinnedRenderer.empty()) {
		const GLuint skinnedProgram = skinnedRenderer.lightingProgram();
		shadows.bindForLighting(skinnedProgram);
		renderer.applyLights(skinnedProgram);
	}

	// Pintar simulacion
	shadows.bindForLighting(program);
	renderer.flush(libMesh, viewProjection);
	skinnedRenderer.drawLighting(viewProjection);
	skinnedRenderer.clear();

	// El tick del combate fija el estado de las particulas, incluso en pausa.
	particleModule.seek(framesInfo.frame);
	particleModule.render(viewCamera.get_view(), viewCamera.get_proj());

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
