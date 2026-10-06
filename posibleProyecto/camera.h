#pragma once
#include "global.h" 

class Camera
{
public:
	Camera(); 
	void set_camera(float widht, float height);

	glm::mat4 get_view() const { return w_view; };
	glm::mat4 get_proj() const { return w_proj; };
private: 
	glm::mat4 w_view;
	glm::mat4 w_proj;

	const float w_far = 100.f;
	const float w_near = 0.1f;
};

 

