#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

//uniform vec3 material_color;
in vec4 clipSpaceCoords;
in vec2 texCoords;

layout(location = 0) out vec4 fragmentColor;

layout(binding = 0) uniform sampler2D reflectionTexture;
layout(binding = 1) uniform sampler2D refractionTexture;
layout(binding = 2) uniform sampler2D dudvMap;

uniform float moveFactor;

const float waveStrength = 0.02;

void main()
{
	vec2 normalizedDeviceSpaceCoords = (clipSpaceCoords.xy/clipSpaceCoords.w)/2 + 0.5f;	//Screen space, convert from (-1,1) interval to (0,1)

	vec2 reflectionCoords = vec2(normalizedDeviceSpaceCoords.x, -normalizedDeviceSpaceCoords.y);
	vec2 refractionCoords = vec2(normalizedDeviceSpaceCoords.x, normalizedDeviceSpaceCoords.y);
	
	vec2 distortion1 = (texture2D(dudvMap, vec2(texCoords.x + moveFactor, texCoords.y)).rg * 2.0 - vec2(1.0)) * waveStrength;
	vec2 distortion2 = (texture2D(dudvMap, vec2(-texCoords.x + moveFactor, texCoords.y + moveFactor)).rg * 2.0 - vec2(1.0)) * waveStrength;
	vec2 totalDistortion = distortion1 + distortion2;

	reflectionCoords+= totalDistortion;
	reflectionCoords.x = clamp(reflectionCoords.x, 0.001, 0.999); //Don't let the distortion go outside the texture
	reflectionCoords.y = clamp(reflectionCoords.y, -0.999, -0.001);

	refractionCoords+= totalDistortion;  
	refractionCoords = clamp(refractionCoords, 0.001, 0.999);

	vec4 reflectionColor = texture2D(reflectionTexture, reflectionCoords);
	vec4 refractionColor = texture2D(refractionTexture, refractionCoords);


	fragmentColor = mix(reflectionColor, refractionColor, 0.3);
	//fragmentColor = refractionColor;
	//fragmentColor = reflectionColor;

	//Blend in a bit of bluish-green tint
	fragmentColor = mix(fragmentColor, vec4(0.0, 0.3, 0.5, 1.0), 0.2);
}
