#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

//uniform vec3 material_color;
in vec4 clipSpaceCoords;
in vec2 texCoords;
in vec3 toCameraVector;
in vec3 fromLightVector;

layout(location = 0) out vec4 fragmentColor;

layout(binding = 0) uniform sampler2D reflectionTexture;
layout(binding = 1) uniform sampler2D refractionTexture;
layout(binding = 2) uniform sampler2D dudvMap;
layout(binding = 3) uniform sampler2D normalMap;
layout(binding = 4) uniform sampler2D depthMap;

uniform vec3 globalLightColor;
uniform float moveFactor;
uniform vec3 globalLightDirection;
uniform float farPlane;
uniform float nearPlane;


const float waveStrength = 0.005;				//Strength of the distortions to the textures
const float fresnelWeight = 2.0f;				//Controls reflection and refraction blending ( >1 means more reflective, <1 means more refractive)
const float shineDamper = 25.0;					//Exponent to damp the specular shine
const float noTransparencyDepth = 5.0f;	//The depth at which the water should be rendered fully rather than alpha blending

//Reflectivity of the water (higher means more angles result in specular highlights, 
//lower means camera need to be close to perfect reflection angle for highlights to appear
const float specularReflectivity = 0.9;		

void main()
{
	vec2 normalizedDeviceSpaceCoords = (clipSpaceCoords.xy/clipSpaceCoords.w)/2 + 0.5f;	//Screen space, convert from (-1,1) interval to (0,1)

	vec2 reflectionCoords = vec2(normalizedDeviceSpaceCoords.x, -normalizedDeviceSpaceCoords.y);
	vec2 refractionCoords = vec2(normalizedDeviceSpaceCoords.x, normalizedDeviceSpaceCoords.y);

	//Find depth by looking at the depthMap of the refraction
	float depth = texture2D(depthMap, refractionCoords).r;
	//Convert from depth-buffer values (which are not linear) to real depth
	float waterFloorDepth = 2.0*nearPlane*farPlane/ (farPlane + nearPlane - (2.0 * depth - 1.0) * (farPlane-nearPlane));					//Calculation found online, do not alter unless necessary

	//Find depth of current fragment
	float surfaceDepth = gl_FragCoord.z;
	//Convert from depth-buffer values (which are not linear) to real depth
	float waterSurfaceDepth = 2.0*nearPlane*farPlane/ (farPlane + nearPlane - (2.0 * surfaceDepth - 1.0) * (farPlane-nearPlane));			//Calculation found online, do not alter unless necessary
	//Finally calculate the actual depth
	float waterDepth = waterFloorDepth - waterSurfaceDepth;



	
	//Sample dudv map with moveFactor applied to the texCoords and weight it
	vec2 distortedTexCoords = texture2D(dudvMap, vec2(texCoords.x + moveFactor, texCoords.y)).rg * 0.1;
	//Add the moveFactor to the y-component of the distortedTexCoords to get distortions in both directions
	distortedTexCoords = texCoords + vec2(distortedTexCoords.x, distortedTexCoords.y+moveFactor);
	//Use the distortedTexCoords to once again sample the dudv map for the final distortion and weight by waveStrength
	vec2 totalDistortion = (texture2D(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * waveStrength;
	totalDistortion *= clamp(waterDepth/noTransparencyDepth, 0, 1);													//Reduce distortion on fragments with low depth

	reflectionCoords+= totalDistortion;
	reflectionCoords.x = clamp(reflectionCoords.x, 0.001, 0.999); //Don't let the distortion go outside the texture
	reflectionCoords.y = clamp(reflectionCoords.y, -0.999, -0.001);

	refractionCoords+= totalDistortion;  
	refractionCoords = clamp(refractionCoords, 0.001, 0.999);

	vec4 reflectionColor = texture2D(reflectionTexture, reflectionCoords);
	vec4 refractionColor = texture2D(refractionTexture, refractionCoords);

	//Find the normals using distortedTexCoords since we want the normals to be distorted over time aswell
	vec4 normalMapColor = texture2D(normalMap, distortedTexCoords);

	//Extract the normal vec3.  Transform X and Z values from [0,1] to [-1,1]
	vec3 normal = vec3(normalMapColor.r * 2.0 - 1.0, normalMapColor.b*3, normalMapColor.g * 2.0 - 1.0);			// Red=X, Blue=Y, Green=Z
	normal = normalize(normal);																					//Normalize the normal (sampled values are not normalized in the texture)
	
	//Fresnel effect
	vec3 viewVector = normalize(toCameraVector);												//Normalize vector to camera to get direction
	float refractiveFactor = dot(viewVector, normal);											//Factor for fresnel effect mixing (based on viewing direction in comparison to the fragment normal)
	refractiveFactor = pow(refractiveFactor, fresnelWeight);									//Scale the factor ( >1 means more reflective, <1 more refractive)
	refractiveFactor = clamp(refractiveFactor, 0.001, 0.999);										//Make sure the factor is in the correct range (Could lead to black artifacts otherwise)



	//Calculate specular reflection
	vec3 reflectedLight = reflect(normalize(globalLightDirection), normal);											//Find the reflected vector of the light
	float specular = max(dot(reflectedLight, viewVector), 0.0);													//Calculate the specularity by comparing the direction of the reflectedLight and the view vector
	specular = pow(specular, shineDamper);																		//Damp the shine from "good reflections"
	vec3 specularHighlights = globalLightColor * specular * specularReflectivity;								//Calculate final specular highlight
	specularHighlights *= clamp(waterDepth/noTransparencyDepth, 0, 1);											//Reduce specularity on fragments close to shores (with low depths)

	//Mix reflection and refraction based on the refractiveFactor to simulate the fresnel effect
	fragmentColor = mix(reflectionColor, refractionColor, refractiveFactor);

	//Blend in a bit of bluish-green tint and add the specularHighlights
	fragmentColor = mix(fragmentColor, vec4(0.0, 0.3, 0.5, 1.0), 0.3) + vec4(specularHighlights, 0.0f);

	fragmentColor.a = clamp(waterDepth/noTransparencyDepth, 0, 1);
	//fragmentColor = vec4(waterDepth/50.0);
}
