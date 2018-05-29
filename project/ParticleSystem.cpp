#include "ParticleSystem.h"
#include <glm/gtx/transform.hpp>
#include <algorithm>
#include <stb_image.h>
#include <iostream>
#include <glm/detail/_vectorize.hpp>
#include <glm/detail/_vectorize.hpp>
#include <glm/detail/_vectorize.hpp>

using namespace glm;



ParticleSystem::ParticleSystem(int size, vec3 position, vec3 offset, float spawnTime, std::string textureAtlasPath, int atlasRows)
	:
	m_max_size(size),
	m_spawnTime(spawnTime),
	m_particle_atlas(UINT32_MAX)
{
	m_particles.reserve(size);
	m_system_position = position;
	m_system_offset = offset;
	//Initialize system
	init(textureAtlasPath, atlasRows);
}

void ParticleSystem::update(mat4 parentModelMatrix)
{
	mat4 newPos = parentModelMatrix * translate(m_system_offset);
	m_system_position = vec3(newPos[3].x, newPos[3].y, newPos[3].z);
}

void ParticleSystem::kill(int id) {}

void ParticleSystem::process_particles(float dt, vec3 cameraPosition, vec3 (*velocityCalculator)(mat4,float,float), mat4 arg0, float arg1, float arg2) {
	float* particle_postion_size_data = new float[m_max_size * 4];
	float* particle_age_data = new float[m_max_size * 2];

	
	//Spawn new particles, m_spawnTime = 0 is considered as a flag to NOT spawn
	if (timeSinceLastSpawn > m_spawnTime && m_spawnTime != 0)
	{
		float exceededTime = timeSinceLastSpawn - m_spawnTime;
		int particlesToSpawn = (exceededTime / m_spawnTime) + 1;			//High spawn rates require multiple spawns per frame
		particlesToSpawn = 1;
		//Spawn as many particles as needed (not exceeding max size)
		while (particlesToSpawn-- > 0 && m_particles.size() < m_max_size) {
			Particle newParticle = { 0.2f, 1.0f, 5, 0, velocityCalculator(arg0, arg1, arg2), m_system_position };
			spawn(newParticle);
		}
		timeSinceLastSpawn = 0;
	}


	//update all current particles, removing those who are dead
	for(auto it = m_particles.begin(); it!= m_particles.end();)
	{
		const Particle test = *it;
		const bool shouldBeKilled = !updateParticle(&(*it), dt, cameraPosition);
		if (shouldBeKilled)
			it = m_particles.erase(it);
		else
		{
			//Find current index, should work even if deletions happen during the loop
			int index = it - m_particles.begin();
			//Dereference iterator for clearer code
			Particle *p = &*it;
			//Fill buffers
			//position buffer
			particle_postion_size_data[index*4] = p->position.x;
			particle_postion_size_data[index*4+1] = p->position.y;
			particle_postion_size_data[index*4+2] = p->position.z;
			particle_postion_size_data[index*4+3] = p->size;

			//Add age data to use with texture atlas and blending
			particle_age_data[index*2] = p->lifetime;
			particle_age_data[index*2+1] = p->life_length;

			//advance iterator to next item (this is not just ++ on pointer, but operator implemented in iterator)
			++it;
		}

	}

	glBindBuffer(GL_ARRAY_BUFFER, m_particles_position_size_buffer);
	glBufferData(GL_ARRAY_BUFFER, m_max_size * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
	glBufferSubData(GL_ARRAY_BUFFER, 0, m_particles.size() * sizeof(GLfloat) * 4, particle_postion_size_data);

	glBindBuffer(GL_ARRAY_BUFFER, m_particles_age_buffer);
	glBufferData(GL_ARRAY_BUFFER, m_max_size * 2 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
	glBufferSubData(GL_ARRAY_BUFFER, 0, m_particles.size() * sizeof(GLfloat) * 2, particle_age_data);

	timeSinceLastSpawn += dt;
	//delayed sorting (1-frame delay), not perfect, but better perofrmance overall
	std::sort(m_particles.begin(), m_particles.end());

}

bool ParticleSystem::updateParticle(Particle* particle, float dt, vec3 cameraPosition) {
	//update current lifetime
	particle->lifetime += dt;

	//Update position based on velocity
	particle->position = particle->position + particle->velocity*dt;

	//Could update velocity based on for instance gravity
	//NOT IMPLEMENTED

	//Calculate distance to camera for sorting
	particle->cameraDistance = length(particle->position - cameraPosition);

	//should particle be alive or not?
	return particle->lifetime < particle->life_length;			
}

void ParticleSystem::spawn(Particle particle)
{
	m_particles.push_back(particle);
}


void ParticleSystem::init(std::string textureAtlasPath, int atlasRows)
{

	const float vertex_buffer_data[] =
	{
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
	};
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

	// The VBO containing the positions and sizes of the particles
	glGenBuffers(1, &m_particles_position_size_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_particles_position_size_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, m_max_size * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	// The VBO containing the colors of the particles
	glGenBuffers(1, &m_particles_age_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_particles_age_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, m_max_size * 2 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	int width, height, components;
	//Load in normalMap data from file
	float *atlasData = stbi_loadf(textureAtlasPath.c_str(), &width, &height, &components, 4);
	if (atlasData == nullptr) {
		std::cout << "Failed to load image: " << &textureAtlasPath << ".\n";
		return;		//Not handled, this will cause the water component to break when used
	}

	//Get a GLuint reference for the texture if one is not already present
	if (m_particle_atlas == UINT32_MAX) {
		glGenTextures(1, &m_particle_atlas);
	}

	//Set texture to modify
	glBindTexture(GL_TEXTURE_2D, m_particle_atlas);
	//Repeat to allow distortions and such to loop around the texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	//Not a visible texture per say, so linear works well enough
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, atlasData);
	m_atlasRows = atlasRows;

}


void ParticleSystem::render()
{
	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, m_particles_position_size_buffer);
	glVertexAttribPointer(1, 4, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, m_particles_age_buffer);
	glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(2);


	glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertice
	glVertexAttribDivisor(1, 1); // positions : one per quad (its center) 
	glVertexAttribDivisor(2, 1); // color : one per quad

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_particle_atlas);

	//Render the quads
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_particles.size());

}

