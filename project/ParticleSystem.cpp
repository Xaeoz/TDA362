#include "ParticleSystem.h"
#include <glm/gtx/transform.hpp>

using namespace glm;



ParticleSystem::ParticleSystem(int size, vec3 position)
	:
	m_max_size(size)
{
	m_particles.reserve(size);
	m_modelMatrix = translate(position);
	//Initialize system
	init();
}

void ParticleSystem::kill(int id) {}

void ParticleSystem::process_particles(float dt) {
	float* particle_postion_size_data = new float[m_max_size * 4];
	float* particle_postion_color_data = new float[m_max_size * 4];

	/*
	//Spawn new particles if the system is not at max size
	if (m_particles.size() < m_max_size)
	{
		Particle newParticle = { 0, 100, 10, vec3(0), vec3(0) };
		spawn(newParticle);
	}*/

	//update all current particles, removing those who are dead
	for(auto it = m_particles.begin(); it!= m_particles.end();)
	{
		const Particle test = *it;
		const bool shouldBeKilled = !updateParticle(&(*it), dt);
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
			particle_postion_size_data[index] = p->position.x;
			particle_postion_size_data[index+1] = p->position.y;
			particle_postion_size_data[index+2] = p->position.z;
			particle_postion_size_data[index+3] = p->size;

			//Probably going to use a texture atlas instead
			particle_postion_color_data[index] = 1.0f;
			particle_postion_color_data[index+1] = 1.0f;
			particle_postion_color_data[index+2] = 1.0f;
			particle_postion_color_data[index+3] = 1.0f;
			++it;
		}

	}

	glBindBuffer(GL_ARRAY_BUFFER, m_particles_position_size_buffer);
	glBufferData(GL_ARRAY_BUFFER, m_max_size * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
	glBufferSubData(GL_ARRAY_BUFFER, 0, m_particles.size() * sizeof(GLfloat) * 4, particle_postion_size_data);

	glBindBuffer(GL_ARRAY_BUFFER, m_particles_color_buffer);
	glBufferData(GL_ARRAY_BUFFER, m_max_size * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
	glBufferSubData(GL_ARRAY_BUFFER, 0, m_particles.size() * sizeof(GLubyte) * 4, particle_postion_color_data);
	

}

bool ParticleSystem::updateParticle(Particle* particle, float dt) {
	particle->lifetime += dt;
	particle->position = particle->position + particle->velocity*dt;
	
	return particle->lifetime < particle->life_length;			//should particle be alive or not?
}

void ParticleSystem::spawn(Particle particle)
{
	m_particles.push_back(particle);
}


void ParticleSystem::init()
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
	glGenBuffers(1, &m_particles_color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_particles_color_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, m_max_size * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

	Particle newParticle = { 0, 100, 10, vec3(0,1,0), vec3(0) };
	spawn(newParticle);
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

	glBindBuffer(GL_ARRAY_BUFFER, m_particles_color_buffer);
	glVertexAttribPointer(2, 4, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(2);


	glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertice
	glVertexAttribDivisor(1, 1); // positions : one per quad (its center) 
	glVertexAttribDivisor(2, 1); // color : one per quad



	//Render the quads
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_particles.size());

}

