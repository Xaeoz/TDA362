#include "terrain.h"
#include "HeightGenerator.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <labhelper.h>

using namespace glm;
using std::string;
using std::vector;

Terrain::Terrain(vec2 originCoord, int tesselation, int meshSimplificationFactor, int chunkSize)
	: verticesPerRow(0)
	, tesselation(tesselation)
	, m_vao(UINT32_MAX)
	, m_positionBuffer(UINT32_MAX)
	, m_uvBuffer(UINT32_MAX)
	, m_indexBuffer(UINT32_MAX)
	, m_numIndices(0)
	, triangleRestartIndex(0)
	, m_texid_diffuse(UINT32_MAX)
	, diffuseMapPath("")
	, meshSimplificationFactor(0)
	, heightCurve()
	, meshSimplificationFactors()
	, baseTesselation(tesselation)
	, generator(tesselation)
	, originCoord(originCoord)
	, visible(false)
	, seen(false)
{

	triangleRestartIndex = 9999999;
	heightCurve = new float[11]{ .0f,  .0f,  .0f,  .1f, .3f, .4f, .5f, .6f, .8f, .9f, 1.0f };
	meshSimplificationFactors = new int[7]{ 0,1,2,3,4,5,6 };
	setLod(baseTesselation, meshSimplificationFactor, chunkSize);
}

void Terrain::setLod(int tesselation, int meshSimplificationFactor, int chunkSize) {
	int factor = meshSimplificationFactors[meshSimplificationFactor] * 4;
	if (factor == 0) factor = 1;
	tesselation = tesselation / factor;
	if (tesselation < baseTesselation / (factor)) tesselation = baseTesselation / (24); //Check so that we dont go to far
	verticesPerRow = sqrt(tesselation/2)+1;
}

void Terrain::decreaseLod(void)
{
	int factor = meshSimplificationFactors[meshSimplificationFactor] * 4;
	if (factor == 0) factor = 1;
	tesselation = tesselation / factor;
	if (tesselation < baseTesselation/(factor)) tesselation = baseTesselation / (24); //Check so that we dont go to far
	verticesPerRow = (sqrt((tesselation) / 2) + 1); 

}

void Terrain::increaseLod(void)
{
	int factor = meshSimplificationFactors[meshSimplificationFactor] * 4;
	if (factor == 0) {
		tesselation *= 1;
	}
	else {
		tesselation = tesselation * factor;
	}
	if (tesselation > baseTesselation) tesselation = baseTesselation; //Check so that we dont go to far
	verticesPerRow = (sqrt((tesselation) / 2) + 1);

}

float* Terrain::generateHeightMap(float * perlinNoise, int perlinNoiseSize)
{
	float* heightMap = NULL;
	float vertexDistance = 1.0f / float(verticesPerRow - 1);
	int nrOfVertices = verticesPerRow * verticesPerRow * 3;
	int rowLength = sqrt(perlinNoiseSize);
	heightMap = new float[nrOfVertices];
	int yOffset = ((int)originCoord.y * rowLength * (rowLength-(verticesPerRow-1))) % perlinNoiseSize + perlinNoiseSize;
	int xOffset = ((int)originCoord.x * (verticesPerRow-1)) % rowLength + rowLength;


	float z = 1;
	int idx = 0;
	
	for (int j = 0; j < verticesPerRow; j++) {
		float x = 0;
		for (int k = 0; k < verticesPerRow; k++) {
			heightMap[idx++] = x;
			int temp = ((yOffset + j*rowLength) % perlinNoiseSize) + (k + xOffset) % (rowLength);
			heightMap[idx++] = perlinNoise[((yOffset + j*rowLength) % perlinNoiseSize) + (k + xOffset) % (rowLength)];
			heightMap[idx++] = z;
			x += vertexDistance;
		}
		z -= vertexDistance;
	}
	return heightMap;
}

float* Terrain::generateVertices(float * heightMap, float heightMultiplier)
{
	int nrOfVertices = verticesPerRow * verticesPerRow * 3;
	float* verts = new float[nrOfVertices];
	float vertexDistance = 1.0f / float(verticesPerRow - 1);

	int idx = 0;
	float z = originCoord.y +1.0f;
	for (int j = 0; j < verticesPerRow; j++) {
		float x = originCoord.x;
		for (int k = 0; k < verticesPerRow; k++) {

			verts[idx++] = x;
			float heightCurveVal = heightCurve[(int)(heightMap[idx] * 10)]; //for debugging
			float heightVal = heightMap[idx];
			verts[idx++] = heightMap[idx] * heightMultiplier * heightCurve[(int)(heightMap[idx]*10)];
			verts[idx++] = z;
			x += vertexDistance;
		}
		z -= vertexDistance;
	}
	return verts;
}

float* Terrain::generateTileTexCoords(int nrOfTiles) {
	//Generate tile texcoordss
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
				for (int k = max(0, j - 2); k <= j; k++) {
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

void Terrain::updateTerrain(float heightMultiplier, float * perlinNoise, int perlinNoiseSize )
{
	float * heightMap = generateHeightMap(perlinNoise, perlinNoiseSize);
	float * verts = generateVertices(heightMap, heightMultiplier);
	int * indices = generateIndices();
	float * tileTexCoords = generateTileTexCoords(16);
	float * surfaceNormals = calculateSurfaceNormals(indices, verts);
	float * vertexNormals = calculateVertexNormals(indices, surfaceNormals);

	printf("Updated terrain \n");
	glBindVertexArray(m_vao);


	glBindBuffer(GL_ARRAY_BUFFER, m_heightBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), heightMap, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	glVertexAttribPointer(5, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);

	// Create a handle for the vertex position buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), verts, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	glVertexAttribPointer(0, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
											// Create a handle for the vertex position buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 2 * sizeof(float), tileTexCoords, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);	

	glBindBuffer(GL_ARRAY_BUFFER, m_vertNormalBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), vertexNormals, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	glVertexAttribPointer(4, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndices * sizeof(int), indices, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
																									//glVertexAttribPointer(3, 1, GL_INT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
																									//glEnableVertexAttribArray(3);

}

void Terrain::initTerrain(float heightMultiplier, float * perlinNoise, int perlinNoiseSize, int chunkSize) {

	int * indices = generateIndices();
	float * heightMap = generateHeightMap(perlinNoise, perlinNoiseSize);
	float * verts = generateVertices(heightMap, heightMultiplier);
	float * tileTexCoords = generateTileTexCoords(16);
	float * surfaceNormals = calculateSurfaceNormals(indices, verts);
	float * vertexNormals = calculateVertexNormals(indices, surfaceNormals);

	//printf("Vertices Per Row: %i \n", verticesPerRow);
	//printf("Number of indices: %i \n", m_numIndices);

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_heightBuffer);													// Create a handle for the vertex position buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_heightBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), heightMap, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	glVertexAttribPointer(5, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(5);

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

	glGenBuffers(1, &m_vertNormalBuffer);													// Create a handle for the vertex position buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_vertNormalBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, verticesPerRow*verticesPerRow * 3 * sizeof(float), vertexNormals, GL_STATIC_DRAW);		// Send the vetex position data to the current buffer
	glVertexAttribPointer(4, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(4);

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
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);*/
	/*glActiveTexture(GL_TEXTURE20);
	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);*/
	/*glActiveTexture(GL_TEXTURE21);
	glBindTexture(GL_TEXTURE_2D, m_texid_normal);*/

	glDrawElements(GL_TRIANGLE_STRIP, m_numIndices, GL_UNSIGNED_INT, 0);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_PRIMITIVE_RESTART);
}

std::vector<glm::vec3> * Terrain::createVectorArray(float * normals) {
	std::vector<glm::vec3> * vectorArray = new std::vector<glm::vec3>[verticesPerRow*verticesPerRow];
	for (int i = 0; i < verticesPerRow*verticesPerRow; i++) {
		vec3 normal = vec3(normals[i * 3] , normals[i * 3 + 1], normals[i * 3 + 2]);
	}

	return vectorArray;
}

void Terrain::loadDiffuseTexture(const std::string &diffusePath)
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // plain RGB
	glGenerateMipmap(GL_TEXTURE_2D);

	std::cout << "Successfully loaded diffuse texture: " << diffusePath << ".\n";
}