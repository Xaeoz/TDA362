#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

uniform vec3 material_color;

in vec2 texCoord;
layout(location = 0) out vec4 fragmentColor;

layout(binding = 10) uniform sampler2D colorMap;

void main() 
{
	fragmentColor = texture2D(colorMap, texCoord);
	//fragmentColor = vec4(0, 0, 0, 1);
}
