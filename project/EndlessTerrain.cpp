#include "EndlessTerrain.h"
#include "HeightGenerator.h";
#include "CollisionDetection.h";
#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <labhelper.h>
#include <random>
#include <math.h>



using namespace glm;
using namespace std;

EndlessTerrain::EndlessTerrain()
	:chunksVisibleInViewDst(0)
	, terrainChunkDictionary()
	, params()
	, meshSimplificationFactor(0)
	, perlinNoise()
	, tesselation(pparams->tesselation)
	, generator()
	, textureArray(UINT32_MAX)

{
	chunksVisibleInViewDst = (int)pparams->maxViewDistance / pparams->chunkSize;
	perlinNoise = new float[pparams->perlinNoiseSize];
	generator.generateSeedArray(pparams->seedArraySize);
	generator.generatePerlinNoise(pparams->perlinNoiseSize, pparams->seedArraySize, pparams->octaves, perlinNoise, pparams->persistance, pparams->lacunarity, pparams->perlinSamplingStartOffset, generator.Local);
}
void EndlessTerrain::updateVisibleChunks(vec3 viewerPosition)
{
	chunksVisibleInViewDst = (int)pparams->maxViewDistance / pparams->chunkSize;
	//Test all chunks from the last frame if they are still visible
	for (auto &const x : terrainChunkDictionary)
	{
		updateVisibleState(x.second, viewerPosition);
	}

	int currentChunkCoordX = (int)(viewerPosition.x / pparams->chunkSize);
	int currentChunkCoordY = (int)(viewerPosition.z / pparams->chunkSize);
	vec2 currentChunkCoord(currentChunkCoordX, currentChunkCoordY);
		
	for (int yOffset = -chunksVisibleInViewDst; yOffset <= chunksVisibleInViewDst; yOffset++)
		for (int xOffset = -chunksVisibleInViewDst; xOffset <= chunksVisibleInViewDst; xOffset++) {

			vec2 viewedChunkCoord = vec2(currentChunkCoordX + xOffset, currentChunkCoordY + yOffset);

			if (checkIfChunkExists(viewedChunkCoord)) {
				pair<int, int> coords(viewedChunkCoord.x, viewedChunkCoord.y);
				map<pair<int, int>, Terrain>::iterator it = terrainChunkDictionary.find(coords);
				updateVisibleState(it->second, viewerPosition);
				if (it->second.visible) {
					updateTerrainChunk(it->second, pparams, viewerPosition);
				}
			}
			else {
				createNewTerrainChunk(viewedChunkCoord, viewerPosition);
			}
		}

	drawChunks();
}

bool EndlessTerrain::checkIfChunkExists(vec2 viewedChunkCoord)
{
	pair<int, int> coords(viewedChunkCoord.x, viewedChunkCoord.y);
	map<pair<int, int>, Terrain>::iterator it = terrainChunkDictionary.find(coords);
	return it != terrainChunkDictionary.end();
}

void EndlessTerrain::createNewTerrainChunk(vec2 viewedChunkCoord, vec3 viewerPosition)
{
	Terrain terrain(viewedChunkCoord, tesselation, calculateLod(viewedChunkCoord, viewerPosition), pparams->chunkSize);
	int lod = calculateLod(terrain.originCoord, viewerPosition);
	terrain.setLod(lod, pparams->chunkSize);
	terrain.initTerrain(pparams->heightMultiplier, perlinNoise, pparams->perlinNoiseSize, pparams->chunkSize);
	pair <pair<int, int>, Terrain> toBeInserted = make_pair(make_pair(viewedChunkCoord.x, viewedChunkCoord.y), terrain);
	terrainChunkDictionary.insert(toBeInserted);
}

void EndlessTerrain::updateTerrainChunk(Terrain& terrain, TerrainParams *tp, vec3 viewerPosition)
{
	int lod = calculateLod(terrain.originCoord, viewerPosition);
	if (terrain.meshSimplificationFactor != lod) {
		terrain.setLod(lod, tp->chunkSize);
		terrain.updateTerrain(pparams->heightMultiplier, perlinNoise, pparams->perlinNoiseSize, lod, true);
	}
}

Terrain EndlessTerrain::getCurrentChunk(vec3 viewPosition)
{
	int currentChunkCoordX = (int)(viewPosition.x / pparams->chunkSize);
	int currentChunkCoordY = (int)(viewPosition.z / pparams->chunkSize);
	pair<int, int> viewPositionPair(currentChunkCoordX, currentChunkCoordY);
	map<pair<int, int>, Terrain>::iterator it = terrainChunkDictionary.find(viewPositionPair);
	Terrain currentChunk = it->second;
	return currentChunk;
	//if (it != terrainChunkDictionary.end()) {
	//	Terrain currentChunk = it->second;
	//	return currentChunk;
	//}
	//else {
	//	createNewTerrainChunk(vec2(currentChunkCoordX, currentChunkCoordY), viewPosition);
	//	map<pair<int, int>, Terrain>::iterator it = terrainChunkDictionary.find(viewPositionPair);
	//	return it->second;
	//}
}

//Used to update terrain from GUI
void EndlessTerrain::updateTerrainChunks(TerrainParams *tp, vec3 viewerPosition)
{
	delete[] perlinNoise;
	perlinNoise = new float[pparams->perlinNoiseSize];
	tp->tesselation = tp->nSquares * 2;
	generator.generatePerlinNoise(
		tp->perlinNoiseSize,
		tp->seedArraySize,
		tp->octaves,
		perlinNoise,
		tp->persistance,
		tp->lacunarity,
		tp->perlinSamplingStartOffset,
		generator.Local
	);
	for (auto &const terrain : terrainChunkDictionary)
	{
		terrain.second.updateTerrain(tp->heightMultiplier, perlinNoise, tp->perlinNoiseSize, terrain.second.meshSimplificationFactor, false);
	}
	
}

float EndlessTerrain::calculateDistance(Terrain& terrain, vec3 viewerPosition)
{
	vec2 coord = vec2(terrain.originCoord*vec2(pparams->chunkSize));
	float dist = floor(distance(vec2(viewerPosition.x, viewerPosition.z), coord));
	return dist;
}

int EndlessTerrain::calculateLod(vec2 terrainPosition, vec3 viewerPosition)
{
	vec2 terrainWorldCoord = vec2(terrainPosition*vec2(pparams->chunkSize));

	int dist = distance(terrainWorldCoord, vec2(viewerPosition.x, viewerPosition.z));
	if (dist < pparams->chunkSize*5) {
		return 0;
	}
	else if (dist >= pparams->chunkSize*5 && dist < pparams->chunkSize * 10) {
		return 1;
	}
	else if (dist >= pparams->chunkSize * 10 && dist < pparams->chunkSize * 15) {
		return 2;
	}
	else if (dist >= pparams->chunkSize * 15 && dist < pparams->chunkSize * 20) {
		return 3;
	}
	else if (dist >= pparams->chunkSize * 20 && dist < pparams->chunkSize * 25) {
		return 4;
	}
	else {
		return 5;
	}

}

void EndlessTerrain::updateVisibleState(Terrain& terrain, vec3 viewerPosition) {
	float viewerDstFromTerrainChunk = calculateDistance(terrain, viewerPosition);
	bool isCloseEnough = viewerDstFromTerrainChunk <= pparams->maxViewDistance;
	terrain.visible = isCloseEnough;
}

void EndlessTerrain::drawChunks()
{
	for (auto &const x : terrainChunkDictionary)
	{
		if (x.second.visible) {
			if (x.second.m_vao == UINT32_MAX) {
				std::cout << "No vertex array is generated, cannot draw anything.\n";
				return;
			}
			glEnable(GL_PRIMITIVE_RESTART);
			glPrimitiveRestartIndex(x.second.triangleRestartIndex);
			glBindVertexArray(x.second.m_vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, x.second.m_indexBuffer);
			glActiveTexture(GL_TEXTURE26);
			glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
			glDrawElements(GL_TRIANGLE_STRIP, x.second.m_numIndices, GL_UNSIGNED_INT, 0);
			glDisable(GL_PRIMITIVE_RESTART);
		}
	}
}

void EndlessTerrain::loadTextures(const std::string * texturePaths, int nTextures)
{
	
	int width=512, height=512, components=3, mipMapLevels=1;

	if (textureArray == UINT32_MAX) {
		glGenTextures(1, &textureArray);
	}

	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipMapLevels, GL_RGB8, width, height, nTextures);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	for (int i = 0; i < nTextures; i++)
	{
		stbi_set_flip_vertically_on_load(true);
		uint8_t * data = stbi_load(texturePaths[i].c_str(), &width, &height, &components, 3);
		if (data == nullptr) {
			std::cout << "Failed to load image: " << texturePaths[i] << ".\n";
			return;
		}
		for (int j = 0; j < mipMapLevels; j++) {
			if(j > 0) glTexSubImage3D(GL_TEXTURE_2D_ARRAY, j, 0, 0, i, width/(j*2), height/(j*2), 1, GL_RGB, GL_UNSIGNED_BYTE, data);
			if(j == 0) glTexSubImage3D(GL_TEXTURE_2D_ARRAY, j, 0, 0, i, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		
		std::cout << "Successfully loaded texture: " << texturePaths[i] << ".\n";
	}

	//glGenerateMipmap(GL_TEXTURE_2D);
	std::cout << "Successfully loaded textureArray \n";
}