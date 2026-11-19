#pragma once
#include "global.h"
#include "Renderer.h"
#include "Entidad.h"
#include "skeleton_animation/SkinnedRenderer.h"

void submitEntidad(Renderer& renderer, const Model& model, const Entidad& e);
void submitEntidad(bm::anim::SkinnedRenderer& renderer, const SkinnedModel& model, const Entidad& e,
    const bm::anim::Animator& animator);
