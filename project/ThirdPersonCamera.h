#pragma once
#include <glm/glm.hpp>

class ThirdPersonCamera
{
public:
	glm::vec3 position;

	void updateCamera(glm::mat4x4 parentModelMatrix);
	glm::mat4x4 generateViewMatrix();

	ThirdPersonCamera(float pitch, float angleAroundParent, float distanceToParent, float maxPitch);

	float calculateHorizontalDistance();

	float calculateVerticalDistance();

	void calculatePosition(float verticalDist, float horizDist, glm::mat4x4 parentModelMatrix);

	void addPitch(float dPitch);
	void addAngleAroundParent(float dYaw);
	void addDistance(float dDistance);

private:
	float pitch;
	float angleAroundParent;
	float distanceToParent;
	float maxPitch;
	float yaw;

};
