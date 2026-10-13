#include "camera.h"

Camera::Camera() {
	w_pos = glm::vec3(0.f, 2.f, 6.f);

	w_yaw = -90.f; 
	w_pitch = -15.f;

	this->updateView();
}

void Camera::set_camera(float widht, float height) {
	w_proj = glm::perspective(glm::radians(45.0f), (float)widht / height, w_near, w_far);
}

void Camera::updateView() {
	w_front = glm::normalize(glm::vec3(
		cos(glm::radians(w_yaw)) * cos(glm::radians(w_pitch)),
		sin(glm::radians(w_pitch)),
		sin(glm::radians(w_yaw)) * cos(glm::radians(w_pitch))));
	w_view = glm::lookAt(w_pos, w_pos + w_front, glm::vec3(0.f, 1.f, 0.f));
}

void Camera::update(const Input& input, float dt) {
	bool wDown = input.keyDown(GLFW_KEY_W);
	bool aDown = input.keyDown(GLFW_KEY_A);
	bool sDown = input.keyDown(GLFW_KEY_S);
	bool dDown = input.keyDown(GLFW_KEY_D);
	bool qDown = input.keyDown(GLFW_KEY_Q);
	bool eDown = input.keyDown(GLFW_KEY_E);
	bool shiftDown = input.keyDown(GLFW_KEY_LEFT_SHIFT);

	const float speed = 1.f;
	const float rotSpeed = 4.f;

	float x_mov, y_mov, z_mov;

	float fwd = (float)wDown - (float)sDown;   // -1, 0 o 1; W+S se anulan
	float strafe = (float)dDown - (float)aDown;
	float turn = (float)eDown - (float)qDown;

	if (shiftDown) {
		x_mov = y_mov = 0.f;
		z_mov = fwd * dt * speed;
		w_yaw += strafe * dt * rotSpeed;
		w_pitch += turn * dt * rotSpeed;
		w_pitch = glm::clamp(w_pitch, -89.f, 89.f);   // para que este en el angulo correcto
	}
	else {
		x_mov = strafe * dt * speed;
		y_mov = fwd * dt * speed;
		z_mov = 0.f;
	}

	w_pos += glm::vec3(x_mov, y_mov, z_mov);

	this->updateView();
}