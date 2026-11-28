#include "camera.h"

Camera::Camera() {
	w_pos = glm::vec3(0.f, 2.f, 6.f);

	w_yaw = -90.f;
	w_pitch = -15.f;

	updateFront();
	updateView();
}

void Camera::setPose(const CameraPose& pose) {   // front y view se recalculan en update
	w_pos = pose.position;
	w_yaw = pose.yaw;
	w_pitch = pose.pitch;
}

void Camera::update(Input& input, const SystemInfo& sysInfo, bool isFreeToMove) {
	set_camera((float)sysInfo.getWidht(), (float)sysInfo.getHeight());
	if (isFreeToMove) rotateWithMouse(input);   // en modo resultado el raton no debe torcer la toma
	updateFront();
	if (isFreeToMove) moveWithKeys(input, sysInfo.getDeltaTime());
	updateView();
}

void Camera::set_camera(float widht, float height) {
	w_proj = glm::perspective(glm::radians(45.0f), (float)widht / height, w_near, w_far);
}

void Camera::rotateWithMouse(Input& input) {
	float mouseDx, mouseDy;
	input.consumeMouseDelta(mouseDx, mouseDy);
	w_yaw += mouseDx * mouseSensitivity;
	w_pitch += mouseDy * mouseSensitivity;
	w_pitch = glm::clamp(w_pitch, -89.f, 89.f);   // a 90 grados lookAt se queda sin "arriba"
}

float Camera::axisFromKeys(const Input& input, int positiveKey, int negativeKey) const {
	return (float)input.keyDown(positiveKey) - (float)input.keyDown(negativeKey);   // -1, 0 o 1
}

glm::vec3 frontFromYawPitch(float yawDegrees, float pitchDegrees) {
	const float yaw = glm::radians(yawDegrees);
	const float pitch = glm::radians(pitchDegrees);
	return glm::normalize(glm::vec3(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch)));
}

void Camera::updateFront() {
	w_front = frontFromYawPitch(w_yaw, w_pitch);
}

void Camera::moveWithKeys(const Input& input, float dt) {
	const glm::vec3 worldUp(0.f, 1.f, 0.f);
	const glm::vec3 right = glm::normalize(glm::cross(w_front, worldUp));

	const float forwardInput = axisFromKeys(input, GLFW_KEY_W, GLFW_KEY_S);
	const float rightInput = axisFromKeys(input, GLFW_KEY_D, GLFW_KEY_A);
	const float upInput = axisFromKeys(input, GLFW_KEY_E, GLFW_KEY_Q);

	w_pos += (w_front * forwardInput + right * rightInput + worldUp * upInput) * speed * dt;
}

void Camera::updateView() {
	w_view = glm::lookAt(w_pos, w_pos + w_front, glm::vec3(0.f, 1.f, 0.f));
}