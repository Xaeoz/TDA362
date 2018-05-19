#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////
uniform vec3 material_color;
uniform float material_reflectivity;
uniform float material_metalness;
uniform float material_fresnel;
uniform float material_shininess;
uniform float material_emission;
uniform int has_diffuse_texture;


uniform int has_emission_texture;
uniform int has_color_texture;
layout(binding = 0) uniform sampler2D colorMap;
layout(binding = 5) uniform sampler2D emissiveMap;
layout(binding = 20) uniform sampler2D diffuseMap;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
layout(binding = 6) uniform sampler2D environmentMap;
layout(binding = 7) uniform sampler2D irradianceMap;
layout(binding = 8) uniform sampler2D reflectionMap;
uniform float environment_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
uniform vec3 point_light_color;
uniform float point_light_intensity_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
#define PI 3.14159265359

///////////////////////////////////////////////////////////////////////////////
// Input varyings from vertex shader
///////////////////////////////////////////////////////////////////////////////
in vec2 texCoord;
in vec3 viewSpaceNormal;
in vec3 viewSpacePosition;
in vec4 normal;
in vec3 positionOut;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform mat4 viewInverse;
uniform vec3 viewSpaceLightPosition;

///////////////////////////////////////////////////////////////////////////////
// Output color
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) out vec4 fragmentColor;


vec3 calculateDirectIllumiunation(vec3 wo, vec3 n)
{
	vec3 myColor = material_color;
	if (has_diffuse_texture == 1) {
		myColor = texture2D(diffuseMap, texCoord).xyz;
	}
	///////////////////////////////////////////////////////////////////////////
	// Task 1.2 - Calculate the radiance Li from the light, and the direction
	//            to the light. If the light is backfacing the triangle, 
	//            return vec3(0); 
	///////////////////////////////////////////////////////////////////////////
	vec3 wi = normalize(viewSpaceLightPosition - viewSpacePosition);
	float d = distance(viewSpaceLightPosition, viewSpacePosition);
	vec3 Li = point_light_intensity_multiplier * point_light_color * (100 / (d*d));
	if (dot(n, wi) <= 0) {
		return vec3(0.0);
	}
	///////////////////////////////////////////////////////////////////////////
	// Task 1.3 - Calculate the diffuse term and return that as the result
	///////////////////////////////////////////////////////////////////////////
	vec3 diffuse_term = myColor * (1.0 / PI) * abs(dot(n, wi)) * Li;

	///////////////////////////////////////////////////////////////////////////
	// Task 2 - Calculate the Torrance Sparrow BRDF and return the light 
	//          reflected from that instead
	///////////////////////////////////////////////////////////////////////////
	vec3 wh = normalize(wi + wo);
	float s = material_shininess;
	//float F = material_fresnel + (1 - material_fresnel) * pow((1 - dot(wh, wi)), 5);
	float F = material_fresnel + (1 - material_fresnel) * pow((1 - dot(wh, wi)), 5);
	float D = ((s + 2) / 2 * PI) * pow(dot(n, wh), s);
	float G = min(1, min((2 * (dot(n, wh) * dot(n, wo))) / dot(wh, wo), (2 * (dot(n, wh)  *dot(n, wi))) / dot(wh, wo)));
	float brdf = (F * D * G) / (4 * dot(n, wo) * dot(n, wi));
	///////////////////////////////////////////////////////////////////////////
	// Task 3 - Make your shader respect the parameters of our material model.
	///////////////////////////////////////////////////////////////////////////
	float m = material_metalness;
	vec3 dielectric_term = brdf * dot(n, wi)*Li + (1 - F) * diffuse_term;
	vec3 metal_term = brdf * myColor * dot(n, wi)*Li;
	vec3 microfacet_term = m * metal_term + (1 - m) * dielectric_term;
	//return brdf * dot(n, wi) * Li;
	float r = material_reflectivity;
	vec3 result = r * microfacet_term + (1 - r) * diffuse_term;
	//return brdf * dot(n, wi) * Li;
	return result;
}

vec3 calculateIndirectIllumination(vec3 wo, vec3 n)
{

	vec3 myColor = material_color;
	if (has_diffuse_texture == 1) {
		myColor = texture2D(diffuseMap, texCoord).xyz;
	}
	///////////////////////////////////////////////////////////////////////////
	// Task 5 - Lookup the irradiance from the irradiance map and calculate
	//          the diffuse reflection
	///////////////////////////////////////////////////////////////////////////
	vec4 nws = viewInverse * vec4(n.x, n.y, n.z, 0);

	// Calculate the spherical coordinates of the direction
	float theta = acos(max(-1.0f, min(1.0f, nws.y)));
	float phi = atan(nws.z, nws.x);
	if (phi < 0.0f) phi = phi + 2.0f * PI;
	// Use these to lookup the color in the environment map
	vec2 lookup = vec2(phi / (2.0 * PI), theta / PI);
	vec4 irradiance = environment_multiplier * texture(irradianceMap, lookup);


	vec3 diffuse_term = myColor * (1.0 / PI) * irradiance.xyz;
	///////////////////////////////////////////////////////////////////////////
	// Task 6 - Look up in the reflection map from the perfect specular 
	//          direction and calculate the dielectric and metal terms. 
	///////////////////////////////////////////////////////////////////////////

	vec3 wi = reflect(wo, n);
	float s = material_shininess;
	float roughness = sqrt(sqrt(2 / (s + 2)));
	vec3 Li = environment_multiplier * textureLod(reflectionMap, lookup, roughness * 7.0).xyz;
	vec3 wh = normalize(wi + wo);
	float F = material_fresnel + (1 - material_fresnel) * pow((1 - dot(wh, wi)), 5);
	vec3 dielectric_term = F*Li + (1 - F)*diffuse_term;
	vec3 metal_term = F * myColor * Li;
	//return diffuse_term;
	float m = material_metalness;
	float r = material_reflectivity;
	vec3 microfacet_term = m * metal_term + (1 - m) * dielectric_term;

	vec3 result = r * microfacet_term + (1 - r) * diffuse_term;
	return result;

}

void main() 
{
	float visibility = 1.0;
	float attenuation = 1.0;

	vec3 wo = -normalize(viewSpacePosition);
	vec3 n = normalize(viewSpaceNormal);

	vec3 myColor = material_color;
	if (has_diffuse_texture == 1) {
		myColor = texture2D(diffuseMap, texCoord).xyz;
	}
	// Direct illumination
	vec3 direct_illumination_term = visibility * calculateDirectIllumiunation(wo, n);

	// Indirect illumination
	vec3 indirect_illumination_term = calculateIndirectIllumination(wo, n);

	///////////////////////////////////////////////////////////////////////////
	// Add emissive term. If emissive texture exists, sample this term.
	///////////////////////////////////////////////////////////////////////////
	vec3 emission_term = material_emission * myColor;
	if (has_emission_texture == 1) {
		emission_term = texture(emissiveMap, texCoord).xyz;
	}

	vec3 shading = 
		direct_illumination_term +
		indirect_illumination_term +
		emission_term;


	//fragmentColor = vec4(indirect_illumination_term, 1.0f);			//Removes issue with emission map
	
	//Actual shading
	
	//fragmentColor = vec4(shading, 1.0f);

	if(normalize(positionOut).y < 0.16) {
		fragmentColor = vec4(normalize(vec3(6, 80, 170)), 1.0f);
	}

	if(normalize(positionOut).y > 0.15) {
		fragmentColor = vec4(normalize(vec3(6, 162, 170)), 1.0f);
	}

	if(normalize(positionOut).y > 0.31) {
		fragmentColor = vec4(normalize(vec3(165, 170, 6)), 1.0f);
	}

	if(normalize(positionOut).y > 0.45) {
		fragmentColor = vec4(normalize(vec3(6, 170, 9)), 1.0f);
	}

	if(normalize(positionOut).y > 0.6) {
		fragmentColor = vec4(normalize(vec3(12, 81, 4)), 1.0f);
	}

	if(normalize(positionOut).y > 0.75) {
		fragmentColor = vec4(normalize(vec3(186, 184, 178)), 1.0f);
	}

	if(normalize(positionOut).y > 0.9) {
		fragmentColor = vec4(1.0, 1.0, 1.0, 1.0f);
	}
	
	//Watch Perlin noise
	//fragmentColor = vec4(vec3(normalize(positionOut*0.5 + vec3(0.5)).y), 1.0f);
	//watch noise array
	
	//fragmentColor = vec4(vec3((positionOut).y), 1.0f);
	return;

}
