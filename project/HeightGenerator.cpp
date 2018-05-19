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

HeightGenerator::HeightGenerator(int tesselation)
	:AMPLITUDE(300.0f)
	,seed(0)
	,rng()
	,dist()
	,seedArray()
	,tesselation(tesselation)
{
	//Generate seed for rng so that we can generate a new seed
	rng.seed(std::random_device()());
	//Create distribution function
	std::uniform_int_distribution<std::default_random_engine::result_type> dist(1, 1000000000000);
	//Generate new seed for later use
	seed = dist(rng);
	//Generate seed array
	generateSeedArray(tesselation);
}

void HeightGenerator::setTesselation(int tesselationIn) 
{
	tesselation = tesselationIn;
};

float HeightGenerator::generateRandomFloat(float min, float max)
{
	/*std::normal_distribution<float> floatDist(RAND_MAX/2, RAND_MAX);*/
	std::uniform_real_distribution<float> floatDist(0.0, 1.0);
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
	rng.seed(x*512343 + z*5234776 + seed);
	float randFloat = generateRandomFloat(0.0, 1.0);
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

void HeightGenerator::generateSeedArray(int size) {
	seedArray = new float[size];
	int rows = sqrt(size);
	for (int i = 0; i < rows; i++)
		for(int j = 0; j < rows; j++){
			seedArray[i * rows + j] = generateNoise(dist(rng), dist(rng));
		}
}

void HeightGenerator::generatePerlinNoise(int size, int nOctaves, float* outputArray, float persistance, float lacunarity)
{
	int rowLength = sqrt(size);
	int scale = 1;

	float maxNoise = -999999;
	float minNoise = 999999;
	int * octaveOffsets = new int[nOctaves * 2];
	for (int i = 0; i < nOctaves; i++) {
		int xOffset = generateRandomFloat(1.0, rowLength);
		octaveOffsets[i * 2] = xOffset;
		int yOffset = generateRandomFloat(1.0, rowLength);
		octaveOffsets[i * 2 + 1] = yOffset;
	}

	for (int i = 0; i < rowLength; i++) {
		for (int j = 0; j < rowLength; j++) {

			float noise = 0.0f;
			float scaleAcc = 0.0f;
			float amplitude = 1.0f;
			float frequency = 1;
			int pitch = rowLength;


			for (int o = 0; o < nOctaves; o++) {
				if(pitch <= 0) pitch = 1;
				int sampleX1 = (i / pitch) * pitch + octaveOffsets[o*2];
				int sampleY1 = (j / pitch) * pitch + octaveOffsets[o*2+1];

				int sampleX2 = (sampleX1 + (int)pitch) % rowLength;
				int sampleY2 = (sampleY1 + (int)pitch) % rowLength;
				//How far into the pitch are we?
				float blendX = (float)(i - sampleX1) / (float)pitch;
				float blendY = (float)(j - sampleY1) / (float)pitch;
				//interpolate
				float interpolatedSample1 = (1.0f - blendY) * seedArray[sampleX1 * rowLength + sampleY1] + blendY * seedArray[sampleX1 * rowLength + sampleY2];
				float interpolatedSample2 = (1.0f - blendY) * seedArray[sampleX2 * rowLength + sampleY1] + blendY * seedArray[sampleX2 * rowLength + sampleY2];
				/*float interpolatedSample1 = interpolate(seedArray[sampleX1 * rowLength + sampleY1], seedArray[sampleX1 * rowLength + sampleY2], blendY);
				float interpolatedSample2 = interpolate(seedArray[sampleX2 * rowLength + sampleY1], seedArray[sampleX2 * rowLength + sampleY2], blendY);*/
				//Accumulate noise
				noise += (((blendX * (interpolatedSample2 - interpolatedSample1) + interpolatedSample1)) * 2 - 1) * amplitude;
				//noise += (interpolatedSample1, interpolatedSample2, blendX) * scale;
				//Accumulate scale
				scaleAcc += amplitude;
				//Half the scale
				frequency *= lacunarity;
				//pitch = rowLength / frequency;
				amplitude *= persistance;
				pitch = rowLength / frequency;
			}

			if (noise > maxNoise) maxNoise = noise;
			if (noise < minNoise) minNoise = noise;

			outputArray[i * (rowLength)+j] = noise;
			//outputArray[i * (rowLength)+j] = seedArray[i*j+j];
		}
	}

	float noiseDifference = maxNoise - minNoise;
	for (int x = 0; x < rowLength; x++) {
		for (int y = 0; y < rowLength; y++) {
			float toNormalize = outputArray[x * (rowLength)+y];
			outputArray[x * (rowLength)+y] = (toNormalize - minNoise) / (noiseDifference);
		}
	}
}