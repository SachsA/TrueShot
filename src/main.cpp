#include "shader.h"
#include "fps_camera.h"
#include "player_controller.h"
#include "weapon_system.h"
#include "physics_types.h"
#include "audio_system.h"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Audio system
AudioSystem* gAudioSystem = nullptr;

// Global camera/controller/weapons
FPSCamera* gCamera = nullptr;
PlayerController* gPlayerController = nullptr;
WeaponSystem* gWeaponSystem = nullptr;

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

    // Player movement
    if (gPlayerController)
        gPlayerController->processInput(window, deltaTime);
    
    // Weapon input
    if (gWeaponSystem)
        gWeaponSystem->processInput(window, deltaTime);

    // Audio controls
    if (gAudioSystem) {
        static bool plusPressed = false, minusPressed = false;
        static bool mPressed = false, nPressed = false;
        
        // Volume control
        bool currentPlus = glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || 
                          glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS;
        bool currentMinus = glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || 
                           glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS;
        
        if (currentPlus && !plusPressed) {
            float vol = std::min(1.0f, gAudioSystem->getMasterVolume() + 0.1f);
            gAudioSystem->setMasterVolume(vol);
            std::cout << "Master Volume: " << (vol * 100.0f) << "%" << std::endl;
        }
        if (currentMinus && !minusPressed) {
            float vol = std::max(0.0f, gAudioSystem->getMasterVolume() - 0.1f);
            gAudioSystem->setMasterVolume(vol);
            std::cout << "Master Volume: " << (vol * 100.0f) << "%" << std::endl;
        }
        
        // Debug toggle
        bool currentM = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
        if (currentM && !mPressed) {
            gAudioSystem->toggleDebugVisualization();
        }
        
        plusPressed = currentPlus;
        minusPressed = currentMinus;
        mPressed = currentM;
    }
}

void printDebugInfo(const PlayerController* controller, const WeaponSystem* weapons) {
    static float debugTimer = 0.0f;
    static float debugInterval = 2.0f; // Print every 2 seconds
    
    debugTimer += 1.0f/60.0f; // Assuming ~60 FPS
    
    if (debugTimer >= debugInterval) {
        const MovementState& movement = controller->getMovementState();
        
        std::cout << "\n=== TRUESHOT DEBUG ===" << std::endl;
        
        // Movement info
        std::cout << "MOVEMENT:" << std::endl;
        std::cout << "  Speed: " << (int)movement.speed << " units/sec" << std::endl;
        std::cout << "  Max Speed: " << (int)movement.maxSpeed << " units/sec" << std::endl;
        std::cout << "  Bhop Combo: " << movement.consecutiveHops << std::endl;
        std::cout << "  On Ground: " << (movement.onGround ? "YES" : "NO") << std::endl;
        
        // Weapon info
        if (weapons && weapons->getCurrentWeapon()) {
            const auto* weapon = weapons->getCurrentWeapon();
            const auto& weaponState = weapons->getWeaponState();
            
            std::cout << "WEAPON: " << weapon->name << std::endl;
            std::cout << "  Ammo: " << weaponState.currentAmmo << "/" << weaponState.reserveAmmo << std::endl;
            std::cout << "  Spread: " << weapons->getCurrentSpread().x << "°" << std::endl;
            std::cout << "  Recoil: (" << weaponState.currentRecoil.x << ", " << weaponState.currentRecoil.y << ")" << std::endl;
            std::cout << "  ADS: " << (weaponState.adsProgress * 100.0f) << "%" << std::endl;
            std::cout << "  State: " << (int)weaponState.state << std::endl;
        }
        
        std::cout << "=====================\n" << std::endl;
        
        debugTimer = 0.0f;
    }
}

void printControls() {
    std::cout << "=== TRUESHOT - Tactical FPS ===" << std::endl;
    std::cout << "\nMOVEMENT CONTROLS:" << std::endl;
    std::cout << "  WASD - Move (strafe while turning for speed!)" << std::endl;
    std::cout << "  SPACE - Jump/Bhop" << std::endl;
    std::cout << "  Mouse - Look around" << std::endl;
    
    std::cout << "\nWEAPON CONTROLS:" << std::endl;
    std::cout << "  Mouse1 - Fire" << std::endl;
    std::cout << "  Mouse2 - Aim Down Sights (ADS)" << std::endl;
    std::cout << "  R - Reload" << std::endl;
    std::cout << "  1-5 - Switch weapons:" << std::endl;
    std::cout << "    1 - Glock-18" << std::endl;
    std::cout << "    2 - Desert Eagle" << std::endl;
    std::cout << "    3 - AK-47" << std::endl;
    std::cout << "    4 - M4A4" << std::endl;
    std::cout << "    5 - AWP" << std::endl;

    std::cout << "\nAUDIO CONTROLS:" << std::endl;
    std::cout << "  + / - - Master volume" << std::endl;
    std::cout << "  M - Toggle audio debug info" << std::endl;
    std::cout << "  N - Toggle own footsteps" << std::endl;
    
    std::cout << "\nTIPS:" << std::endl;
    std::cout << "  • Strafe jump for speed (A/D + mouse turn)" << std::endl;
    std::cout << "  • Crouch reduces spread" << std::endl;
    std::cout << "  • Moving increases spread" << std::endl;
    std::cout << "  • ADS for better accuracy" << std::endl;
    std::cout << "  • Learn recoil patterns for spray control!" << std::endl;
    
    std::cout << "\nESC - Exit" << std::endl;
    std::cout << "==============================\n" << std::endl;
}

int main() {
    // Print controls first
    printControls();
    
    // Initialisation de GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Création de la fenêtre
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "TrueShot - Tactical FPS", nullptr, nullptr);
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

    // Création de la caméra et du controller
    FPSCamera camera(glm::vec3(0.0f, Physics::PLAYER_HEIGHT, 3.0f));
    PlayerController playerController(&camera);
    WeaponSystem weaponSystem(&camera, &playerController);
    AudioSystem audioSystem;
    
    // Set global pointers
    gCamera = &camera;
    gPlayerController = &playerController;
    gWeaponSystem = &weaponSystem;
    gAudioSystem = &audioSystem;

    // Initialize audio system
    if (!audioSystem.initialize()) {
        std::cerr << "Failed to initialize audio system" << std::endl;
    }
    weaponSystem.setAudioSystem(&audioSystem);

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

    // Targets pour tir (cubes colorés)
    float cubeVertices[] = {
        // positions             // couleurs (différentes faces)
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

    // Positions de cubes/targets pour tester le tir
    glm::vec3 targetPositions[] = {
        glm::vec3(0.0f, 1.0f, -10.0f),   // Target proche
        glm::vec3(5.0f, 1.5f, -15.0f),   // Target à droite
        glm::vec3(-5.0f, 1.5f, -15.0f),  // Target à gauche
        glm::vec3(0.0f, 2.0f, -25.0f),   // Target loin en hauteur
        glm::vec3(10.0f, 1.0f, -20.0f),  // Target très à droite
        glm::vec3(-10.0f, 1.0f, -20.0f), // Target très à gauche
        glm::vec3(0.0f, 0.5f, -35.0f),   // Target très loin
        glm::vec3(3.0f, 3.0f, -12.0f),   // Target en hauteur
    };
    int numTargets = sizeof(targetPositions) / sizeof(targetPositions[0]);

    // Crosshair simple (sera dans le HUD plus tard)
    float crosshairVertices[] = {
        // Ligne horizontale
        -0.02f, 0.0f, 0.0f,  1.0f, 1.0f, 1.0f,
         0.02f, 0.0f, 0.0f,  1.0f, 1.0f, 1.0f,
        // Ligne verticale
         0.0f, -0.02f, 0.0f, 1.0f, 1.0f, 1.0f,
         0.0f,  0.02f, 0.0f, 1.0f, 1.0f, 1.0f
    };
    unsigned int crosshairVAO, crosshairVBO;
    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);

    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Timing
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    std::cout << "TrueShot initialized! Ready for tactical action!" << std::endl;

    // Boucle principale
    while (!glfwWindowShouldClose(window)) {
        // Calcul du deltaTime
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input processing
        processInput(window, deltaTime);
        
        // Physics/weapons/audio update
        playerController.update(deltaTime);
        weaponSystem.update(deltaTime);
        audioSystem.update(deltaTime);

        audioSystem.setListenerFromCamera(&camera, &playerController);

        // Debug info
        printDebugInfo(&playerController, &weaponSystem);

        // Rendu
        glClearColor(0.05f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Matrices de projection et vue
        float fov = 75.0f;
        
        // Adjust FOV for ADS
        if (weaponSystem.isAiming() && weaponSystem.getCurrentWeapon()) {
            float adsFOV = fov * weaponSystem.getCurrentWeapon()->stats.adsFOVMultiplier;
            float adsProgress = weaponSystem.getWeaponState().adsProgress;
            fov = glm::mix(fov, adsFOV, adsProgress);
        }
        
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.getViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // Rendu du sol
        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);

        glBindVertexArray(floorVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Rendu des targets/cubes
        for (int i = 0; i < numTargets; ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, targetPositions[i]);
            
            // Rotation lente pour les voir bouger
            model = glm::rotate(model, (float)glfwGetTime() * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
            
            // Scale différente selon la distance pour le challenge
            float distance = glm::length(targetPositions[i]);
            float scale = 1.0f + (distance / 50.0f); // Plus loin = plus gros
            model = glm::scale(model, glm::vec3(scale));
            
            shader.setMat4("model", model);

            glBindVertexArray(cubeVAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        // Crosshair simple (rendu en dernier, sans depth test)
        glDisable(GL_DEPTH_TEST);
        
        model = glm::mat4(1.0f);
        // Position le crosshair au centre de l'écran
        glm::vec3 cameraPos = playerController.getPosition();
        glm::vec3 cameraForward = camera.getForward();
        model = glm::translate(model, cameraPos + cameraForward * 2.0f);
        
        shader.setMat4("model", model);

        glBindVertexArray(crosshairVAO);
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, 4);
        glBindVertexArray(0);
        
        glEnable(GL_DEPTH_TEST);

        // Swap et events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteBuffers(1, &floorEBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);
    glDeleteVertexArrays(1, &crosshairVAO);
    glDeleteBuffers(1, &crosshairVBO);
    audioSystem.shutdown();

    glfwTerminate();
    return 0;
}
