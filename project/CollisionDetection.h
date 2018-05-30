#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <GL/glew.h>
#include <map>
#include "terrain.h"

using namespace glm;
using namespace std;

class CollisionDetection {
public:
	float radius;

	CollisionDetection(float radius = 1);
	bool willCollide(Terrain terrainChunk, vec3 nextPos, float speed);
	bool willCollidePlane(Terrain terrainChunk, vec3 nextPos, float speed);
	bool willCollideTriangle(Terrain terrainChunk, vec3 nextPos, float speed);
};
