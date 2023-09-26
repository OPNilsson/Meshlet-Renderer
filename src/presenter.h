#pragma once
#ifndef HEADERGUARD_PRESENTER_H
#define HEADERGUARD_PRESENTER_H

//#include "jsvr/vrHmd.h"
#include "jsvk/jsvkSwapchain.h"

#include <GLFW/glfw3.h>

#include <string>
#include <vector>



namespace jsk {
	class Presenter {
	private:
		//vr::VRVulkanTextureData_t m_vulkanData;
		jsvk::VulkanSwapchain* m_pSwapchain;
		GLFWwindow* m_pWindow;
		bool m_usingVR;
		bool m_usingSwapchain;
		bool m_framebufferResized;

		static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
			auto app = reinterpret_cast<Presenter*>(glfwGetWindowUserPointer(window));
			app->m_framebufferResized = true;
		}

	public:
		//jsvr::HmdInstance* m_pHmd;

		Presenter();

		~Presenter();
		//void updateHMDPose() { m_pHmd->updatehmdPose(); }
		void setUpVulkanData(jsvk::VulkanDevice* device, int width, int height, VkSampleCountFlagBits msaaSamples, VkInstance* instance);
		//vr::VRVulkanTextureData_t getVulkanData() { return m_vulkanData; };
		void* getPresentationEngine();
		void initGLFW(int width, int height);
		GLFWwindow* getWindow() { return m_pWindow; };
		//void initVRPresentation(jsvr::HmdInstance* pHmd);
		void initSwapchainPresentation(VkInstance instance);
		VkSurfaceKHR getSurface();
		void connectToWindow(jsvk::VulkanDevice* device, int width, int height);
		VkFormat Presenter::getImageFormat();
		VkExtent2D getExtent();
		std::vector<VkImageView> getImageViews();

	};
}


#endif // HEADERGUARD_PRESENTER_H