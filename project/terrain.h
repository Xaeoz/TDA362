#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <GL/glew.h>
#include "HeightGenerator.h"

using namespace glm;
class Terrain {
public:
	int verticesPerRow;
	int tesselation;
	int triangleRestartIndex;
	int meshSimplificationFactor;
	float * heightCurve;
	int * meshSimplificationFactors;
	int baseTesselation;
	bool visible;
	bool seen;
	vec2 originCoord;
	HeightGenerator generator;
	GLuint m_vao;
	GLuint m_positionBuffer;
	GLuint m_uvBuffer;
	GLuint m_indexBuffer;
	GLuint m_numIndices;
	GLuint m_texid_diffuse;
	GLuint m_vertNormalBuffer;
	GLuint m_heightBuffer;
	std::string diffuseMapPath;


	Terrain(vec2 originCoord, int tesselation, int meshSimplificationFactor, int chunkSize);

	void setLod(int tesselation, int meshSimplificationFactor, int chunkSize);

	void decreaseLod(void);

	void increaseLod(void);

	float * generateHeightMap(float * perlinNoise, int perlinNoiseSize);

	float * generateVertices(float * heightMap, float heightMultiplier);

	float * generateTileTexCoords(int nrOfTiles);

	int * generateIndices(void);

	float * calculateSurfaceNormals(int * indices, float * verts);

	float * calculateVertexNormals(int * indices, float * surfaceNormals);

	void updateTerrain(float heightMultiplier, float * perlinNoise, int perlinNoiseSize);

	void initTerrain(float heightMultiplier, float * perlinNoise, int perlinNoiseSize, int chunkSize);

	void submitTriangles(void);

	void loadDiffuseTexture(const std::string & diffusePath);

	std::vector<glm::vec3> * createVectorArray(float * normals);



};
