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
using namespace std;
using std::string;
using std::vector;

HeightGenerator::HeightGenerator()
	:seed(0)
	,rng()
	,dist()
	,seedArray()
{
	//Generate seed for rng so that we can generate a new seed
	this->rng.seed(std::random_device()());
	//Create distribution function
	std::uniform_int_distribution<std::default_random_engine::result_type> dist(1, 1000000000000);
	//Generate new seed for later use
	seed = dist(rng);
}


float HeightGenerator::generateRandomFloat(float min, float max)
{
	std::uniform_real_distribution<float> floatDist(min, max);
	return floatDist(rng);
}

float HeightGenerator::generateNoise(int x, int z)
{
	// Set seed based on texCoords. We set the seed to get the same noise for the same x and z.
	// It is important for the algorithm
	this->rng.seed(x * 6123433 + z * 5234776 + seed);
	//Generate noise between 0 and 1 from seed
	float randFloat = generateRandomFloat(0.0f, 1.0f);
	return randFloat;
}

void HeightGenerator::generateSeedArray(int size) {
	seedArray = new float[size];
	int rows = sqrt(size);
	for (int i = 0; i < rows; i++)
		for (int j = 0; j < rows; j++) {
			seedArray[i * rows + j] = generateNoise(dist(rng), dist(rng));
		}
}

float cosineInterpolation(float a, float b, float blend)
{
	float theta = blend * M_PI;
	float f = (1.0f - cos(theta))  * 0.5f;
	return a * (1.0f - f) + b * f;
}

float evaluate(float value)
{
	float a = 3;
	float b = 2.2f;
	
	return pow(value, a) / (pow(value, a) + pow(b - b*value, a));
}

float * HeightGenerator::generateFalloffMap(int size)
{
	float * falloffMap = new float[size*size];

	for (int i = 0 ; i < size; i++)
		for (int j = 0; j < size; j++)
		{
			float x = (i / (float)size) * 2 - 1;
			float y = (j / (float)size) * 2 - 1;

			float value = fmax(abs(x), abs(y));
			falloffMap[i*size + j] = evaluate(value);
		}

	std::cout << "Generated falloff map \n";
	return falloffMap;
}

void HeightGenerator::generatePerlinNoise(int outputArraySize, int seedArraySize, int nOctaves, float * outputArray, float persistance, float lacunarity, float start, NormalizeMode normalizeMode)
{
	fill_n(outputArray, outputArraySize, 0); //clear array of old values in-case it is a reassignment
	delete[] outputArray;
	outputArray = new float[outputArraySize];
	int rowLength = sqrt(outputArraySize);
	int scale = 1;
	int * octaveOffsets = new int[nOctaves * 2];
	float maxNoise = -999999;
	float minNoise = 999999;
	float maxPossibleHeight = 0;
	float amplitude = 1.0f;
	float frequency = 1.0f;
	float * falloffMap = generateFalloffMap(sqrt(outputArraySize));

	/*for (int i = 0; i < nOctaves; i++) 
	{
		int xOffset = generateRandomFloat(0.0, rowLength);
		octaveOffsets[i * 2] = xOffset;
		int yOffset = generateRandomFloat(0.0, rowLength);
		octaveOffsets[i * 2 + 1] = yOffset;

		maxPossibleHeight += amplitude;
		amplitude *= persistance;
	}*/

	for (int i = 0; i < rowLength; i++) 
		for (int j = 0; j < rowLength; j++) 
		{

			int pitch = rowLength;
			float noise = 0.0f;
			amplitude = 1.0f;
			frequency = 1.0f;

			for (int o = 0; o < nOctaves; o++) 
			{
				if(pitch <= 0) pitch = 1;
				int sampleX1 = abs((i / pitch) * pitch + (int)start) % rowLength;
				int sampleY1 = abs((j / pitch) * pitch + (int)start) % rowLength;

				int sampleX2 = (sampleX1 + pitch) % rowLength;
				int sampleY2 = (sampleY1 + pitch) % rowLength;

				//How far into the pitch are we?
				float blendX = (float)((i - sampleX1 + rowLength + (int)start) % rowLength) / (float)pitch;
				float blendY = (float)((j - sampleY1 + rowLength + (int)start) % rowLength) / (float)pitch;

				//interpolate
				float interpolatedSample1 = cosineInterpolation(seedArray[(sampleX1 * rowLength + sampleY1) % seedArraySize], seedArray[(sampleX1 * rowLength + sampleY2) % seedArraySize], blendY);
				float interpolatedSample2 = cosineInterpolation(seedArray[(sampleX2 * rowLength + sampleY1) % seedArraySize], seedArray[(sampleX2 * rowLength + sampleY2) % seedArraySize], blendY);
				
				//Accumulate noise
				noise += (cosineInterpolation(interpolatedSample1, interpolatedSample2, blendX) * 2 - 1) * amplitude;
				
				frequency *= lacunarity;
				amplitude *= persistance;
				pitch = rowLength / frequency;

			}

			if (noise > maxNoise) maxNoise = noise;
			if (noise < minNoise) minNoise = noise;
			outputArray[i * (rowLength)+j] = noise;
		}
	


		float noiseDifference = maxNoise - minNoise;
		for (int x = 0; x < rowLength; x++) {
			for (int y = 0; y < rowLength; y++) {
				if (normalizeMode == Local) {
					float toNormalize = outputArray[x * (rowLength)+y];
					outputArray[x * rowLength + y] = clamp((toNormalize - minNoise) / (noiseDifference) - falloffMap[x * rowLength + y], 0.01f, FLT_MAX) ;
				}
				else {
					float normalizedHeight = (outputArray[x * (rowLength) + y] + 1) / (2.f * maxPossibleHeight / 1.5f);
					outputArray[x * (rowLength)+y] = clamp(normalizedHeight - falloffMap[x * rowLength + y], 0.0001f, FLT_MAX) ;
				}
			}
		}

		delete[] falloffMap;
		std::cout << "Generated perlin noise map \n";
}

