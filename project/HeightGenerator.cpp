#include "HeightGenerator.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <labhelper.h>
#include <random>
#include <math.h>

using namespace glm;
using std::string;
using std::vector;

HeightGenerator::HeightGenerator()
	:AMPLITUDE(300.0f)
	,seed(0)
	,rng()
	,dist()
{
	//Generate seed for rng so that we can generate a new seed
	rng.seed(std::random_device()());
	//Create distribution function
	std::uniform_int_distribution<std::mt19937_64::result_type> dist(1, 100000000000);
	//Generate new seed for later use
	seed = dist(rng);
}

float HeightGenerator::generateRandomFloat(float min, float max)
{
	std::uniform_real_distribution<float> floatDist(min, max);
	return floatDist(rng);
}

float HeightGenerator::generateHeight(float x, float z, float vertexDistance)
{
	//Generate random height between amplitude and -amplitude
	return getInterpolatedNoise(x, z, vertexDistance) * AMPLITUDE;
}

float HeightGenerator::generateNoise(int x, int z)
{
	//Set seed based on texCoords. We set the seed to get the same noise for the same x and z.
	//it is important for the algorithm
	rng.seed(x*4312432 + z*423423532 + seed);
	float randFloat = generateRandomFloat(-1.0, 1.0);
	//Generate noise between 0 and 1 from seed
	return randFloat;
}

float HeightGenerator::getSmoothedNoise(int x, int z, float sl) 
{
	float corners = (generateNoise(x-1, z-1) + generateNoise(x+1, z-1) + generateNoise(x-1, z+1 + generateNoise(x+1, z+1)) / 16.0f);
	float sides = (generateNoise(x, z - 1) + generateNoise(x-1, z) + generateNoise(x, z + 1) + generateNoise(x +1, z)) / 8.0f;
	float center = generateNoise(x, z) / 4.0f;
	return corners + sides + center;
}

float HeightGenerator::getInterpolatedNoise(float x, float z, float sl) 
{
	int intX = (int)(x/sl);
	int intZ = (int)(z/sl);
	float fracX = (x - intX*sl)/sl;
	float fracZ = (z - intZ*sl)/sl;

	float v1 = getSmoothedNoise(intX, intZ, sl);
	float v2 = getSmoothedNoise(intX + 1, intZ, sl);
	float v3 = getSmoothedNoise(intX, intZ + 1, sl);
	float v4 = getSmoothedNoise(intX +1, intZ + 1, sl);
	float i1 = interpolate(v1, v2, fracX);
	float i2 = interpolate(v3, v4, fracX);
	float interpolated = interpolate(i1, i2, fracZ);
	return interpolate(i1, i2, fracZ);
}

//Cosine interpolation
float HeightGenerator::interpolate(float a, float b, float blend)
{
	float theta = blend * M_PI;
	float f = (1.0f - cos(theta))  * 0.5f;
	return a * (1.0f - f) + b * f;
}