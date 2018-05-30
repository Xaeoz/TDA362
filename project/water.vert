#version 420

layout(location = 0) in vec3 position;

out vec4 clipSpaceCoords;
out vec2 texCoords;
out vec3 toCameraVector;
out vec3 fromLightVector;

//Used to transform into projected view space (or clipSpace)
uniform mat4 modelViewProjectionMatrix;
//Used seperately to transform into world space
uniform mat4 modelMatrix;
//Position of active camera
uniform vec3 cameraPosition;
//Position of the global light (like the sun)
uniform vec3 globalLightPosition;

//Tiling of the texCoords
const float tiling = 100.0;


void main()
{
	//Calculate world position of vertex
	vec4 worldPosition = modelMatrix * vec4(position, 1.0);

	//Calculate the coordinates of this vertex in clipSpace (similar/same as screen space, just not normalized)
	clipSpaceCoords = modelViewProjectionMatrix * vec4(position.x, position.y, position.z, 1.0);
	//Set the gl_position to the clipSpaceCoords, this is the value that is interpolated when assigning fragCoords in fragmentShader
	gl_Position = clipSpaceCoords;
	//Calculate texCoords from position of vertex, then add on tiling
	texCoords = vec2(position.x/2.0 + 0.5, position.z/2.0 + 0.5) * tiling;

	toCameraVector = cameraPosition - worldPosition.xyz;
	fromLightVector = worldPosition.xyz - globalLightPosition;
	 
}
