#include "HeightGenerator.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <labhelper.h>
#include <random>
#include <math.h>

#include "Material.h"

using namespace glm;
using namespace std;
using std::string;
using std::vector;

Material::Material(char* materialName, vec3 baseColour, float baseHeight, vec3 tint, float tintStrength, float blendStrength, float textureScale)
	:baseColour(baseColour)
	,baseHeight(baseHeight)
	,materialName(materialName)
	,tint(tint)
	,tintStrength(tintStrength)
	,blendStrength(blendStrength)
	,textureScale(textureScale)
{}