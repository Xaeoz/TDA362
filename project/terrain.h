#pragma once
#include <string>
#include <GL/glew.h>
#include "HeightGenerator.h"

class Terrain {
public:
	int verticesPerRow;
	int tesselation;
	int triangleRestartIndex;
	HeightGenerator generator;
	GLuint m_vao;
	GLuint m_positionBuffer;
	GLuint m_uvBuffer;
	GLuint m_indexBuffer;
	GLuint m_numIndices;


	Terrain(int tesselation, HeightGenerator generator);

	float * generateVertices(void);


	float * generateTileTexCoords(int nrOfTiles);

	int * generateIndices(void);

	void initTerrain(void);

	void submitTriangles(void);

};
