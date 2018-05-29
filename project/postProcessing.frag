#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

in vec2 texCoord;

layout(binding = 0) uniform sampler2D colorMap;
layout(binding = 1) uniform sampler2D depthMap;

void main() 
{
	float depth = texture2D(depthMap, texCoord).r;

	gl_FragColor = texture2D(colorMap, texCoord);

	//gl_FragColor = vec4(vec3(pow(depth,20)), 1);
	//gl_FragColor = vec4(texCoord,0, 1);
	//fragmentColor = vec4(0, 0, 0, 1);
}
