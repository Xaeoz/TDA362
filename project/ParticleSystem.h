#pragma once
#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>

struct Particle {
	float lifetime;
	float life_length;
	float size;
	glm::vec3 velocity;
	glm::vec3 position;
};

class ParticleSystem {
 public:
	// Members
	std::vector<Particle> m_particles;
	glm::mat4x4 m_modelMatrix;
	int m_max_size;
	GLuint m_vertex_buffer;
	GLuint m_vao;
	GLuint m_particles_position_size_buffer;
	GLuint m_particles_color_buffer;


	// Ctor/Dtor
	ParticleSystem() : m_max_size(0) {};
	explicit ParticleSystem(int size, glm::vec3 position);
	~ParticleSystem() = default;

	// Methods
	void kill(int id);
	void spawn(Particle particle);
	void init();
	void render();
	void process_particles(float dt);
	bool updateParticle(Particle* particle, float dt);
};
