#pragma once
#ifndef VK_SWAPCHAIN_H
#define VK_SWAPCHAIN_H
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <algorithm>


#include "jsvkUtilities.h"

namespace jsvk {
	


	class VulkanSwapchain {

	private:
		VkInstance m_pInstance;
		VkDevice m_pDevice;
		VkPhysicalDevice m_pPhysicalDevice;
		VkSurfaceFormatKHR m_surfaceFormat;
		VkPresentModeKHR m_presentMode;
		VkSwapchainKHR m_pSwapchain;

	public:
		// combined present and graphics queue index
		struct QueueFamilyIndices {
			uint32_t graphicsFamily = UINT32_MAX;
			uint32_t presentFamily = UINT32_MAX;
		} m_indices;
		//QueueFamilyIndices m_indices;
		uint32_t m_presentQueueIndex;
		VkQueue m_presentQueue;
		VkQueue m_graphicsQueue;

		struct SwapChainSupportDetails {
			VkSurfaceCapabilitiesKHR capabilites;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		} m_swapChainSupport;
		VkSurfaceKHR m_pSurface;
		VkFormat m_swapchainImageFormat;
		VkExtent2D m_extent;
		std::vector<VkImage> m_swapchainImages;
		std::vector<VkImageView> m_swapchainImageViews;
		uint32_t m_imageCount;

		VulkanSwapchain()
			: m_pInstance(VK_NULL_HANDLE)
			, m_pDevice(VK_NULL_HANDLE)
			, m_pPhysicalDevice(VK_NULL_HANDLE)
			, m_surfaceFormat()
			, m_presentMode()
			, m_pSwapchain(nullptr)
			, m_indices()
			, m_presentQueueIndex(UINT32_MAX)
			, m_presentQueue(VK_NULL_HANDLE)
			, m_graphicsQueue(VK_NULL_HANDLE)
			, m_swapChainSupport()
			, m_pSurface(VK_NULL_HANDLE)
			, m_swapchainImageFormat(VK_FORMAT_UNDEFINED)
			, m_extent()
			, m_swapchainImages()
			, m_swapchainImageViews()
			, m_imageCount(0)
		{

		}

		VkPhysicalDevice getPhysicalDevice(){
			return m_pPhysicalDevice;
		}

		VkDevice getLogicalDevice(){
			return m_pDevice;
		}

		~VulkanSwapchain() {
			std::cout << "Destroying swapchain" << std::endl;

			if (m_pSwapchain != VK_NULL_HANDLE && m_pDevice != VK_NULL_HANDLE)
				{
					for (uint32_t i = 0; i < m_swapchainImages.size(); i++)
					{
						vkDestroyImageView(m_pDevice, m_swapchainImageViews[i], nullptr);
					}
					vkDestroySwapchainKHR(m_pDevice, m_pSwapchain, nullptr);
			}

				
			if (m_pSurface != VK_NULL_HANDLE && m_pInstance != VK_NULL_HANDLE)
				{
					vkDestroySurfaceKHR(m_pInstance, m_pSurface, nullptr);
			}
			
			m_pSurface = VK_NULL_HANDLE;
			m_pSwapchain = VK_NULL_HANDLE;
			
		}

		void createSurface(VkInstance instance, GLFWwindow* window) {
			m_pInstance = instance;
			VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &m_pSurface));
		}

		// Connect the swapchain to the vulkandevice
		void connect(jsvk::VulkanDevice* vulkandevice, GLFWwindow* window) {
			m_pPhysicalDevice = vulkandevice->m_pPhysicalDevice;
			m_pDevice = vulkandevice->m_pLogicalDevice;

			m_indices = findQueueFamilies(m_pPhysicalDevice, m_pSurface);
			m_presentQueueIndex = m_indices.presentFamily;


			if (m_presentQueueIndex == UINT32_MAX) {
				throw std::runtime_error("Graphics card does not support a present queue.");
			}

			vkGetDeviceQueue(m_pDevice, m_indices.graphicsFamily, 0, &m_graphicsQueue);
			vkGetDeviceQueue(m_pDevice, m_indices.presentFamily, 0, &m_presentQueue);

			m_swapChainSupport = querySwapChainSupport(m_pPhysicalDevice, m_pSurface);

			m_surfaceFormat = chooseSwapSurfaceFormat(m_swapChainSupport.formats);
			m_presentMode = chooseSwapPresentMode(m_swapChainSupport.presentModes);
			m_extent = chooseSwapExtent(m_swapChainSupport.capabilites, window);
		}

		void createSwapchain(int width = 0, int height = 0) {
			VkSwapchainKHR oldSwapchain = m_pSwapchain;


			// However, simply sticking to this minimum means that we may sometimes have to wait on 
			// the driver to complete internal operations before we can acquire another image to render to.
			// Therefore it is recommended to request at least one more image than the minimum:
			m_imageCount = m_swapChainSupport.capabilites.minImageCount + 1;
			if (m_swapChainSupport.capabilites.maxImageCount > 0 && m_imageCount > m_swapChainSupport.capabilites.maxImageCount) {
				m_imageCount = m_swapChainSupport.capabilites.maxImageCount;
			}

			VkSwapchainCreateInfoKHR createInfo = jsvk::init::swapchainCreateInfoHKR();
			createInfo.surface = m_pSurface;

			createInfo.minImageCount = m_imageCount;
			createInfo.imageFormat = m_surfaceFormat.format;
			createInfo.imageColorSpace = m_surfaceFormat.colorSpace;
			// TODO: more elegant solution needed
			if (width != 0 || height != 0) {
				createInfo.imageExtent.width = width;
				createInfo.imageExtent.height = height;
				m_extent.width = width;
				m_extent.height = height;

			}
			else {
				createInfo.imageExtent = m_extent;
			}
			// This needs to be more than one for sterioscopic 3D applications
			createInfo.imageArrayLayers = 1;
			// Change this for deferred shading
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			uint32_t queueFamilyIndices[] = { m_indices.graphicsFamily, m_indices.presentFamily };

			if (m_indices.graphicsFamily != m_indices.presentFamily) {
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else {
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0; // Optional
				createInfo.pQueueFamilyIndices = nullptr; // Optional
			}

			createInfo.preTransform = m_swapChainSupport.capabilites.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

			createInfo.presentMode = m_presentMode;
			createInfo.clipped = VK_TRUE;

			//createInfo.oldSwapchain = VK_NULL_HANDLE;
			createInfo.oldSwapchain = oldSwapchain;

			VK_CHECK(vkCreateSwapchainKHR(m_pDevice, &createInfo, nullptr, &m_pSwapchain));

			// If an existing swap chain is re-created, destroy the old swap chain
			// This also cleans up all the presentable images
			if (oldSwapchain != VK_NULL_HANDLE)
			{
				for (uint32_t i = 0; i < m_imageCount; i++)
				{
					vkDestroyImageView(m_pDevice, m_swapchainImageViews[i], nullptr);
				}
				vkDestroySwapchainKHR(m_pDevice, oldSwapchain, nullptr);
			}


			VK_CHECK(vkGetSwapchainImagesKHR(m_pDevice, m_pSwapchain, &m_imageCount, nullptr));
			m_swapchainImages.resize(m_imageCount);
			VK_CHECK(vkGetSwapchainImagesKHR(m_pDevice, m_pSwapchain, &m_imageCount, m_swapchainImages.data()));

			m_swapchainImageFormat = m_surfaceFormat.format;


			// do some recreate swapchain when screen is resized

			m_swapchainImageViews.resize(m_swapchainImages.size());

			for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
				// If you were working on a stereographic 3D application, then you would 
				// create a swap chain with multiple layers. You could then create multiple
				// image views for each image representing the views for the left and right 
				// eyes by accessing different layers.

				// Create Image view
				m_swapchainImageViews[i] = jsvk::createImageView(m_pDevice, m_swapchainImages[i], m_swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			}
		}

		QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
			QueueFamilyIndices indices = {};

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			int i = 0;
			for (const auto& queueFamily : queueFamilies) {
				if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					indices.graphicsFamily = i;
				}

				VkBool32 presentSupport;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

				if (queueFamily.queueCount > 0 && presentSupport) {
					indices.presentFamily = i;
				}

				if ((indices.graphicsFamily != UINT32_MAX) && (indices.presentFamily != UINT32_MAX)) {
					break;
				}

				++i;
			}
			return indices;
		}

		SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
			SwapChainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilites);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

			if (formatCount != 0) {
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0) {
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

			for (const auto& availableFormat : availableFormats) {
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return availableFormat;
				}
			}

			// IF desired colorspace and format is not found we settle for first format
			return availableFormats[0];
		}

		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {

			for (const auto& availablePresentMode : availablePresentModes) {
				if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
					return availablePresentMode;
				}
				//else if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				//	return availablePresentMode;
				//} //else if (availablePresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
				//	return availablePresentMode;
				//}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}

		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
			if (capabilities.currentExtent.width != UINT32_MAX) {
				return capabilities.currentExtent;
			}
			else {
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);

				VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

				actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
				actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

				return actualExtent;
			}
		}

		VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE) {
			VkPresentInfoKHR presentInfo = jsvk::init::presentInfoKHR();
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &m_pSwapchain;
			presentInfo.pImageIndices = &imageIndex;

			if (waitSemaphore != VK_NULL_HANDLE) {
				presentInfo.pWaitSemaphores = &waitSemaphore;
				presentInfo.waitSemaphoreCount = 1;
			}

			return vkQueuePresentKHR(queue, &presentInfo);
		}

		VkResult acquireNextImage(VkSemaphore presentCompleteSempaphore, uint32_t* imageIndex) {
			// timeout of UINT64_MAX to always wait until the next image is aquired
			return vkAcquireNextImageKHR(m_pDevice, m_pSwapchain, UINT64_MAX, presentCompleteSempaphore, (VkFence)nullptr, imageIndex);
		}
	};
}

#endif // VK_SWAPCHAIN_H