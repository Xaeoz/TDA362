#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <GL/glew.h>
#include <map>
#include "terrain.h"
#include "HeightGenerator.h";

using namespace glm;
using namespace std;

class EndlessTerrain {
public:
	float maxViewDist;
	int chunkSize;
	int chunksVisibleInViewDst;
	map<pair<int, int>, Terrain> terrainChunkDictionary;
	int tesselation;
	int meshSimplificationFactor;
	vector<Terrain> terrainChunksVisibleLastUpdate;
	float * perlinNoise;
	HeightGenerator generator;
	int perlinNoiseSize;

	struct TerrainParams {
		int octaves = 4;
		float lacunarity = 2.4;
		float persistance = 0.8;
		float heightMultiplier = 300;
		int perlinNoiseSize = 40000; //Must have integer root (e.g. 1, 2, 64, 900, etc..)
		int seedArraySize = 40000;
	};

	TerrainParams params;
	TerrainParams *pparams = &params;

	EndlessTerrain(int chunkSize, float maxViewDist, int tesselation);

	void updateVisibleChunks(vec3 viewerPosition);

	void drawChunks();

	void updateVisibleState(Terrain & terrain, vec3 viewerPosition);

	float calculateDistance(Terrain & terrain, vec3 viewerPosition);


	void setVisibility(Terrain& terrain, bool visible);

	void updateTerrainChunks(TerrainParams * tp);

	void updateTerrainParams(TerrainParams * tp, int octaves, float persistance, float lacunarity, float heightMultiplier, int perlinNoiseSize);

};
