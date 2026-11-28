#pragma once
#include <filesystem>
#include <vector>
#include "CameraPath.h"

void saveMontage(const std::filesystem::path& file, const std::vector<CameraShot>& shots);   
std::vector<CameraShot> loadMontage(const std::filesystem::path& file);                    