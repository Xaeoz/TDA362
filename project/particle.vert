#version 400 compatibility
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec4 positionSize;
//vec2 with {age, maxAge} (Cannot be uniform since it varie between particles
layout(location = 2) in vec2 ageMaxAge;



uniform mat4 viewMatrix;
uniform mat4 viewProjectionMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform int atlasRows;


out float life;
out vec2 texCoords;
out vec2 texCoordsNext;
out float blend;
out float depth;
out vec4 clipSpaceCoords;

void main()
{
	vec3 cameraRight = vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
	vec3 cameraUp = vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

	vec3 positionWorldSpace = positionSize.xyz + cameraRight * vPosition.x * positionSize.w
									+ cameraUp * vPosition.y * positionSize.w;
	
	life = clamp(ageMaxAge.x/ageMaxAge.y, 0.001f, 0.999f);
	vec2 scaledTexCoords = vPosition.xy + vec2(0.5);	//Positions are [-0.5, 0.5] texCoords should be [0,1]
	scaledTexCoords.y = 1.0f-scaledTexCoords.y;			//Texture coordinates are inversed in y-direction
	
	float atlasTotalStep = atlasRows*atlasRows*life;
	int atlasStep = int(atlasTotalStep);
	float coordStepSize = 1.0f/atlasRows;

	float xOffset = int(atlasStep%atlasRows)*coordStepSize;
	float yOffset = 1.0f-int(atlasStep/atlasRows)*coordStepSize;

	int atlasStepNext = atlasStep + 1;

	float xOffsetNext = int(atlasStepNext%atlasRows)*coordStepSize;
	float yOffsetNext = 1.0f-int(atlasStepNext/atlasRows)*coordStepSize;

	vec2 offset = vec2(xOffset, yOffset);
	vec2 offsetToNext = vec2(xOffsetNext, yOffsetNext);

	//Output to fragment shader
	texCoords = offset+ scaledTexCoords/atlasRows;
	texCoordsNext = offsetToNext + scaledTexCoords/atlasRows;

	blend = atlasTotalStep-atlasStep;
	//Calculate the coordinates of this vertex in clipSpace (similar/same as screen space, just not normalized)
	clipSpaceCoords = viewProjectionMatrix * vec4(positionWorldSpace, 1.0);
	gl_Position = clipSpaceCoords;


	
}

/*
    life = particle.w;
    // Particle is already in view space.    
    vec4 particle_vs = vec4(particle.xyz, 1.0);
    // Calculate one projected corner of a quad at the particles view space depth.
    vec4 proj_quad    = P  * vec4(1.0, 1.0, particle_vs.z, particle_vs.w);
    // Calculate the projected pixel size.
    vec2 proj_pixel   = vec2(screen_x, screen_y) * proj_quad.xy / proj_quad.w;
    // Use scale factor as sum of x and y sizes.
    float scale_factor = (proj_pixel.x+proj_pixel.y);
    // Transform position.
    gl_Position  = P * particle_vs;
    // Scale the point with regard to the previosly defined scale_factor
    // and the life (it will get larger the older it is)
    gl_PointSize = scale_factor * mix(0.0, 5.0, pow(life, 1.0/4.0));
*/