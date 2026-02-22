#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>
#include <vector>

using namespace std;

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

struct Door {
    gps::Model3D model;
    glm::vec3 balama;
    bool closeActivated = false;
    double closeAt = 0.0;
    float angle = 0.0f;
    bool open = false;

    AABB doorColliders;
};

struct FloorPatch {
    glm::vec3 min;
    glm::vec3 max;
    float topY; // suprafata podelei
};

std::vector<AABB> colliders;
std::vector<FloorPatch> floors;

float playerRadius = 0.15f;

float eyeHeight = 1.7f; // camera sta deasupra la podea cu 1.7m
float playerHeight = 1.7f;
float vitezaY = 0.0f;
float gravity = 9.81f;

// miniaudio
ma_engine audioEngine;
ma_sound monsterSound;
bool audio = false;
float monsterMaxDistance = 10.0f;  // distanta la care sa fie minim volumu
float monsterMinDistance = 1.5f;  // distanta la care sa fie maxim volumu
float monsterMaxVolume = 0.9f;
float currentVolume = 0.0f;

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
// lanterna
GLint flashPosLoc;
GLint flashDirLoc;
GLint flashColorLoc;
GLint unghiInterior;
GLint unghiExterior;
GLint constantAttLoc;
GLint linearAttLoc;
GLint quadraticAttLoc;

// lampa
GLint lampOnLoc;
GLint lampPosLoc;
GLint lampColorLoc;
GLint lampConstLoc;
GLint lampLinearLoc;
GLint lampQuadLoc;

glm::vec3 lampOffset(0.0f);
float lampScale = 1.0f;
float lampRotY = 0.0f;

// umbre pe flashlight
unsigned int SHADOW_W = 1024;
unsigned int SHADOW_H = 1024;

gps::Shader shadowShader;
GLuint depthFBO = 0; // framebuffer care scrie in depthMap
GLuint depthMap = 0; // textura de adancime
GLint shadowModelLoc;
GLint shadowLightSpaceLoc;

// in shaderul basic 
GLint lightSpaceMatrixLoc;
GLint shadowMapLoc;

glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
glm::vec3 flashPosWorld = glm::vec3(0.0f);
glm::vec3 flashDirWorld = glm::vec3(0.0f, 0.0f, -1.0f);


int gPolyMode = 0;         // 1, 2, 3
int gShadingMode = 0;      // 0 smooth, 1 flat
GLint shadingModeLoc = -1; // uniform location


// camera
gps::Camera myCamera(
    glm::vec3(29.393f, 16.0f, 13.0f),   // Camera position
    glm::vec3(0.0f, 0.0f, -10.0f),  // Camera Target
    glm::vec3(0.0f, 1.0f, 0.0f));   // Up vector

GLfloat cameraSpeed = 0.025f;

GLboolean pressedKeys[1024];

// models
gps::Model3D spital;
std::vector<Door> doors;
gps::Model3D lampa;
GLfloat angle;


// Pt miscare cu mausul
bool firstMouse = true;
float lastX = 1024.0f / 2.0;
float lastY = 768.0f / 2.0;
float yaw = -90.f;    // rotatie in dreapta si in stg (a avionului)
float pitch = 0.0f;   // rotatie in sus si in jos (a avionului)

// animatie de prezentare
bool presentationMode = false;
float tourTime = 0.0f;

glm::vec3 tourCenter = glm::vec3(15.0f, 10.0f, 0.0f);
float tourRadius = 12.0f;
float tourHeight = 2.5f;
float tourSpeed = 0.5f; // radiani/sec


// pt usa
gps::Model3D modelUsa;
float unghiUsa = 0.0f;
bool seDeschide = false;

// Monstru
gps::Model3D monstru;
glm::mat4 monstruModel = glm::mat4(1.0f);
glm::vec3 monstruScale = glm::vec3(1.0);
glm::vec3 monstruPos = glm::vec3(8.1208585f, 8.2434795f, -10.987354f);
float monstruYaw = 0.0f;
float monstruYawOffset = -90.0f;

// Lampa
glm::mat4 lampModel = glm::mat4(1.0f);
glm::vec3 lampWorldPos;
bool lampOn = false;

// shaders
gps::Shader myBasicShader;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    // permite sa tin apasta o tasta incontinuu (pe W/A/S/D )
	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        lampOn = !lampOn;
    }

    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        gShadingMode = 1 - gShadingMode;
    }

    // presentation mode
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        presentationMode = !presentationMode;
        tourTime = 0.0f;
        firstMouse = true; // ca sa nu sara camera cand revin pe mouse

        // reset pozitie
        if (!presentationMode) {  
            myCamera.setPosition(glm::vec3(29.393f, 16.0f, 13.0f)); // pozitia default

            // sa puna view de la camera inapoi, unde e noua pozitie (adica inapoi la inceputul jocului)
            view = myCamera.getViewMatrix();
            myBasicShader.useShaderProgram();
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        }
    }

}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        
        double now = glfwGetTime();

        for (Door& d : doors) {
            bool newState = !d.open;
            d.open = newState;

            if (newState == true) { // s-a deschis acuma
                d.closeActivated = true;
                d.closeAt = now + 3.0;
            }
            else {  // l-am inchis eu, manual
                d.closeActivated = false;
            }
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos; // inversam pt ca axa Y e inversat in openGL (y creste in jos)
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw += xOffset;
    pitch += yOffset;

    // limietz unghiul sa nu se dea peste cap
    if (pitch > 89.0f)
        pitch = 89.f;
    if (pitch < -89.0f)
        pitch = -89.f;

    myCamera.rotate(pitch, yaw);

    // Matricea de vizualizare (view)
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Actualizez si matricea normala pt lumini (se schimba view : World -> View/Eye space
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model)); // normala se transforma cu inversa transpusa pt a ramane perpendiculare pe suprafete in scalarile sa ramana
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
}

// Coliziunile
bool intersectionsAABB_XYZ(const glm::vec3& playerPos, float playerR, float h, const AABB& box) {
    
    float playerMinY = playerPos.y - h; 
    float playerMaxY = playerPos.y; // ochiul camerei

    // playerMaxY < box.min.y => jucatorul e sub cutie
    // playerMinY > box.max.y => jucatorul e deasupra cutiei
    // cutia [0,2], jucator [1,3] => se suprapun, altfel NU
    bool suprapunereY = (playerMaxY >= box.min.y) && (playerMinY <= box.max.y);
    if (suprapunereY == false) return false; // nu e coliziune
    
    // clamp(value, min, max) 
    //       --> value < min => value = min
    //           value > max => value = max
    //           else value = value
    float nearestPointFromBoxToPlayerX = glm::clamp(playerPos.x, box.min.x, box.max.x);
    float nearestPointFromBoxToPlayerZ = glm::clamp(playerPos.z, box.min.z, box.max.z);

    // cat de departe sunt de acel BOX
    float playerPosToBoxX = playerPos.x - nearestPointFromBoxToPlayerX;
    float playerPosToBoxZ = playerPos.z - nearestPointFromBoxToPlayerZ;

    float dx = playerPosToBoxX;
    float dz = playerPosToBoxZ;

    // Formula distantei sqrt(dx^2 + dz^2) < r (daca e mai mic, inseamna ca intersectam cutia)
    return (dx * dx + dz * dz) < (playerR * playerR);
}

bool colliders_XZ(const glm::vec3& playerPos) {
    for (AABB& box : colliders) {
        if (intersectionsAABB_XYZ(playerPos, playerRadius, playerHeight, box))
            return true;
    }

    for (Door& door : doors) {
        if (door.open) continue;
        if (door.angle > 5.0f) continue; // aproape inchisa, dar inca las sa treaca

        if (intersectionsAABB_XYZ(playerPos, playerRadius, playerHeight, door.doorColliders)) {
            return true;
        }
    }
    return false;
}

bool tryMoveWithCollision(gps::MOVE_DIRECTION dir, float speed) {
    glm::vec3 oldPos = myCamera.getPosition();

    myCamera.move(dir, speed);

    if (colliders_XZ(myCamera.getPosition())) {
        myCamera.setPosition(oldPos);
        return false;  // nu am reusit miscarea
    }

    return true; // am resuit sa ma misc
}

void processMovement() {

    bool moved = false;

    if (pressedKeys[GLFW_KEY_W]) moved |= tryMoveWithCollision(gps::MOVE_FORWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_S]) moved |= tryMoveWithCollision(gps::MOVE_BACKWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_A]) moved |= tryMoveWithCollision(gps::MOVE_LEFT, cameraSpeed);
    if (pressedKeys[GLFW_KEY_D]) moved |= tryMoveWithCollision(gps::MOVE_RIGHT, cameraSpeed);

    if (moved == true) {
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    static bool prev1 = false, prev2 = false, prev3 = false;

    bool now1 = pressedKeys[GLFW_KEY_1];
    bool now2 = pressedKeys[GLFW_KEY_2];
    bool now3 = pressedKeys[GLFW_KEY_3];


    if (now1 && !prev1) gPolyMode = 0; // solid
    if (now2 && !prev2) gPolyMode = 1; // wireframe
    if (now3 && !prev3) gPolyMode = 2; // points / poligonal

    prev1 = now1; 
    prev2 = now2; 
    prev3 = now3;



    // Lampa statica

    // mutare lampa (I/J/K/L, U/O pe Y)
    if (pressedKeys[GLFW_KEY_J]) lampOffset.x -= 0.02f;
    if (pressedKeys[GLFW_KEY_L]) lampOffset.x += 0.02f;
    if (pressedKeys[GLFW_KEY_I]) lampOffset.z -= 0.02f;
    if (pressedKeys[GLFW_KEY_K]) lampOffset.z += 0.02f;
    if (pressedKeys[GLFW_KEY_U]) lampOffset.y += 0.02f;
    if (pressedKeys[GLFW_KEY_O]) lampOffset.y -= 0.02f;

    // scalare (N/M)
    if (pressedKeys[GLFW_KEY_N]) lampScale -= 0.01f;
    if (pressedKeys[GLFW_KEY_M]) lampScale += 0.01f;
    lampScale = glm::clamp(lampScale, 0.2f, 3.0f);

    // rotatie lampa (Z/X)
    if (pressedKeys[GLFW_KEY_Z]) lampRotY -= 1.0f;
    if (pressedKeys[GLFW_KEY_X]) lampRotY += 1.0f;



    // restul codului default

	//if (pressedKeys[GLFW_KEY_W]) {
	//	myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	//	//update view matrix
 //       view = myCamera.getViewMatrix();
 //       myBasicShader.useShaderProgram();
 //       glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
 //       // compute normal matrix for teapot
 //       normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//}

	//if (pressedKeys[GLFW_KEY_S]) {
	//	myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
 //       //update view matrix
 //       view = myCamera.getViewMatrix();
 //       myBasicShader.useShaderProgram();
 //       glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
 //       // compute normal matrix for teapot
 //       normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//}

	//if (pressedKeys[GLFW_KEY_A]) {
	//	myCamera.move(gps::MOVE_LEFT, cameraSpeed);
 //       //update view matrix
 //       view = myCamera.getViewMatrix();
 //       myBasicShader.useShaderProgram();
 //       glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
 //       // compute normal matrix for teapot
 //       normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//}

	//if (pressedKeys[GLFW_KEY_D]) {
	//	myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
 //       //update view matrix
 //       view = myCamera.getViewMatrix();
 //       myBasicShader.useShaderProgram();
 //       glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
 //       // compute normal matrix for teapot
 //       normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//}

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }


    // animatie pentru usa
    for (Door& d : doors) { 
        if (d.open && d.angle < 80.0f) d.angle += 1.0f;
        if (!d.open && d.angle > 0.0f) d.angle -= 1.0f;
    }
}

float getFloorGroundY(glm::vec3& playerPos, bool* gasit) {
    *gasit = false;
    float best = -99999;

    float feetY = playerPos.y - eyeHeight;

    for (FloorPatch& f : floors) {
        if (playerPos.x >= f.min.x && playerPos.x <= f.max.x && playerPos.z >= f.min.z && playerPos.z <= f.max.z) {

            // daca sunt mai multe pach, il luam pe cel de sub picioare
            if (f.topY <= feetY + 1.0f) {
                if (*gasit == false || f.topY > best) {
                    best = f.topY;
                    *gasit = true;
                }
            }
        }
    }
    return best;
}

void putPlayerOnGround(float delta) {
    glm::vec3 pos = myCamera.getPosition();

    bool podea = false;
    float groundY = getFloorGroundY(pos, &podea);

    float targetY = -9999;
    if (podea == true) {
        targetY = groundY + eyeHeight;


        // ma fixez pe podea daca exista, altfel cad
        if (pos.y <= targetY) {
            pos.y = targetY;
            vitezaY = 0.0f;
        }
        else {
            // cad
            vitezaY -= gravity * delta;
            pos.y += vitezaY * delta;
        }
    }
    else {
        vitezaY -= gravity * delta;
        pos.y += vitezaY * delta;
    }
    
    myCamera.setPosition(pos);
}

void initFloors() {
    floors.clear();

    // FLOOR_01
    floors.push_back({
     glm::vec3(24.338f, 13.306f,  3.3604f),
     glm::vec3(36.338f, 13.906f, 14.3604f),
     13.906f // max.Y
        });


    // FLOOR_02
    floors.push_back({
    glm::vec3(26.838f, 13.283f, -1.9674f),
    glm::vec3(32.838f, 13.883f,  4.0326f),
    13.883f
        });


    // FLOOR_03
    floors.push_back({
    glm::vec3(31.475f, 13.393f, 0.8944f),
    glm::vec3(34.475f, 13.993f, 2.8944f),
    13.993f
        });

    // FLOOR_04
    floors.push_back({
    glm::vec3(18.029f, 0.29391f, -15.9008f),
    glm::vec3(38.029f, 0.89391f,  4.0992f),
    0.89391f
        });

    // FLOOR_05
    floors.push_back({
    glm::vec3(8.053f, 0.66541f, -11.3607f),
    glm::vec3(20.053f, 1.26541f,  8.6393f),
    1.26541f
        });

    // Treptele
    // FLOOR_06
    floors.push_back({
    glm::vec3(10.681f, 1.2825f, 2.8586f),
    glm::vec3(16.681f, 1.8825f, 4.8586f),
    1.8825f
        });

    // FLOOR_06.a
    floors.push_back({
    glm::vec3(10.769f, 1.9081f, 4.3903f),
    glm::vec3(16.769f, 2.5081f, 6.3903f),
    2.5081f
        });

    // FLOOR_06.b
    floors.push_back({
    glm::vec3(10.839f, 2.5138f, 5.4999f),
    glm::vec3(16.839f, 3.1138f, 7.4999f),
    3.1138f
        });

    // FLOOR_06.c
    floors.push_back({
    glm::vec3(10.874f, 3.1306f, 6.2858f),
    glm::vec3(16.874f, 3.7306f, 8.2858f),
    3.7306f
        });


    // FLOOR_06.d
    floors.push_back({
    glm::vec3(6.221f, 3.5164f, 7.3976f),
    glm::vec3(16.221f, 4.1164f, 12.3976f),
    4.1164f
        });


    // FLOOR_06.e
    floors.push_back({
    glm::vec3(8.223f, 3.4785f, 5.9827f),
    glm::vec3(11.823f, 4.0785f, 7.9827f),
    4.0785f
        });

    // FLOOR_06.f
    floors.push_back({
    glm::vec3(8.551f, 3.9834f, 4.6654f),
    glm::vec3(12.151f, 4.5834f, 6.6654f),
    4.5834f
        });

    // FLOOR_06.g
    floors.push_back({
    glm::vec3(8.55f, 4.4903f, 3.5525f),
    glm::vec3(12.15f, 5.0903f, 5.5525f),
    5.0903f
        });


    // FLOOR_06.h
    floors.push_back({
    glm::vec3(8.577f, 5.0261f, 2.5665f),
    glm::vec3(12.177f, 5.6261f, 4.5665f),
    5.6261f
        });


    // FLOOR_06.i
    floors.push_back({
    glm::vec3(8.594f, 5.6224f, 1.4417f),
    glm::vec3(12.194f, 6.2224f, 3.4417f),
    6.2224f
        });

    // FLOOR_07
    floors.push_back({
    glm::vec3(-6.8409f, 5.8607f, -16.2228f),
    glm::vec3(25.1591f, 6.4607f,  1.7772f),
    6.4607f
        });


    // FLOOR_08
    floors.push_back({
    glm::vec3(14.886f, 5.8305f, -2.8052f),
    glm::vec3(24.886f, 6.4305f, 13.1948f),
    6.4305f
        });

    // FLOOR_09
    floors.push_back({
    glm::vec3(-9.086411f, 5.5586f,  -5.2313f),
    glm::vec3(8.913589f, 6.1936f,  16.7687f),
    6.1936f
        });

    // FLOOR_10
    floors.push_back({
    glm::vec3(-40.311f, 5.2691f, -1.1571f),
    glm::vec3(-8.311f, 5.8691f, 10.8429f),
    5.8691f
        });

    // FLOOR_11
    floors.push_back({
    glm::vec3(-13.218f, 5.4341f, 8.9690f),
    glm::vec3(-8.218f, 6.0341f, 16.9690f),
    6.0341f
        });

}

void initColliders() {
    colliders.clear();

    // Logica: (x_joc, y_joc, z_joc) = (x_blender, z_blender, -y_blender)

   // WALL_01 din Blender:
   // Location = (25.518, -9.7843, 16.023)
   // Dimensions = (2, 9, 4)
   // 
   // In OpenGL:
   // Location = (25.518, 16.023, 9.7843)
   // Dimensions = (2, 4, 9)
   // 
   // half = (1, 2, 4.5)
   // 
   // 
   // min = loc - half = (24.518, 14.02285f, 5.284325f)
   // max = loc + half = (26.518, 18.02285f, 14.284325f)
    colliders.push_back({
        glm::vec3(24.51828f, 14.02285f, 5.284325f),
        glm::vec3(26.51828f, 18.02285f, 14.284325f)
    });

    // WALL_02
    colliders.push_back({
        glm::vec3(21.935f, 14.272f, 13.862f),
        glm::vec3(33.935f, 18.272f, 16.462f)
    });


    // WALL_03
    colliders.push_back({
        glm::vec3(31.562f, 12.907f, 11.163f),
        glm::vec3(36.562f, 17.907f, 16.163f)
    });


    // WALL_04
    colliders.push_back({
        glm::vec3(34.428f, 13.617f, -1.1727f),
        glm::vec3(36.428f, 18.617f, 14.8273f)
    });

    // WALL_05
    colliders.push_back({
    glm::vec3(28.293f, 13.155f, -0.98222f),
    glm::vec3(37.293f, 18.155f, -0.96222f)
        });


    // WALL_06
    colliders.push_back({
    glm::vec3(24.572f, 14.007f, -3.9879f),
    glm::vec3(28.572f, 19.007f, 6.0121f)
        });


    // WALL_07.a
    colliders.push_back({
     glm::vec3(34.143f, -1.2034f, -3.67564f),
     glm::vec3(35.143f, 18.7966f,  2.32436f)
        });


    // WALL_07.b
    colliders.push_back({
    glm::vec3(29.067f, 4.7489f, -1.34673f),
    glm::vec3(35.067f, 13.7489f, -0.34673f)
        });

    // WALL_07.c
    colliders.push_back({
    glm::vec3(32.714f, -6.7508f, -3.31315f),
    glm::vec3(32.734f, 13.2492f,  2.68685f)
        });

    // WALL_07.d
    colliders.push_back({
    glm::vec3(29.867f, -6.6146f, 0.7269f),
    glm::vec3(37.867f, 13.3854f, 6.7269f)
        });

    // WALL_08
    colliders.push_back({
    glm::vec3(36.18f, -1.0232f, -7.3653f),
    glm::vec3(38.18f,  4.9768f,  0.6347f)
        });

    // WALL_09
    colliders.push_back({
    glm::vec3(33.991f, -0.1958f, -3.5552f),
    glm::vec3(38.991f,  5.8042f,  0.4448f)
        });


    // WALL_10
    colliders.push_back({
    glm::vec3(17.447f, -0.6153f, -12.764f),
    glm::vec3(37.447f,  5.3847f,  -10.764f)
        });

    // WALL_11
    colliders.push_back({
    glm::vec3(22.778f, -0.6018f, -3.4507f),
    glm::vec3(32.778f,  5.3982f, -1.4507f)
        });

    // WALL_12
    colliders.push_back({
    glm::vec3(18.815f, -0.3637f, -4.5424f),
    glm::vec3(25.815f,  5.6363f, -2.5424f)
        });

    // WALL_13
    colliders.push_back({
    glm::vec3(4.97f,  0.1043f, -11.263f),
    glm::vec3(24.97f, 5.1043f,  -9.263f)
        });


    // WALL_14
    colliders.push_back({
    glm::vec3(18.252f, 0.2742f, -11.195f),
    glm::vec3(19.852f, 6.2742f,  -8.195f)
        });

    // WALL_15
    colliders.push_back({
    glm::vec3(18.532f, -0.1655f, -6.8315f),
    glm::vec3(19.932f,  5.8345f, -3.8315f)
        });

    // WALL_16
    colliders.push_back({
    glm::vec3(17.041f, 0.2085f, -5.3695f),
    glm::vec3(18.641f, 5.2085f, -1.7695f)
        });


    // WALL_17
    colliders.push_back({
    glm::vec3(15.754f, 0.1291f, -4.1757f),
    glm::vec3(17.354f, 5.1291f, -0.5757f)
        });

    // WALL_18
    colliders.push_back({
    glm::vec3(14.368f, 0.6216f, -3.0558f),
    glm::vec3(15.968f, 5.6216f, 16.9442f)
        });


    // WALL_19
    colliders.push_back({
    glm::vec3(6.9739f, 0.6186f, -10.0428f),
    glm::vec3(8.9739f, 5.6186f,  -0.0428f)
        });

    // WALL_20
    colliders.push_back({
    glm::vec3(36.632f, -3.029341f, -14.9506f),
    glm::vec3(38.632f,  2.970659f,  -1.9506f)
        });

    // WALL_21
    colliders.push_back({
    glm::vec3(11.713f, 1.0181f, 0.8638f),
    glm::vec3(11.913f, 7.0181f, 7.8638f)
        });

    // WALL_22
    colliders.push_back({
    glm::vec3(8.7269f, 0.7405f, 0.1482f),
    glm::vec3(9.1269f, 7.7405f, 12.1482f)
        });

    // WALL_23
    colliders.push_back({
    glm::vec3(11.604f, 6.3103f, 0.9507f),
    glm::vec3(15.004f, 8.3103f, 1.3507f)
        });

    // WALL_24
    colliders.push_back({
    glm::vec3(9.04f,  3.4523f, 10.744f),
    glm::vec3(15.04f, 13.4523f, 11.144f)
        });

    // WALL_25
    colliders.push_back({
    glm::vec3(-3.928f, 6.3543f, -16.066f),
    glm::vec3(28.072f, 10.3543f, -15.066f)
        });

    // WALL_26
    colliders.push_back({
    glm::vec3(15.297f, 6.228f, -6.5815f),
    glm::vec3(25.297f, 10.228f, -5.5815f)
        });

    // WALL_27
    colliders.push_back({
    glm::vec3(3.6572f, 6.072f, -6.9105f),
    glm::vec3(13.2572f, 10.072f, -5.9105f)
        });

    // WALL_28
    colliders.push_back({
    glm::vec3(16.409f, 6.1849f, -2.2714f),
    glm::vec3(18.409f, 10.1849f, -1.8714f)
        });

    // WALL_29
    colliders.push_back({
    glm::vec3(19.638f, 6.3716f, -2.3595f),
    glm::vec3(21.238f, 10.3716f, -1.9595f)
        });


    // WALL_30
    colliders.push_back({
    glm::vec3(16.51f, 6.181f, 11.658f),
    glm::vec3(24.51f, 10.181f, 12.258f)
        });

    // WALL_31
    colliders.push_back({
    glm::vec3(20.585f, 6.1001f, -8.0096f),
    glm::vec3(21.185f, 10.1001f, -2.0096f)
        });

    // WALL_32
    colliders.push_back({
    glm::vec3(24.023f, 6.2826f, -7.3388f),
    glm::vec3(24.623f, 10.2826f, 12.6612f)
        });

    // WALL_33
    colliders.push_back({
    glm::vec3(16.693f, 6.4224f, -2.009f),
    glm::vec3(17.293f, 10.4224f, 11.991f)
        });

    // WALL_34
    colliders.push_back({
    glm::vec3(3.7221f, 7.3869f, -6.327f),
    glm::vec3(5.7221f, 11.3869f,  9.673f)
        });

    // WALL_35
    colliders.push_back({
    glm::vec3(-3.0055f, 6.0311f, -17.0259f),
    glm::vec3(-2.0055f, 10.0311f,  1.9741f)
        });

    // WALL_36
    colliders.push_back({
    glm::vec3(-2.9436f, 6.2005f, 7.4530f),
    glm::vec3(-1.9436f, 10.2005f, 11.4530f)
        });

    // WALL_37
    colliders.push_back({
    glm::vec3(-2.9588f, 6.0911f, 12.8160f),
    glm::vec3(-1.9588f, 10.0911f, 16.8160f)
        });

    // WALL_38
    colliders.push_back({
    glm::vec3(1.1994f, 6.5170f, 1.8683f),
    glm::vec3(4.1994f, 10.5170f, 2.4683f)
        });

    // WALL_39
    colliders.push_back({
    glm::vec3(-11.0608f, 6.2128f, 1.9153f),
    glm::vec3(-1.0608f, 10.2128f, 2.5153f)
        });

    // WALL_40
    colliders.push_back({
    glm::vec3(-9.0322f, 6.2145f, 7.5728f),
    glm::vec3(-3.0322f, 10.2145f, 8.1728f)
        });

    // WALL_41
    colliders.push_back({
    glm::vec3(-2.63573f, 6.1690f, 15.5700f),
    glm::vec3(3.36427f, 10.1690f, 16.1700f)
        });

    // WALL_42
    colliders.push_back({
    glm::vec3(2.6809f, 5.9255f, 7.6603f),
    glm::vec3(8.6809f, 9.9255f, 8.2603f)
        });

    // WALL_43
    colliders.push_back({
    glm::vec3(2.6271f, 6.4477f, 7.3670f),
    glm::vec3(3.2271f, 10.4477f, 17.3670f)
        });

    // WALL_44
    colliders.push_back({
    glm::vec3(18.889f, 6.1319f, -15.886f),
    glm::vec3(19.489f, 10.1319f, -5.886f)
        });

    // WALL_45
    colliders.push_back({
    glm::vec3(14.62f, 6.4383f, 0.21109f),
    glm::vec3(16.02f, 9.4383f, 1.41109f)
        });

    // WALL_46
    colliders.push_back({
    glm::vec3(-11.2973f, 5.8030f, 0.5957f),
    glm::vec3(-6.6973f, 9.8030f, 3.5957f)
        });

    // WALL_47
    colliders.push_back({
    glm::vec3(-10.5313f, 5.9953f, 6.5459f),
    glm::vec3(-6.5313f, 9.9953f, 9.5459f)
        });

    // WALL_48
    colliders.push_back({
    glm::vec3(-20.574f, 5.6494f, 0.5233f),
    glm::vec3(-8.574f, 9.6494f, 1.5233f)
        });

    // WALL_49
    colliders.push_back({
    glm::vec3(-20.78f, 5.4472f, 8.6192f),
    glm::vec3(-8.78f, 9.4472f, 9.6192f)
        });

    // WALL_50
    colliders.push_back({
    glm::vec3(-43.919f, 5.9109f, 7.0724f),
    glm::vec3(-19.919f, 9.9109f, 8.0724f)
        });

    // WALL_51
    colliders.push_back({
    glm::vec3(-44.33f, 5.7957f, 4.2033f),
    glm::vec3(-20.33f, 9.7957f, 5.2033f)
        });

    // WALL_52
    colliders.push_back({
    glm::vec3(-20.558f, 5.8154f, -0.1220f),
    glm::vec3(-19.958f, 9.8154f,  5.8780f)
        });

    // WALL_53
    colliders.push_back({
    glm::vec3(-20.452f, 5.8414f, 7.1015f),
    glm::vec3(-19.852f, 9.8414f, 8.7015f)
        });

    // WALL_54
    colliders.push_back({
    glm::vec3(-37.502f, 5.9344f, 4.7839f),
    glm::vec3(-36.902f, 9.9344f, 7.7839f)
        });

    // WALL_55
    colliders.push_back({
    glm::vec3(-12.059f, 6.3424f, 9.9550f),
    glm::vec3(-11.059f, 10.3424f, 16.9550f)
        });

    // WALL_56
    colliders.push_back({
    glm::vec3(-11.9666f, 6.3460f, 16.3605f),
    glm::vec3(-1.9666f, 9.5060f, 17.1495f)
        });

    // WALL_57
    colliders.push_back({
    glm::vec3(-12.1524f, 6.3791f, 9.8590f),
    glm::vec3(-2.1524f, 9.5391f, 10.6490f)
        });

}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");

    // dezactivez cursorul
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetMouseButtonCallback(myWindow.getWindow(), mouseButtonCallback); // am adaugat functia asta pt a procesa click-rile de la maus
}

bool initAudio() {
    if (ma_engine_init(NULL, &audioEngine) != MA_SUCCESS) {
        cout << "Eroare la pornire!";
        return false;
    }

    if (ma_sound_init_from_file(&audioEngine, "sounds/monsterSound.mp3", MA_SOUND_FLAG_STREAM, NULL, NULL, &monsterSound) != MA_SUCCESS) {
        cout << " Eroare la citirea soundului!";
        return false;
    }

    ma_sound_set_looping(&monsterSound, MA_TRUE);
    ma_sound_set_volume(&monsterSound, 0.0f);
    ma_sound_start(&monsterSound); // il pornesc cu volum 0
    audio = true;

    return true;
}

void initOpenGLState() {
    glClearColor(0.01f, 0.01f, 0.015f, 1.0f); // intunecat
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	//glEnable(GL_CULL_FACE); // cull face
	//glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    spital.LoadModel("models/hospital_fara_usa.obj");

    doors.resize(5);

    const char* files[5] = {
        "models/usa1.obj",
        "models/usa2.obj",
        "models/usa3.obj",
        "models/usa4.obj",
        "models/usa5.obj",
    };

    // Bounding-Box pentru fiecare usa -> coordonate luate din Blender
    // usa1: hinge = (-7.110000, 0.000000, 5.048760), boundingBoxMin = (-7.226481, 6.492236, 5.048760), boundingBoxMax = (-6.993520, 8.933995, 6.168565)
    // usa2 : hinge = (-7.110000, 0.000000, 3.886890), boundingBoxMin = (-7.226480, 6.492236, 3.886890), boundingBoxMax = (-6.993520, 8.933995, 5.006695)
    // usa3 : hinge = (-2.100000, 0.000000, 11.499999), boundingBoxMin = (-2.216480, 6.481153, 11.499999), boundingBoxMax = (-1.983519, 8.922913, 12.619803)
    // usa4 : hinge = (18.410000, 0.000000, -2.046392), boundingBoxMin = (18.410000, 6.481153, -2.162872), boundingBoxMax = (19.529806, 8.922913, -1.929912)
    // usa5 : hinge = (19.086430, 0.000000, -8.086966), boundingBoxMin = (18.969950, 1.341153, -8.086966), boundingBoxMax = (19.202909, 3.782912, -6.967161)

    glm::vec3 balama[5] = {             
        {-7.1100005f, 0.0f, 6.1685650f},  // usa1 
        {-7.1100000f, 0.0f, 3.886890f},   // usa2  
        {-2.0999995f, 0.0f, 12.6198030f}, // usa3  
        {19.5298060f, 0.0f, -2.0463920f}, // usa4 
        {19.0864295f, 0.0f, -6.9671610f}  // usa5
    };

    AABB doorCollidersAABB[5] = {
        { glm::vec3(-7.226481f, 6.492236f,  5.048760f), glm::vec3(-6.993520f, 8.933995f,  6.168565f) }, // usa1
        { glm::vec3(-7.226480f, 6.492236f,  3.886890f), glm::vec3(-6.993520f, 8.933995f,  5.006695f) }, // usa2
        { glm::vec3(-2.216480f, 6.481153f, 11.499999f), glm::vec3(-1.983519f, 8.922913f, 12.619803f) }, // usa3
        { glm::vec3(18.410000f, 6.481153f, -2.162872f), glm::vec3(19.529806f, 8.922913f, -1.929912f) }, // usa4
        { glm::vec3(18.969950f, 1.341153f, -8.086966f), glm::vec3(19.202909f, 3.782912f, -6.967161f) }  // usa5
    };

    for (int i = 0; i < 5; i++) {
        doors[i].model.LoadModel(files[i]);
        doors[i].balama = balama[i];
        doors[i].doorColliders = doorCollidersAABB[i];
    }

    lampa.LoadModel("models/lampa.obj");

    monstru.LoadModel("models/monstru.obj");
}

void initShadowResources() {
    // depthFBO pt stocare, ca sa nu ajunga aceasta randare pe ecran, ci intr-o textura
    glGenFramebuffers(1, &depthFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);

    // creez textura
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);

    //                                    32biti           rezolutie shadow map
    //glTexImage2D(tip textura,level, textura de adancime, (1024x1024)  , border, format,             type,     nu incarc nimic, doar aloc spatiu
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOW_W, SHADOW_H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // cand textura e micsorata pe ecran, adica un texel acopera mai multi pixeli (downscale)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // cand textura e marita pe ecran, adica un texel trb sa acopere mai multi pixeli (upscale)

    // coordonatele UV -> S = U si T = V
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.f, 1.f, 1.f, 1.f }; 
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // atasez textura de adancime la FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

    // dezactivez output de culoare ca nu am nevoie, altfel punem GL_COLOR_ATTACHMENT
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // resetez, ca sa nu stric randarea normala
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // 0 => default framevuffer == ecranul
    glBindTexture(GL_TEXTURE_2D, 0); // dezleg textura curenta

    // shadow shader
    shadowShader.loadShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");
    shadowShader.useShaderProgram();
    shadowModelLoc = glGetUniformLocation(shadowShader.shaderProgram, "model");
    shadowLightSpaceLoc = glGetUniformLocation(shadowShader.shaderProgram, "lightSpaceMatrix");

    // uniforme in shaderul basic
    myBasicShader.useShaderProgram();
    lightSpaceMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceMatrix");
    shadowMapLoc = glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap");
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
}

void initUniforms() {
	myBasicShader.useShaderProgram();
    // smooth vs flat
    shadingModeLoc = glGetUniformLocation(myBasicShader.shaderProgram, "uShadingMode");
    glUniform1i(shadingModeLoc, gShadingMode);

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
    // Am modificat din 20.0f -> 200.0f (pentru ca far sa fie 200.0)
 	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 200.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    // flashlight
    flashPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "flashPos");
    flashDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "flashDir");
    flashColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "flashColor");
    unghiInterior = glGetUniformLocation(myBasicShader.shaderProgram, "unghiInterior");
    unghiExterior = glGetUniformLocation(myBasicShader.shaderProgram, "unghiExterior");
    constantAttLoc = glGetUniformLocation(myBasicShader.shaderProgram, "constantAttenuation");
    linearAttLoc = glGetUniformLocation(myBasicShader.shaderProgram, "linearAttenuation");
    quadraticAttLoc = glGetUniformLocation(myBasicShader.shaderProgram, "quadraticAttenuation");

    glUniform3f(flashColorLoc, 1.0f, 1.0f, 1.0f);

    // unghiuri 
    glUniform1f(unghiInterior, cos(glm::radians(12.5f)));
    glUniform1f(unghiExterior, cos(glm::radians(17.5f)));

    // atenuarea
    glUniform1f(constantAttLoc, 1.0f);
    glUniform1f(linearAttLoc, 0.09f);
    glUniform1f(quadraticAttLoc, 0.032f);

    // lampa
    lampOnLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lampOn");
    lampPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lampPos");
    lampColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lampColor");
    lampConstLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lampConst");
    lampLinearLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lampLinear");
    lampQuadLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lampQuad");

    glUniform3f(lampColorLoc, 1.0f, 0.95f, 0.8f); // usor galbui

    glUniform1f(lampConstLoc, 1.0f);
    glUniform1f(lampLinearLoc, 0.09f);
    glUniform1f(lampQuadLoc, 0.032f);

}

void updatePresentation(float delta) {
    tourTime += delta;

    float a = tourTime * tourSpeed; // viteza constanta in timp real, nu depinde de FPS
    glm::vec3 pos;
    // cos(a) = proiectia pe axa X
    // sin(a) = proiectia pe axa Y
    // cos^2 + sin^2 = 1 => (x,y) e pe cerc
    
    // Cercul cu raza R => x = R * cos(a)
    //                     y = R * sim(a)
    // Pt ca centrul meu nu este centrul hartei... trb sa fac::  x = center.x + R * cos(a)
    pos.x = tourCenter.x + cos(a) * tourRadius;  
    pos.z = tourCenter.z + sin(a) * tourRadius;     
    pos.y = tourCenter.y + tourHeight; // constant pe Y

    myCamera.setPosition(pos);

    glm::vec3 dir = glm::normalize(tourCenter - pos); // directia camerei (camera -> centrul meu)
    float yawTour = glm::degrees(atan2(dir.x, dir.z)); // dir.x/dir.y => tg(yam) => yam = arctg(dir.x, dir.y) => atan2 pt ca imi da unghiu vectorilui dintre ele fara impartire
    float pitchTour = glm::degrees(asin(dir.y)); 
    // FORMULE: https://learnopengl.com/Getting-started/Camera
    // dir.x = cos(yaw) * cos(pitch)
    // dir.y = sin(pitch)                => pitch = arcsin(dir.y)
    // dir.z = sin(yaw) * cos(pitch)

    myCamera.rotate(pitchTour, yawTour);

    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // normalMatrix default
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
}


float functieHelper(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

void updateMonsterAudio(float delta) {
    if (audio == false) return;

    glm::vec3 cameraPos = myCamera.getPosition();
    float distance = glm::length(cameraPos - monstruPos);

    float targetVolume = 0.0f;

    if (distance <= monsterMaxDistance) { // daca e prea departe targetVolume ramane 0
        float temp = (distance - monsterMinDistance) / (monsterMaxDistance - monsterMinDistance); // distance = monsterMinDistance = 0 => aproape 
                                                                                                  // distance = monsterMaxDistance = 1 => departe
        temp = functieHelper(temp); // 0 maxim <-> 1 minim
        targetVolume = monsterMaxVolume * (1.0 - temp); 
    }

    // fade usor (fade are acelasi viteza in timp real indiferent de FPS
    float speedIncrease = delta * 6.0f; 
    speedIncrease = functieHelper(speedIncrease); 
    currentVolume = currentVolume + (targetVolume - currentVolume) * speedIncrease;

    ma_sound_set_volume(&monsterSound, currentVolume);
}

float normalizeAngle(float diff) {
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return diff;
}

float turnAngleDegree(float current, float target, float speed) {
    float diff = normalizeAngle(target - current);
    // current = 350 si target = 10
    // 10-350 = -340 ar intoarce monstru enorm, dar drumul ar fi +20
    // Prin formula: target - current si o impachetez in -180,180, si astfel se intoarce mereu pe cel mai scurt drum
    return current + diff * speed;
}

void updateMonstru(float delta) {
    glm::vec3 cameraPos = myCamera.getPosition();
    glm::vec3 toCamera = cameraPos - monstruPos;

    // yam pe XZ
    glm::vec2 toCameraXZ(toCamera.x, toCamera.z);
    if (glm::length(toCameraXZ) > 0.00001f) {
        float targetYaw = glm::degrees(atan2(toCamera.x, toCamera.z)); // rotire dreapta/stanga
        float speed = 3.0f;

        monstruYaw = turnAngleDegree(monstruYaw, targetYaw, speed * delta);
    }

    // M = T(p) * R * T(-p)
    monstruModel = glm::mat4(1.0f);
    monstruModel = glm::translate(monstruModel, monstruPos); // aduc pivotul in origine (monstruPos va fi pct in jurul caruia fac rotatia)
    monstruModel = glm::rotate(monstruModel, glm::radians(monstruYaw + monstruYawOffset), glm::vec3(0, 1, 0)); // in jurul lui Y
    monstruModel = glm::translate(monstruModel, -monstruPos); // readuce la pozitia initiala
}

void updateDoorStatus() {
    double now = glfwGetTime();

    for (Door& d : doors) {
        if (d.closeActivated && d.open && now >= d.closeAt) {
            d.open = false;
            d.closeActivated = false;
        }
    }
}

// pun lanterna putin in dreapta si mai jos (ca sa nu fie pe directia ochilor, sa vad umbrele)
void updateFlashlightFromCamera() {
    glm::vec3 camPos = myCamera.getPosition();
    glm::vec3 camDir = glm::normalize(myCamera.getFront());

    // dot(camDir, glm::vec3(0, 1, 0))) -> cosinusul dintre camDir si vertical
    // daca dot =~ 1 atunci => cos = 0 sau 180 (ma uit aproape perfect in sus/jos) => camDir || up (in world)
    // deci vom alege alt pct de referinta daca sunt paralele pt ca ulterior CROSS product va da 0 (daca cele 2 sunt paralele)
    glm::vec3 up;
    if (fabs(glm::dot(camDir, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    else {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // cross(a,b) => da un vector perpendicular pe amandoi (dupa il normalizam [lungime 1])
    glm::vec3 right = glm::normalize(glm::cross(camDir, up));

    // il punem in "mana"
    float offsetRight = 0.25f;
    float offsetDown = 0.15f;

    flashPosWorld = camPos + right * offsetRight - up * offsetDown;
    flashDirWorld = camDir;
}

// shadow map, textura de adancime
void renderDepthPass() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // fac randarea pe solid 

    glm::vec3 lightPos = flashPosWorld;
    glm::vec3 lightDir = glm::normalize(flashDirWorld);

    float fov = glm::radians(35.0f);
    float nearPlane = 0.5f;
    float farPlane = 50.0f;

    // aleg un vector UP care sa nu fie paralel cu directia de privire
    glm::vec3 up = (fabs(glm::dot(lightDir, glm::vec3(0, 1, 0))) > 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);

    // construiesc matricea "lightSpaceMatrix" = P * V (ca sa duc din word space -> light space in shader)
    glm::mat4 lightProjection = glm::perspective(fov, 1.0f, nearPlane, farPlane);
    glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, up);
    lightSpaceMatrix = lightProjection * lightView;

    // randare in Shadow MAP ( nu pe ecran)
    glViewport(0, 0, SHADOW_W, SHADOW_H);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);

    // curarare depth buffer
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0); // valoarea cea mai indepartate va fi 1.0
    glClear(GL_DEPTH_BUFFER_BIT); // goleste shadow map

    shadowShader.useShaderProgram();
    glUniformMatrix4fv(shadowLightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    // Desenez fiecare obiect cu matriceaModel a lui in SHADOW MAP

    glUniformMatrix4fv(shadowModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    spital.Draw(shadowShader);

    for (Door& d : doors) {
        glm::mat4 m = model;
        m = glm::translate(m, d.balama);
        m = glm::rotate(m, glm::radians(d.angle), glm::vec3(0, 1, 0));
        m = glm::translate(m, -d.balama);

        glUniformMatrix4fv(shadowModelLoc, 1, GL_FALSE, glm::value_ptr(m));
        d.model.Draw(shadowShader);
    }

    glUniformMatrix4fv(shadowModelLoc, 1, GL_FALSE, glm::value_ptr(monstruModel));
    monstru.Draw(shadowShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
}

void renderSpital(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    //send spital model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send spital normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw spital
    spital.Draw(shader);
}

void renderDoor(Door& d, gps::Shader shader) {
    shader.useShaderProgram();

    glm::mat4 m = model;
    m = glm::translate(m, d.balama);
    m = glm::rotate(m, glm::radians(d.angle), glm::vec3(0, 1, 0));
    m = glm::translate(m, -d.balama);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));

    glm::mat3 nm = glm::mat3(glm::inverseTranspose(view * m));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(nm));

    d.model.Draw(shader);
}

void renderLampa(gps::Shader shader) {
    shader.useShaderProgram();

    // send model to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lampModel));

    glm::mat3 normalMatrixLamp = glm::mat3(glm::inverseTranspose(view * lampModel));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixLamp));

    lampa.Draw(shader);
}

void renderMonstru(gps::Shader shader) {
    shader.useShaderProgram();

    // trimit catre shader matricea model ( pun monstru in lume: din model space in world space
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(monstruModel));

    // view * model: din model space -> eye space  
    // trimit in shader ca sa transform vectorii normali corect at cand obiectul are transformari
    glm::mat3 normalMatrixForMonstru = glm::mat3(glm::inverseTranspose(view * monstruModel));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixForMonstru));

    monstru.Draw(shader);
}

void renderScene() {
    updateFlashlightFromCamera();  // lanterna din mana
    renderDepthPass();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    myBasicShader.useShaderProgram();
    glUniform1i(shadingModeLoc, gShadingMode); // decide cum sa calculeze iluminarea smooth/flat se interschimba 

    // ii dau la shader matricea care transforma un punct din World space in Light Space ca sa compar dupa adancimile pt SHADOW MAPPING
    glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    // pentru adancimile luminii
    glActiveTexture(GL_TEXTURE3); // setez slotul (3)
    glBindTexture(GL_TEXTURE_2D, depthMap); // pun depthMap in slotul 3
    glUniform1i(shadowMapLoc, 3); // shader: citeste de la slotul 3

    // Spotlight
    // pozitia si directia lanternei (spotlight); trimit la fiecare frame ca pozitia/directia se schimba in continuu
    glUniform3fv(flashPosLoc, 1, glm::value_ptr(flashPosWorld));
    glUniform3fv(flashDirLoc, 1, glm::value_ptr(flashDirWorld));

    // controlez lampa (on/off)
    glUniform1i(lampOnLoc, lampOn ? 1 : 0);
    glUniform3fv(lampPosLoc, 1, glm::value_ptr(lampWorldPos));

    // modurile
    switch (gPolyMode) {
    case 0: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  break; 
    case 1: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  break;
    case 2: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
    }
    if (gPolyMode == 2) glPointSize(5.0f); // desenez puncte cu diametru de 5 pixeli (implicit este 1)


    // render scene
    renderSpital(myBasicShader);

    for (Door& d : doors)
        renderDoor(d, myBasicShader);

    // render lampa
    glm::vec3 baseLampPos = lampWorldPos;
    // model pentru lampa: translate + rotate + scale
    lampModel = glm::mat4(1.0f);
    lampModel = glm::translate(lampModel, baseLampPos + lampOffset); // lampOffset controlabil din taste
    lampModel = glm::rotate(lampModel, glm::radians(lampRotY), glm::vec3(0, 1, 0)); // lampRotY controlabil
    lampModel = glm::scale(lampModel, glm::vec3(lampScale)); // lampScale controlabil

    renderLampa(myBasicShader);

    renderMonstru(myBasicShader);
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data

    if (audio == true) {
        ma_sound_uninit(&monsterSound);
        ma_engine_uninit(&audioEngine);
        audio = false;
    }
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
    initColliders();
    initFloors();
	initShaders();
    initShadowResources();
	initUniforms();
    setWindowCallbacks();

    if (!initAudio()) cout << "Audio init failed!";

	glCheckError();

    double lastTime = glfwGetTime();

	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        double now = glfwGetTime();
        float delta = float(now - lastTime);
        lastTime = now;

        if (presentationMode) {
            updatePresentation(delta);
        }
        else {
            processMovement();
            putPlayerOnGround(delta);
        }
        updateMonstru(delta);
        updateMonsterAudio(delta);
	    renderScene();

        // inchidere automata a usii
        updateDoorStatus();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}