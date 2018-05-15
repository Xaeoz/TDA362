#include "terrain.h"
#include "HeightGenerator.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <labhelper.h>
#include <random>

using namespace glm;
using std::string;
using std::vector;

Terrain::Terrain(int tesselation, HeightGenerator generator)
	: verticesPerRow(0)
	, tesselation(tesselation)
	, generator(generator)
	, m_vao(UINT32_MAX)
	, m_positionBuffer(UINT32_MAX)
	, m_uvBuffer(UINT32_MAX)
	, m_indexBuffer(UINT32_MAX)
	, m_numIndices(0)
	, triangleRestartIndex(0)
{
	verticesPerRow = (sqrt(tesselation / 2) + 1);
	triangleRestartIndex = 9999999;
}

float* Terrain::generateVertices(void)
{
	float* verts = NULL;
	float vertexDistance = 2.0f / float(verticesPerRow - 1);
	verts = new float[verticesPerRow*verticesPerRow * 3];

	float z = 2;
	int idx = 0;
	for (int j = 0; j < verticesPerRow; j++) {
		float x = 0;
		for (int k = 0; k < verticesPerRow; k++) {
			verts[idx++] = x;
			verts[idx++] = generator.generateHeight(x/64.0f, z/64.0f, vertexDistance);
			verts[idx++] = z;
			x += vertexDistance;
		}
		z -= vertexDistance;
	}
	return verts;
}

float* Terrain::generateTileTexCoords(int nrOfTiles) {
	//Generate tile texcoords
	float* tileTexCoords = NULL;
	tileTexCoords = new float[verticesPerRow*verticesPerRow * 2];
	float vt = 1.0f * nrOfTiles;
	int idx = 0;
	for (int jt = 0; jt < verticesPerRow; jt++) {
		float ut = 0;
		for (int kt = 0; kt < verticesPerRow; kt++) {
			tileTexCoords[idx++] = ut;
			tileTexCoords[idx++] = vt;
			ut += (1.0f * nrOfTiles / float(verticesPerRow - 1));
		}
		vt -= (1.0f * nrOfTiles / float(verticesPerRow - 1));
	}
	return tileTexCoords;
}

int* Terrain::generateIndices(void) {
	int rows = sqrt(tesselation / 2);
	int columns = rows + 1;
	int* indices = NULL;
	m_numIndices = 2 * rows*columns + rows;
	indices = new int[m_numIndices];
	int idx = 0;
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < columns; c++) {

			indices[idx++] = (r + 1) * columns + c;
			indices[idx++] = r * columns + c;
			if (r == rows - 1) {
				indices[idx] = (r + 1) * columns + c - 1;
			}
		}

		if (r < rows - 1) {
			indices[idx++] = triangleRestartIndex;
		}
	}

	return indices;
}

void Terrain::initTerrain(void) {
	int * indices = generateIndices();
	float * verts = generateVertices();
	float * tileTexCoords = generateTileTexCoords(16);

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
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 2 * sizeof(float), tileTexCoords, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(2);

	glGenBuffers(1, &m_indexBuffer);													// Create a handle for the vertex position buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndices * sizeof(int), indices, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	//glVertexAttribPointer(3, 1, GL_INT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	//glEnableVertexAttribArray(3);
}

void Terrain::submitTriangles(void)
{
	if (m_vao == UINT32_MAX) {
		std::cout << "No vertex array is generated, cannot draw anything.\n";
		return;
	}
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(triangleRestartIndex);
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	/*glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glActiveTexture(GL_TEXTURE20);
	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
	glActiveTexture(GL_TEXTURE21);
	glBindTexture(GL_TEXTURE_2D, m_texid_normal);*/

	glDrawElements(GL_TRIANGLE_STRIP, m_numIndices, GL_UNSIGNED_INT, 0);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_PRIMITIVE_RESTART);
}