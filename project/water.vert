#version 420

layout(location = 0) in vec3 position;

out vec4 clipSpaceCoords;

uniform mat4 modelViewProjectionMatrix;



void main()
{
	clipSpaceCoords = modelViewProjectionMatrix * vec4(position.x, position.y, position.z, 1.0);
	gl_Position = clipSpaceCoords;
}
