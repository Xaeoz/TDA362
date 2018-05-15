#version 420

layout(location = 0) in vec3 position;

out vec4 clipSpaceCoords;
out vec2 texCoords;

uniform mat4 modelViewProjectionMatrix;

const float tiling = 6.0;


void main()
{
	clipSpaceCoords = modelViewProjectionMatrix * vec4(position.x, position.y, position.z, 1.0);
	gl_Position = clipSpaceCoords;
	texCoords = vec2(position.x/2.0 + 0.5, position.z/2.0 + 0.5) * tiling;

}
