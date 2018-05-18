#version 420

layout(location = 0) in vec3 position;
uniform mat4 modelViewProjectionMatrix;

out vec2 texCoord;

void main()
{
	gl_Position = modelViewProjectionMatrix * vec4(position.x, position.y, position.z, 1.0);
	texCoord = 0.5 * (position.xz + vec2(1,1)); 
}
