#include "ThirdPersonCamera.h"
#include <glm/gtx/transform.hpp>

using namespace glm;

ThirdPersonCamera::ThirdPersonCamera(float pitch, float angleAroundPlayer, float distanceToParent, float maxPitch)
	:
	pitch(pitch),
	angleAroundParent(angleAroundPlayer),
	distanceToParent(distanceToParent),
	maxPitch(maxPitch)
{

}

float ThirdPersonCamera::calculateHorizontalDistance()
{
	return distanceToParent * cos(radians(pitch));
}

float ThirdPersonCamera::calculateVerticalDistance()
{
	return distanceToParent * sin(radians(pitch));
}

void ThirdPersonCamera::calculatePosition(float verticalDist, float horizDist, mat4x4 parentModelMatrix)
{
	//float parentYRot = parentModelMatrix.; //Calculate euler angles here
	//float theta = parentYRot + angleAroundParent;
	//float offsetX = horizDist * sin(radians(theta));
	//float offsetZ = horizDist * cos(radians(theta));
	//position.x = parentModelMatrix[3].x - offsetZ;
	//position.y = parentModelMatrix[3].y + verticalDist;
	//position.z = parentModelMatrix[3].z - offsetX;
	//yaw = 180 - theta;

	mat4x4 modifiedMat = parentModelMatrix * rotate(angleAroundParent, vec3(0, 1, 0));
	//Still strange because of model facing in -x direction
	vec3 parentDirection = vec3(-modifiedMat[0][0],-modifiedMat[1][0], modifiedMat[2][0]);
	vec3 parentPosition = vec3(modifiedMat[3][0], modifiedMat[3][1], modifiedMat[3][2]);
	vec3 newPosition= parentPosition - parentDirection * distanceToParent * cos(pitch) + vec3(0,-1,0) * distanceToParent * sin(pitch);
	position = newPosition;



}

//Change the pitch of the camera (pitch in terms of its angle upwards from the parent)
void ThirdPersonCamera::addPitch(float dpitch)
{
	float newPitch = pitch + dpitch;
	pitch = clamp(newPitch, -maxPitch, maxPitch);
}

//Change the angle the camera is in comparison to the forward of the parent
void ThirdPersonCamera::addAngleAroundParent(float dYaw)
{
	float newAngleAroundPlayer = angleAroundParent + dYaw;
	angleAroundParent = newAngleAroundPlayer;
}

//Change the distance to the parent of the camera
void ThirdPersonCamera::addDistance(float dDistance)
{
	float newDistance = clamp(distanceToParent + dDistance, 10.0f, 100.f);
	distanceToParent = newDistance;
}

//Updates the camera position based on the parents modelMatrix
void ThirdPersonCamera::updateCamera(mat4x4 parentModelMatrix)
{
	//These are from an old version
	float hDist = calculateHorizontalDistance();
	float vDist = calculateVerticalDistance();

	//Actually calculate and update the function
	calculatePosition(vDist, hDist, parentModelMatrix);
}

mat4x4 ThirdPersonCamera::generateViewMatrix()
{
	
	return mat4x4();
}




