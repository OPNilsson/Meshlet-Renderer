#include "lodCamera.hpp"
#include "../inputHandler.h"

#include <chrono>
#include <algorithm>
#include <array>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

extern lod::KeyboardController::KeyMappings keys;

namespace lod
{
    static glm::vec2 getLookDirection(
        GLFWwindow *_pWindow)
    {
        glm::vec2 direction(0.0f, 0.0f);
        glm::vec2 right(0.0f, 1.0f);
        glm::vec2 up(1.0f, 0.0f);

        if (glfwGetKey(_pWindow, keys.lookUp) == GLFW_PRESS)
        {
            direction += up;
        }

        if (glfwGetKey(_pWindow, keys.lookDown) == GLFW_PRESS)
        {
            direction -= up;
        }

        if (glfwGetKey(_pWindow, keys.lookLeft) == GLFW_PRESS)
        {
            direction -= right;
        }

        if (glfwGetKey(_pWindow, keys.lookRight) == GLFW_PRESS)
        {
            direction += right;
        }

        if (glm::length(direction) != 0.0f)
        {
            direction = glm::normalize(direction);
        }

        return direction;
    }

    static glm::vec3 getMoveDirection(
        GLFWwindow *_pWindow,
        glm::vec3 _forward,
        glm::vec3 _up)
    {
        glm::vec3 direction(0.0f, 0.0f, 0.0f);

        if (glfwGetKey(_pWindow, keys.moveForward) == GLFW_PRESS)
        {
            direction += _forward;
        }

        if (glfwGetKey(_pWindow, keys.moveBackward) == GLFW_PRESS)
        {
            direction -= _forward;
        }

        if (glfwGetKey(_pWindow, keys.moveLeft) == GLFW_PRESS)
        {
            direction -= glm::normalize(glm::cross(_forward, _up));
        }

        if (glfwGetKey(_pWindow, keys.moveRight) == GLFW_PRESS)
        {
            direction += glm::normalize(glm::cross(_forward, _up));
        }

        if (glfwGetKey(_pWindow, keys.moveUp) == GLFW_PRESS)
        {
            direction += _up;
        }

        if (glfwGetKey(_pWindow, keys.moveDown) == GLFW_PRESS)
        {
            direction -= _up;
        }

        if (glm::length(direction) != 0.0f)
        {
            direction = glm::normalize(direction);
        }

        return direction;
    }

    float getFOV(GLFWwindow *_pWindow, float fov_current)
    {
        float fov = fov_current;

        float increment = (glfwGetKey(_pWindow, keys.moveBoost) == GLFW_PRESS) ? 0.01 : 0.50;

        if ((glfwGetKey(_pWindow, keys.INC_FOV[0]) == GLFW_PRESS) || (glfwGetKey(_pWindow, keys.INC_FOV[1]) == GLFW_PRESS))
        {
            fov += increment;
        }
        else if ((glfwGetKey(_pWindow, keys.DEC_FOV[0]) == GLFW_PRESS) || (glfwGetKey(_pWindow, keys.DEC_FOV[1]) == GLFW_PRESS))
        {
            fov -= increment;
        }
        else if ((glfwGetKey(_pWindow, keys.RESET_FOV[0]) == GLFW_PRESS) || (glfwGetKey(_pWindow, keys.RESET_FOV[1]) == GLFW_PRESS))
        {
            fov = 45.0f;
        }

        return fov;
    }

    void buildFrustum(Camera &cam)
    {
        glm::mat4 view = cam.view;
        glm::mat4 projection = cam.projection;
        glm::mat4 vp = projection * view; // view-projection matrix

        Frustum frustum;

        // Left plane
        frustum.left.normal.x = vp[0][3] + vp[0][0];
        frustum.left.normal.y = vp[1][3] + vp[1][0];
        frustum.left.normal.z = vp[2][3] + vp[2][0];
        frustum.left.distance = vp[3][3] + vp[3][0];

        // Right plane
        frustum.right.normal.x = vp[0][3] - vp[0][0];
        frustum.right.normal.y = vp[1][3] - vp[1][0];
        frustum.right.normal.z = vp[2][3] - vp[2][0];
        frustum.right.distance = vp[3][3] - vp[3][0];

        // Bottom plane
        frustum.bottom.normal.x = vp[0][3] + vp[0][1];
        frustum.bottom.normal.y = vp[1][3] + vp[1][1];
        frustum.bottom.normal.z = vp[2][3] + vp[2][1];
        frustum.bottom.distance = vp[3][3] + vp[3][1];

        // Top plane
        frustum.top.normal.x = vp[0][3] - vp[0][1];
        frustum.top.normal.y = vp[1][3] - vp[1][1];
        frustum.top.normal.z = vp[2][3] - vp[2][1];
        frustum.top.distance = vp[3][3] - vp[3][1];

        // Near plane
        frustum.near.normal.x = vp[0][3] + vp[0][2];
        frustum.near.normal.y = vp[1][3] + vp[1][2];
        frustum.near.normal.z = vp[2][3] + vp[2][2];
        frustum.near.distance = vp[3][3] + vp[3][2];

        // Far plane
        frustum.far.normal.x = vp[0][3] - vp[0][2];
        frustum.far.normal.y = vp[1][3] - vp[1][2];
        frustum.far.normal.z = vp[2][3] - vp[2][2];
        frustum.far.distance = vp[3][3] - vp[3][2];

        // Normalize the planes
        for (auto &plane : {&frustum.left, &frustum.right, &frustum.bottom, &frustum.top, &frustum.near, &frustum.far})
        {
            float length = glm::length(plane->normal);
            plane->normal /= length;
            plane->distance /= length;
        }

        cam.frustum = frustum;
    }

    void updateCamera(
        GLFWwindow *_pWindow,
        float _deltaTime,
        Camera &_cam)
    {
        int framebufferWidth;
        int framebufferHeight;
        glfwGetFramebufferSize(_pWindow, &framebufferWidth, &framebufferHeight);

        _cam.aspect = float(framebufferWidth) / float(framebufferHeight);

        glm::vec2 lookDirection = getLookDirection(_pWindow);
        _cam.yaw += lookDirection.y * _deltaTime * _cam.sensitivity;
        _cam.pitch += lookDirection.x * _deltaTime * _cam.sensitivity;
        _cam.pitch = glm::clamp(_cam.pitch, -89.99f, 89.99f);

        glm::vec3 forward;
        forward.x = glm::cos(glm::radians(_cam.yaw)) * glm::cos(glm::radians(_cam.pitch));
        forward.y = glm::sin(glm::radians(_cam.pitch));
        forward.z = glm::sin(glm::radians(_cam.yaw)) * glm::cos(glm::radians(_cam.pitch));

        float moveSpeed = 0.0f;
        if (glfwGetKey(_pWindow, keys.moveBoost) == GLFW_PRESS)
        {
            moveSpeed = _cam.boostSpeed;
        }
        else if (glfwGetKey(_pWindow, keys.moveSlow) == GLFW_PRESS)
        {
            moveSpeed = _cam.slowSpeed;
        }
        else
        {
            moveSpeed = _cam.moveSpeed;
        }

        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::vec3 moveDirection = getMoveDirection(_pWindow, forward, up);

        // This makes the camera move towards the worldCenter every frame.
        if (_cam.autoMove == true)
        {
            glm::vec3 directionToTarget = glm::normalize(_cam.worldCenter - _cam.position);
            moveDirection += directionToTarget;
        }

        _cam.position += moveDirection * _deltaTime * moveSpeed;

        // Calculate the velocity
        _cam.velocity = (_cam.position - _cam.oldPostion) / _deltaTime;
        _cam.oldPostion = _cam.position;

        _cam.fov = getFOV(_pWindow, _cam.fov);

        _cam.view = glm::lookAt(_cam.position, _cam.position + forward, up);
        _cam.projection = glm::perspective(glm::radians(_cam.fov), _cam.aspect, _cam.near, _cam.far);

        buildFrustum(_cam);
    }

}