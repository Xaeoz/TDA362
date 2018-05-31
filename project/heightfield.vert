#version 420
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec3 positionIn;
layout(location = 2) in vec2 texCoordIn;
layout(location = 4) in vec3 normalIn;
layout(location = 5) in vec3 heightIn;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform vec4 clippingPlane;
uniform mat4 normalMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform float density;
uniform float gradient;
uniform vec3 cameraPosition;
uniform vec3 globalLightPosition;
///////////////////////////////////////////////////////////////////////////////
// Output to fragment shader
///////////////////////////////////////////////////////////////////////////////
out vec2 texCoord;
out vec4 normal;
out vec3 position;
out vec3 viewSpacePosition;
out vec3 viewSpaceNormal;
out float visibility;
out vec3 toLightVector;
out vec3 surfaceNormal;

void main() 
{
	texCoord = texCoordIn;
	normal = vec4(normalize(normalIn), 0.0);
	viewSpaceNormal = normalize((normalMatrix * normal)).xyz;
	viewSpacePosition = (modelViewMatrix * vec4(positionIn, 1.0f)).xyz;
	//Direct Illumination
	surfaceNormal =  normalize((modelMatrix *normal)).xyz;
	vec4 worldPosition = modelMatrix * vec4(position, 1.0);
	toLightVector = globalLightPosition - worldPosition.xyz;
	//Normalized heights used for lighting
	position = heightIn;
	//Fog
	vec4 positionRelativeToCam = modelViewMatrix * vec4(positionIn, 1.0f);
	float dist = length(positionRelativeToCam.xyz);
	visibility = clamp(exp(-pow((dist*density), gradient)), 0, 1);

	gl_Position = modelViewProjectionMatrix * vec4(positionIn, 1.0f);
	//Needed to clip terrain from unecessary render passes
	gl_ClipDistance[0] = dot(modelMatrix*vec4(positionIn.x, positionIn.y, positionIn.z, 1.0f), clippingPlane);
}

//  