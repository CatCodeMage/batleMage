#pragma once

struct SceneViewport {   // en pixeles del framebuffer, con el origen abajo a la izquierda (como glViewport)
	int x;
	int y;
	int width;
	int height;

	float aspectRatio() const { return (float)width / (float)height; }
	bool isVisible() const { return width > 0 && height > 0; }
};