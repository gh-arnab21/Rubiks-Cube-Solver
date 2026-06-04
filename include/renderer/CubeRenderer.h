/**
 * @file CubeRenderer.h
 * @brief OpenGL-based Rubik's Cube 3D renderer with interactive controls
 */

#pragma once

#include <vector>
#include <array>
#include <memory>
#include <chrono>
#include <string>
#include <functional>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "cube/RubiksCube.h"

namespace cube_solver {

/**
 * @struct Material
 * @brief Material properties for Phong shading
 */
struct Material {
    glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;
};

/**
 * @struct Light
 * @brief Point light source
 */
struct Light {
    glm::vec3 position = glm::vec3(5.0f, 5.0f, 5.0f);
    glm::vec3 ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 faceColor;
    float cubieID; // 0.0 to 26.0
};

struct RenderCubie {
    int id;
    int gridX, gridY, gridZ;
    glm::mat4 baseTransform;
};

/**
 * @class CubeRenderer
 * @brief Main renderer for interactive 3D cube visualization
 */
class CubeRenderer {
public:
    bool initialize(int width, int height, const std::string& title);
    void shutdown();
    bool render(const RubiksCube& cube);
    void handleInput();
    void resetCamera();
    void setLevitationEnabled(bool enabled) { levitationEnabled = enabled; }
    void setAnimationSpeed(float speed) { animationSpeed = (speed > 0.1f) ? speed : 0.1f; }
    void setSolutionText(const std::string& text) { solutionText = text; }
    int getWidth() const { return windowWidth; }
    int getHeight() const { return windowHeight; }
    bool shouldClose() const;
    std::vector<std::pair<Face, MoveType>> getQueuedMoves();
    
    // Callback for GLFW keys
    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    
    // Callback for interactive action keys (SPACE, S, G, etc.)
    std::function<void(int)> onActionKey;
    
    // Public cubie state (needed by InteractiveSolver for reset)
    std::array<RenderCubie, 27> renderCubies;
    
    // Animation queue (public so InteractiveSolver can push moves)
    std::vector<std::pair<Face, MoveType>> animQueue;
    void queueAnimation(Face f, MoveType t) { animQueue.push_back({f, t}); }
    
private:
    // Window management
    int windowWidth = 1200;
    int windowHeight = 800;
    GLFWwindow* glfwWindow = nullptr;
    
    // Graphics resources
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    unsigned int shaderProgram = 0;
    
    // Cubie data
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    // Slice animation state
    bool isAnimatingSlice = false;
    Face animFace = Face::U;
    MoveType animMoveType = MoveType::Normal;
    float sliceAnimTime = 0.0f;
    float sliceAnimDuration = 0.3f; // 300ms
    float sliceAnimTargetAngle = 0.0f;
    
    // Camera and view
    glm::vec3 cameraPosition = glm::vec3(3.5f, 3.5f, 3.5f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float cameraDistance = 7.0f;
    float cameraAngleX = 0.5f;  // Pitch
    float cameraAngleY = 0.5f;  // Yaw
    
    // Cube rotation (proper SLERP: interpolate from startRotation -> targetRotation)
    glm::quat cubeRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::quat startRotation = cubeRotation;    // fixed origin for each SLERP
    glm::quat targetRotation = cubeRotation;
    float rotationAnimationTime = 0.0f;
    float rotationAnimationDuration = 0.5f;
    
    // Levitation effect
    bool levitationEnabled = false;
    float levitationTime = 0.0f;
    float levitationHeight = 0.3f;
    float levitationFrequency = 0.5f;
    
    // Animation
    float animationSpeed = 1.0f;
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    float deltaTime = 0.016f;  // ~60 FPS
    
    // Input state
    std::vector<std::pair<Face, MoveType>> inputMoves;
    bool keys[512] = {false};
    double lastMouseX = 0, lastMouseY = 0;
    bool mousePressed = false;
    
    // Lighting and materials
    Light light;
    Material cubeMaterial;
    Material stickerMaterial;
    
    unsigned int compileShaders();
    void generateCubeGeometry(const RubiksCube& cube);
    glm::vec3 getStickerColor(Color color);
    void updateGeometry(const RubiksCube& cube);
    void renderPhong(const glm::mat4& view, const glm::mat4& projection);
    void updateRotationAnimation();
    void updateLevitation();
    void updateCamera();
    void renderText();

    // Text rendering
    std::string solutionText;
    unsigned int textVAO = 0;
    unsigned int textVBO = 0;
    unsigned int textEBO = 0;
    unsigned int textShaderProgram = 0;
    unsigned int compileTextShader();
};

} // namespace cube_solver
