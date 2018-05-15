#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

//uniform vec3 material_color;
in vec4 clipSpaceCoords;

layout(location = 0) out vec4 fragmentColor;

layout(binding = 9) uniform sampler2D reflectionTexture;
layout(binding = 10) uniform sampler2D refractionTexture;

void main()
{
	vec2 normalizedDeviceSpaceCoords = (clipSpaceCoords.xy/clipSpaceCoords.w)/2 + 0.5f;	//Screen space, convert from (-1,1) interval to (0,1)
	vec4 reflectionColor = texture2D(reflectionTexture, vec2(normalizedDeviceSpaceCoords.x, -normalizedDeviceSpaceCoords.y));
	vec4 refractionColor = texture2D(refractionTexture, vec2(normalizedDeviceSpaceCoords.x, normalizedDeviceSpaceCoords.y));
	fragmentColor = mix(reflectionColor, refractionColor, 0.3);
	//fragmentColor = refractionColor;
	//fragmentColor = reflectionColor;
}
