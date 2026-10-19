#pragma once
#include "global.h" 
#include "Input.h"
#include "systemInfo.h"

class Camera
{
public:
	Camera(); 
	void set_camera(float widht, float height);
	void update(const Input & input, const SystemInfo& sysInfo);
	bool procesOrders(const Input& input, float& fwd, float& strafe, float& turn);

	glm::mat4 get_view() const { return w_view; };
	glm::mat4 get_proj() const { return w_proj; };
private: 
	void updateView();

	glm::mat4 w_view;
	glm::mat4 w_proj;

	glm::vec3 w_pos;    
	float     w_yaw, w_pitch;
	glm::vec3 w_front;

	const float w_far = 100.f;
	const float w_near = 0.1f;

	const float speed = 10.f;
	const float rotSpeed = 40.f;
};

 

