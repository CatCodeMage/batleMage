#pragma once

constexpr int NO_VIEW = -1;

struct SceneViewport {   // en pixeles del framebuffer, con el origen abajo a la izquierda (como glViewport)
	int x;
	int y;
	int width;
	int height;

	float aspectRatio() const { return (float)width / (float)height; }
	bool isVisible() const { return width > 0 && height > 0; }
	bool contains(float pixelX, float pixelY) const { return pixelX >= x && pixelX < x + width && pixelY >= y && pixelY < y + height; }
};