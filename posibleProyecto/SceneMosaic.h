#pragma once
#include <array>
#include <vector>
#include "SceneViewport.h"

// Cuantas vistas se ven a la vez y como se reparte la zona de la escena entre ellas
class SceneMosaic {
public:
	int getViewCount() const { return LAYOUTS[w_layoutIndex].viewCount; }
	bool canAddViews() const { return w_layoutIndex + 1 < (int)LAYOUTS.size(); }
	bool canRemoveViews() const { return w_layoutIndex > 0; }
	void addViews();      // 1 -> 2 -> 4 -> 6 -> 8
	void removeViews();   // 8 -> 6 -> 4 -> 2 -> 1
	std::vector<SceneViewport> splitArea(const SceneViewport& area) const;   // de izquierda a derecha y de arriba abajo

private:
	struct Layout {
		int viewCount;
		int columns;
		int rows;
	};
	static constexpr std::array<Layout, 5> LAYOUTS{ {
		{ 1, 1, 1 },{ 2, 2, 1 },{ 4, 2, 2 },{ 6, 3, 2 },{ 8, 4, 2 }
	} };
	int w_layoutIndex = 1;   // empieza con 2 vistas, como en la fase 1
};