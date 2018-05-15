#pragma once
#include <string>
#include <GL/glew.h>
#include "fbo.h"
#include <glm/glm.hpp>


class Water {
public:
	GLuint m_texid_dudv;
	GLuint m_vao;
	GLuint m_positionBuffer;
	GLuint m_indexBuffer;
	FboInfo m_reflectionFbo;
	FboInfo m_refractionFbo;
	glm::mat4 m_modelMatrix;
	float m_moveFactor;

#define WAVE_SPEED 0.001f

	Water(void);

	void init(glm::vec3 &position, float scale, glm::vec2 &reflectionMapRes, glm::vec2 &refractionMapRes);
	// generate mesh
	void generateMesh(void);

	// render height map
	void render(void);

};
