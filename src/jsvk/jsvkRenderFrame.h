#pragma once
#ifndef HEADERGUARD_RENDERFRAME
#define HEADERGUARD_RENDERFRAME

#include <vulkan/vulkan.h>
#include <vector>

#include "jsvkDevice.h"
#include "jsvkSwapchain.h"



#endif // HEADERGUARD_RENDERFRAME


namespace jsvk {

	class RenderFrame
	{
	public:
		VkSemaphore m_imageAvailableSemaphore;
		VkSemaphore m_renderFinishedSemaphore;
		VkFence m_inFlightFence;
		jsvk::VulkanDevice* m_pVulkanDevice;
		VkCommandPool m_commandPool;
		jsvk::VulkanSwapchain* m_pSwapchain;
		VkCommandBuffer m_mainCmdBuffer;
		// descriptor pool and sets ?
		// swapchain rendertarget ?

		void init(jsvk::VulkanDevice* device, jsvk::VulkanSwapchain* swapchain, bool staticPool = false);

		// constructor
		RenderFrame();

		// destructor
		~RenderFrame();

		void createSyncObjects();
		void createCommandPool(VkCommandPoolCreateFlags flags = 0);
		VkCommandBuffer createTempBuffer(bool primary = true);
		VkCommandBuffer getStaticBuffer(VkCommandBufferUsageFlags flags = 0);
		void createPrimaryBuffer(bool primary = true);
		void reset();
		VkResult begin(uint32_t* imageIndex);
		VkResult end(uint32_t* imageIndex);
	};
}