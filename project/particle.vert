#version 400 compatibility
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec4 positionSize;
layout(location = 2) in vec4 color;

uniform mat4 viewProjectionMatrix;
uniform float screen_x;
uniform float screen_y;
out float life;

void main()
{
	vec3 position = vPosition + positionSize.xyz;
	gl_Position = viewProjectionMatrix * vec4(position.xyz, 1);


	
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