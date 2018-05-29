#pragma once
#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>

struct Particle {
	float lifetime;
	float life_length;
	float size;
	float cameraDistance;
	glm::vec3 velocity;
	glm::vec3 position;
	bool operator<(Particle& that) {
		// Sort in reverse order : far particles drawn first.
		return this->cameraDistance > that.cameraDistance;
	}
};

class ParticleSystem {
 public:
	// Members
	std::vector<Particle> m_particles;
	glm::vec3 m_system_position;			
	glm::vec3 m_system_offset;				//Used in update to "follow" a parent
	int m_max_size;
	GLuint m_vertex_buffer;
	GLuint m_vao;
	GLuint m_particles_position_size_buffer;
	GLuint m_particles_age_buffer;
	GLuint m_particle_atlas;
	float m_spawnTime;	//time between each spawn
	int m_atlasRows;
	


	// Ctor/Dtor
	ParticleSystem() : m_max_size(0) {};
	explicit ParticleSystem(int size, glm::vec3 position, glm::vec3 offset, float spawnTime, std::string textureAtlasPath, int atlasRows);
	~ParticleSystem() = default;

	// Methods
	void update(glm::mat4x4);
	void kill(int id);
	void spawn(Particle particle);
	void init(std::string textureAtlasPath, int atlasRows);
	void render();
	void process_particles(float dt, glm::vec3 cameraPosition, glm::vec3 (*velocityCalculator)(glm::mat4, float, float), glm::mat4 arg0, float arg1, float arg2);
	bool updateParticle(Particle * particle, float dt, glm::vec3 cameraPosition);

private:
	float timeSinceLastSpawn = 0;
};
