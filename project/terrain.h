#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <GL/glew.h>
#include "HeightGenerator.h"

using namespace glm;
class Terrain {
public:
	int baseTesselation;
	int tesselation;
	int baseVerticesPerRow;
	int verticesPerRow;
	int triangleRestartIndex;
	int meshSimplificationFactor;
	int * meshSimplificationFactors;
	int chunkSize;
	float * heightCurve;
	float * heightMap; 
	bool visible;
	vec2 originCoord;
	HeightGenerator generator;
	GLuint m_vao;
	GLuint m_positionBuffer;
	GLuint m_uvBuffer;
	GLuint m_indexBuffer;
	GLuint m_numIndices;
	GLuint m_vertNormalBuffer;
	GLuint m_heightBuffer;


	Terrain(vec2 originCoord, int tesselation, int meshSimplificationFactor, int chunkSize);

	void setLod(int meshSimplificationFactor, int chunkSize);

	float * generateHeightMap(float * perlinNoise, int perlinNoiseSize);

	float * generateVertices(float * heightMap, float heightMultiplier);

	float * generateTileTexCoords(int nrOfTiles);

	int * generateIndices(void);

	float * calculateSurfaceNormals(int * indices, float * verts);

	float * calculateVertexNormals(int * indices, float * surfaceNormals);
	
	void initTerrain(float heightMultiplier, float * perlinNoise, int perlinNoiseSize, int chunkSize);

	void updateTerrain(float heightMultiplier, float * perlinNoise, int perlinNoiseSize, int meshSimplificationFactor = 0, bool updateLod = false);

};
