#pragma once
#ifndef VK_RESOURCES_H
#define VK_RESOURCES_H

#include <vulkan/vulkan.h>

#include "structures.h"
#include "jsvkSwapchain.h"
#include "jsvkDevice.h"
#include "jsvkMemoryManager.h"
#include "meshlet_builder.hpp"
#include "jsvkBuffer.h"
#include "jsvkTexture.h"
#include "jsvkRenderFrame.h"

#include "../presenter.h"

namespace jsvk
{

	class Resources
	{
	public:
		class ResourcesType
		{
		public:
			ResourcesType() { getRegistry().push_back(this); }

		public:
			virtual bool isAvailable() const = 0;
			virtual const char *name() const = 0;
			virtual Resources *create() const = 0;
			virtual unsigned int priority() const { return 0xFF; }
		};

		typedef std::vector<ResourcesType *> ResourcesRegistry;

		static ResourcesRegistry &getRegistry()
		{
			static ResourcesRegistry s_registry;
			return s_registry;
		}

	public:
		std::vector<jsvk::RenderPass> m_renderPasses;
		std::vector<VkFramebuffer> m_swapchainFramebuffers;
		std::array<VkPipelineLayout, 2> m_meshShaderPipelineLayouts;
		std::array<VkPipeline, 4> m_meshShaderPipelines;
		VkExtent2D m_extent;
		Scene scene;

		virtual void createRenderPass() = 0;
		virtual void createFramebuffer() = 0;
		virtual void createPipeline() = 0;
		virtual int loadModel(std::vector<std::string> modelPaths) = 0;
		virtual void hotReloadPipeline() = 0;

		virtual void init(jsvk::VulkanDevice *pVulkanDevice, jsk::Presenter *presenter, VkSampleCountFlagBits msaaSamples, int width, int height) = 0;
		virtual void deinit() = 0;

		Resources() : m_renderPasses(1), m_meshShaderPipelineLayouts(), m_meshShaderPipelines(){};
		/*
		jsvk::RenderPass getRenderPass() { return m_renderPass; };
		std::array<VkPipelineLayout, 2> getPipelineLayouts() { return m_meshShaderPipelineLayouts; };
		std::array<VkPipeline, 2> getPipelines() { return m_meshShaderPipelines; };
		*/
	};

	class ResourcesVS : public Resources
	{
	public:
		class TypeCmd : Resources::ResourcesType
		{
			bool isAvailable() const { return 1; }
			const char *name() const { return "Vertex Resources"; }
			Resources *create() const
			{
				ResourcesVS *resources = new ResourcesVS();
				return resources;
			}

			unsigned int priority() const { return 10; }
		};
		jsvk::VulkanDevice *m_pVulkanDevice;
		jsk::Presenter *m_pPresenter;
		VkSampleCountFlagBits m_msaaSamples;
		jsvk::MemoryManager *m_pMemManager;
		VkDescriptorSetLayout m_descriptorSetlayouts[4];
		// std::vector<VkFramebuffer> m_swapchainFramebuffers;
		std::array<Shader, 2> m_shaders;

		// constructor
		ResourcesVS();

		// destructor
		~ResourcesVS();

		int loadModel(std::vector<std::string> modelPaths) override;

		void createRenderPass() override;
		void createFramebuffer() override;
		void hotReloadPipeline() override;
		void createPipeline() override;
		void init(jsvk::VulkanDevice *pVulkanDevice, jsk::Presenter *presenter, VkSampleCountFlagBits msaaSamples, int width, int height) override;
		void deinit() override;
	};

	class ResourcesXRVS : public Resources
	{
	public:
		class TypeCmd : Resources::ResourcesType
		{
			bool isAvailable() const { return 1; }
			const char *name() const { return "XR Vertex Resources"; }
			Resources *create() const
			{
				ResourcesXRVS *resources = new ResourcesXRVS();
				return resources;
			}

			unsigned int priority() const { return 10; }
		};
		jsvk::VulkanDevice *m_pVulkanDevice;
		jsk::Presenter *m_pPresenter;
		VkSampleCountFlagBits m_msaaSamples;
		jsvk::MemoryManager *m_pMemManager;
		VkDescriptorSetLayout m_descriptorSetlayouts[4];
		// std::vector<VkFramebuffer> m_swapchainFramebuffers;
		std::array<Shader, 2> m_shaders;

		// constructor
		ResourcesXRVS();

		// destructor
		~ResourcesXRVS();

		int loadModel(std::vector<std::string> modelPaths) override;

		void createRenderPass() override;
		void hotReloadPipeline() override;
		void createFramebuffer() override;
		void createPipeline() override;
		void init(jsvk::VulkanDevice *pVulkanDevice, jsk::Presenter *presenter, VkSampleCountFlagBits msaaSamples, int width, int height) override;
		void deinit() override;
	};

	class ResourcesMS : public Resources
	{
	public:
		class TypeCmd : Resources::ResourcesType
		{
			bool isAvailable() const { return 1; }
			const char *name() const { return "Mesh Resources"; }
			Resources *create() const
			{
				ResourcesMS *resources = new ResourcesMS();
				return resources;
			}

			unsigned int priority() const { return 10; }
		};

		jsvk::VulkanDevice *m_pVulkanDevice;
		jsk::Presenter *m_pPresenter;
		// std::vector<VkFramebuffer> m_swapchainFramebuffers;
		VkSampleCountFlagBits m_msaaSamples;
		std::vector<GameObject> m_geos;
		jsvk::MemoryManager *m_pMemManager;
		jsvk::Buffer m_uniformBuffer;
		VkDeviceSize m_dynamicAlignment;
		void *DynamicObjectDataPointer;
		CullStats *m_cullStats;
		VkDescriptorSetLayout m_layouts[4];
		std::vector<VkDescriptorSet> sceneSets;
		std::vector<VkDescriptorSet> objSets;
		std::vector<VkDescriptorSet> imgSets;
		std::vector<VkDescriptorSet> geoSets;
		std::vector<jsvk::Texture> textures;
		bool useTask;
		std::vector<VkDescriptorPool> m_descriptorPools;
		SceneData m_sceneData[2];
		jsvk::Buffer m_mainBuffer;

		// constructor
		ResourcesMS();

		// destructor
		~ResourcesMS();

		int loadModel(std::vector<std::string> modelPaths) override;

		void createRenderPass() override;
		void createFramebuffer() override;
		void createPipeline() override;
		void hotReloadPipeline() override;
		void init(jsvk::VulkanDevice *pVulkanDevice, jsk::Presenter *presenter, VkSampleCountFlagBits msaaSamples, int width, int height) override;
		void deinit() override;
	};
}

#endif // VK_RESOURCES_H
