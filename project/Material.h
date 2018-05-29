#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <GL/glew.h>
#include <map>
#include "terrain.h"

using namespace glm;
using namespace std;

class Material {
public:
	char* materialName;
	vec3 baseColour;
	float baseHeight;
	vec3 tint;
	float tintStrength;
	float blendStrength;
	float textureScale;
	Material(char* materialName, vec3 baseColour, float baseHeight, vec3 tint, float tintStrength, float blendStrength, float textureScale);
};
