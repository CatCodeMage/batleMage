#include "MaterialLibrary.h"

MaterialId MaterialLibrary::add(const Material& m, const std::string& key) {
	auto it = m_keyId.find(key);

	if (it != m_keyId.end())
	{
		m_materials[it->second] = m;
		return it->second;
	}

	MaterialId id = static_cast<MaterialId>(m_materials.size());
	m_materials.push_back(m);
	m_keyId[key] = id;
	return id;
}

const Material& MaterialLibrary::get(MaterialId id) const {
	if (id >= m_materials.size()) {
		throw std::out_of_range("MaterialId fuera de rango: " + std::to_string(id));
	}

	return m_materials[id];
}

std::size_t MaterialLibrary::size() const {
	return m_materials.size();
}
// en caso de que en un futuro querramos otros tipos de materiales
void MaterialLibrary::loadFromJson(const std::filesystem::path& path) {
	std::ifstream in(path);

	if (!in.is_open()) {
		throw std::runtime_error("No se pudo abrir " + path.string());
	}

	nlohmann::json j;

	in >> j;

	for (auto& [name, matJson] : j.items()) {
		Material mat;

		mat.name = name;

		if (matJson.contains("albedoTexture")) {
			auto& c = matJson["albedoColor"];
			mat.albedoColor = glm::vec4(c[0], c[1], c[2], c.size() > 3 ? c[3].get<float>() : 1.0f);
		}

		if (matJson.contains("emissive")) {
			auto& e = matJson["emissive"];
			mat.emissive = glm::vec3(e[0], e[1], e[2]);
		}

		if (matJson.contains("roughness")) {
			mat.roughness = matJson["roughness"];
		}

		if (matJson.contains("metallic")) {
			mat.metallic = matJson["metallic"];
		}
		
		if (matJson.contains("opacity")) {
			mat.opacity = matJson["opacity"];
		}

		add(mat, name);
	}
}
