#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "fps_camera.h"
#include "player_controller.h"
#include "physics_types.h"

// Settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Global camera/controller
FPSCamera* gCamera = nullptr;
PlayerController* gPlayerController = nullptr;

// Mouse state
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// GLFW callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
        return;
    }
    
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos; // inversé : le haut c'est positif

    lastX = (float)xpos;
    lastY = (float)ypos;

    if (gPlayerController) {
        gPlayerController->processMouseInput(xoffset, yoffset);
    }
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (gPlayerController)
        gPlayerController->processInput(window, deltaTime);
}

void printDebugInfo(const PlayerController* controller) {
    static float debugTimer = 0.0f;
    static float debugInterval = 1.0f; // Print every 1 second
    
    debugTimer += 1.0f/60.0f; // Assuming ~60 FPS
    
    if (debugTimer >= debugInterval) {
        const MovementState& state = controller->getMovementState();
        
        std::cout << "=== STRAFE DEBUG ===" << std::endl;
        std::cout << "Speed: " << (int)state.speed << " units/sec" << std::endl;
        std::cout << "Max Speed: " << (int)state.maxSpeed << " units/sec" << std::endl;
        std::cout << "Efficiency: " << (int)(state.strafeEfficiency * 100.0f) << "%" << std::endl;
        std::cout << "Bhop Combo: " << state.consecutiveHops << std::endl;
        std::cout << "Air Time: " << state.airTime << "s" << std::endl;
        std::cout << "On Ground: " << (state.onGround ? "YES" : "NO") << std::endl;
        if (state.hitWall) {
            std::cout << "WALL HIT! Bounced." << std::endl;
        }
        std::cout << "===================" << std::endl;
        
        debugTimer = 0.0f;
    }
}

int main() {
    // Initialisation de GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Création de la fenêtre
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "TrueShot - Physics System", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialisation de GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Viewport et callbacks
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // OpenGL options
    glEnable(GL_DEPTH_TEST);

    // Création de la caméra et du controller avec nouveau système physique
    FPSCamera camera(glm::vec3(0.0f, Physics::PLAYER_HEIGHT, 3.0f));
    PlayerController playerController(&camera);
    gCamera = &camera;
    gPlayerController = &playerController;

    // Shaders
    Shader shader("shaders/basic.vert", "shaders/basic.frag");

    // Sol : VAO/VBO/EBO (plus grand pour tester)
    float floorVertices[] = {
        // positions           // couleurs (gris avec grille)
        -50.0f, 0.0f, -50.0f,  0.6f, 0.6f, 0.6f,
         50.0f, 0.0f, -50.0f,  0.6f, 0.6f, 0.6f,
         50.0f, 0.0f,  50.0f,  0.6f, 0.6f, 0.6f,
        -50.0f, 0.0f,  50.0f,  0.6f, 0.6f, 0.6f
    };
    unsigned int floorIndices[] = {
        0, 1, 2,
        2, 3, 0
    };
    unsigned int floorVAO, floorVBO, floorEBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glGenBuffers(1, &floorEBO);

    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // couleur
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Cube : VAO/VBO/EBO (target pour movement test)
    float cubeVertices[] = {
        // positions             // couleurs
        -0.5f,  0.5f,  0.5f,     1.0f, 0.0f, 0.0f,  // Front-top-left (rouge)
         0.5f,  0.5f,  0.5f,     0.0f, 1.0f, 0.0f,  // Front-top-right (vert)
         0.5f, -0.5f,  0.5f,     0.0f, 0.0f, 1.0f,  // Front-bottom-right (bleu)
        -0.5f, -0.5f,  0.5f,     1.0f, 1.0f, 0.0f,  // Front-bottom-left (jaune)
        -0.5f,  0.5f, -0.5f,     1.0f, 0.0f, 1.0f,  // Back-top-left (magenta)
         0.5f,  0.5f, -0.5f,     0.0f, 1.0f, 1.0f,  // Back-top-right (cyan)
         0.5f, -0.5f, -0.5f,     1.0f, 0.5f, 0.0f,  // Back-bottom-right (orange)
        -0.5f, -0.5f, -0.5f,     0.5f, 0.0f, 1.0f   // Back-bottom-left (violet)
    };
    unsigned int cubeIndices[] = {
        // Face avant
        0, 1, 2, 2, 3, 0,
        // Face droite
        1, 5, 6, 6, 2, 1,
        // Face arrière
        5, 4, 7, 7, 6, 5,
        // Face gauche
        4, 0, 3, 3, 7, 4,
        // Face dessus
        4, 5, 1, 1, 0, 4,
        // Face dessous
        3, 2, 6, 6, 7, 3
    };
    unsigned int cubeVAO, cubeVBO, cubeEBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // couleur
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Positions de cubes pour tester le movement
    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f, 0.5f, -5.0f),   // Cube devant
        glm::vec3(10.0f, 0.5f, 0.0f),   // Cube à droite
        glm::vec3(-10.0f, 0.5f, 0.0f),  // Cube à gauche
        glm::vec3(0.0f, 0.5f, -15.0f),  // Cube loin
        glm::vec3(5.0f, 2.0f, -5.0f),   // Cube en l'air
    };
    int numCubes = sizeof(cubePositions) / sizeof(cubePositions[0]);

    // Timing
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    std::cout << "=== TrueShot Physics System - OPTIMIZED ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "WASD - Move (try strafing while turning!)" << std::endl;
    std::cout << "SPACE - Jump/Bhop" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "STRAFE TIPS:" << std::endl;
    std::cout << "- Hold A/D while turning mouse for speed" << std::endl;
    std::cout << "- Best angle: ~30 degrees" << std::endl;
    std::cout << "- Chain jumps for bhop combo!" << std::endl;
    std::cout << "==========================================" << std::endl;

    // Boucle principale
    while (!glfwWindowShouldClose(window)) {
        // Calcul du deltaTime
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input processing
        processInput(window, deltaTime);
        
        // Physics update (avec fixed timestep)
        playerController.update(deltaTime);

        // Debug info
        printDebugInfo(&playerController);

        // Rendu
        glClearColor(0.05f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Matrices
        glm::mat4 projection = glm::perspective(glm::radians(75.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.getViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // Sol
        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);

        glBindVertexArray(floorVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Cubes de repère
        for (int i = 0; i < numCubes; ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            
            // Rotation lente pour voir le movement
            if (i < 4) { // Pas le cube en l'air
                model = glm::rotate(model, (float)glfwGetTime() * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            
            shader.setMat4("model", model);

            glBindVertexArray(cubeVAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        // Swap et events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
