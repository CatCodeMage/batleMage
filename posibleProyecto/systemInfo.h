#pragma once
#include "global.h"

class SystemInfo
{
private:
	float w_time0;
	int w_widht;
	int w_height;
public:
	void getCurrentState(GLFWwindow* window);
	float getTimeStored() const { return w_time0; };
	int getWidht() const { return w_widht; };
	int getHeight() const { return w_height; };
};

