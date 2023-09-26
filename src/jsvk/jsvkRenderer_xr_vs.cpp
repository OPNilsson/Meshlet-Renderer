#include "jsvkRenderer.h"
#include "jsvkDevice.h"
#include "jsvkSwapchain.h"
#include "meshlet_builder.hpp"
#include "structures.h"
#include "jsvkResources.h"

#include <vulkan/vulkan.h>
#include <iostream>
#include <chrono>

namespace jsvk
{
	class VertRendererXR : public Renderer
	{
	public:
		class TypeCmd : Renderer::RenderType
		{
			bool isAvailable() const { return 1; }
			const char *name() const { return "XR Vertex Renderer"; }
			Renderer *create() const
			{
				VertRendererXR *renderer = new VertRendererXR();
				return renderer;
			}

			unsigned int priority() const { return 10; }
		};

	public:
		bool useTask = true;
		bool init(jsvk::VulkanDevice *m_pVulkanDevice, jsvk::Resources *pResources) override;
		void deinit() override;
		void draw(uint32_t imageIndex, VkCommandBuffer *primaryBuffer) override;
		void updateUniforms() override;
		void drawDynamic(GameObject *mesh, uint32_t imageIndex, VkCommandBuffer *primaryBuffer, VkCommandBufferInheritanceInfo &inheritInfo, GameObject *offsetMesh, int gameObjectOffset) override;
		jsvk::ResourcesVS *getResources() { return m_pResources; }

		VertRendererXR() {}

	private:
		VkCommandPool m_commandPool;
		std::vector<VkCommandBuffer> m_commandBuffers;
		jsvk::VulkanDevice *m_pVulkanDevice;
		jsvk::ResourcesVS *m_pResources;
		bool first = true;

		void createCommmandBuffers()
		{

			m_commandBuffers.resize(m_pResources->m_swapchainFramebuffers.size());

			VkCommandBufferAllocateInfo allocInfo = jsvk::init::commandBufferAllocateInfo();
			allocInfo.commandPool = m_commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

			VK_CHECK(vkAllocateCommandBuffers(m_pVulkanDevice->Device(), &allocInfo, m_commandBuffers.data()));

			for (size_t i = 0; i < m_commandBuffers.size(); ++i)
			{
				VkCommandBufferInheritanceInfo inheritInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};

				inheritInfo.renderPass = m_pResources->m_renderPasses[0].renderPass;
				inheritInfo.framebuffer = m_pResources->m_swapchainFramebuffers[i];
				VkCommandBufferBeginInfo beginInfo = jsvk::init::commandBufferBeginInfo();
				// double check these flags
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT; // Optional had to change to get rid of a warning I did not understand
				beginInfo.pInheritanceInfo = &inheritInfo;																		   // Optional

				VK_CHECK(vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo));

				// only renderpass
				// std::array<VkClearValue, 2> clearValues = {};
				// clearValues[0].color = { 0.392f, 0.584f, 0.929f, 1.0f };
				// clearValues[1].depthStencil = { 1.0f, 0 };

				// VkRenderPassBeginInfo renderPassInfo = jsvk::init::renderPassBeginInfo();
				// renderPassInfo.renderPass = m_pResources->m_renderPass.renderPass;
				// renderPassInfo.framebuffer = m_pResources->m_swapchainFramebuffers[i];
				// renderPassInfo.renderArea.offset = { 0,0 };
				// renderPassInfo.renderArea.extent = m_pResources->m_pSwapchain->m_extent;

				// renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
				// renderPassInfo.pClearValues = clearValues.data();

				// vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport = {};
				viewport.width = m_pResources->m_extent.width;
				viewport.height = m_pResources->m_extent.height;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

				VkRect2D scissor = {};
				scissor.extent.width = m_pResources->m_extent.width;
				scissor.extent.height = m_pResources->m_extent.height;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

				// vertex and indexbuffer
				VkDeviceSize nOffsets[1] = {0};
				vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, &m_pResources->m_pMemManager->m_vertexBuffer.m_pBuffer, nOffsets);
				vkCmdBindIndexBuffer(m_commandBuffers[i], m_pResources->m_pMemManager->m_vertexBuffer.m_pBuffer, m_pResources->m_pMemManager->indexBuffer_offset, VK_INDEX_TYPE_UINT32);

				// DrawNoVr(m_commandBuffers[i], offscreenPipelineLayout, shaders);

				VkDeviceSize offsets[] = {0};

				int mesh_id = 0;
				int offset = 0;
				for (int j = 0; j < 1; ++j)
				{ // m_pResources->m_shaders.size(); ++i) {
					vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pResources->m_shaders[j].pipeline);

					for (int k = 0; k < m_pResources->m_shaders[j].num_meshes; ++k)
					{
						uint32_t dynamicOffset = (mesh_id) * static_cast<uint32_t>(sizeof(DynamicUniformBufferObject));
						m_pResources->m_shaders[j].meshes[k].Draw(m_commandBuffers[i], *m_pResources->m_shaders[j].pipelineLayout, dynamicOffset);

						++mesh_id;
					}
				}

				// vkCmdEndRenderPass(m_commandBuffers[i]);
				VK_CHECK((vkEndCommandBuffer(m_commandBuffers[i])));
			}
		}

		void createCommandPool()
		{

			jsvk::QueueFamilyIndices queueFamilyIndices;
			queueFamilyIndices = jsvk::findQueueFamilies(m_pVulkanDevice->m_pPhysicalDevice, m_pResources->m_pPresenter->getSurface());

			VkCommandPoolCreateInfo poolInfo = jsvk::init::commandPoolCreateInfo();
			poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VK_CHECK(vkCreateCommandPool(m_pVulkanDevice->Device(), &poolInfo, nullptr, &m_commandPool));
		}
	};

	// mucho importanto!
	// used to instantiate the class so the Registry can find it
	static VertRendererXR::TypeCmd s_type_cmdbuffer_vk;

	bool VertRendererXR::init(jsvk::VulkanDevice *pVulkanDevice, jsvk::Resources *pResources)
	{
		std::cout << "Initializing Renderer" << std::endl;
		m_pVulkanDevice = pVulkanDevice;
		m_pResources = (jsvk::ResourcesVS *)pResources;
		createCommandPool();
		createCommmandBuffers();
		return 1;
	}

	void VertRendererXR::deinit()
	{
		std::cout << "Destroying Renderer" << std::endl;

		if (m_commandBuffers.size() > 0)
		{
			vkFreeCommandBuffers(m_pVulkanDevice->Device(), m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
		}
		if (m_commandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_pVulkanDevice->Device(), m_commandPool, nullptr);
		}
	}

	void VertRendererXR::updateUniforms()
	{
		// static auto startTime = std::chrono::high_resolution_clock::now();

		// auto currentTime = std::chrono::high_resolution_clock::now();
		// float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		jsvk::UniformBufferObject ubo = {};
		// ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
		// ubo.model = glm::rotate(ubo.model, time * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// ubo.view = glm::lookAt(glm::vec3(5.0f, 5.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// ubo.proj = glm::perspective(glm::radians(45.0f), m_pResources->m_pSwapchain->m_extent.width / (float)m_pResources->m_pSwapchain->m_extent.height, 0.1f, 10.0f);

		ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
		ubo.model = glm::rotate(ubo.model, glm::radians(190.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), m_pResources->m_extent.width / (float)m_pResources->m_extent.height, 0.1f, 10.0f);

		std::vector<glm::mat4> poseMatrices;
		poseMatrices.push_back(ubo.proj * ubo.view * ubo.model);
		m_pResources->m_pMemManager->UpdateUBO(poseMatrices);
	}

	void VertRendererXR::drawDynamic(GameObject *mesh, uint32_t imageIndex, VkCommandBuffer *primaryBuffer, VkCommandBufferInheritanceInfo &inheritInfo, GameObject *offsetMesh, int gameObjectOffset)
	{
	}

	void VertRendererXR::draw(uint32_t imageIndex, VkCommandBuffer *primaryBuffer)
	{

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = {0.392f, 0.584f, 0.929f, 1.0f};
		clearValues[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo renderPassInfo = jsvk::init::renderPassBeginInfo();
		renderPassInfo.renderPass = m_pResources->m_renderPasses[0].renderPass;
		renderPassInfo.framebuffer = m_pResources->m_swapchainFramebuffers[imageIndex];
		renderPassInfo.renderArea.extent.width = m_pResources->m_renderPasses[0].width;
		renderPassInfo.renderArea.extent.height = m_pResources->m_renderPasses[0].height;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(*primaryBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		vkCmdExecuteCommands(*primaryBuffer, 1, &m_commandBuffers[imageIndex]);

		vkCmdEndRenderPass(*primaryBuffer);

		VK_CHECK(vkEndCommandBuffer(*primaryBuffer));
	}

}