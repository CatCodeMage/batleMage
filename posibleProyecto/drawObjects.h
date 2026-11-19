#pragma once
#include "global.h"
#include "Renderer.h"
#include "Entidad.h"
#include <span>

void submitEntidad(Renderer& renderer, const ModelLibrary& models, const Entidad& e,
    std::span<const ModelAnimation> animations = {}, double timeSeconds = 0.0);

void submitEntidades(Renderer& renderer, const ModelLibrary& models, const std::vector<Entidad>& entities,
    std::span<const ModelAnimation> animations = {}, double timeSeconds = 0.0);
