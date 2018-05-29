#pragma once
#include <string>
#include <GL/glew.h>
#include <random>

class HeightGenerator {
public:
	int seed;
	std::mt19937_64 rng;
	std::uniform_int_distribution<std::default_random_engine::result_type> dist;
	float* seedArray;
	int tesselation;
	float start;

	enum NormalizeMode
	{
		Local,
		Global
	};


	HeightGenerator();

	float generateRandomFloat(float min, float max);

	float generateNoise(int x, int z);

	void generateSeedArray(int size);

	float * generateFalloffMap(int size);

	void generatePerlinNoise(int outputArraySize, int seedArraySize, int nOctaves, float * outputArray, float persistance, float lacunarity, float start, NormalizeMode mode);





};
