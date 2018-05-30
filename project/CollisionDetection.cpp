#include "CollisionDetection.h"

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

CollisionDetection::CollisionDetection(float radius)
	: radius(radius)
{}

bool CollisionDetection::willCollide(Terrain terrainChunk, vec3 nextPos, float speed)
{
	float mindist = 999999999;
	for (int i = 0; i < (terrainChunk.verticesPerRow*terrainChunk.verticesPerRow); i++)
	{
		vec3 vertex(terrainChunk.verts[i * 3], terrainChunk.verts[i * 3 + 1], terrainChunk.verts[i * 3 + 2]);
		float dist = distance(nextPos,vertex);
		if (mindist > dist) mindist = dist; //Debugging
		if (dist < this->radius)
			return true;
	}
	return false;
}

bool CollisionDetection::willCollidePlane(Terrain terrainChunk, vec3 nextPos, float speed)
{
	float mindist = 999999999;
	int verticesPerRow = terrainChunk.verticesPerRow*3;
	//plane x = sa + tb + c
	for (int i = 0; i < (terrainChunk.verticesPerRow-1); i++)
		for(int j = 0; j < terrainChunk.verticesPerRow -1; j++)
	{
		int rowIndex = (i + 1) * verticesPerRow + j*3;
		int colIndex = i * verticesPerRow + j * 3;
		vec3 vertex1(terrainChunk.verts[colIndex], terrainChunk.verts[colIndex + 1], terrainChunk.verts[colIndex + 2]);
		vec3 vertex2(terrainChunk.verts[colIndex + 3], terrainChunk.verts[colIndex + 4], terrainChunk.verts[colIndex +5]);
		vec3 vertex3(terrainChunk.verts[rowIndex], terrainChunk.verts[rowIndex + 1], terrainChunk.verts[rowIndex + 2]);
		vec3 vertex4(terrainChunk.verts[rowIndex + 3], terrainChunk.verts[rowIndex + 4], terrainChunk.verts[rowIndex + 5]);
		vec3 vector1 = vertex2 - vertex1;
		vec3 vector2 = vertex3 - vertex1;

		vec3 surfaceNormal = normalize(cross(vector2, vector1));

		vec3 vertex5 = vertex1 + (vertex4 - vertex1)*0.5f;
		vec3 vectorPlaneSphere = nextPos - vertex5;

		float dotProduct = dot(surfaceNormal,vectorPlaneSphere);
		float dist = abs(dot(vectorPlaneSphere, surfaceNormal));
		if (abs(dist) < mindist) mindist = dist; //For debugging
		if (abs(dist) < this->radius)
			return true;
	}
	return false;
}

bool isIntersectingTriangle(vec3 A, vec3 B, vec3 C, vec3 P, float r)
{
	A = A - P;
	B = B - P;
	C = C - P;

	vec3 N = normalize(cross(B - A,C - A));

	float d = dot(A, N);
	bool sep = (d*d > r*r);
	return !sep;
}

bool CollisionDetection::willCollideTriangle(Terrain terrainChunk, vec3 nextPos, float speed)
{
	float mindist = 999999999;
	int verticesPerRow = terrainChunk.verticesPerRow * 3;
	//plane x = sa + tb + c
	for (int i = 0; i < terrainChunk.verticesPerRow - 1; i++)
		for (int j = 0; j < terrainChunk.verticesPerRow - 1; j++)
		{
				int rowIndex = (i + 1) * verticesPerRow + j * 3;
				int colIndex = i * verticesPerRow + j * 3;
				vec3 vertex1(terrainChunk.verts[colIndex], terrainChunk.verts[colIndex + 1], terrainChunk.verts[colIndex + 2]);
				vec3 vertex2(terrainChunk.verts[colIndex + 3], terrainChunk.verts[colIndex + 4], terrainChunk.verts[colIndex + 5]);
				vec3 vertex3(terrainChunk.verts[rowIndex], terrainChunk.verts[rowIndex + 1], terrainChunk.verts[rowIndex + 2]);
				vec3 vertex4(terrainChunk.verts[rowIndex + 3], terrainChunk.verts[rowIndex + 4], terrainChunk.verts[rowIndex + 5]);

				bool test1 = isIntersectingTriangle(vertex1, vertex2, vertex3, nextPos, this->radius);
				bool test2 = isIntersectingTriangle(vertex2, vertex3, vertex4, nextPos, this->radius);
				if (test1 || test2)
					return true;
		}
	return false;
}





