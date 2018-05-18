#pragma once
#include <vector>
#include <glm/glm.hpp>
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
	GLuint m_texid_diffuse;
	GLuint m_vertNormalBuffer;

	std::string diffuseMapPath;


	Terrain(int tesselation, HeightGenerator generator);

	float * generateVertices(int octaves, float scalingBias);


	float * generateTileTexCoords(int nrOfTiles);

	int * generateIndices(void);

	float * calculateSurfaceNormals(int * indices, float * verts);

	float * calculateVertexNormals(int * indices, float * surfaceNormals);

	void updateTerrain(int octaves, float scalingBias);

	void initTerrain(void);

	void submitTriangles(void);

	void loadDiffuseTexture(const std::string & diffusePath);

	std::vector<glm::vec3> * createVectorArray(float * normals);

};
