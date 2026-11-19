#include "simulationData.h"

void simulationData::changeLights(std::vector<SceneLight>& newLights) {
	lights = newLights;
}

void simulationData::init(GLFWwindow* window) {
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
}

simulationData::~simulationData() {
	glUseProgram(0);
	skinnedRenderer.destroy();
	animators.clear();
	shadows.destroy();
	if (program) glDeleteProgram(program);
	libModel.destroy();
	libMesh.destroy();
	textures.destroy();
}

void simulationData::clearLastFrame() {
	glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void simulationData::updateFrame(GLFWwindow* window) {
	glfwSwapBuffers(window);
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

void simulationData::procesMainBodyLoopRealTime(GLFWwindow* window, SystemInfo& sysInfo) {
	clearLastFrame();

	// System
	sysInfo.getCurrentState(window);
	float time = sysInfo.getTimeStored();

	// Camara
	viewCamera.update(keysInput, sysInfo);

	glm::mat4 vp = viewCamera.get_proj() * viewCamera.get_view();

	submitEntities(map.entities);

	submitEntities(framesInfo.combat[framesInfo.frame]);

	renderer.submitLights(lights, time);

	//renderizar sombras
	shadows.render(lights, [&]() { renderer.drawShadowGeometry(libMesh); }, &skinnedRenderer);
	if (!skinnedRenderer.empty()) {
		const GLuint skinnedProgram = skinnedRenderer.lightingProgram();
		shadows.bindForLighting(skinnedProgram);
		renderer.applyLights(skinnedProgram);
	}

	// Pintar
	shadows.bindForLighting(program);
	renderer.flush(libMesh, vp);
	skinnedRenderer.drawLighting(vp);
	skinnedRenderer.clear();

	updateFrame(window);
}
