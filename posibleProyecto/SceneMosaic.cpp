#include "SceneMosaic.h"

void SceneMosaic::addViews() {
	if (canAddViews()) ++w_layoutIndex;
}

void SceneMosaic::removeViews() {
	if (canRemoveViews()) --w_layoutIndex;
}

std::vector<SceneViewport> SceneMosaic::splitArea(const SceneViewport& area) const {
	const Layout& layout = LAYOUTS[w_layoutIndex];
	std::vector<SceneViewport> views;
	views.reserve(layout.viewCount);

	for (int row = 0; row < layout.rows; ++row) {
		// bordes calculados con division entera: las celdas encajan sin huecos aunque no divida exacto
		const int top = area.height * row / layout.rows;
		const int bottom = area.height * (row + 1) / layout.rows;
		for (int column = 0; column < layout.columns; ++column) {
			const int left = area.width * column / layout.columns;
			const int right = area.width * (column + 1) / layout.columns;
			views.push_back({ area.x + left, area.y + area.height - bottom, right - left, bottom - top });   // fila 0 arriba; OpenGL cuenta desde abajo
		}
	}
	return views;
}