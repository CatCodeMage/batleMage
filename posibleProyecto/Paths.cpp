#include "Paths.h"

static inline const std::filesystem::path DATA_ROOT = "..";

std::filesystem::path dataPath(const std::string& relative) {
    return DATA_ROOT / relative;
}