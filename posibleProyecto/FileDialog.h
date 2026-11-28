#pragma once
#include <filesystem>
#include <optional>

struct FileTypeFilter {
	const char* extension;     // ".json": se anade si el nombre escrito no la lleva
	const char* description;   // lo que se ve en la ventana, p. ej. "Montaje (JSON)"
};

// ventanas del explorador del sistema; vacio si el usuario cancela
std::optional<std::filesystem::path> askFileToSave(const char* title, const std::filesystem::path& suggestedFile, FileTypeFilter filter);
std::optional<std::filesystem::path> askFileToOpen(const char* title, const std::filesystem::path& suggestedFile, FileTypeFilter filter);