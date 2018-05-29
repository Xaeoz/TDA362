#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

uniform vec3 material_color;

in  vec2 texCoord;
in  vec4 normal;
in  vec3 viewSpacePosition;
in  vec3 position;

in  vec3 viewSpaceNormal;
in	vec3 point_light_color;
in	vec3 viewSpaceLightPosition;
in float point_light_intensity_multiplier;


const int materialsCount = 6;
uniform vec3 baseColours[materialsCount];
uniform float baseHeights[materialsCount];
uniform float baseBlends[materialsCount];
uniform float baseColourStrengths[materialsCount];
uniform float baseTextureScales[materialsCount];
uniform float environmentMultiplier;
float minHeight = 0.0f;
float maxHeight = 1.0f;
float epsilon = 0.000001f;
vec3 scaledWorldPos = position/0.3;


layout(binding = 26) uniform sampler2DArray textureArray;
layout(location = 0) out vec4 fragmentColour;

float inverseLerp(float a, float b, float val) {
	return clamp((val-a)/(b-a), minHeight, maxHeight);
}

//Calculate some basic ambient lighting
vec3 calculateIllumiunation(vec3 wo, vec3 n, vec3 colorIn)
{
	float cosTheta = clamp(dot(n, wo), 0, 1);
	return colorIn * cosTheta *environmentMultiplier;
}

//Blend together textures projected from all axes based on the vertex normal.
vec3 triplanar(vec3 worldPos, float scale, vec3 blendAxes, int textureIndex) {
	vec3 scaledWorldPos = position / scale;
	float bias= 0.01f;
	vec3 xProjection = texture(textureArray, vec3(scaledWorldPos.yz,textureIndex), bias).xyz * blendAxes.x;
	vec3 yProjection = texture(textureArray, vec3(scaledWorldPos.xz,textureIndex), bias).xyz * blendAxes.y;
	vec3 zProjection = texture(textureArray, vec3(scaledWorldPos.xy,textureIndex), bias).xyz * blendAxes.z;
	vec3 projectionSum = xProjection + yProjection + zProjection;
	return projectionSum;
}

void main() 
{
	vec3 wo = -normalize(viewSpacePosition);
	vec3 n = normalize(viewSpaceNormal);
	float heightPercent = inverseLerp(minHeight, maxHeight, position.y); //Percentage of maximum height

	vec3 outputColour;
	vec3 blendAxes = abs(normal.xyz);
	blendAxes /= (blendAxes.x + blendAxes.y + blendAxes.z);


	for(int i = 0; i < materialsCount; i++) {
		float colourStrength = inverseLerp(-baseBlends[i]/2 - epsilon, baseBlends[i]/2, heightPercent - baseHeights[i]);
		vec3 baseColour = baseColours[i] * baseColourStrengths[i];
		vec3 textureColour = triplanar(position, baseTextureScales[i], blendAxes, i)*(1-baseColourStrengths[i]);
		outputColour = outputColour * (1-colourStrength) + (baseColour+textureColour)*colourStrength;
	}

	fragmentColour =  vec4(calculateIllumiunation(wo, n, outputColour), 1.0);
	return;
}

