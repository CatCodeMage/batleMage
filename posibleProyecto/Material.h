#pragma once

#include "global.h"

struct Material
{
	GLuint albedoTexture = 0;
	glm::vec4 albedoColor{ 1.0f };
	glm::vec3 emissive{ 0.0f };
	float roughness = 1.0f;
	float metallic = 0.0f;
	float opacity = 1.0f;
};