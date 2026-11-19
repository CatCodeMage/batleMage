#pragma once
#include "global.h" 
#include "Input.h"
#include "systemInfo.h"

struct CameraPose {   // lo minimo para reconstruir la vista; el campo de vision es fijo
	glm::vec3 position;
	float yaw;
	float pitch;
};

class Camera
{
public:
	Camera(); 
	void set_camera(float widht, float height);

	glm::mat4 get_view() const { return w_view; };
	glm::mat4 get_proj() const { return w_proj; };
	CameraPose getPose() const { return { w_pos, w_yaw, w_pitch }; }
	void setPose(const CameraPose& pose);
	void update(Input& input, const SystemInfo& sysInfo, bool isFreeToMove);
private: 
	void updateView();

	void rotateWithMouse(Input& input);
	void moveWithKeys(const Input& input, float dt);
	float axisFromKeys(const Input& input, int positiveKey, int negativeKey) const;
	void updateFront();

	const float mouseSensitivity = 0.1f;   // grados por píxel

	glm::mat4 w_view;
	glm::mat4 w_proj;

	glm::vec3 w_pos;    
	float     w_yaw, w_pitch;
	glm::vec3 w_front;

	const float w_far = 100.f;
	const float w_near = 0.1f;

	const float speed = 10.f;
};

 

