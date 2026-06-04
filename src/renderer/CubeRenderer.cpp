#include "renderer/CubeRenderer.h"
#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include "utils/stb_easy_font.h"

namespace cube_solver {

// Static pointer for GLFW callbacks
static CubeRenderer* currentRenderer = nullptr;

static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (currentRenderer) {
        currentRenderer->keyCallback(window, key, scancode, action, mods);
    }
}

bool CubeRenderer::initialize(int width, int height, const std::string& title) {
    windowWidth = width;
    windowHeight = height;
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // Enable MSAA 4x
    
    glfwWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!glfwWindow) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(glfwWindow);
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE); // Enable MSAA
    
    currentRenderer = this;
    glfwSetKeyCallback(glfwWindow, glfwKeyCallback);
    
    RubiksCube defaultCube;
    generateCubeGeometry(defaultCube);
    shaderProgram = compileShaders();
    
    textShaderProgram = compileTextShader();
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glGenBuffers(1, &textEBO);
    
    lastFrameTime = std::chrono::high_resolution_clock::now();
    
    return true;
}

void CubeRenderer::shutdown() {
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (EBO != 0) glDeleteBuffers(1, &EBO);
    if (shaderProgram != 0) glDeleteProgram(shaderProgram);
    
    if (glfwWindow) {
        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
    }
}

bool CubeRenderer::render(const RubiksCube& cube) {
    if (glfwWindowShouldClose(glfwWindow)) {
        return false;
    }
    
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastFrameTime);
    deltaTime = std::min(0.05f, duration.count() / 1000000.0f); // cap at 50ms to prevent spiral
    lastFrameTime = currentTime;
    
    glfwPollEvents();
    handleInput();
    
    updateRotationAnimation();
    updateLevitation();
    updateCamera();
    updateGeometry(cube);
    
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glm::mat4 projection = glm::perspective(glm::radians(50.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
    
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, up);
    
    renderPhong(view, projection);
    
    renderText();
    
    glfwSwapBuffers(glfwWindow);
    
    return true;
}

void CubeRenderer::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
        
        Face face = Face::U;
        bool isMove = true;
        
        switch (key) {
            case GLFW_KEY_U: face = Face::U; break;
            case GLFW_KEY_D: face = Face::D; break;
            case GLFW_KEY_L: face = Face::L; break;
            case GLFW_KEY_R: face = Face::R; break;
            case GLFW_KEY_F: face = Face::F; break;
            case GLFW_KEY_B: face = Face::B; break;
            default: isMove = false; break;
        }
        
        if (isMove) {
            MoveType type = MoveType::Normal;
            if (mods & GLFW_MOD_SHIFT) type = MoveType::Prime;
            if (mods & GLFW_MOD_CONTROL) type = MoveType::Double;
            inputMoves.push_back({face, type});
        } else {
            // Pass it up to InteractiveSolver
            if (onActionKey) {
                onActionKey(key);
            }
        }
    }
}

void CubeRenderer::handleInput() {
    if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(glfwWindow, &xpos, &ypos);
        if (mousePressed) {
            float dx = (float)(xpos - lastMouseX);
            float dy = (float)(ypos - lastMouseY);
            cameraAngleY -= dx * 0.01f;
            cameraAngleX += dy * 0.01f;
            
            // Constrain pitch
            cameraAngleX = std::max(-1.5f, std::min(1.5f, cameraAngleX));
        }
        lastMouseX = xpos;
        lastMouseY = ypos;
        mousePressed = true;
    } else {
        mousePressed = false;
    }
}

void CubeRenderer::resetCamera() {
    cameraDistance = 7.0f;
    cameraAngleX = 0.5f;
    cameraAngleY = 0.5f;
    cubeRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    startRotation = cubeRotation;
    targetRotation = cubeRotation;
}

bool CubeRenderer::shouldClose() const {
    return glfwWindowShouldClose(glfwWindow);
}

std::vector<std::pair<Face, MoveType>> CubeRenderer::getQueuedMoves() {
    auto moves = inputMoves;
    inputMoves.clear();
    return moves;
}

unsigned int CubeRenderer::compileShaders() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aUV;
        layout (location = 3) in vec3 aFaceColor;
        layout (location = 4) in float aCubieID;
        
        out vec3 FragPos;
        out vec3 Normal;
        out vec2 UV;
        out vec3 FaceColor;
        
        uniform mat4 models[27];
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            mat4 model = models[int(aCubieID)];
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            UV = aUV;
            FaceColor = aFaceColor;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 FragPos;
        in vec3 Normal;
        in vec2 UV;
        in vec3 FaceColor;
        
        out vec4 FragColor;
        
        uniform vec3 viewPos;
        
        float sdRoundBox(vec2 p, vec2 b, float r) {
            vec2 q = abs(p) - b + r;
            return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
        }
        
        void main() {
            vec3 baseColor = vec3(0.04); // Deep black chassis
            
            // Sticker definition: slightly inset, rounded corners
            float d = sdRoundBox(UV, vec2(0.85), 0.12);
            if (length(FaceColor) > 0.01) {
                // Anti-aliased transition using fwidth
                float edge = fwidth(d);
                float alpha = smoothstep(edge, -edge, d);
                baseColor = mix(vec3(0.04), FaceColor, alpha);
            }
            
            // Clean flat directional lighting
            vec3 ambient = 0.45 * baseColor;
            vec3 norm = normalize(Normal);
            
            // Soft directional light from top-right-front
            vec3 lightDir = normalize(vec3(0.3, 1.0, 0.5));
            
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * baseColor * 0.55;
            
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            
            // Subtle, sharp specular highlight for semi-gloss plastic
            float spec = pow(max(dot(norm, halfwayDir), 0.0), 64.0);
            vec3 specular = 0.2 * smoothstep(0.1, 0.2, spec) * vec3(1.0);
            
            vec3 result = ambient + diffuse + specular;
            FragColor = vec4(result, 1.0);
        }
    )";
    
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

glm::vec3 CubeRenderer::getStickerColor(Color color) {
    switch (color) {
        case Color::WHITE:  return glm::vec3(1.0f, 1.0f, 1.0f);
        case Color::YELLOW: return glm::vec3(1.0f, 0.83f, 0.0f);
        case Color::GREEN:  return glm::vec3(0.0f, 0.62f, 0.38f);
        case Color::BLUE:   return glm::vec3(0.0f, 0.32f, 0.73f);
        case Color::RED:    return glm::vec3(0.77f, 0.12f, 0.23f);
        case Color::ORANGE: return glm::vec3(1.0f, 0.35f, 0.0f);
        default:            return glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

void CubeRenderer::generateCubeGeometry(const RubiksCube& cube) {
    vertices.clear();
    indices.clear();
    
    float size = 0.48f;      // slightly smaller than 0.5 to create a microscopic air gap
    float radius = 0.06f;    // bevel radius
    float inner = size - radius;
    int resolution = 16;
    glm::vec3 plasticColor = glm::vec3(0.08f, 0.08f, 0.08f); // dark inner gaps
    
    // Define faces (normals) and up vectors for UV mapping
    glm::vec3 faceNormals[6] = {
        {0, 1, 0}, {0, -1, 0}, {-1, 0, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0, -1}
    };
    glm::vec3 faceUps[6] = {
        {0, 0, -1}, {0, 0, 1}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}
    };
    Color faceColors[6] = {
        Color::WHITE, Color::YELLOW, Color::ORANGE, Color::RED, Color::GREEN, Color::BLUE
    };
    
    int id = 0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                renderCubies[id].id = id;
                renderCubies[id].gridX = x;
                renderCubies[id].gridY = y;
                renderCubies[id].gridZ = z;
                // Place it in the grid initially
                renderCubies[id].baseTransform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
                
                for (int f = 0; f < 6; ++f) {
                    glm::vec3 n = faceNormals[f];
                    glm::vec3 up = faceUps[f];
                    glm::vec3 right = glm::cross(n, up);
                    
                    unsigned int faceStartIdx = vertices.size();
                    
                    // Generate rounded box face
                    for (int i = 0; i <= resolution; ++i) {
                        for (int j = 0; j <= resolution; ++j) {
                            float u = (float)i / resolution * 2.0f - 1.0f; // -1 to 1
                            float v = (float)j / resolution * 2.0f - 1.0f; // -1 to 1
                            
                            glm::vec3 pos = n * size + right * (u * size) + up * (v * size);
                            glm::vec3 closestInner = glm::clamp(pos, -glm::vec3(inner), glm::vec3(inner));
                            
                            glm::vec3 diff = pos - closestInner;
                            float dist = glm::length(diff);
                            glm::vec3 normal = (dist > 0.0001f) ? (diff / dist) : n;
                            
                            glm::vec3 finalPos = closestInner + normal * radius;
                            
                            // Determine if this face is on the exterior of the puzzle
                            bool isExterior = false;
                            if (f == 0 && y == 1) isExterior = true;
                            if (f == 1 && y == -1) isExterior = true;
                            if (f == 2 && x == -1) isExterior = true;
                            if (f == 3 && x == 1) isExterior = true;
                            if (f == 4 && z == 1) isExterior = true;
                            if (f == 5 && z == -1) isExterior = true;
                            
                            glm::vec3 color = glm::vec3(0.0f);
                            if (isExterior) {
                                color = getStickerColor(faceColors[f]);
                            }
                            
                            vertices.push_back({finalPos, normal, glm::vec2(u, v), color, (float)id});
                        }
                    }
                    
                    for (int i = 0; i < resolution; ++i) {
                        for (int j = 0; j < resolution; ++j) {
                            unsigned int idx = faceStartIdx + i * (resolution + 1) + j;
                            indices.push_back(idx);
                            indices.push_back(idx + 1);
                            indices.push_back(idx + (resolution + 1));
                            
                            indices.push_back(idx + 1);
                            indices.push_back(idx + (resolution + 1) + 1);
                            indices.push_back(idx + (resolution + 1));
                        }
                    }
                }
                id++;
            }
        }
    }
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, faceColor));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, cubieID));
    glEnableVertexAttribArray(4);
    
    glBindVertexArray(0);
}

void CubeRenderer::updateGeometry(const RubiksCube& cube) {
    if (!isAnimatingSlice && !animQueue.empty()) {
        auto move = animQueue.front();
        animQueue.erase(animQueue.begin());
        
        isAnimatingSlice = true;
        animFace = move.first;
        animMoveType = move.second;
        sliceAnimTime = 0.0f;
        
        // Right hand rule: clockwise looking down the normal is a negative angle
        float angle = -glm::pi<float>() / 2.0f; 
        if (animMoveType == MoveType::Prime) angle = glm::pi<float>() / 2.0f;
        if (animMoveType == MoveType::Double) angle = -glm::pi<float>();
        sliceAnimTargetAngle = angle;
    }
}

void CubeRenderer::renderPhong(const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(shaderProgram);
    
    // Global levitation and rotation
    glm::mat4 globalModel = glm::mat4(1.0f);
    if (levitationEnabled) {
        float levY = std::sin(levitationTime) * levitationHeight;
        globalModel = glm::translate(globalModel, glm::vec3(0.0f, levY, 0.0f));
    }
    globalModel *= glm::mat4_cast(cubeRotation);
    
    // Process slice animation
    float easeT = 0.0f;
    float currentAngle = 0.0f;
    if (isAnimatingSlice) {
        sliceAnimTime += deltaTime * animationSpeed;
        float t = std::min(sliceAnimTime / sliceAnimDuration, 1.0f);
        
        // Pure Cubic Ease-Out for smooth, snappy layer turns without drift
        float invT = 1.0f - t;
        easeT = 1.0f - (invT * invT * invT);
        
        currentAngle = easeT * sliceAnimTargetAngle;
    }
    
    auto getFaceAxis = [](Face f) {
        switch(f) {
            case Face::U: return glm::vec3(0, 1, 0);
            case Face::D: return glm::vec3(0, -1, 0);
            case Face::L: return glm::vec3(-1, 0, 0);
            case Face::R: return glm::vec3(1, 0, 0);
            case Face::F: return glm::vec3(0, 0, 1);
            case Face::B: return glm::vec3(0, 0, -1);
            default: return glm::vec3(0,1,0);
        }
    };
    
    auto isCubieAffected = [](Face f, int x, int y, int z) {
        switch(f) {
            case Face::U: return y == 1;
            case Face::D: return y == -1;
            case Face::L: return x == -1;
            case Face::R: return x == 1;
            case Face::F: return z == 1;
            case Face::B: return z == -1;
            default: return false;
        }
    };
    
    glm::vec3 axis = getFaceAxis(animFace);
    glm::mat4 modelsArray[27];
    
    for (int i = 0; i < 27; ++i) {
        glm::mat4 localModel = renderCubies[i].baseTransform;
        
        if (isAnimatingSlice && isCubieAffected(animFace, renderCubies[i].gridX, renderCubies[i].gridY, renderCubies[i].gridZ)) {
            glm::mat4 pivot = glm::rotate(glm::mat4(1.0f), currentAngle, axis);
            localModel = pivot * localModel;
        }
        
        modelsArray[i] = globalModel * localModel;
    }
    
    GLint loc = glGetUniformLocation(shaderProgram, "models");
    glUniformMatrix4fv(loc, 27, GL_FALSE, glm::value_ptr(modelsArray[0]));
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(light.position));
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPosition));
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Bake animation when finished
    if (isAnimatingSlice && sliceAnimTime >= sliceAnimDuration) {
        glm::mat4 finalPivot = glm::rotate(glm::mat4(1.0f), sliceAnimTargetAngle, axis);
        for (int i = 0; i < 27; ++i) {
            if (isCubieAffected(animFace, renderCubies[i].gridX, renderCubies[i].gridY, renderCubies[i].gridZ)) {
                renderCubies[i].baseTransform = finalPivot * renderCubies[i].baseTransform;
                
                // Snap baseTransform matrix exactly to 90 degrees to eliminate floating-point drift
                for (int col = 0; col < 4; ++col) {
                    for (int row = 0; row < 4; ++row) {
                        renderCubies[i].baseTransform[col][row] = std::round(renderCubies[i].baseTransform[col][row]);
                    }
                }
                
                glm::vec3 pos(renderCubies[i].gridX, renderCubies[i].gridY, renderCubies[i].gridZ);
                glm::vec4 newPos = finalPivot * glm::vec4(pos, 1.0f);
                renderCubies[i].gridX = (int)std::round(newPos.x);
                renderCubies[i].gridY = (int)std::round(newPos.y);
                renderCubies[i].gridZ = (int)std::round(newPos.z);
            }
        }
        isAnimatingSlice = false;
    }
}

void CubeRenderer::updateRotationAnimation() {
    if (rotationAnimationTime < rotationAnimationDuration) {
        rotationAnimationTime += deltaTime * animationSpeed;
        // Normalized t clamped to [0,1] for smooth SLERP
        float t = std::min(rotationAnimationTime / rotationAnimationDuration, 1.0f);
        // Ease-in-out cubic for smoother feel
        t = t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        cubeRotation = glm::slerp(startRotation, targetRotation, t);
    } else {
        cubeRotation = targetRotation;
    }
}

void CubeRenderer::updateLevitation() {
    if (levitationEnabled) {
        levitationTime += deltaTime * levitationFrequency * 10.0f;
        if (levitationTime > 2.0f * 3.14159f) {
            levitationTime -= 2.0f * 3.14159f;
        }
    }
}

void CubeRenderer::updateCamera() {
    cameraPosition.x = cameraDistance * std::sin(cameraAngleY) * std::cos(cameraAngleX);
    cameraPosition.y = cameraDistance * std::sin(cameraAngleX);
    cameraPosition.z = cameraDistance * std::cos(cameraAngleY) * std::cos(cameraAngleX);
}

unsigned int CubeRenderer::compileTextShader() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec4 aColor;
        
        out vec4 FragColor;
        
        uniform mat4 projection;
        
        void main() {
            FragColor = aColor;
            gl_Position = projection * vec4(aPos, 1.0);
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec4 FragColor;
        out vec4 finalColor;
        void main() {
            finalColor = FragColor;
        }
    )";
    
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

void CubeRenderer::renderText() {
    if (solutionText.empty()) return;
    
    static char buffer[99999]; // static buffer
    int num_quads = stb_easy_font_print(0, 0, (char*)solutionText.c_str(), nullptr, buffer, sizeof(buffer));
    
    std::vector<unsigned int> textIndices;
    textIndices.reserve(num_quads * 6);
    for (int i = 0; i < num_quads; ++i) {
        textIndices.push_back(i*4 + 0);
        textIndices.push_back(i*4 + 1);
        textIndices.push_back(i*4 + 2);
        textIndices.push_back(i*4 + 0);
        textIndices.push_back(i*4 + 2);
        textIndices.push_back(i*4 + 3);
    }
    
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, num_quads * 4 * 16, buffer, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, textIndices.size() * sizeof(unsigned int), textIndices.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 16, (void*)12);
    glEnableVertexAttribArray(1);
    
    glUseProgram(textShaderProgram);
    
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, -1.0f, 1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(20.0f, windowHeight - 40.0f, 0.0f));
    model = glm::scale(model, glm::vec3(2.0f, 2.0f, 1.0f));
    
    projection = projection * model;
    
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glDisable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLES, textIndices.size(), GL_UNSIGNED_INT, 0);
    glEnable(GL_DEPTH_TEST);
    
    glBindVertexArray(0);
}

} // namespace cube_solver
