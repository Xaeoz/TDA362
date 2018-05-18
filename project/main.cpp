

#include "Water.h"
#ifdef _WIN32
extern "C" _declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
#endif

#include <GL/glew.h>
#include <algorithm>
#include <chrono>

#include <labhelper.h>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;

#include <Model.h>
#include "hdr.h"
#include "HeightGenerator.h"
#include "terrain.h"





using std::min;
using std::max;

///////////////////////////////////////////////////////////////////////////////
// Various globals
///////////////////////////////////////////////////////////////////////////////
SDL_Window* g_window = nullptr;
float currentTime  = 0.0f;
float previousTime = 0.0f;
float deltaTime    = 0.0f;
bool showUI = false;
int windowWidth, windowHeight;
int octaves = 4;
float scalingBias = 0.6f;

//int tesselation = 262144;
int size = 400;
int tesselation = ((size/6)*2)*((size/6)*2);
//int tesselation = 2000;
HeightGenerator heightGenerator(tesselation);
Terrain terrain(tesselation, heightGenerator);


//TODO: Remove this in favour of utilizing illumination map in the shaders
//Global Light (the sun)
vec3 globalLightPosition = vec3(0, 100, -1000);				//Position of the imaginary "sun"
vec3 globalLightColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);		//Color of the light it is emitting
vec3 globalLightDirection = normalize(vec3(2500, -100, 10000));		//Direction of the global light

///////////////////////////////////////////////////////////////////////////////
// Shader programs
///////////////////////////////////////////////////////////////////////////////
GLuint shaderProgram; // Shader for rendering the final image
GLuint simpleShaderProgram; // Shader used to draw the shadow map
GLuint backgroundProgram;
GLuint heightShader;
GLuint waterShader;

////////////////////////////////////////////////////////////////////////////////
// Water
///////////////////////////////////////////////////////////////////////////////
Water water;


///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
float environment_multiplier = 2.0f;
GLuint environmentMap, irradianceMap, reflectionMap;
const std::string envmap_base_name = "001";


///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
vec3 lightPosition;
vec3 point_light_color = vec3(1.f, 1.0f, 1.f);

float point_light_intensity_multiplier = 100.0f;





///////////////////////////////////////////////////////////////////////////////
// Camera parameters.
///////////////////////////////////////////////////////////////////////////////
//vec3 cameraPosition(-270.0f, 300.0f, 70.0f);
vec3 cameraPosition(0.0f, 50.0f, 70.0f);
vec3 cameraDirection = normalize(vec3(0.0f) - cameraPosition);
float cameraSpeed = 10.0f;

vec3 worldUp(0.0f, 1.0f, 0.0f);

///////////////////////////////////////////////////////////////////////////////
// Models
///////////////////////////////////////////////////////////////////////////////
labhelper::Model *fighterModel = nullptr;
labhelper::Model *landingpadModel = nullptr;
labhelper::Model *sphereModel = nullptr;

mat4 roomModelMatrix;
mat4 landingPadModelMatrix; 
mat4 fighterModelMatrix;
mat4 terrainModelMatrix;

void loadShaders(bool is_reload)
{
	GLuint shader = labhelper::loadShaderProgram("../project/simple.vert", "../project/simple.frag", is_reload);
	if (shader != 0) simpleShaderProgram = shader; 
	shader = labhelper::loadShaderProgram("../project/background.vert", "../project/background.frag", is_reload);
	if (shader != 0) backgroundProgram = shader;
	shader = labhelper::loadShaderProgram("../project/water.vert", "../project/water.frag");
	if (shader != 0) waterShader = shader;
	shader = labhelper::loadShaderProgram("../project/shading.vert", "../project/shading.frag", is_reload);
	if (shader != 0) shaderProgram = shader;
}

void initGL()
{
	///////////////////////////////////////////////////////////////////////
	//		Load Shaders
	///////////////////////////////////////////////////////////////////////
	backgroundProgram   = labhelper::loadShaderProgram("../project/background.vert",  "../project/background.frag");
	shaderProgram       = labhelper::loadShaderProgram("../project/shading.vert",     "../project/shading.frag");
	simpleShaderProgram = labhelper::loadShaderProgram("../project/simple.vert",      "../project/simple.frag");
	heightShader		= labhelper::loadShaderProgram("../project/heightfield.vert", "../project/shading.frag");
	waterShader			= labhelper::loadShaderProgram("../project/water.vert",		  "../project/water.frag");

	///////////////////////////////////////////////////////////////////////
	// Load models and set up model matrices
	///////////////////////////////////////////////////////////////////////
	fighterModel    = labhelper::loadModelFromOBJ("../scenes/NewShip.obj");
	landingpadModel = labhelper::loadModelFromOBJ("../scenes/landingpad.obj");
	sphereModel     = labhelper::loadModelFromOBJ("../scenes/sphere.obj");

	float landingPadYPosition = 5.0f;

	roomModelMatrix = mat4(1.0f);

	fighterModelMatrix = translate((landingPadYPosition + 15) * worldUp);
	landingPadModelMatrix = translate(landingPadYPosition * worldUp);
	vec3 scaleFactor = vec3(size*3, 100, size*3); //Use this to scale the map
	terrainModelMatrix = glm::scale((vec3(1.0f, 1.0f, 1.0f)*scaleFactor))*translate(vec3(-1.0f, -0.50f, -1.0f));
	waterModelMatrix = translate(waterYPosition * worldUp);
	terrain.initTerrain(octaves, scalingBias);





	///////////////////////////////////////////////////////////////////////
	//		Setup Water class
	///////////////////////////////////////////////////////////////////////
	const float waterYPos = -20.0f;
	const float waterSize = 500.0f;

	//Reflection map resolution (higher is better and slower)
	const float reflectionX = 1280;
	const float reflectionY = 720;

	//Refraction map resolution (higher is better and slower)
	const float refractionX = 1280;
	const float refractionY = 720;

	water.init(waterYPos*worldUp, waterSize, vec2(reflectionX, reflectionY), vec2(refractionX, refractionY));

	///////////////////////////////////////////////////////////////////////
	// Load environment map
	///////////////////////////////////////////////////////////////////////
	const int roughnesses = 8;
	std::vector<std::string> filenames;
	for (int i = 0; i < roughnesses; i++)
		filenames.push_back("../scenes/envmaps/" + envmap_base_name + "_dl_" + std::to_string(i) + ".hdr");

	reflectionMap = labhelper::loadHdrMipmapTexture(filenames);
	environmentMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + ".hdr");
	irradianceMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + "_irradiance.hdr");

	//Load terrain textures
	std::string heightFieldFilePath = "../res/land_with_pond.png";
	std::string heightFieldTexture = "../res/wildgrass.png";
	std::string heightFieldNormals = "../res/NormalMap.png";
	//terrain.loadHeightField(heightFieldFilePath);
	terrain.loadDiffuseTexture(heightFieldTexture);
	//terrain.loadNormalMap(heightFieldNormals);

	glEnable(GL_DEPTH_TEST);	// enable Z-buffering 
	glEnable(GL_CULL_FACE);		// enables backface culling
	glEnable(GL_CLIP_DISTANCE0); //Enable clipping plane 0 for use in vertex shaders


}

void debugDrawLight(const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix, const glm::vec3 &worldSpaceLightPos)
{
	mat4 modelMatrix = glm::translate(worldSpaceLightPos);
	glUseProgram(shaderProgram);
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * modelMatrix);
	labhelper::render(sphereModel);
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", projectionMatrix * viewMatrix);
	labhelper::debugDrawLine(viewMatrix, projectionMatrix, worldSpaceLightPos);
}


void drawBackground(const mat4 &viewMatrix, const mat4 &projectionMatrix)
{
	glUseProgram(backgroundProgram);
	labhelper::setUniformSlow(backgroundProgram, "environment_multiplier", environment_multiplier);
	labhelper::setUniformSlow(backgroundProgram, "inv_PV", inverse(projectionMatrix * viewMatrix));
	labhelper::setUniformSlow(backgroundProgram, "camera_pos", cameraPosition);
	labhelper::drawFullScreenQuad();
}

void drawScene(GLuint currentShaderProgram, const mat4 &viewMatrix, const mat4 &projectionMatrix, const mat4 &lightViewMatrix, const mat4 &lightProjectionMatrix, const glm::vec4 clipPlane)
{
	vec4 viewSpaceLightPosition = viewMatrix * vec4(lightPosition, 1.0f);

	/*//Land + Heightfield
	glUseProgram(heightShader);
	labhelper::setUniformSlow(heightShader, "point_light_color", point_light_color);
	labhelper::setUniformSlow(heightShader, "point_light_intensity_multiplier", point_light_intensity_multiplier);
	labhelper::setUniformSlow(heightShader, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));
	labhelper::setUniformSlow(heightShader, "viewSpaceLightDir", normalize(vec3(viewMatrix * vec4(-lightPosition, 0.0f))));
	// Environment
	labhelper::setUniformSlow(heightShader, "environment_multiplier", environment_multiplier * 1);

	// camera
	labhelper::setUniformSlow(heightShader, "viewInverse", inverse(viewMatrix));
	labhelper::setUniformSlow(heightShader, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * terrainModelMatrix);
	mat4 a = inverse(transpose(viewMatrix * terrainModelMatrix));
	labhelper::setUniformSlow(heightShader, "modelViewMatrix", viewMatrix * terrainModelMatrix);
	labhelper::setUniformSlow(heightShader, "normalMatrix", a);
	//material
	labhelper::setUniformSlow(heightShader, "material_reflectivity", .1f);
	labhelper::setUniformSlow(heightShader, "material_metalness", 1.0f);
	labhelper::setUniformSlow(heightShader, "material_fresnel", 1.0f);
	labhelper::setUniformSlow(heightShader, "material_shininess", 1.0f);
	labhelper::setUniformSlow(heightShader, "material_emission", .5f);
	labhelper::setUniformSlow(heightShader, "has_diffuse_texture", 1);
	labhelper::setUniformSlow(heightShader, "has_emission_texture", 0);
	terrain.submitTriangles();*/

	glUseProgram(currentShaderProgram);
	labhelper::setUniformSlow(currentShaderProgram, "clippingPlane", clipPlane);
	// Light source
	labhelper::setUniformSlow(currentShaderProgram, "point_light_color", point_light_color);
	labhelper::setUniformSlow(currentShaderProgram, "point_light_intensity_multiplier", point_light_intensity_multiplier);
	labhelper::setUniformSlow(currentShaderProgram, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));
	labhelper::setUniformSlow(currentShaderProgram, "viewSpaceLightDir", normalize(vec3(viewMatrix * vec4(-lightPosition, 0.0f))));


	// Environment
	labhelper::setUniformSlow(currentShaderProgram, "environment_multiplier", environment_multiplier);

	// camera
	labhelper::setUniformSlow(currentShaderProgram, "viewInverse", inverse(viewMatrix));

	//// landing pad 
	//labhelper::setUniformSlow(currentShaderProgram, "modelMatrix", landingPadModelMatrix);
	//labhelper::setUniformSlow(currentShaderProgram, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * landingPadModelMatrix);
	//labhelper::setUniformSlow(currentShaderProgram, "modelViewMatrix", viewMatrix * landingPadModelMatrix);
	//labhelper::setUniformSlow(currentShaderProgram, "normalMatrix", inverse(transpose(viewMatrix * landingPadModelMatrix)));

	//labhelper::render(landingpadModel);

	//// Fighter
	//labhelper::setUniformSlow(currentShaderProgram, "modelMatrix", fighterModelMatrix);
	//labhelper::setUniformSlow(currentShaderProgram, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * fighterModelMatrix);
	//labhelper::setUniformSlow(currentShaderProgram, "modelViewMatrix", viewMatrix * fighterModelMatrix);
	//labhelper::setUniformSlow(currentShaderProgram, "normalMatrix", inverse(transpose(viewMatrix * fighterModelMatrix)));

	//labhelper::render(fighterModel);
	
	
}

void drawWater(const mat4 &viewMatrix, const mat4 &projectionMatrix)
{
	water.m_moveFactor += WAVE_SPEED;
	if (water.m_moveFactor >= 1.0f) water.m_moveFactor = 0;			//Reset counter if it passes 1.0f
	//Set active shader program
	glUseProgram(waterShader);

	glEnable(GL_BLEND);			//Enable alpha blending (used to smooth out water edges)
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Setup data in shaders, for information on each field, refer to the shader
	labhelper::setUniformSlow(waterShader, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * water.m_modelMatrix);
	labhelper::setUniformSlow(waterShader, "moveFactor", water.m_moveFactor);
	labhelper::setUniformSlow(waterShader, "cameraPosition", cameraPosition);
	labhelper::setUniformSlow(waterShader, "modelMatrix", water.m_modelMatrix);
	labhelper::setUniformSlow(waterShader, "globalLightPosition", globalLightPosition);
	labhelper::setUniformSlow(waterShader, "globalLightColor", globalLightColor);
	labhelper::setUniformSlow(waterShader, "globalLightDirection", globalLightDirection);
	//These need to match the near and far plane of the current projection matrix
	labhelper::setUniformSlow(waterShader, "nearPlane", 1.0f);
	labhelper::setUniformSlow(waterShader, "farPlane", 2000.0f);
	//Binds necessary textures, VAO, and index buffer, and then calls drawElements
	water.render();
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	
	glDisable(GL_BLEND);											//Disable blend again after rendering
}

void drawQuad(float width, float height)
{

	glUseProgram(simpleShaderProgram);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, (water.m_reflectionFbo.colorTextureTargets.at(0)));

	//labhelper::drawFullScreenQuad();
	//return;
	GLboolean previous_depth_state;
	glGetBooleanv(GL_DEPTH_TEST, &previous_depth_state);
	glDisable(GL_DEPTH_TEST);
	static GLuint vertexArrayObject = 0;
	static int nofVertices = 6;
	// do this initialization first time the function is called... 
	if (vertexArrayObject == 0)
	{
		glGenVertexArrays(1, &vertexArrayObject);
		static const glm::vec2 positions[] = {
			{ -1.0f, 1.0f-height*2 },{ width*2 - 1.0f, 1.0f-height*2 },{ width*2 - 1.0f, 1.0f },
		{ -1.0f, 1.0f -height * 2 },{ width * 2 - 1.0f, 1.0f },{ -1.0f, 1.0f }
		};
		labhelper::createAddAttribBuffer(vertexArrayObject, positions, sizeof(positions), 0, 2, GL_FLOAT);
	}
	glBindVertexArray(vertexArrayObject);
	glDrawArrays(GL_TRIANGLES, 0, nofVertices);
	if (previous_depth_state) glEnable(GL_DEPTH_TEST);

	
}

void drawTerrain(const mat4 &viewMatrix, const mat4 &projectionMatrix, const glm::vec4 clipPlane)
{
	vec4 viewSpaceLightPosition = viewMatrix * vec4(lightPosition, 1.0f);
	//Land + Heightfield
	glUseProgram(heightShader);
	labhelper::setUniformSlow(heightShader, "modelMatrix", terrainModelMatrix);
	labhelper::setUniformSlow(heightShader, "clippingPlane", clipPlane);
	labhelper::setUniformSlow(heightShader, "point_light_color", point_light_color);
	labhelper::setUniformSlow(heightShader, "point_light_intensity_multiplier", point_light_intensity_multiplier);
	labhelper::setUniformSlow(heightShader, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));
	labhelper::setUniformSlow(heightShader, "viewSpaceLightDir", normalize(vec3(viewMatrix * vec4(-lightPosition, 0.0f))));
	// Environment
	labhelper::setUniformSlow(heightShader, "environment_multiplier", environment_multiplier * 3);

	// camera
	labhelper::setUniformSlow(heightShader, "viewInverse", inverse(viewMatrix));
	labhelper::setUniformSlow(heightShader, "has_diffuse_texture", 0);
	labhelper::setUniformSlow(heightShader, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * terrainModelMatrix);
	mat4 a = inverse(transpose(viewMatrix * terrainModelMatrix));
	labhelper::setUniformSlow(heightShader, "modelViewMatrix", viewMatrix * terrainModelMatrix);
	labhelper::setUniformSlow(heightShader, "normalMatrix", a);
	//material
	labhelper::setUniformSlow(heightShader, "material_reflectivity", .01f);
	labhelper::setUniformSlow(heightShader, "material_metalness", 1.0f);
	labhelper::setUniformSlow(heightShader, "material_fresnel", 1.0f);
	labhelper::setUniformSlow(heightShader, "material_shininess", 1.0f);
	labhelper::setUniformSlow(heightShader, "material_emission", 1.0f);
	labhelper::setUniformSlow(heightShader, "has_diffuse_texture", 1);
	labhelper::setUniformSlow(heightShader, "has_emission_texture", 0);
	terrain.submitTriangles();
}

///////////////////////////////////////////////////////////////////////////
// Draws everything that does not require multiple passes
///////////////////////////////////////////////////////////////////////////
void drawSinglePassScene(GLuint currentShaderProgram, const mat4 &viewMatrix, const mat4 &projectionMatrix, const mat4 &lightViewMatrix, const mat4 &lightProjectionMatrix, const glm::vec4 clipPlane)
{
	drawBackground(viewMatrix, projectionMatrix);
	drawScene(shaderProgram, viewMatrix, projectionMatrix, lightViewMatrix, lightProjectionMatrix, clipPlane);
	drawTerrain(viewMatrix, projectionMatrix, clipPlane);
	debugDrawLight(viewMatrix, projectionMatrix, vec3(lightPosition));
}

void display(void)
{
	///////////////////////////////////////////////////////////////////////////
	// Check if window size has changed and resize buffers as needed
	///////////////////////////////////////////////////////////////////////////
	{
		int w, h;
		SDL_GetWindowSize(g_window, &w, &h);
		if (w != windowWidth || h != windowHeight) {
			windowWidth = w;
			windowHeight = h;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// setup matrices 
	///////////////////////////////////////////////////////////////////////////

	mat4 projMatrix = perspective(radians(70.0f), float(windowWidth) / float(windowHeight), 1.0f, 4000.0f);

	mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraDirection, worldUp);

	vec4 lightStartPosition = vec4(40.0f, 60.0f, 0.0f, 1.0f);
	lightPosition = vec3(rotate(currentTime, worldUp) * lightStartPosition);
	//lightPosition = lightStartPosition;
	mat4 lightViewMatrix = lookAt(lightPosition, vec3(0.0f), worldUp);
	mat4 lightProjMatrix = perspective(radians(45.0f), 1.0f, 25.0f, 100.0f);

	///////////////////////////////////////////////////////////////////////////
	// Bind the environment map(s) to unused texture units
	///////////////////////////////////////////////////////////////////////////
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, environmentMap);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, irradianceMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, reflectionMap);
	glActiveTexture(GL_TEXTURE0);

	///////////////////////////////////////////////////////////////////////////
	// Draw scene above water for reflections
	///////////////////////////////////////////////////////////////////////////
	glBindFramebuffer(GL_FRAMEBUFFER, water.m_reflectionFbo.framebufferId);
	glViewport(0, 0, water.m_reflectionFbo.width, water.m_reflectionFbo.height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//distance to move camera to move "below" water
	float distanceToMove = 2 * (cameraPosition.y - water.m_modelMatrix[3].y);  
	vec3 reflectionCameraPosition = cameraPosition - worldUp * distanceToMove;

	//Look at the inversed y direction (inverse pitch)
	mat4 reflectionViewMatrix = lookAt(reflectionCameraPosition, reflectionCameraPosition + vec3(cameraDirection.x, -cameraDirection.y, cameraDirection.z), worldUp);
	float renderPassOffset = 1.0f;
	drawSinglePassScene(shaderProgram, reflectionViewMatrix, projMatrix, lightViewMatrix, lightProjMatrix, vec4(0, 1.0f, 0, -water.m_modelMatrix[3].y +1.0f));

	///////////////////////////////////////////////////////////////////////////
	// Draw below water for refraction
	///////////////////////////////////////////////////////////////////////////
	glBindFramebuffer(GL_FRAMEBUFFER, water.m_refractionFbo.framebufferId);
	glViewport(0, 0, water.m_refractionFbo.width, water.m_refractionFbo.height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Offset the clipping plane slightly above the water to avoid black artefact lines along intersections with shallow terrain
	drawSinglePassScene(shaderProgram, viewMatrix, projMatrix, lightViewMatrix, lightProjMatrix, vec4(0, -1, 0, water.m_modelMatrix[3].y+1));

	///////////////////////////////////////////////////////////////////////////
	// Draw from camera
	///////////////////////////////////////////////////////////////////////////
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0,0,0,1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawSinglePassScene(shaderProgram, viewMatrix, projMatrix, lightViewMatrix, lightProjMatrix, vec4(0));

	
	//drawWater(viewMatrix, projMatrix);
	//drawQuad(0.5f, 0.5f);


} 

bool handleEvents(void)
{
	// check events (keyboard among other)
	SDL_Event event;
	bool quitEvent = false;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE)) {
			quitEvent = true;
		}
		if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g) {
			showUI = !showUI;
		}
		if (event.type == SDL_MOUSEMOTION && !ImGui::IsMouseHoveringAnyWindow()) {
			static int prev_xcoord = event.motion.x;
			static int prev_ycoord = event.motion.y;
			int delta_x = event.motion.x - prev_xcoord;
			int delta_y = event.motion.y - prev_ycoord;

			if (event.button.button & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				float rotationSpeed = 0.005f;
				mat4 yaw = rotate(rotationSpeed * -delta_x, worldUp);
				mat4 pitch = rotate(rotationSpeed * -delta_y, normalize(cross(cameraDirection, worldUp)));
				cameraDirection = vec3(pitch * yaw * vec4(cameraDirection, 0.0f));
			}
			prev_xcoord = event.motion.x;
			prev_ycoord = event.motion.y;
		}
	}

	// check keyboard state (which keys are still pressed)
	const uint8_t *state = SDL_GetKeyboardState(nullptr);
	vec3 cameraRight = cross(cameraDirection, worldUp);


	if (state[SDL_SCANCODE_W]) {
		cameraPosition += cameraSpeed* cameraDirection;
	}
	if (state[SDL_SCANCODE_S]) {
		cameraPosition -= cameraSpeed * cameraDirection;
	}
	if (state[SDL_SCANCODE_A]) {
		cameraPosition -= cameraSpeed * cameraRight;
	}
	if (state[SDL_SCANCODE_D]) {
		cameraPosition += cameraSpeed * cameraRight;
	}
	if (state[SDL_SCANCODE_Q]) {
		cameraPosition -= cameraSpeed * worldUp;
	}
	if (state[SDL_SCANCODE_E]) {
		cameraPosition += cameraSpeed * worldUp;
	}
	if (state[SDL_SCANCODE_P]) {
		terrain.initTerrain(octaves, scalingBias);
	}
	if (state[SDL_SCANCODE_Z]) {
		if(octaves > 2) octaves -= 2;
		printf("Octaves: %i \n", octaves);
		terrain.updateTerrain(octaves, scalingBias);
	}
	if (state[SDL_SCANCODE_X]) {
		if (octaves < 20) octaves += 2;
		printf("Octaves: %i \n", octaves);
		terrain.updateTerrain(octaves, scalingBias);
	}
	if (state[SDL_SCANCODE_C]) {
		if (scalingBias > 0.2f) scalingBias -= 0.2f;
		printf("bias: %f \n", scalingBias);
		terrain.updateTerrain(octaves, scalingBias);
	}
	if (state[SDL_SCANCODE_V]) {
		if (scalingBias < 4.0f) scalingBias += 0.2f;
		printf("bias: %f \n", scalingBias);
		terrain.updateTerrain(octaves, scalingBias);
	}
	return quitEvent;
}

void gui()
{
	// Inform imgui of new frame
	ImGui_ImplSdlGL3_NewFrame(g_window);

	// ----------------- Set variables --------------------------
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	// ----------------------------------------------------------
	// Render the GUI.
	ImGui::Render();
}

int main(int argc, char *argv[])
{
	g_window = labhelper::init_window_SDL("OpenGL Project");

	initGL();

	bool stopRendering = false;
	auto startTime = std::chrono::system_clock::now();
	

	while (!stopRendering) {
		//update currentTime
		std::chrono::duration<float> timeSinceStart = std::chrono::system_clock::now() - startTime;
		previousTime = currentTime;
		currentTime  = timeSinceStart.count();
		deltaTime    = currentTime - previousTime;
		// render to window
		display();

		// Render overlay GUI.
		if (showUI) {
			gui();
		}

		// Swap front and back buffer. This frame will now been displayed.
		SDL_GL_SwapWindow(g_window);

		// check events (keyboard among other)
		stopRendering = handleEvents();
	}
	// Free Models
	labhelper::freeModel(fighterModel);
	labhelper::freeModel(landingpadModel);
	labhelper::freeModel(sphereModel);

	// Shut down everything. This includes the window and all other subsystems.
	labhelper::shutDown(g_window);
	return 0;
}
