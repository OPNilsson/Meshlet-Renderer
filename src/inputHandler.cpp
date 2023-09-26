#include "inputHandler.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "jsvk/lodCamera.hpp"

// std includes
#include <stdexcept>
#include <array>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <iomanip>

extern int CURRENTPIPE;
extern float ROTATION;
extern int HOTRELOAD;

extern lod::Camera camera;

extern int MAX_LOD;

inline lod::KeyboardController::KeyMappings keys;

extern bool DistanceThresholds;

void limitLockedLOD()
{
	if (camera.lockedLOD < -1)
	{
		camera.lockedLOD = -1;
	}
	else if (camera.lockedLOD > MAX_LOD)
	{
		camera.lockedLOD = MAX_LOD;
	}
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{

	if (key == keys.pipeline && action == GLFW_PRESS)
		// CURRENTPIPE = (CURRENTPIPE + 3) % 9;
		CURRENTPIPE = CURRENTPIPE == 0 ? CURRENTPIPE + 2 : (CURRENTPIPE + 1) % 4;
	else if (key == keys.rotate_left && (action == GLFW_PRESS || action == GLFW_REPEAT))
		ROTATION += 3.0f;
	else if (key == keys.rotate_right && (action == GLFW_PRESS || action == GLFW_REPEAT))
		ROTATION -= 3.0f;
	else if (key == keys.reload_pipeline && action == GLFW_PRESS)
		HOTRELOAD = 1;

	// The following are camera controls that are here in order to not be called multiple times per frame
	// This enables a flag which will use the position of the camera when the key was pressed
	if (key == keys.lockLOD && action == GLFW_PRESS)
	{
		if (!camera.locked)
		{
			camera.locked = true;
			camera.lockedPos = camera.position;
		}
		else
		{
			camera.locked = false;
		}
	}

	if (key == keys.discrete_toggle && action == GLFW_PRESS)
	{
		camera.discrete = !camera.discrete;
	}

	if (key == keys.change_threshold && action == GLFW_PRESS)
	{
		DistanceThresholds = !DistanceThresholds;
	}

	if (key == keys.autoMove && action == GLFW_PRESS)
	{
		camera.autoMove = !camera.autoMove;
	}

	// Increase/Decrease LoD manually when lod is unlocked
	if (key == keys.INC_LOD && action == GLFW_PRESS)
	{
		camera.lockedLOD++;

		limitLockedLOD();
	}
	else if (key == keys.DEC_LOD && action == GLFW_PRESS)
	{
		camera.lockedLOD--;

		limitLockedLOD();
	}
}

/*
void ProcessVREvent(const vr::VREvent_t event) {
	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceActivated:
	{
		std::cout << "New device " << event.trackedDeviceIndex << " connected." << std::endl;
	}
	break;
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		std::cout << "Device " << event.trackedDeviceIndex << " disconnected." << std::endl;
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		std::cout << "Device " << event.trackedDeviceIndex << " updated." << std::endl;
	}
	break;
	}
}
*/

/*
void handleInput(vr::IVRSystem* m_pHmd) {
	vr::VREvent_t event;

	while (m_pHmd->PollNextEvent(&event, sizeof(event))) {
		ProcessVREvent(event);
	}

	for (vr::TrackedDeviceIndex_t device = 0; device < vr::k_unMaxTrackedDeviceCount; ++device) {
		vr::VRControllerState_t state;
		if (m_pHmd->GetControllerState(device, &state, sizeof(state))) {
			if (vr::ButtonMaskFromId(vr::EVRButtonId::k_EButton_SteamVR_Trigger) & state.ulButtonPressed)
				std::cout << "Pressing trigger" << std::endl;
			//state.ulButtonPressed
		}
	}
}
*/