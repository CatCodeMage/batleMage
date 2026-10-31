#pragma once
#include "global.h"

class SystemInfo
{
private:
	float w_time0;
	float w_time1;
	int w_widht = 0;
	int w_height = 0;
public:
	SystemInfo();
	void getCurrentState(GLFWwindow* window);
	float getTimeStored() const { return w_time1; };
	float getDeltaTime() const { return w_time1 - w_time0; };
	int getWidht() const { return w_widht; };
	int getHeight() const { return w_height; };
	void toggleCursor(GLFWwindow* w);
};

