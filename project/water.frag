#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

//uniform vec3 material_color;
in vec2 texCoords;

layout(location = 0) out vec4 fragmentColor;

layout(binding = 9) uniform sampler2D reflectionTexture;
layout(binding = 10) uniform sampler2D refractionTexture;

void main()
{
	vec4 reflectionColor = texture2D(reflectionTexture, texCoords);
	vec4 refractionColor = texture2D(refractionTexture, texCoords);
	fragmentColor = mix(reflectionColor, refractionColor, 0.5);
}
