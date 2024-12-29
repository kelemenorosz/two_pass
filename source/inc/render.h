#pragma once

//Abstract render class
class Render {

public:

	//Pure virtual functions
	virtual void LoadContent() = 0;
	virtual void OnRender() = 0;
	virtual void OnUpdate(double elapsedTime) = 0;
	virtual void OnMouseWheel(float mouseWheelData) = 0;

};