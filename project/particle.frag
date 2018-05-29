#version 420 compatibility
// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

in float life;
in float blend;
in vec2 texCoords;
in vec2 texCoordsNext;
in float depth;
in vec4 clipSpaceCoords;


uniform int atlasRows;
uniform float farPlane;
uniform float nearPlane;


layout(binding = 0) uniform sampler2D textureAtlas;
layout(binding = 1) uniform sampler2D depthMap;

const float blendStrength = 2;
const float fadeDistance = 7.0f;
const float fadeStrength = 2.0f;
const float lifeFadeThreshold = 0.9f; //Ratio at which fragments start fading because of long life
void main()
{

	gl_FragColor = texture2D(textureAtlas, texCoords);
	gl_FragColor = mix(gl_FragColor, texture2D(textureAtlas, texCoordsNext), pow(blend, blendStrength));

	vec2 normalizedDeviceSpaceCoords = (clipSpaceCoords.xy/clipSpaceCoords.w)/2 + 0.5f;

	//Fade out fragments that overlap/are close to other objects
	float depthValueOfSurface = texture2D(depthMap, vec2(normalizedDeviceSpaceCoords.x, normalizedDeviceSpaceCoords.y)).r;
	
	float actualSurfaceDepth = 2.0*nearPlane*farPlane/ (farPlane + nearPlane - (2.0 * depthValueOfSurface - 1.0) * (farPlane-nearPlane));	
	float actualFragmentDepth =  2.0*nearPlane*farPlane/ (farPlane + nearPlane - (2.0 * gl_FragCoord.z - 1.0) * (farPlane-nearPlane));
	
	float depthDifference = actualSurfaceDepth-actualFragmentDepth;

	gl_FragColor.a = min(gl_FragColor.a, pow(clamp(depthDifference/fadeDistance,0,1), fadeStrength));

	float lifeFactor = 1.0f/(1.0f-lifeFadeThreshold); //Allows alpha to go from 1.0 -> 0 as life goes from lifeFadeThreshold -> 1.0f 
	gl_FragColor.a = min(gl_FragColor.a, 1.0f - lifeFactor*(life-lifeFadeThreshold));


}

/*
    // Basse color.
    gl_FragColor = texture2D(colortexture,gl_PointCoord);
    // Make it darker the older it is.
    gl_FragColor.xyz *= (1.0-life);
    // Make it fade out the older it is, also all particles have a 
    // very low base alpha so they blend together.
    gl_FragColor.w = gl_FragColor.w * (1.0-pow(life, 4.0))*0.05;
	*/