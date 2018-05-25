#include "EndlessTerrain.h"
#include "HeightGenerator.h";

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

EndlessTerrain::EndlessTerrain(int chunkSize, float maxViewDist, int tesselation)
	:chunksVisibleInViewDst(0)
	, chunkSize(chunkSize)
	, maxViewDist(maxViewDist)
	, terrainChunkDictionary()
	, tesselation(tesselation)
	, meshSimplificationFactor(0)
	, terrainChunksVisibleLastUpdate()
	, perlinNoise()
	, generator(tesselation)
	, params()
{
	chunksVisibleInViewDst = (int)maxViewDist / chunkSize;
	perlinNoise = new float[pparams->perlinNoiseSize];
	//generator.generateSeedArray(tesselation);
	generator.generatePerlinNoise(pparams->perlinNoiseSize, pparams->octaves, perlinNoise, pparams->persistance, pparams->heightMultiplier, pparams->heightMultiplier);
}

void EndlessTerrain::updateVisibleChunks(vec3 viewerPosition)
{
	//Test all chunks from the last frame
	for (auto &const x : terrainChunkDictionary)
	{
		updateVisibleState(x.second, viewerPosition);
	}
	terrainChunksVisibleLastUpdate.clear();

	int currentChunkCoordX = (int)(viewerPosition.x / chunkSize);
	int currentChunkCoordY = (int)(viewerPosition.z / chunkSize);
	
	for (int yOffset = -chunksVisibleInViewDst; yOffset <= chunksVisibleInViewDst; yOffset++)
		for (int xOffset = -chunksVisibleInViewDst; xOffset <= chunksVisibleInViewDst; xOffset++) {
			vec2 viewedChunkCoord = vec2(currentChunkCoordX + xOffset, currentChunkCoordY + yOffset);

			pair<int, int> coords(viewedChunkCoord.x, viewedChunkCoord.y);
			map<pair<int, int>, Terrain>::iterator it = terrainChunkDictionary.find(coords);
			if (it != terrainChunkDictionary.end()) {
				updateVisibleState(it->second, viewerPosition);
			}
			else {
				Terrain terrain(viewedChunkCoord, tesselation, meshSimplificationFactor, chunkSize);
				terrain.initTerrain(pparams->heightMultiplier, perlinNoise, pparams->perlinNoiseSize, chunkSize);
				pair <pair<int, int>, Terrain> toBeInserted = make_pair(make_pair(viewedChunkCoord.x, viewedChunkCoord.y), terrain);
				terrainChunkDictionary.insert(toBeInserted);
			}
		}
	drawChunks();
}

void EndlessTerrain::drawChunks()
{
	for (auto &const x : terrainChunkDictionary)
	{
		if(x.second.visible)
			x.second.submitTriangles();
	}
}

void EndlessTerrain::updateVisibleState(Terrain& terrain, vec3 viewerPosition) {
	float viewerDstFromTerrainChunk = calculateDistance(terrain, viewerPosition);
	setVisibility(terrain, viewerDstFromTerrainChunk <= maxViewDist);
}

float EndlessTerrain::calculateDistance(Terrain& terrain, vec3 viewerPosition)
{
	vec2 coord = vec2(terrain.originCoord*vec2(chunkSize));
	return floor(distance(vec2(viewerPosition.x, viewerPosition.z), coord));
}

void EndlessTerrain::setVisibility(Terrain& terrain, bool visible)
{
	terrain.visible = visible;
	if (visible)
		terrainChunksVisibleLastUpdate.push_back(terrain);
}

void EndlessTerrain::updateTerrainChunks(TerrainParams *tp)
{
	perlinNoise = new float[pparams->perlinNoiseSize];
	generator.generatePerlinNoise(tp->perlinNoiseSize, tp->octaves, perlinNoise, tp->persistance, tp->lacunarity, tp->heightMultiplier);
	for (auto &const terrain : terrainChunkDictionary)
	{
		terrain.second.updateTerrain(tp->heightMultiplier, perlinNoise, tp->perlinNoiseSize);
	}
	
}

void EndlessTerrain::updateTerrainParams(TerrainParams *tp, int octaves, float persistance, float lacunarity, float heightMultiplier, int perlinNoiseSize)
{
	tp->octaves = octaves;
	tp->persistance = persistance;
	tp->lacunarity = lacunarity;
	tp->heightMultiplier = heightMultiplier;
	tp->perlinNoiseSize = perlinNoiseSize;
}