#include "simulationData.h"
#include <imgui.h>
#include "CombatPanel.h"
#include "CameraPathPanel.h"
#include "MontageFile.h"
#include <algorithm>
#include "FileDialog.h"
#include "CombatSimulation.h"
#include <utility>

namespace {
	constexpr float VIEW_NEAR_PLANE = 0.1f;
	constexpr float VIEW_FAR_PLANE = 300.0f;   // con 100 se cortaria el mapa visto desde lejos, deberia leerse del mapa, luego lo ajustamos
	constexpr const char* DEFAULT_MONTAGE_FILE = "db/montaje.json";   // solo lo que propone la ventana: se puede elegir otro
	constexpr FileTypeFilter MONTAGE_FILE_TYPE{ ".json", "Montaje (JSON)" };
}

void simulationData::createDefaultSceneCameras() {
	characterIds = characterIdsIn(framesInfo.combat.front());
	const int firstCharacter = characterIds.size() > 0 ? characterIds[0] : NO_CHARACTER;
	const int secondCharacter = characterIds.size() > 1 ? characterIds[1] : firstCharacter;

	sceneCameras = {
		makeSceneCamera(SceneCameraType::Free,         firstCharacter),
		makeSceneCamera(SceneCameraType::Duel,         firstCharacter),
		makeSceneCamera(SceneCameraType::FollowBehind, firstCharacter),
		makeSceneCamera(SceneCameraType::FollowBehind, secondCharacter),
		makeSceneCamera(SceneCameraType::FollowFront,  firstCharacter),
		makeSceneCamera(SceneCameraType::FirstPerson,  secondCharacter),
		makeSceneCamera(SceneCameraType::WholeMap,     firstCharacter),
		makeSceneCamera(SceneCameraType::Overhead,     firstCharacter),
	};
}

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
	textures.init();
	skinnedRenderer.initialize();
	loadAssetManifest(dataPath("db/assets.json"), libMesh, libModel, libMaterial, textures, skinnedRenderer.maxBones());
	iceLanceVfx.init(libMesh, textures, libMaterial);

	map.load(dataPath("db/map.json"), libModel);

	lights = setLights(); // en teoria mas adelante deberiamos cargar las lights por el JSON, lo dejo como aviso
	//de momento un parche en main pero tiene que cambiar.

	program = setShaders();
	 
	renderer.init(program);
	shadows.procesInit();
	keysInput.init(window);

	framesInfo = generateCombat(libModel, dataPath("db/combate_generado.json"));
	createDefaultSceneCameras();

	if (!framesInfo.particleFrames.empty()) {
		particleModule.initialize(std::move(framesInfo.particleStyles));
	}
	particleModule.setFrames(std::move(framesInfo.particleFrames), framesInfo.ticksPerSecond);

	visualInterface.init(window);

	constexpr int FIRST_FRAME = 0;
	cameraPath.saveShot(FIRST_FRAME, makeSceneCamera(SceneCameraType::WholeMap, NO_CHARACTER));   // plano por defecto: el mapa entero
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
	viewCamera.update(keysInput, sysInfo, isRecording && isControllingCamera);
	sceneCameras[freeCameraView].freePose = viewCamera.getPose();
	if (isRecording) saveShotIfRequested();
}

void simulationData::saveShotIfRequested() {
	const bool hasPressedSave = keysInput.wasKeyPressed(GLFW_KEY_SPACE);
	const bool isTypingInPanel = visualInterface.isUsingKeyboard();   // p. ej. escribiendo el frame exacto
	if (!hasPressedSave || isTypingInPanel) return;

	const SceneCamera& selectedCamera = sceneCameras[selectedViewIndex];
	cameraPath.saveShot(framesInfo.frame, selectedCamera);
	std::cout << "Plano guardado en el frame " << framesInfo.frame << ": " << sceneCameraTypeName(selectedCamera.type)
		<< " (" << cameraPath.getShots().size() << " planos)\n";
}

void simulationData::procesMainBodyLoopRealTime(GLFWwindow* window, SystemInfo& sysInfo) {
	clearLastFrame(window, sysInfo);

	// UI
	const bool arePanelsVisible = interactionMode == InteractionMode::Editing;
	visualInterface.beginFrame(arePanelsVisible);

	// System
	visualInterface.setMouseEnabled(arePanelsVisible);
	updateViewSelection(window, currentViews(sysInfo), sysInfo.getHeight());   // clic selecciona; doble clic en una libre la controla
	updateInteractionMode(window);

	// Camara
	updateCamera(sysInfo);

	submitEntities(map.entities);

	submitEntities(framesInfo.combat[framesInfo.frame]);

	renderer.submitLights(lights, sysInfo.getTimeStored());

	const std::vector<Entidad>& frameEntities = framesInfo.combat[framesInfo.frame];
	const double replaySeconds = static_cast<double>(framesInfo.frame) / CombatSimulation::TICKS_PER_SECOND;
	iceLanceVfx.submitFrame(renderer, framesInfo.combat, framesInfo.impacts, framesInfo.frame,
		mainCameraView(frameEntities).view, replaySeconds);   // se envian una vez por frame: miran a una sola camara

	// Las sombras no dependen de la camara: una vez por frame, aunque haya varias vistas
	shadows.render(lights, [&]() { renderer.drawShadowGeometry(libMesh); }, &skinnedRenderer);
	prepareLightingPrograms();
	particleModule.seek(framesInfo.frame);   // el tick fija el estado de las particulas, incluso en pausa; se pintan en cada vista

	// Pintar simulacion
	const std::vector<SceneViewport> views = currentViews(sysInfo);   // otra vez: el doble clic puede haber cambiado el modo
	drawSceneViews(views);

	renderer.clearFrame();
	skinnedRenderer.clear();

	// El tick del combate fija el estado de las particulas, incluso en pausa.
	particleModule.seek(framesInfo.frame);
	particleModule.render(viewCamera.get_view(), viewCamera.get_proj());

	// Pintar la UI
	if (arePanelsVisible) {
		drawCombatPanel(framesInfo);
		const CameraPathPanelAction requestedAction = drawCameraPathPanel(cameraPathMode, cameraPath, sceneCameras[selectedViewIndex], framesInfo, sceneMosaic, characterIds);
		handleCameraPathPanelAction(requestedAction);
		if (cameraPathMode == CameraPathMode::Recording) {
			drawViewHeaders(sceneCameras, views, selectedViewIndex, sysInfo.getHeight());
			drawSelectedCameraPanel(sceneCameras[selectedViewIndex], selectedViewIndex, characterIds);
		}
	}
	else visualInterface.drawCameraModeHint();

	// Mostrar el resultado
	updateFrame(window);
}
void simulationData::updateInteractionMode(GLFWwindow* window) {
	const bool isControllingCamera = interactionMode == InteractionMode::ControllingCamera;
	const bool hasPressedEscape = keysInput.wasKeyPressed(GLFW_KEY_ESCAPE);
	const bool hasLostFocus = !glfwGetWindowAttrib(window, GLFW_FOCUSED);   // Alt+Tab: no volver con el cursor bloqueado
	if (isControllingCamera && (hasPressedEscape || hasLostFocus)) {
		interactionMode = InteractionMode::Editing;
		keysInput.releaseCursor(window);
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

void simulationData::drawSceneInViewport(const SceneViewport& viewport, const CameraView& cameraView) {
	if (!viewport.isVisible()) return;   // ventana minimizada, o los paneles ocupan todo

	glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
	const glm::mat4 projection = glm::perspective(glm::radians(cameraView.fieldOfViewDegrees),
		viewport.aspectRatio(), VIEW_NEAR_PLANE, VIEW_FAR_PLANE);
	const glm::mat4 viewProjection = projection * cameraView.view;

	const glm::vec3 cameraPosition = glm::vec3(glm::inverse(cameraView.view)[3]);   // la columna 3 de la inversa es donde esta la camara
	renderer.drawLighting(libMesh, libMaterial, viewProjection, cameraPosition);
	skinnedRenderer.drawLighting(viewProjection, libMaterial, cameraPosition);
	particleModule.render(cameraView.view, projection);
}
 
std::vector<SceneViewport> simulationData::currentViews(const SystemInfo& sysInfo) const {
	const bool isFullScreen = interactionMode == InteractionMode::ControllingCamera;
	if (isFullScreen) return { SceneViewport{ 0, 0, sysInfo.getWidht(), sysInfo.getHeight() } };

	const SceneViewport sceneArea = visualInterface.getCentralArea(sysInfo.getWidht(), sysInfo.getHeight());
	const bool isPlayingBack = cameraPathMode == CameraPathMode::Playback;
	if (isPlayingBack) return { sceneArea };   // Resultado: una sola camara, la del plano de este frame
	return sceneMosaic.splitArea(sceneArea);
}

void simulationData::updateViewSelection(GLFWwindow* window, const std::vector<SceneViewport>& views, int framebufferHeight) {
	selectedViewIndex = (std::min)(selectedViewIndex, sceneMosaic.getViewCount() - 1);   // si se quitan vistas, la seleccionada puede desaparecer
	const bool isEditing = interactionMode == InteractionMode::Editing;
	if (!isEditing || !visualInterface.wasSceneClicked()) return;
	const bool isPlayingBack = cameraPathMode == CameraPathMode::Playback;
	if (isPlayingBack) {   // en Resultado no hay vistas que elegir: el doble clic solo pone la imagen a pantalla entera
		if (visualInterface.wasSceneDoubleClicked()) enterFullScreen(window);
		return;
	}
	const int clickedView = visualInterface.findViewUnderMouse(views, framebufferHeight);
	if (clickedView == NO_VIEW) return;

	selectedViewIndex = clickedView;
	const bool isFreeCamera = sceneCameras[clickedView].type == SceneCameraType::Free;
	if (isFreeCamera && visualInterface.wasSceneDoubleClicked()) startControllingCamera(window, clickedView);
}

void simulationData::enterFullScreen(GLFWwindow* window) {
	interactionMode = InteractionMode::ControllingCamera;
	keysInput.captureCursor(window);
}

void simulationData::startControllingCamera(GLFWwindow* window, int viewIndex) {
	freeCameraView = viewIndex;
	viewCamera.setPose(sceneCameras[viewIndex].freePose);   // empieza donde estaba esa vista
	enterFullScreen(window);
} 

CameraView simulationData::cameraViewForView(int viewIndex, const std::vector<Entidad>& frameEntities) const {
	const bool isPlayingBack = cameraPathMode == CameraPathMode::Playback;
	if (isPlayingBack) return cameraPath.viewAtFrame(framesInfo.frame, frameEntities);   // Resultado: planos y transiciones

	const bool isControllingCamera = interactionMode == InteractionMode::ControllingCamera;
	return cameraViewFor(sceneCameras[isControllingCamera ? freeCameraView : viewIndex], frameEntities);   // a pantalla entera, solo la que se mueve
}

CameraView simulationData::mainCameraView(const std::vector<Entidad>& frameEntities) const {
	return cameraViewForView(selectedViewIndex, frameEntities);   // en Resultado y a pantalla entera ya devuelve la que se ve en grande
}

void simulationData::drawSceneViews(const std::vector<SceneViewport>& views) {
	const std::vector<Entidad>& frameEntities = framesInfo.combat[framesInfo.frame];
	for (int viewIndex = 0; viewIndex < (int)views.size(); ++viewIndex)
		drawSceneInViewport(views[viewIndex], cameraViewForView(viewIndex, frameEntities));
}

void simulationData::handleCameraPathPanelAction(CameraPathPanelAction action) {
	switch (action) {
	case CameraPathPanelAction::SaveMontage: saveMontageFile(); break;
	case CameraPathPanelAction::LoadMontage: loadMontageFile(); break;
	case CameraPathPanelAction::None:        break;
	}
}

void simulationData::saveMontageFile() const {
	const std::optional<std::filesystem::path> chosenFile = askFileToSave("Guardar montaje", dataPath(DEFAULT_MONTAGE_FILE), MONTAGE_FILE_TYPE);
	if (chosenFile) {
		try {
			saveMontage(*chosenFile, cameraPath.getShots());
			std::cout << "Montaje guardado en " << chosenFile->string() << " (" << cameraPath.getShots().size() << " planos)\n";
		}
		catch (const std::exception& error) {
			std::cout << "No se ha guardado el montaje: " << error.what() << "\n";
		}
	}
}

void simulationData::loadMontageFile() {
	const std::optional<std::filesystem::path> chosenFile = askFileToOpen("Cargar montaje", dataPath(DEFAULT_MONTAGE_FILE), MONTAGE_FILE_TYPE);
	if (chosenFile){ 
		try {
			cameraPath.replaceShots(loadMontage(*chosenFile));   // si falla, salta antes de tocar la lista
			std::cout << "Montaje cargado de " << chosenFile->string() << ": " << cameraPath.getShots().size() << " planos\n";
		}
		catch (const std::exception& error) {
			std::cout << "No se ha cargado el montaje: " << error.what() << "\n";
		}
	}
}