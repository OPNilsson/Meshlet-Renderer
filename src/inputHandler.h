#pragma once
#ifndef HEADERGUARD_INPUTHANDLER_H
#define HEADERGUARD_INPUTHANDLER_H

// #include <headers/openvr.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <vector>

// void handleInput(vr::IVRSystem* m_pHmd);
// void ProcessVREvent(const vr::VREvent_t event);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

namespace lod
{
    class KeyboardController
    {
    public:
        struct KeyMappings
        {
            // Native Jinsoku keys
            int pipeline = GLFW_KEY_P;
            int rotate_left = GLFW_KEY_J;
            int rotate_right = GLFW_KEY_L;

            int reload_pipeline = GLFW_KEY_R;

            // Added LOD keys
            std::vector<int> INC_FOV = {GLFW_KEY_KP_ADD, GLFW_KEY_EQUAL};
            std::vector<int> DEC_FOV = {GLFW_KEY_KP_SUBTRACT, GLFW_KEY_MINUS};
            std::vector<int> RESET_FOV = {GLFW_KEY_ENTER, GLFW_KEY_KP_ENTER};

            int change_threshold = GLFW_KEY_T;

            int autoMove = GLFW_KEY_M;
            int discrete_toggle = GLFW_KEY_F;

            // Camera Movement
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;
            int moveUp = GLFW_KEY_SPACE;
            int moveDown = GLFW_KEY_LEFT_SHIFT;

            int moveBoost = GLFW_KEY_LEFT_ALT;
            int moveSlow = GLFW_KEY_LEFT_CONTROL;

            int lookLeft = GLFW_KEY_LEFT;
            int lookRight = GLFW_KEY_RIGHT;
            int lookUp = GLFW_KEY_UP;
            int lookDown = GLFW_KEY_DOWN;

            // LOD keys
            int lockLOD = GLFW_KEY_L;
            int INC_LOD = GLFW_KEY_PERIOD;
            int DEC_LOD = GLFW_KEY_COMMA;
        }; // struct KeyMappings

    }; // class KeyboardController
}

#endif // HEADERGUARD_INPUTHANDLER_H