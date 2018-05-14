#include <string>
#include <GL/glew.h>

class HeightField {
public:
	int m_meshResolution; // triangles edges per quad side
	GLuint m_texid_hf;
	GLuint m_texid_diffuse;
	GLuint m_texid_normal;
	GLuint m_vao;
	GLuint m_positionBuffer;
	GLuint m_uvBuffer;
	GLuint m_indexBuffer;
	GLuint m_numIndices;
	GLuint m_normalBuffer;
	std::string m_heightFieldPath;
	std::string m_diffuseTexturePath;
	std::string m_normalPath;

	HeightField(void);

	// load height field
	void loadHeightField(const std::string &heigtFieldPath);

	// load diffuse map
	void loadDiffuseTexture(const std::string &diffusePath);

	void loadNormalMap(const std::string &normalPath);

	// generate mesh
	void generateMesh(int tesselation);

	// render height map
	void submitTriangles(void);

};
