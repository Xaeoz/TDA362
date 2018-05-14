
#include "heightfield.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <labhelper.h>

using namespace glm;
using std::string;
using std::vector;

HeightField::HeightField(void)
	: m_meshResolution(0)
	, m_vao(UINT32_MAX)
	, m_positionBuffer(UINT32_MAX)
	, m_uvBuffer(UINT32_MAX)
	, m_indexBuffer(UINT32_MAX)
	, m_numIndices(0)
	, m_texid_hf(UINT32_MAX)
	, m_texid_diffuse(UINT32_MAX)
	, m_heightFieldPath("")
	, m_diffuseTexturePath("")
	, m_texid_normal(UINT32_MAX)
	, m_normalBuffer(UINT32_MAX)
	, m_normalPath("")
{
}

void HeightField::loadHeightField(const std::string &heigtFieldPath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	float * data = stbi_loadf(heigtFieldPath.c_str(), &width, &height, &components, 1);
	if (data == nullptr) {
		std::cout << "Failed to load image: " << heigtFieldPath << ".\n";
		return;
	}

	if (m_texid_hf == UINT32_MAX) {
		glGenTextures(1, &m_texid_hf);
	}
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data); // just one component (float)

	m_heightFieldPath = heigtFieldPath;
	std::cout << "Successfully loaded heigh field texture: " << heigtFieldPath << ".\n";
}

void HeightField::loadNormalMap(const std::string &normalPath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	float * data = stbi_loadf(normalPath.c_str(), &width, &height, &components, 3);
	if (data == nullptr) {
		std::cout << "Failed to load image: " << normalPath << ".\n";
		return;
	}

	if (m_texid_normal == UINT32_MAX) {
		glGenTextures(1, &m_texid_normal);
	}
	glBindTexture(GL_TEXTURE_2D, m_texid_normal);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, data); // just one component (float)

	m_normalPath = normalPath;
	std::cout << "Successfully loaded normal map: " << normalPath << ".\n";
}

void HeightField::loadDiffuseTexture(const std::string &diffusePath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	uint8_t * data = stbi_load(diffusePath.c_str(), &width, &height, &components, 3);
	if (data == nullptr) {
		std::cout << "Failed to load image: " << diffusePath << ".\n";
		return;
	}

	if (m_texid_diffuse == UINT32_MAX) {
		glGenTextures(1, &m_texid_diffuse);
	}

	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // plain RGB
	glGenerateMipmap(GL_TEXTURE_2D);

	std::cout << "Successfully loaded diffuse texture: " << diffusePath << ".\n";
}


void HeightField::generateMesh(int tesselation)
{
	// generate a mesh in range -1 to 1 in x and z
	// (y is 0 but will be altered in height field vertex shader)
	int verticesPerRow = sqrt(tesselation / 2) + 1;
	float* verts = NULL;
	verts = new float[verticesPerRow*verticesPerRow*3];
	

	GLuint vertsBuffer;
	GLuint indBuffer;

	//Generate Vertices
	float z = 1;
	int idx = 0;
	for(int j = 0; j < verticesPerRow; j++) {
		float x = -1;
		for (int k = 0; k < verticesPerRow; k++) {
			verts[idx++] = x;
			verts[idx++] = 0;
			verts[idx++] = z;
			x += (2.0f/float(verticesPerRow-1));
			//printf("Vertex: %i,%i \n", j, k);
			//printf("X: %f, \n Y: %f, \n Z: %f \n", verts[idx-3], verts[idx-2], verts[idx-1]);
		}
		//printf("Z Dec before: %f", z);
		z -= (2.0f / float(verticesPerRow-1));
	}

	/*printf("\n VERTICES \n");
	for (int o = 0; o < verticesPerRow*verticesPerRow*3; o++) {
		printf("%f, ", verts[o]);
		if (o % 3 == 2) {
			printf("\n");
		}
	}*/

	//Generate texCoords
	float* texCoords = NULL;
	texCoords = new float[verticesPerRow*verticesPerRow*2];
	float v = 1;
	idx = 0;
	for (int j = 0; j < verticesPerRow; j++) {
		float u = 0;
		for (int k = 0; k < verticesPerRow; k++) {
			texCoords[idx++] = u;
			texCoords[idx++] = v;
			u += (1.0f / float(verticesPerRow - 1));
			//printf("texCoord: %i,%i \n", j, k);
			//printf("u: %f, \n v: %f \n", texCoords[idx - 2], texCoords[idx - 1]);
		}
		//printf("V Dec before: %f \n", v);
		v -= (1.0f / float(verticesPerRow - 1));
	}


	//Generate indices

	int rows = sqrt(tesselation / 2);
	int columns = rows+1;
	int* indices = NULL;
	int indicesAmount = 2 * rows*columns +  rows;
	indices = new int[indicesAmount];
	idx = 0;
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < columns; c++) {
			
			indices[idx++] = (r + 1) * columns + c;
			indices[idx++] = r * columns + c;
			//printf("%i, %i,  ", indices[idx - 2], indices[idx - 1]);
			if (r == rows - 1) {
				indices[idx] = (r + 1) * columns + c - 1;
			}
		}

		if (r < rows - 1) {
			indices[idx++] = 999999999;
			//printf(", %i, ", indices[-1]);
		}
	}

	/*for (int o = 0; o < indicesAmount; o++) {
		printf("%i, ", indices[o]);
		if (o % 3 == 2) {
			printf("\n");
		}
	}*/

	m_numIndices = indicesAmount;

	printf("Vertices Per Row: %i \n", verticesPerRow);
	printf("Number of indices: %i \n", m_numIndices);

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_positionBuffer);													// Create a handle for the vertex position buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), verts, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	glVertexAttribPointer(0, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &m_uvBuffer);													// Create a handle for the vertex position buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 2 * sizeof(float), texCoords, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(2);

	glGenBuffers(1, &m_indexBuffer);													// Create a handle for the vertex position buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndices * sizeof(int), indices, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	glVertexAttribPointer(3, 1, GL_INT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(3);

	//glGenBuffers(1, &m_normalBuffer);													// Create a handle for the vertex position buffer
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_normalBuffer);									// Set the newly created buffer as the current one
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 2 * sizeof(float), texCoords, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	//glVertexAttribPointer(4, 1, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	//glEnableVertexAttribArray(4);


}

void HeightField::submitTriangles(void)
{
	if (m_vao == UINT32_MAX) {
		std::cout << "No vertex array is generated, cannot draw anything.\n";
		return;
	}
	

	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(999999999);
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glActiveTexture(GL_TEXTURE20);
	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
	glActiveTexture(GL_TEXTURE21);
	glBindTexture(GL_TEXTURE_2D, m_texid_normal);


	glDrawElements(GL_TRIANGLE_STRIP, m_numIndices, GL_UNSIGNED_INT, 0);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_PRIMITIVE_RESTART);
}