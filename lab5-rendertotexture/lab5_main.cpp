
#include <GL/glew.h>

// STB_IMAGE for loading images of many filetypes
#include <stb_image.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;

#include <labhelper.h>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <Model.h>
#include "hdr.h"

using std::min;
using std::max;

///////////////////////////////////////////////////////////////////////////////
// Various globals
///////////////////////////////////////////////////////////////////////////////
SDL_Window* g_window = nullptr;
float currentTime = 0.0f;

///////////////////////////////////////////////////////////////////////////////
// Shader programs
///////////////////////////////////////////////////////////////////////////////
GLuint backgroundProgram, shaderProgram, postFxShader;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
float environment_multiplier = 1.0f;
GLuint environmentMap, irradianceMap, reflectionMap;
const std::string envmap_base_name = "001";

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
float point_light_intensity_multiplier = 1000.0f;
vec3 point_light_color = vec3(1.f, 1.f, 1.f);
const vec3 lightPosition = vec3(20.0f, 40.0f, 0.0f);

///////////////////////////////////////////////////////////////////////////////
// Camera parameters.
///////////////////////////////////////////////////////////////////////////////
vec3 securityCamPos = vec3(70.0f, 50.0f, -70.0f);
vec3 securityCamDirection = normalize(-securityCamPos);
vec3 cameraPosition(-70.0f, 50.0f, 70.0f);
vec3 cameraDirection = normalize(vec3(0.0f) - cameraPosition);
float cameraSpeed = 1.0f;

vec3 worldUp(0.0f, 1.0f, 0.0f);

///////////////////////////////////////////////////////////////////////////////
// Models
///////////////////////////////////////////////////////////////////////////////
const std::string model_filename = "../scenes/NewShip.obj";
labhelper::Model *landingpadModel = nullptr;
labhelper::Model *fighterModel = nullptr;
labhelper::Model *sphereModel = nullptr;
labhelper::Model *cameraModel = nullptr;

///////////////////////////////////////////////////////////////////////////////
// Post processing effects
///////////////////////////////////////////////////////////////////////////////
enum PostProcessingEffect
{
	None = 0,
	Sepia = 1,
	Mushroom = 2,
	Blur = 3,
	Grayscale = 4,
	Composition = 5,
	Mosaic = 6,
	Separable_blur = 7,
	Bloom = 8
};

int currentEffect = PostProcessingEffect::None;
int filterSize = 1;
int filterSizes[12] = {3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25};

///////////////////////////////////////////////////////////////////////////////
// Framebuffers
///////////////////////////////////////////////////////////////////////////////

struct FboInfo;
std::vector<FboInfo> fboList;

struct FboInfo {
	GLuint framebufferId;
	GLuint colorTextureTarget;
	GLuint depthBuffer;
	int width;
	int height;
	bool isComplete;

	FboInfo(int w, int h) {
		isComplete = false;
		width = w;
		height = h;
		// Generate two textures and set filter parameters (no storage allocated yet)
		glGenTextures(1, &colorTextureTarget);
		glBindTexture(GL_TEXTURE_2D, colorTextureTarget);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glGenTextures(1, &depthBuffer);
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// allocate storage for textures
		resize(width, height);

		///////////////////////////////////////////////////////////////////////
		// Generate and bind framebuffer
		///////////////////////////////////////////////////////////////////////
		// >>> @task 1
		// check if framebuffer is complete
		isComplete = checkFramebufferComplete();

		// bind default framebuffer, just in case.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// if no resolution provided
	FboInfo() : isComplete(false)
		, framebufferId(UINT32_MAX)
		, colorTextureTarget(UINT32_MAX)
		, depthBuffer(UINT32_MAX)
		, width(0)
		, height(0)
	{};

	void resize(int w, int h) {
		width = w;
		height = h;
		// Allocate a texture
		glBindTexture(GL_TEXTURE_2D, colorTextureTarget);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		// generate a depth texture
		glBindTexture(GL_TEXTURE_2D, depthBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	}

	bool checkFramebufferComplete(void) {
		// Check that our FBO is correctly set up, this can fail if we have
		// incompatible formats in a buffer, or for example if we specify an
		// invalid drawbuffer, among things.
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			labhelper::fatal_error("Framebuffer not complete");
		}
		
		return (status == GL_FRAMEBUFFER_COMPLETE);
	}
};


void initGL()
{
	// enable Z-buffering
	glEnable(GL_DEPTH_TEST);

	// enable backface culling
	glEnable(GL_CULL_FACE);

	// Load some models.
	landingpadModel = labhelper::loadModelFromOBJ("../scenes/landingpad.obj");
	cameraModel = labhelper::loadModelFromOBJ("../scenes/camera.obj");
	fighterModel = labhelper::loadModelFromOBJ("../scenes/NewShip.obj");

	// load and set up default shader
	backgroundProgram = labhelper::loadShaderProgram("../lab5-rendertotexture/shaders/background.vert", "../lab5-rendertotexture/shaders/background.frag");
	shaderProgram     = labhelper::loadShaderProgram("../lab5-rendertotexture/shaders/simple.vert",     "../lab5-rendertotexture/shaders/simple.frag");
	postFxShader      = labhelper::loadShaderProgram("../lab5-rendertotexture/shaders/postFx.vert",     "../lab5-rendertotexture/shaders/postFx.frag");

	///////////////////////////////////////////////////////////////////////////
	// Load environment map
	///////////////////////////////////////////////////////////////////////////
	const int roughnesses = 8;
	std::vector<std::string> filenames;
	for (int i = 0; i < roughnesses; i++)
		filenames.push_back("../scenes/envmaps/" + envmap_base_name + "_dl_" + std::to_string(i) + ".hdr");

	reflectionMap = labhelper::loadHdrMipmapTexture(filenames);
	environmentMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + ".hdr");
	irradianceMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + "_irradiance.hdr");

	///////////////////////////////////////////////////////////////////////////
	// Setup Framebuffers
	///////////////////////////////////////////////////////////////////////////
	int w, h;
	SDL_GetWindowSize(g_window, &w, &h);
}

void drawScene(const mat4 &view, const mat4 &projection)
{
	glUseProgram(backgroundProgram);
	labhelper::setUniformSlow(backgroundProgram, "environment_multiplier", environment_multiplier);
	labhelper::setUniformSlow(backgroundProgram, "inv_PV", inverse(projection * view));
	labhelper::setUniformSlow(backgroundProgram, "camera_pos", cameraPosition);
	labhelper::drawFullScreenQuad();

	glUseProgram(shaderProgram);
	// Light source
	vec4 viewSpaceLightPosition = view * vec4(lightPosition, 1.0f);
	labhelper::setUniformSlow(shaderProgram, "point_light_color", point_light_color);
	labhelper::setUniformSlow(shaderProgram, "point_light_intensity_multiplier", point_light_intensity_multiplier);
	labhelper::setUniformSlow(shaderProgram, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));

	// Environment
	labhelper::setUniformSlow(shaderProgram, "environment_multiplier", environment_multiplier);

	// camera
	labhelper::setUniformSlow(shaderProgram, "viewInverse", inverse(view));

	// landing pad 
	mat4 modelMatrix(1.0f);
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", projection * view * modelMatrix);
	labhelper::setUniformSlow(shaderProgram, "modelViewMatrix", view * modelMatrix);
	labhelper::setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(view * modelMatrix)));
	
	labhelper::render(landingpadModel);

	// Fighter
	mat4 fighterModelMatrix = translate(15.0f * worldUp) * rotate(currentTime * -float(M_PI) / 4.0f, worldUp);
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", projection * view * fighterModelMatrix);
	labhelper::setUniformSlow(shaderProgram, "modelViewMatrix", view * fighterModelMatrix);
	labhelper::setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(view * fighterModelMatrix)));

	labhelper::render(fighterModel);
}



void display()
{
	///////////////////////////////////////////////////////////////////////////
	// Check if any framebuffer needs to be resized
	///////////////////////////////////////////////////////////////////////////
	int w, h;
	SDL_GetWindowSize(g_window, &w, &h);

	for (int i = 0; i < fboList.size(); i++) {
		if (fboList[i].width != w || fboList[i].height != h)
			fboList[i].resize(w, h);
	}

	///////////////////////////////////////////////////////////////////////////
	// setup matrices
	///////////////////////////////////////////////////////////////////////////
	mat4 securityCamViewMatrix = lookAt(securityCamPos, securityCamPos + securityCamDirection, worldUp);
	mat4 securityCamProjectionMatrix = perspective(radians(30.0f), float(w) / float(h), 15.0f, 1000.0f);

	mat4 projectionMatrix = perspective(radians(45.0f), float(w) / float(h), 10.0f, 1000.0f);
	mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraDirection, worldUp);

	///////////////////////////////////////////////////////////////////////////
	// Bind the environment map(s) to unused texture units
	///////////////////////////////////////////////////////////////////////////
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, environmentMap);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, irradianceMap);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, reflectionMap);

	///////////////////////////////////////////////////////////////////////////
	// draw scene from security camera
	///////////////////////////////////////////////////////////////////////////
	// >>> @task 2
	// ...

	///////////////////////////////////////////////////////////////////////////
	// draw scene from camera
	///////////////////////////////////////////////////////////////////////////
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // to be replaced with another framebuffer when doing post processing
	glViewport(0, 0, w, h);
	glClearColor(0.2, 0.2, 0.8, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawScene(viewMatrix, projectionMatrix); // using both shaderProgram and backgroundProgram

	// camera (obj-model)
	glUseProgram(shaderProgram);
	labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * inverse(securityCamViewMatrix) * scale(vec3(10.0f)) * rotate(float(M_PI), vec3(0.0f, 1.0, 0.0)));
	labhelper::setUniformSlow(shaderProgram, "modelViewMatrix", viewMatrix * inverse(securityCamViewMatrix));
	labhelper::setUniformSlow(shaderProgram, "normalMatrix", inverse(transpose(viewMatrix * inverse(securityCamViewMatrix))));
	
	labhelper::render(cameraModel);

	///////////////////////////////////////////////////////////////////////////
	// Post processing pass(es)
	///////////////////////////////////////////////////////////////////////////
	glUseProgram( 0 );

	CHECK_GL_ERROR();
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
		if (event.type == SDL_MOUSEMOTION && !ImGui::IsMouseHoveringAnyWindow()) {
			// More info at https://wiki.libsdl.org/SDL_MouseMotionEvent
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

			if (event.button.button & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
				float rotationSpeed = 0.005f;
				mat4 yaw = rotate(rotationSpeed * -delta_x, worldUp);
				mat4 pitch = rotate(rotationSpeed * -delta_y, normalize(cross(securityCamDirection, worldUp)));
				securityCamDirection = vec3(pitch * yaw * vec4(securityCamDirection, 0.0f));
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

	return quitEvent;
}

void gui()
{
	// Inform imgui of new frame
	ImGui_ImplSdlGL3_NewFrame(g_window);

	// ----------------- Set variables --------------------------
	ImGui::Text("Post-processing effect");
	ImGui::RadioButton("None", &currentEffect, PostProcessingEffect::None);
	ImGui::RadioButton("Sepia", &currentEffect, PostProcessingEffect::Sepia);
	ImGui::RadioButton("Mushroom", &currentEffect, PostProcessingEffect::Mushroom);
	ImGui::RadioButton("Blur", &currentEffect, PostProcessingEffect::Blur);
	ImGui::SameLine();
	ImGui::SliderInt("Filter size", &filterSize, 1, 12);
	ImGui::RadioButton("Grayscale", &currentEffect, PostProcessingEffect::Grayscale);
	ImGui::RadioButton("All of the above", &currentEffect, PostProcessingEffect::Composition);
	ImGui::RadioButton("Mosaic", &currentEffect, PostProcessingEffect::Mosaic);
	ImGui::RadioButton("Separable Blur", &currentEffect, PostProcessingEffect::Separable_blur);
	ImGui::RadioButton("Bloom", &currentEffect, PostProcessingEffect::Bloom);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	// ----------------------------------------------------------

	// Render the GUI.
	ImGui::Render();
}

int main(int argc, char *argv[])
{
	g_window = labhelper::init_window_SDL("OpenGL Lab 5");

	initGL();

	bool stopRendering = false;
	auto startTime = std::chrono::system_clock::now();

	while (!stopRendering) {
		//update currentTime
		std::chrono::duration<float> timeSinceStart = std::chrono::system_clock::now() - startTime;
		currentTime = timeSinceStart.count();

		// render to window
		display();

		// Render overlay GUI.
		//gui();

		// Swap front and back buffer. This frame will now been displayed.
		SDL_GL_SwapWindow(g_window);

		// check events (keyboard among other)
		stopRendering = handleEvents();
	}

	// Free Models
	labhelper::freeModel(landingpadModel);
	labhelper::freeModel(cameraModel);
	labhelper::freeModel(fighterModel);
	labhelper::freeModel(sphereModel);

	// Shut down everything. This includes the window and all other subsystems.
	labhelper::shutDown(g_window);
	return 0;
}
