#include "camera.h"

Camera::Camera() {
	w_view = glm::lookAt(glm::vec3(2.0f, 2.0f, 3.0f),   // dónde está la cámara
		glm::vec3(0.0f, 0.0f, 0.0f),   // a dónde mira
		glm::vec3(0.0f, 1.0f, 0.0f));  // qué es "arriba"
}

void Camera::set_camera(float widht, float height) {
	w_proj = glm::perspective(glm::radians(45.0f), (float)widht / height, w_near, w_far);
}