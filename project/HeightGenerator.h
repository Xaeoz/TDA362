#pragma once
#include <string>
#include <GL/glew.h>
#include <random>

class HeightGenerator {
public:
	const float AMPLITUDE;
	int seed;
	std::mt19937_64 rng;
	std::uniform_int_distribution<std::mt19937_64::result_type> dist;



	HeightGenerator();

	float generateRandomFloat(float min, float max);

	float generateHeight(float x, float z, float vertexDistance);

	float generateNoise(int x, int z);

	float getSmoothedNoise(int x, int z, float sl);

	float getInterpolatedNoise(float x, float z, float sl);

	float interpolate(float a, float b, float blend);

};
