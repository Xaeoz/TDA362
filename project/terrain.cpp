#include "terrain.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <labhelper.h>

using namespace glm;
using namespace std;
using std::string;
using std::vector;

float sqr5(float x)
{
	return x*x*x;
}

Terrain::Terrain(vec2 originCoord, int tesselation, int meshSimplificationFactor, int chunkSize)
	: tesselation(tesselation)
	, baseTesselation(tesselation)
	, verticesPerRow(0)
	, baseVerticesPerRow(sqrt(baseTesselation / 2) + 1)
	, m_vao(UINT32_MAX)
	, m_positionBuffer(UINT32_MAX)
	, m_uvBuffer(UINT32_MAX)
	, m_indexBuffer(UINT32_MAX)
	, m_numIndices(0)
	, triangleRestartIndex(0)
	, meshSimplificationFactor(meshSimplificationFactor)
	, heightCurve()
	, meshSimplificationFactors()
	, originCoord(originCoord)
	, visible(false)
	, chunkSize(chunkSize)
	, heightMap()
	, verts()

{
	triangleRestartIndex = UINT32_MAX;
	heightCurve = new float[1000];
	for (int i = 0; i < 1000; i++) heightCurve[i] = sqr5(float(i) / 1000);
	meshSimplificationFactors = new int[7]{ 0,1,2,3,4,5,6 };
	setLod(meshSimplificationFactor, chunkSize);
}

void Terrain::setLod(int meshSimplificationFactor, int chunkSize) {
	int factor = meshSimplificationFactors[meshSimplificationFactor] * 4;
	if (factor == 0) factor = 1;
	int tesselation = baseTesselation / factor;
	if (tesselation < baseTesselation / (factor)) tesselation = baseTesselation / (24); //Check so that we dont go to far
	this->verticesPerRow = sqrt(tesselation /2)+1;
	this->tesselation = tesselation;
	this->meshSimplificationFactor = meshSimplificationFactor;
}

void Terrain::generateHeightMap(float * perlinNoise, int perlinNoiseSize)
{
	float vertexDistance = 1.0f / float(verticesPerRow - 1);
	int nrOfVertices = verticesPerRow * verticesPerRow * 3;
	int rowLength = sqrt(perlinNoiseSize);
	delete[] heightMap;
	heightMap = new float[nrOfVertices];
	int yOffset = ((int)originCoord.y * rowLength * (rowLength-(baseVerticesPerRow-1))) % perlinNoiseSize + perlinNoiseSize;
	int xOffset = ((int)originCoord.x * (baseVerticesPerRow-1)) % rowLength + rowLength;
	int verticesToSkip = pow(2, meshSimplificationFactor);


	float z = 1;
	int idx = 0;
	
	for (int j = 0; j < verticesPerRow; j++) {
		float x = 0;
		for (int k = 0; k < verticesPerRow; k++) {
			heightMap[idx++] = x;
			float perlinVal = perlinNoise[((yOffset + (j + j*meshSimplificationFactor)*rowLength) % perlinNoiseSize) + (k + k*meshSimplificationFactor + xOffset) % (rowLength)];
			heightMap[idx++] = perlinVal;
			heightMap[idx++] = z;
			x += vertexDistance;
		}
		z -= vertexDistance;
	}
}

float* Terrain::generateVertices(float * heightMap, float heightMultiplier)
{
	int nrOfVertices = verticesPerRow * verticesPerRow * 3;
	delete[] verts;
	verts = new float[nrOfVertices];
	float vertexDistance = 1.0f / float(verticesPerRow - 1);

	int idx = 0;
	float z = originCoord.y +1.0f;
	for (int j = 0; j < verticesPerRow; j++) {
		float x = originCoord.x;
		for (int k = 0; k < verticesPerRow; k++) {

			verts[idx++] = x*chunkSize;
			float heightCurveVal = heightCurve[(int)(heightMap[idx] * 10)]; //for debugging
			float heightVal = heightMap[idx];
			verts[idx++] = heightMap[idx] * heightMultiplier * sqr5(heightMap[idx]);//heightCurve[(int)(heightMap[idx] * 1000)];
			verts[idx++] = z*chunkSize;
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
	m_numIndices = 2 * rows*columns + rows -1;
	indices = new int[m_numIndices];
	int idx = 0;
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < columns; c++) {

			indices[idx++] = (r + 1) * columns + c;
			indices[idx++] = r * columns + c;

		}

		if (r < rows - 1) {
			indices[idx++] = triangleRestartIndex;
		}
	}

	return indices;
}

//Calculates normals of all triangle surfaces.
float * Terrain::calculateSurfaceNormals(int * indices, float * verts) {

	float * surfaceNormals = new float[tesselation*3];
	int idx = 0;
	int clockWise = 1;
	for (int i = 0; i < m_numIndices - 2; i++) {
		if (indices[i] == triangleRestartIndex) clockWise = (clockWise +1) % 2;
		if (indices[i] == triangleRestartIndex || indices[i + 1] == triangleRestartIndex || indices[i + 2] == triangleRestartIndex) continue;
		int i1 = indices[i];
		int i2 = indices[i + 1];
		int i3 = indices[i + 2];

		vec3 v1 = vec3(verts[i1*3], verts[i1*3 + 1], verts[i1*3 + 2]);
		vec3 v2 = vec3(verts[i2*3], verts[i2*3 + 1], verts[i2*3 + 2]);
		vec3 v3 = vec3(verts[i3*3], verts[i3*3 + 1], verts[i3*3 + 2]);
		
		vec3 vec1 = v2 - v1;
		vec3 vec2 = v3 - v1;
		vec3 normal;
		if (i % 2 == clockWise) {
			normal = normalize(cross(vec2, vec1));
		}
		else {
			normal = normalize(cross(vec1, vec2));
		}

		surfaceNormals[idx++] = normal.x;
		surfaceNormals[idx++] = normal.y;
		surfaceNormals[idx++] = normal.z;
		
	}

	return surfaceNormals;
}

//Calculates vertexNormals based on all the surface normals of all adjacent triangles
float * Terrain::calculateVertexNormals(int * indices, float * surfaceNormals)
{
	float * vertexNormals = new float[verticesPerRow*verticesPerRow * 3];
	for (int i = 0; i < verticesPerRow*verticesPerRow; i++) {
		vec3 normalSum = vec3(0);
		int restartIndexesPassed = 0;
		for (int j = 0; j < m_numIndices; j++) {
			int test = indices[j];
			if (indices[j] == triangleRestartIndex) restartIndexesPassed++;
			if (indices[j] == i) {
				for (int k = glm::max(0, j - 2); k <= j; k++) {
					if(!(indices[k] == triangleRestartIndex || indices[k+1] == triangleRestartIndex || indices[k+2] == triangleRestartIndex) && k < m_numIndices - 2)
						normalSum += vec3(surfaceNormals[(k - restartIndexesPassed*3)*3], surfaceNormals[(k - restartIndexesPassed*3)*3 + 1], surfaceNormals[(k - restartIndexesPassed*3)*3 + 2]);
				}
			}
		}
		vec3 normal = normalize(normalSum);
		vertexNormals[i * 3] = normal.x;
		vertexNormals[i * 3 + 1] = normal.y;
		vertexNormals[i * 3 + 2] = normal.z;
	}
	return vertexNormals;
}

void Terrain::initTerrain(float heightMultiplier, float * perlinNoise, int perlinNoiseSize, int chunkSize) {

	int * indices = generateIndices();
	generateHeightMap(perlinNoise, perlinNoiseSize);
	generateVertices(heightMap, heightMultiplier);
	float * tileTexCoords = generateTileTexCoords(16);
	float * surfaceNormals = calculateSurfaceNormals(indices, verts);
	float * vertexNormals = calculateVertexNormals(indices, surfaceNormals);

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_heightBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_heightBuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), heightMap, GL_STATIC_DRAW);
	glVertexAttribPointer(5, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(5);

	glGenBuffers(1, &m_positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), verts, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &m_indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndices * sizeof(int), indices, GL_STATIC_DRAW);

	glGenBuffers(1, &m_uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 2 * sizeof(float), tileTexCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(2);

	glGenBuffers(1, &m_vertNormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertNormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), vertexNormals, GL_STATIC_DRAW);
	glVertexAttribPointer(4, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(4);

}

void Terrain::updateTerrain(float heightMultiplier, float * perlinNoise, int perlinNoiseSize, int meshSimplificationFactor, bool updateLod)
{
	generateHeightMap(perlinNoise, perlinNoiseSize);
	generateVertices(heightMap, heightMultiplier);
	int * indices = generateIndices();
	float * tileTexCoords = generateTileTexCoords(16);
	float * surfaceNormals = calculateSurfaceNormals(indices, verts);
	float * vertexNormals = calculateVertexNormals(indices, surfaceNormals);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_heightBuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), heightMap, GL_STATIC_DRAW);
	glVertexAttribPointer(5, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);

	glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), verts, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndices * sizeof(int), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(3, 1, GL_INT, false/*normalized*/, 0/*stride*/, 0/*offset*/);

	glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 2 * sizeof(float), tileTexCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);	

	glBindBuffer(GL_ARRAY_BUFFER, m_vertNormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), vertexNormals, GL_STATIC_DRAW);
	glVertexAttribPointer(4, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);


}