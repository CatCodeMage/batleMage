#pragma once
#include "global.h"
#include "Material.h"

using MaterialId = uint32_t;

class MaterialLibrary
{
public:
	MaterialId add(const Material& m, const std::string& key);
	const Material& get(MaterialId id) const;
	std::size_t size() const;

private:
	std::vector<Material> m_materials;
	std::unordered_map<std::string, MaterialId> m_keyId;
};