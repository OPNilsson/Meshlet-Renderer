#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <chrono>
#include <vector>
#include <unordered_map>

namespace lod
{
    struct Plane
    {
        glm::vec3 normal;
        float distance;
    };

    struct Frustum
    {
        lod::Plane left, right, top, bottom, near, far;
    };

    struct resolution
    {
        int x = 1280;
        int y = 720;
    }; // struct resolution

    struct Camera
    {
        float fov_default = 45.0f;
        float fov = 45.0f;
        float aspect = 1.0f;
        float near = 0.01f;
        float far = 10'000.0f;

        float lookSpeed = 0.01f;   // The speed at which the camera rotates
        float moveSpeed = 0.01f;   // The speed at which the camera moves
        float boostSpeed = 0.05f;  // The speed at which the camera moves when boost is held
        float slowSpeed = 0.0005f; // The speed at which the camera moves when slow is held

        float sensitivity = 0.05f;

        int offset = -1; // The LoD mesh offset to start drawing at
        int start = -1;  // The meshlet index to start drawing
        int end = -1;    // The meshlet index to stop drawing

        float pitch = 0.0f;
        float yaw = 0.0f;

        glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);

        float pixelThreshold = 0.005f;                                                                          // Min size of a node to check
        float SSEThreshold = 0.75f;                                                                             // The SSE threshold to use for the meshlet
        std::vector<float> thresholds = {0.5f, 1.5f, 2.0f, 3.0f, 4.0f, 5.0f, 8.0f, 10.0f, 15.0f, 18.0f, 20.0f}; // Good for the bunny and teapot

        glm::vec3 position = glm::vec3(-10.0f, 0.0f, 0.0f);
        glm::vec3 oldPostion = glm::vec3(-10.0f, 0.0f, 0.0f);

        glm::mat4 view = glm::lookAt(position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, near, far);

        int lod = 0;

        bool locked = false;
        glm::vec3 lockedPos = position;
        int lockedLOD = 0;

        std::chrono::steady_clock::time_point lastPrintTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point lastUpdateTime = std::chrono::steady_clock::now();
        std::unordered_map<unsigned int, uint8_t> drawnLODs;

        std::vector<bool> drawnModels;

        resolution res;

        glm::vec3 worldCenter = glm::vec3(0.0f, 0.0f, 0.0f);

        bool autoMove = false; // Will move the camera towards the center of the scene

        Frustum frustum;

        bool discrete = false;

    }; // struct Camera

    void updateCamera(
        GLFWwindow *_pWindow,
        float _deltaTime,
        Camera &_cam);

    void buildFrustum(Camera &cam);

} // namespace lod
