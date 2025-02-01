//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#include <iostream>
#include <algorithm>

#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib") 

int glWindowWidth = 3072;
int glWindowHeight = 1920;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const unsigned int SHADOW_WIDTH = 16384;
const unsigned int SHADOW_HEIGHT = 16384;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;


gps::Camera myCamera(
	glm::vec3(0.0f, 5.737f, 5.5f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.14f;

bool pressedKeys[1024];
float angleY = 0.0f;
GLfloat lightAngle;

gps::Model3D scene;
gps::Model3D ground;
gps::Model3D lightCube;
gps::Model3D screenQuad;
gps::Model3D tankChasis;
gps::Model3D tankTurret;
gps::Model3D glass;
gps::Model3D glass2;
gps::Model3D alien;
gps::Model3D cloud;
gps::Model3D thunder;

gps::Shader myCustomShader;
gps::Shader myCustomShader2;

gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

bool showDepthMap;

float turretRotationAngle = 180.0f;
float turretRotationTarget = 180.0f; 
float turretRotationSpeed = 8.0f; 
float turretRotationProgress = 0.0f; 
float turretRotationStart = 180.0f; 
float rotationWaitTime = 0.0f; 
bool isRotating = true; 

float shipRotationAngle = 0.0f; 
float shipRotationSpeed = 20.0f; 

GLuint textureID;

gps::SkyBox mySkyBox;
gps::Shader skyboxShader;
std::vector<const GLchar*> faces;

struct PointLightData {
	glm::vec3 position;
	glm::vec3 color;
	float constant;
	float linear;
	float quadratic;
};

struct Flashlight {
	glm::vec3 position;     
	glm::vec3 direction;   
	float cutOff;     
	float outerCutOff; 
	glm::vec3 color;     
	bool activated;
};

struct SpotLight {
	glm::vec3 position;
	glm::vec3 direction;
	float cutOff;
	float outerCutOff;
	glm::vec3 color;
};

PointLightData pointLights[3] = {
	{ glm::vec3(-22.306f, 2.5047f, -73.507f), glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 0.14f, 0.07f },
	{ glm::vec3(-41.567f, 7.7917f, -36.565f), glm::vec3(1.0f, 0.0f, 1.0f),  1.0f, 0.14f, 0.07f },
	{ glm::vec3(-4.2532f, 7.9324f, -36.532f), glm::vec3(1.0f, 0.0f, 1.0f),  1.0f, 0.14f, 0.07f },
};

SpotLight spotLights[2] = {
	{ glm::vec3(-16.58f, 14.26f, -67.669f), glm::vec3(0.177492f, -0.314493f, -0.932518f), glm::cos(glm::radians(10.0f)), glm::cos(glm::radians(15.0f)), glm::vec3(1.0f, 1.0f, 1.0f) },
	{ glm::vec3(-26.452f, 14.014f, -67.323f), glm::vec3(-0.324584f, -0.409992f, -0.85238f), glm::cos(glm::radians(10.0f)), glm::cos(glm::radians(15.0f)), glm::vec3(1.0f, 1.0f, 1.0f) }
};

Flashlight flashlight = {
	myCamera.getCameraPosition(),  
	myCamera.getCameraDirection(), 
	glm::cos(glm::radians(12.5f)),                   
	glm::cos(glm::radians(17.5f)),
	glm::vec3(1.0f, 1.0f, 1.0f),
	true
};

enum RenderMode { Punct, WIREFRAME, SOLID };
RenderMode currentMode = SOLID;

GLenum glCheckError_(const char* file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO	
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

float lastX = 400, lastY = 300; 
bool firstMouse = true;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xOffset = xpos - lastX;
	float yOffset = lastY - ypos; 
	lastX = xpos;
	lastY = ypos;

	const float sensitivity = 0.08f;
	xOffset *= sensitivity;
	yOffset *= sensitivity;

	myCamera.rotate(yOffset, xOffset);
}

void updateFlashlightPositionAndDirection() {
	flashlight.position = myCamera.getCameraPosition();
	flashlight.direction = myCamera.getCameraDirection();
}

glm::vec3 points[] = {
	glm::vec3(0.0f, 5.737f, 5.5f),
	glm::vec3(-3.4723f, 5.737f, -21.084f),
	glm::vec3(-5.9421f, 4.302f, -29.299f),
	glm::vec3(-2.5874f, 3.3309f, -49.548f),
	glm::vec3(-2.5874f, 5.1649f, -73.176f),
	glm::vec3(-2.5874f, 20.946f, -103.81f),
	glm::vec3(-13.938f, 20.946f, -122.15f)
};

int currentPoint = 0; 
float animationStartTime = 0.0f; 
bool isAnimating = false; 
float movementDuration = 5.0f;
bool isRotatingCamera = false;
float rotationDuration = 3.0f; 
bool isWaitingForRotation = false;
float rotationStartTime = 0.0f;
float totalRotationYawSoFar = 0.0f;
glm::vec3 currentPos, direction;

bool lightToggle = false;
bool thunderToggle = false;

float lastLightningTime = 0.0f; 
float lightningDuration = 0.2f;  
float nextLightningInterval = 15; 

void updateCameraAnimation() {
	if (isAnimating) {
		if (!isRotatingCamera) {
			float t = (glfwGetTime() - animationStartTime) / movementDuration;
			if (t > 1.0f) {
				t = 0.0f;
				currentPoint++;
				if (currentPoint >= sizeof(points) / sizeof(points[0]) - 1) {
					isRotatingCamera = true;
					rotationStartTime = glfwGetTime();
					totalRotationYawSoFar = 0.0f;
				}
				animationStartTime = glfwGetTime();
			}
			if (currentPoint < sizeof(points) / sizeof(points[0]) - 1) {
				currentPos = points[currentPoint] + t * (points[currentPoint + 1] - points[currentPoint]);
				myCamera.setPosition(currentPos);
				direction = glm::normalize(points[currentPoint + 1] - currentPos);
				myCamera.setDirection(direction);
			}
		}
		if (isRotatingCamera) {
			float elapsedRotationTime = glfwGetTime() - rotationStartTime;
			if (totalRotationYawSoFar < 180.0f) {
				float rotationStepYaw = (180.0f * elapsedRotationTime / rotationDuration) - totalRotationYawSoFar;
				myCamera.rotate(0.0f, rotationStepYaw);
				direction = glm::normalize(glm::vec3(
					cos(glm::radians(myCamera.getYaw())) * cos(glm::radians(45.0f)),
					0.0f, sin(glm::radians(myCamera.getYaw())) * cos(glm::radians(45.0f))));
				myCamera.setDirection(direction);
				totalRotationYawSoFar += rotationStepYaw;
			}
			else {
				direction = glm::normalize(glm::vec3(
					cos(glm::radians(myCamera.getYaw())) * cos(glm::radians(45.0f)),
					sin(glm::radians(-10.0f)), sin(glm::radians(myCamera.getYaw())) * cos(glm::radians(45.0f))));
				myCamera.setDirection(direction);
				isRotatingCamera = false;
				isAnimating = false;
			}
		}
	}
}
float posx, posy;

void processLightning() {
	float currentTime = static_cast<float>(glfwGetTime());
	if (thunderToggle) {
		if (currentTime - lastLightningTime > lightningDuration) {
			thunderToggle = false;
			nextLightningInterval = (std::rand() % 12 + 5);
		}
	}
	else {
		if (currentTime - lastLightningTime > nextLightningInterval) {
			thunderToggle = true;
			posx = (std::rand() % 221 - 110);
			posy = (std::rand() % 60 + 187);
			lastLightningTime = currentTime; 
		}
	}
	myCustomShader.useShaderProgram();
	GLint showThunderLoc = glGetUniformLocation(myCustomShader.shaderProgram, "showThunder");
	glUniform1i(showThunderLoc, thunderToggle);
}

void processMovement()
{
	if (pressedKeys[GLFW_KEY_Q]) {
		angleY -= 1.0f;
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angleY += 1.0f;
	}

	if (pressedKeys[GLFW_KEY_J]) {
		lightAngle -= 1.0f;
		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle += 1.0f;
		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}
	
	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}
	if (pressedKeys[GLFW_KEY_Z]) {
		myCamera.rotate(0.0f, -1.0f);  
	}
	if (pressedKeys[GLFW_KEY_C]) {
		myCamera.rotate(0.0f, 1.0f); 
	}

	if (pressedKeys[GLFW_KEY_X]) {
		lightToggle = !lightToggle;
	}

	if (pressedKeys[GLFW_KEY_V]) {
		turretRotationAngle += 15.0f; 
		if (turretRotationAngle >= 360.0f) {
			turretRotationAngle -= 360.0f; 
		}
	}

	if (pressedKeys[GLFW_KEY_T]) {
		glm::vec3 direction = myCamera.getCameraDirection();
		std::cout << "Camera Direction: ("
			<< direction.x << ", "
			<< direction.y << ", "
			<< direction.z << ")" << std::endl;
	}

	if (pressedKeys[GLFW_KEY_F]) {
		flashlight.activated = !flashlight.activated;
	}

	if (pressedKeys[GLFW_KEY_1]) { 
		currentMode = Punct;
	}
	if (pressedKeys[GLFW_KEY_2]) { 
		currentMode = WIREFRAME;
	}
	if (pressedKeys[GLFW_KEY_3]) {
		currentMode = SOLID;
	}

	if (isRotating) {
		float deltaRotation = turretRotationTarget - turretRotationAngle;
		if (fabs(deltaRotation) > 0.1f) {  
			turretRotationAngle += (deltaRotation > 0.0f ? 1 : -1) * turretRotationSpeed * 0.016f;
		}
		else {
			turretRotationAngle = turretRotationTarget;  
			isRotating = false;
			rotationWaitTime = 15.0f; 
		}
	}
	if (rotationWaitTime > 0.0f) {
		rotationWaitTime -= 0.016f;
		if (rotationWaitTime <= 0.0f) {
			turretRotationStart = turretRotationAngle;
			turretRotationTarget = turretRotationStart + (rand() % 181 - 90); 
			isRotating = true; 
		}
	}

	shipRotationAngle += shipRotationSpeed * 0.016f; 
	if (shipRotationAngle >= 360.0f) {
		shipRotationAngle -= 360.0f; 
	}

	if (pressedKeys[GLFW_KEY_SPACE] && !isAnimating) {
		isAnimating = true;
		animationStartTime = glfwGetTime(); 
		currentPoint = 0; 
	}

	updateCameraAnimation();
	updateFlashlightPositionAndDirection();
	processLightning();
}

void setRenderingMode() {
	switch (currentMode) {
		case Punct:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case WIREFRAME:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case SOLID:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
	}
}

bool initOpenGLWindow() {
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Window scaling for HiDPI displays
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	// For sRGB framebuffer
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

	// For anti-aliasing
	glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);

	// Lock the mouse inside the window and disable the cursor
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

#if not defined (_APPLE_)
	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();
#endif

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	// For retina display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initSkybox() {
	faces.push_back("skybox/nightsky_rt.tga");
	faces.push_back("skybox/nightsky_lf.tga");
	faces.push_back("skybox/nightsky_up.tga");
	faces.push_back("skybox/nightsky_dn.tga");
	faces.push_back("skybox/nightsky_bk.tga");
	faces.push_back("skybox/nightsky_ft.tga");
	mySkyBox.Load(faces);
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

struct TransparentObject {
	gps::Model3D* object; 
	glm::vec3 position;   
	glm::vec3 scale;
	float depth;          
};

float calculateDistanceToCamera(const glm::vec3& position, const glm::vec3& cameraPosition) {
	return glm::length(position - cameraPosition);  
}

std::vector<TransparentObject> transparentObjects;

void populateTransparentObjects() {
	transparentObjects.clear();
	glm::vec3 cameraPosition = myCamera.getCameraPosition();

	TransparentObject obj1 = { &glass, glm::vec3(-4.121f, 1.9089f, -49.339f), glm::vec3(0.210f), calculateDistanceToCamera(glm::vec3(-4.121f, 1.9089f, -49.339f), cameraPosition) };
	TransparentObject obj2 = { &glass2, glm::vec3(-4.6064f, 1.9629f, -49.202f), glm::vec3(0.210f), calculateDistanceToCamera(glm::vec3(-4.6064f, 1.9629f, -49.202f), cameraPosition) };
	TransparentObject obj3 = { &alien, glm::vec3(70.019f, 32.527f, -84.426f), glm::vec3(1.000f), calculateDistanceToCamera(glm::vec3(70.019f, 32.527f, -84.426f), cameraPosition) };
	TransparentObject obj4 = { &cloud, glm::vec3(1.504f, 85.709f, -217.0f), glm::vec3(15.0f,  8.811f, 19.635f), calculateDistanceToCamera(glm::vec3(70.019f, 32.527f, -84.426f), cameraPosition) };
	TransparentObject obj5 = { &cloud, glm::vec3(91.504f, 85.709f, -217.0f), glm::vec3(15.0f,  8.811f, 19.635f), calculateDistanceToCamera(glm::vec3(70.019f, 32.527f, -84.426f), cameraPosition) };
	TransparentObject obj6 = { &cloud, glm::vec3(-91.504f, 85.709f, -217.0f), glm::vec3(15.0f,  8.811f, 19.635f), calculateDistanceToCamera(glm::vec3(70.019f, 32.527f, -84.426f), cameraPosition) };

	transparentObjects.push_back(obj1);
	transparentObjects.push_back(obj2);
	transparentObjects.push_back(obj3);
	transparentObjects.push_back(obj4);
	transparentObjects.push_back(obj5);
	transparentObjects.push_back(obj6);

	std::sort(transparentObjects.begin(), transparentObjects.end(), [](const TransparentObject& a, const TransparentObject& b) {
		return a.depth > b.depth;});
}


void initObjects() {
	scene.LoadModel("objects/scene/scena.obj");
	tankChasis.LoadModel("objects/tankChasis/tankChasis.obj");
	tankTurret.LoadModel("objects/turret/turret.obj");
	lightCube.LoadModel("objects/cube/cube.obj");
	screenQuad.LoadModel("objects/quad/quad.obj");
	glass.LoadModel("objects/bottles/bottle1.obj");
	glass2.LoadModel("objects/bottles/bottle2.obj");
	alien.LoadModel("objects/alienship/alien.obj");
	cloud.LoadModel("objects/cloud/untitled.obj");
	thunder.LoadModel("objects/thunder/thunder.obj");
	initSkybox();
}


void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	myCustomShader2.loadShader("shaders/shaderStart2.vert", "shaders/shaderStart2.frag");
	myCustomShader2.useShaderProgram();
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	lightShader.useShaderProgram();
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();
	depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
	depthMapShader.useShaderProgram();
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	
	lightDir = glm::vec3(0.0f, 30.0f, -150.0f);
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	for (int i = 0; i < 3; ++i) {
		std::string baseName = "pointLights[" + std::to_string(i) + "].";

		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "position").c_str()), 1, glm::value_ptr(pointLights[i].position));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "color").c_str()), 1, glm::value_ptr(pointLights[i].color));
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "constant").c_str()), pointLights[i].constant);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "linear").c_str()), pointLights[i].linear);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "quadratic").c_str()), pointLights[i].quadratic);
	}

	for (int i = 0; i < 2; ++i) {  
		std::string baseName = "spotLights[" + std::to_string(i) + "].";
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "position").c_str()), 1, glm::value_ptr(spotLights[i].position));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "direction").c_str()), 1, glm::value_ptr(spotLights[i].direction));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "color").c_str()), 1, glm::value_ptr(spotLights[i].color));
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "cutOff").c_str()), spotLights[i].cutOff);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "outerCutOff").c_str()), spotLights[i].outerCutOff);
	}


	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.position"), 1, glm::value_ptr(myCamera.getCameraPosition()));
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.direction"), 1, glm::value_ptr(myCamera.getCameraDirection()));
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.cutOff"), flashlight.cutOff);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.outerCutOff"), flashlight.outerCutOff);
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.color"), 1, glm::value_ptr(flashlight.color));
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.activated"), flashlight.activated);

	myCustomShader2.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	lightDir = glm::vec3(0.0f, 30.0f, -150.0f);
	lightColor = glm::vec3(0.0f, 1.0f, 0.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	for (int i = 0; i < 3; ++i) {
		std::string baseName = "pointLights[" + std::to_string(i) + "].";

		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "position").c_str()), 1, glm::value_ptr(pointLights[i].position));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "color").c_str()), 1, glm::value_ptr(pointLights[i].color));
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "constant").c_str()), pointLights[i].constant);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "linear").c_str()), pointLights[i].linear);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + "quadratic").c_str()), pointLights[i].quadratic);
	}

	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.position"), 1, glm::value_ptr(myCamera.getCameraPosition()));
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.direction"), 1, glm::value_ptr(myCamera.getCameraDirection()));
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.cutOff"), flashlight.cutOff);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.outerCutOff"), flashlight.outerCutOff);
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.color"), 1, glm::value_ptr(flashlight.color));
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "flashlight.activated"), flashlight.activated);
	
	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

}

void initFBO() {
	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
	glGenFramebuffers(1, &shadowMapFBO);
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
		0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	//TODO - Return the light-space transformation matrix
	glm::mat4 lightView = glm::lookAt((glm::inverseTranspose(glm::mat3(lightRotation)) * lightDir), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const GLfloat near_plane = -300.1f, far_plane = 300.0f;
	glm::mat4 lightProjection = glm::ortho(-200.0f, 200.0f, -200.0f, 200.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView ;
	return lightSpaceTrMatrix;
}


void drawObjects(gps::Shader shader, bool depthPass) {

	shader.useShaderProgram();

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	scene.Draw(shader);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-22.27f, 2.36f, -74.339f));
	model = glm::scale(model, glm::vec3(0.800f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	tankChasis.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-22.306f, 1.8247f, -73.507f));
	model = glm::scale(model, glm::vec3(0.800f));
	model = glm::rotate(model, glm::radians(turretRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 1.654f)); 

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	tankTurret.Draw(shader);

	if (thunderToggle) {
		
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(posx, 5.86f, -posy));
		model = glm::scale(model, glm::vec3(4.246f, 14.461f, 3.346f));

		glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		if (!depthPass) {
			normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
			glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
		}

		thunder.Draw(shader);
	}
}

void drawTransparentObjects(gps::Shader shader, bool depthPass) {
	shader.useShaderProgram();

	for (const TransparentObject& obj : transparentObjects) {
		model = glm::mat4(1.0f);
		model = glm::translate(model, obj.position);
		if (obj.scale == glm::vec3(1.000f)) {
			model = glm::rotate(model, glm::radians(shipRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		}
		model = glm::scale(model, obj.scale);

		glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
		if (!depthPass) {
			normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
			glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
		}
		obj.object->Draw(shader);
	}
}

void renderScene() {

	// depth maps creation pass
	//TODO - Send the light-space transformation matrix to the depth map creation shader and
	//		 render the scene in the depth map

	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawObjects(depthMapShader, true);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// render depth map on screen - toggled with the M key
	if (showDepthMap) {
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
	}
	else {
		// final scene rendering pass (with shadows)

		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		gps::Shader currentShader;

		if (lightToggle) {
			currentShader = myCustomShader2;
			lightColor = glm::vec3(0.0f, 1.0f, 0.0f);
		}
		else {
			currentShader = myCustomShader;
			lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		}

		currentShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		lightDirLoc = glGetUniformLocation(currentShader.shaderProgram, "lightDir");
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

		lightColorLoc = glGetUniformLocation(currentShader.shaderProgram, "lightColor");
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));

		glUniform3fv(glGetUniformLocation(currentShader.shaderProgram, "flashlight.position"), 1, glm::value_ptr(flashlight.position));
		glUniform3fv(glGetUniformLocation(currentShader.shaderProgram, "flashlight.direction"), 1, glm::value_ptr(flashlight.direction));
		glUniform1i(glGetUniformLocation(currentShader.shaderProgram, "flashlight.activated"), flashlight.activated);

		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(currentShader.shaderProgram, "shadowMap"), 3);

		glUniformMatrix4fv(glGetUniformLocation(currentShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(computeLightSpaceTrMatrix()));

		drawObjects(currentShader, false);

		populateTransparentObjects();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		
		drawTransparentObjects(currentShader, false);
		glDisable(GL_BLEND);
		
		glUniform3fv(glGetUniformLocation(currentShader.shaderProgram, "flashlight.position"), 1, glm::value_ptr(flashlight.position));
		glUniform3fv(glGetUniformLocation(currentShader.shaderProgram, "flashlight.direction"), 1, glm::value_ptr(flashlight.direction));
		setRenderingMode();

		//draw a white cube around the light
		lightShader.useShaderProgram();

		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		model = lightRotation;
		model = glm::translate(model, 1.0f * lightDir);
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);

		mySkyBox.Draw(skyboxShader, view, projection);
	}
}

void cleanup() {
	glDeleteTextures(1, &depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}

int main(int argc, const char* argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();
	initFBO();

	PlaySound(L"sound/thunder-sounds-long-73277.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {
		processMovement();
		renderScene();
		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();
	PlaySound(NULL, NULL, 0);

	return 0;
}
