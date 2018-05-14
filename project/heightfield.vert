#version 420
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texCoordIn;
layout(binding = 9) uniform sampler2D heightMap;

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
out vec3 viewSpacePosition;
out vec3 viewSpaceNormal;

void main() 
{
	vec3 normal;
	float stepLength = 0.001;
	vec3 point1 = vec3(texCoordIn.x - stepLength, texture2D(heightMap, vec2(texCoordIn.x - stepLength, texCoordIn.y + stepLength)).x, texCoordIn.y + stepLength);
	vec3 point2 = vec3(texCoordIn.x - stepLength, texture2D(heightMap, vec2(texCoordIn.x - stepLength, texCoordIn.y - stepLength)).x, texCoordIn.y - stepLength);
	vec3 point3 = vec3(texCoordIn.x + stepLength, texture2D(heightMap, vec2(texCoordIn.x + stepLength, texCoordIn.y + stepLength)).x, texCoordIn.y + stepLength);
	vec3 vector31 = point3 - point1;
	vec3 vector21 = point2 - point1;
	//normal = vec3(0.0f,1.0f,0.0f);
	normal = normalize(cross(vector31, vector21));
	viewSpaceNormal = (normalMatrix *  vec4(normal, 0.0)).xyz;
	//viewSpaceNormal = vec3(0.0f, 1.0f, 0.0f);
	viewSpaceNormal = normalize(viewSpaceNormal);
	gl_Position = modelViewProjectionMatrix * vec4(position.x, texture2D(heightMap, texCoordIn.xy).x, position.z, 1.0f);
	viewSpacePosition = (modelViewMatrix * vec4(position.x, texture2D(heightMap, texCoordIn.xy).x, position.z, 1.0f)).xyz;
	
	texCoord = texCoordIn;
}

//  