#pragma once
#include "global.h"
// Raíz del repo vista desde el directorio de trabajo de Visual Studio (la carpeta del proyecto).

std::filesystem::path dataPath(const std::string& relative);