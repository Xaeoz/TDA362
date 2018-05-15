#include <vector>

#include "Water.h"
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

using namespace glm;
using std::string;
using std::vector;

Water::Water()
	:
	m_texid_dudv(UINT32_MAX)
	, m_vao(UINT32_MAX)
	, m_positionBuffer(UINT32_MAX)
	, m_indexBuffer(UINT32_MAX)
	, m_modelMatrix()
	, m_moveFactor(0)
{
}

void Water::init(vec3 &position, float scale, vec2 &reflectionMapRes, vec2 &refractionMapRes)
{
	m_modelMatrix = glm::scale(vec3(scale, 1, scale))*translate(position);
	//Initialize Fbo objects
	m_reflectionFbo.resize(reflectionMapRes.x, reflectionMapRes.y);
	m_refractionFbo.resize(refractionMapRes.x, refractionMapRes.y);
	//Create mesh and corresponding buffers
	generateMesh();

	//Load textures
	int width, height, components;
	std::string dudvPath = "../res/waterDUDV.png";
	float *data = stbi_loadf(dudvPath.c_str(), &width, &height, &components, 1);
	if (data == nullptr) {
		std::cout << "Failed to load image: " << dudvPath << ".\n";
		return;
	}
	if (m_texid_dudv == UINT32_MAX) {
		glGenTextures(1, &m_texid_dudv);
	}
	glBindTexture(GL_TEXTURE_2D, m_texid_dudv);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data);
}


////////////////////////////////////////////
//	Generate the mesh through indices and vertices. Also generate bindings for the VAO and buffers
//////////////////////////////////////////
void Water::generateMesh()
{
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	//const float magnitude = 100;

	float verts[] = { -1, 0, 1,	//v0
		1, 0, 1, 	//v1
		1, 0, -1, 	//v2 
		-1, 0, -1,	//v3
	};

	//for (int i = 0; i < sizeof(verts) / sizeof(float); i++)
	//{
	//	verts[i] = verts[i] * magnitude;
	//}

	glGenBuffers(1, &m_positionBuffer);													// Create a handle for the vertex position buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);				// Send the vetex position data to the current buffer
	glVertexAttribPointer(0, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(0);

	const int indices[] = { 0,1,3, 3,1,2 };

	glGenBuffers(1, &m_indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}


void Water::render()
{
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_reflectionFbo.colorTextureTargets[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_refractionFbo.colorTextureTargets[0]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_texid_dudv);

}
