#pragma once
#include "global.h"

// Capa propia sobre Dear ImGui: el resto del programa no toca los backends
class UserInterface {
public:
	void init(GLFWwindow* window);   // despues de keysInput.init: ImGui encadena los callbacks que ya hay
	void beginFrame(bool arePanelsVisible);
	void endFrame();                 // dibuja la interfaz encima de la escena
	void destroy();
	void setMouseEnabled(bool isEnabled);
	bool isUsingKeyboard() const;
	bool wasSceneClicked() const;
	void drawCameraModeHint();
};