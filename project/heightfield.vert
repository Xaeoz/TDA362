#version 420
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texCoordIn;
layout(location = 4) in vec3 normalIn;
layout(binding = 9) uniform sampler2D heightMap;
layout(binding = 21) uniform sampler2D heightNormals;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////

uniform mat4 normalMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
///////////////////////////////////////////////////////////////////////////////
// Output to fragment shader
///////////////////////////////////////////////////////////////////////////////
out vec2 texCoord;
out vec4 normal;
out vec3 viewSpacePosition;
out vec3 viewSpaceNormal;
out vec3 positionOut;

void main() 
{
	normal = vec4(normalIn, 0.0);
	viewSpaceNormal = normalize((normalMatrix * normal)).xyz;
	//viewSpaceNormal = vec3(0.0f, 1.0f, 0.0f);
	//viewSpaceNormal = normalize(viewSpaceNormal);
	//gl_Position = modelViewProjectionMatrix * vec4(position.x, 0-texture2D(heightMap, normalTexCoord.xy).x, position.z, 1.0f);
	gl_Position = modelViewProjectionMatrix * vec4(position.x, position.y, position.z, 1.0f);
	viewSpacePosition = (modelViewMatrix * vec4(position.x, position.y, position.z, 1.0f)).xyz;
	//viewSpacePosition = (modelViewMatrix * vec4(position.x, position.x, position.z, 1.0f)).xyz;
	positionOut = position;
	texCoord = texCoordIn;
}

//  