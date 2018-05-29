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

	struct TerrainParams {
		int octaves = 5;
		float lacunarity = 4.6f;
		float persistance = 0.2f;
		float heightMultiplier = 1000.f;
		int perlinNoiseSize = 250* 250;
		int seedArraySize = 1000*1000;
		int meshSimplificationFactor = 0;
		float perlinSamplingStartOffset = 1.f;
		int nSquares = 40*40;
		int tesselation = nSquares * 2;
		int chunkSize = 2000;
		float maxViewDistance = chunkSize*4;
	};

	TerrainParams params;
	TerrainParams *pparams = &params;


	int chunksVisibleInViewDst;
	int tesselation;
	int meshSimplificationFactor;
	int perlinNoiseSize;
	float * perlinNoise;
	map<pair<int, int>, Terrain> terrainChunkDictionary;
	HeightGenerator generator;
	GLuint textureArray;



	EndlessTerrain();

	bool checkIfChunkExists(vec2 viewedChunkCoord);

	void updateVisibleChunks(vec3 viewerPosition);

	void createNewTerrainChunk(vec2 viewedChunkCoord, vec3 viewerPosition);

	void updateTerrainChunk(Terrain & terrain, TerrainParams * tp, vec3 viewerPosition);

	void drawChunks();

	void updateVisibleState(Terrain & terrain, vec3 viewerPosition);

	float calculateDistance(Terrain & terrain, vec3 viewerPosition);

	int calculateLod(vec2 terrainPosition, vec3 viewerPosition);

	void updateTerrainChunks(TerrainParams * tp, vec3 viewerPosition);

	void loadTextures(const std::string * texturePaths, int nTextures);

};
