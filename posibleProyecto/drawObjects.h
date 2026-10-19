#pragma once
#include "global.h"
#include "Renderer.h"
#include "Entidad.h"

void submitEntidad(Renderer& renderer, const Entidad& e);

void submitEntidades(Renderer& renderer, const std::vector<Entidad>& entities);