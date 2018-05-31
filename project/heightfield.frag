#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;
#define PI 3.14159265359
uniform vec3 material_color;

in  vec2 texCoord;
in  vec4 normal;
in  vec3 viewSpacePosition;
in  vec3 position;

in  vec3 viewSpaceNormal;
in	vec3 point_light_color;
in	vec3 viewSpaceLightPosition;
in vec3 toCameraVector;
in vec3 toLightVector;
in float point_light_intensity_multiplier;
in float visibility;
in vec3 surfaceNormal;

const int materialsCount = 6;
uniform vec3 baseColours[materialsCount];
uniform float baseHeights[materialsCount];
uniform float baseBlends[materialsCount];
uniform float baseColourStrengths[materialsCount];
uniform float baseTextureScales[materialsCount];
uniform float environmentMultiplier;
uniform mat4 viewInverse;
uniform bool fogActive;
uniform vec3 fogColor;
uniform bool calculateDirectLighting;

float minHeight = 0.0f;
float maxHeight = 1.0f;
float epsilon = 0.000001f;
vec3 scaledWorldPos = position/0.3;

layout(binding = 26) uniform sampler2DArray textureArray;
layout(binding = 7) uniform sampler2D irradianceMap;
layout(binding = 8) uniform sampler2D reflectionMap;
layout(location = 0) out vec4 fragmentColour;

float inverseLerp(float a, float b, float val) {
	return clamp((val-a)/(b-a), minHeight, maxHeight);
}

//Calculate some basic ambient lighting
vec3 calculateIllumiunation(vec3 wo, vec3 n, vec3 colorIn)
{
	vec4 nws = viewInverse * vec4(n.x, n.y, n.z, 0);

	// Calculate the spherical coordinates of the direction
	float theta = acos(max(-1.0f, min(1.0f, nws.y)));
	float phi = atan(nws.z, nws.x);
	if (phi < 0.0f) phi = phi + 2.0f * PI;

	//Lookup the color in the environment map
	vec2 lookup = vec2(phi / (2.0 * PI), theta / PI);
	vec4 irradiance = environmentMultiplier * texture(irradianceMap, lookup);
	vec3 diffuse_term = colorIn * (1.0 / PI) * irradiance.xyz;
	vec3 wi = reflect(wo, n);

	float material_shininess = 0.9f;
	float material_fresnel = 1.f;
	float material_metalness = 0.9f;
	float material_reflectivity = 0.9f;

	float s = 0.5f; //material_shininess
	float roughness = sqrt(sqrt(2 / (s + 2)));
	vec3 Li = environmentMultiplier * textureLod(reflectionMap, lookup, roughness * 7.0).xyz;
	vec3 wh = normalize(wi + wo);
	float F = material_fresnel + (1 - material_fresnel) * pow((1 - dot(wh, wi)), 5);
	vec3 dielectric_term = F*Li + (1 - F)*diffuse_term;
	vec3 metal_term = F * colorIn * Li;
	//return diffuse_term;
	float m = material_metalness;
	float r = material_reflectivity;
	vec3 microfacet_term = m * metal_term + (1 - m) * dielectric_term;

	vec3 result = r * microfacet_term + (1 - r) * diffuse_term;
	return result;
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
	vec3 wo = -normalize(viewSpaceLightPosition);
	vec3 n = normalize(viewSpaceNormal.xyz);
	vec3 unitSurfaceNormal = normalize(surfaceNormal);
	vec3 unitLightVector = normalize(toLightVector);
	float nDot = clamp(dot(unitLightVector, unitSurfaceNormal), 0, 1);

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

	fragmentColour =  vec4(calculateIllumiunation(wo, n, outputColour), 1.0f);
	if(calculateDirectLighting)
	{
		fragmentColour =  vec4(calculateIllumiunation(wo, n, outputColour), 1.0f) +  vec4(calculateIllumiunation(wo, n, outputColour), 1.0f)*nDot*environmentMultiplier;
	}
	if(fogActive) {
		fragmentColour =  mix(vec4(fogColor, 1.0), fragmentColour, visibility);
	}
	return;
}

