#include "FileDialog.h"
#include <string>
#include <tinyfiledialogs.h>

namespace {
	constexpr int SINGLE_FILE = 0;   // sin seleccion multiple

	std::string toUtf8(const std::filesystem::path& path) {   // la libreria trabaja en UTF-8 en Windows: una carpeta con acentos llega bien
		std::filesystem::path nativePath = std::filesystem::absolute(path);   // con ruta relativa la ventana podria abrirse en otra carpeta
		nativePath.make_preferred();
		const std::u8string utf8 = nativePath.u8string();
		return std::string(utf8.begin(), utf8.end());
	}

	std::filesystem::path fromUtf8(const char* utf8) {
		return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(utf8)));
	}
}

std::optional<std::filesystem::path> askFileToSave(const char* title, const std::filesystem::path& suggestedFile, FileTypeFilter filter) {
	const std::string suggestedFileText = toUtf8(suggestedFile);
	const std::string pattern = std::string("*") + filter.extension;
	const char* const patterns[] = { pattern.c_str() };
	const char* chosenFile = tinyfd_saveFileDialog(title, suggestedFileText.c_str(), 1, patterns, filter.description);
	if (!chosenFile) return std::nullopt;

	std::filesystem::path file = fromUtf8(chosenFile);
	if (!file.has_extension()) file += filter.extension;   // si se escribe "pelea1", se guarda como "pelea1.json"
	return file;
}

std::optional<std::filesystem::path> askFileToOpen(const char* title, const std::filesystem::path& suggestedFile, FileTypeFilter filter) {
	const std::string suggestedFileText = toUtf8(suggestedFile);
	const std::string pattern = std::string("*") + filter.extension;
	const char* const patterns[] = { pattern.c_str() };
	const char* chosenFile = tinyfd_openFileDialog(title, suggestedFileText.c_str(), 1, patterns, filter.description, SINGLE_FILE);
	if (!chosenFile) return std::nullopt;
	return fromUtf8(chosenFile);
}