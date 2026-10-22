#pragma once
#include "global.h"
#include "Renderer.h"
#include "Entidad.h"

void submitEntidad(Renderer& renderer, const ModelLibrary& models, const Entidad& e);

void submitEntidades(Renderer& renderer, const ModelLibrary& models, const std::vector<Entidad>& entities);