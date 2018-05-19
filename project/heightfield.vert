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
uniform vec4 clippingPlane;
uniform mat4 normalMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 modelMatrix;
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
	//float stepLength = 0.001;
	//vec3 point1 = vec3(position.x, texture2D(heightMap, vec2(texCoordIn.x, texCoordIn.y)).x, position.z);
	//vec3 point2 = vec3(position.x, texture2D(heightMap, vec2(texCoordIn.x, texCoordIn.y + stepLength)).x, position.z + stepLength);
	//vec3 point3 = vec3(position.x + stepLength, texture2D(heightMap, vec2(texCoordIn.x + stepLength, texCoordIn.y)).x, position.z);
	//vec3 vector31 = point3 - point1;
	//vec3 vector21 = point2 - point1;
	//normal = normalize(vec4(cross(vector21, vector31), 1.0));
	viewSpaceNormal = normalize((normalMatrix * normal)).xyz;
	//viewSpaceNormal = vec3(0.0f, 1.0f, 0.0f);
	//viewSpaceNormal = normalize(viewSpaceNormal);
	//gl_Position = modelViewProjectionMatrix * vec4(position.x, 0-texture2D(heightMap, normalTexCoord.xy).x, position.z, 1.0f);
	gl_Position = modelViewProjectionMatrix * vec4(position*0.5 + vec3(0.5), 1.0f);
	//gl_Position = modelViewProjectionMatrix * vec4(position.x, 1.0f, position.z, 1.0f);
	viewSpacePosition = (modelViewMatrix * vec4(position.x, position.y, position.z, 1.0f)).xyz;
	//viewSpacePosition = (modelViewMatrix * vec4(position.x, position.x, position.z, 1.0f)).xyz;
	//positionOut = vec3(position*0.5 + vec3(0.5));
	positionOut = position;
	//Needed to clip terrain from unecessary render passes
	gl_ClipDistance[0] = dot(modelMatrix*vec4(position.x, 0-texture2D(heightMap, texCoordIn.xy).x, position.z, 1.0f), clippingPlane);
	texCoord = texCoordIn;
}

//  