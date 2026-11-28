 #pragma once
#include "global.h"
#include "Material.h"

using MaterialId = uint32_t;
constexpr MaterialId INVALID_MATERIAL_ID = UINT32_MAX;

class MaterialLibrary
{
public:
	MaterialId add(const Material& m, const std::string& key);
	const Material& get(MaterialId id) const;
	std::size_t size() const;
	void loadFromJson(const std::filesystem::path& path); // en caso de que en un futuro querramos otros tipos de materiales

private:
	std::vector<Material> m_materials;
	std::unordered_map<std::string, MaterialId> m_keyId;
};