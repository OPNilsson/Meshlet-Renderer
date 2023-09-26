#pragma once
#ifndef HEADERGUARD_VKRENDERCONTEXT
#define HEADERGUARD_VKRENDERCONTEXT
#include <vulkan/vulkan.h>

#include "jsvkSwapchain.h"
#include "jsvkRenderFrame.h"
#include "jsvkRenderer.h"

#include "lodGeometry.hpp"

#include "../presenter.h"

namespace jsvk
{

	class RenderContext
	{
	public:
		std::vector<jsvk::RenderFrame *> m_renderFrames;
		jsvk::Renderer *m_pRenderer;
		// jsvk::ThreadPool threadPool;

		struct threadData
		{
			VkCommandPool cmdPool;
			VkCommandBuffer cmdBuffer;
		};

		virtual VkResult acquireNextImage(uint32_t *imageIndex) = 0;
		virtual VkResult presentImage(VkCommandBuffer primaryBuffer, uint32_t *imageIndex) = 0;
		virtual VkCommandBuffer getTempBuffer() = 0;
		virtual void Render() = 0;

		// virtual ~RenderContext() = 0;
		virtual void deinit() = 0;
		//{
		//	std::cout << "Destroying Render Context" << std::endl;
		//	//Destroy renderframes
		//	if (m_renderFrames.size() > 0) {
		//		for (auto frame : m_renderFrames) {
		//			delete frame;
		//		}
		//	}

		//	if (m_pRenderer != nullptr) {
		//		m_pRenderer->deinit();
		//		m_pRenderer = nullptr;
		//	}
		//}
	};

	class VKRenderContext : public RenderContext
	{
	public:
		jsk::Presenter *m_pPresenter;
		jsvk::VulkanSwapchain *m_pSwapchain;
		jsvk::VulkanDevice *m_pVulkanDevice;
		uint32_t m_currentFrame;
		const int m_MAX_FRAMES_IN_FLIGHT = 2;

		// constructor
		VKRenderContext(jsk::Presenter *presenter, jsvk::VulkanDevice *pDevice);
		// destructor
		void deinit() override;

		VkResult acquireNextImage(uint32_t *imageIndex) override;
		VkResult presentImage(VkCommandBuffer primaryBuffer, uint32_t *imageIndex) override;
		VkCommandBuffer getTempBuffer() override;
		VkCommandBuffer getPrimaryBuffer();
		void Render() override;
	};
}

#endif // HEADERGUARD_VKRENDERCONTEXT