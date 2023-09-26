#include "jsvkResources.h"
#include "structures.h"


#include <array>

namespace jsvk {

	// used to instantiate the class so the Registry can find it
	static ResourcesVS::TypeCmd s_type_resources_vs;

	ResourcesVS::ResourcesVS() 
	{

	}

	void ResourcesVS::createFramebuffer() {
		uint32_t numImageViews = m_pPresenter->getImageViews().size();
		m_swapchainFramebuffers.resize(numImageViews);

		for (size_t i = 0; i < numImageViews; i++) {
			std::array<VkImageView, 3> attachments = {
				m_renderPasses[0].color.view,
				m_renderPasses[0].depth.view,
				m_pPresenter->getImageViews()[i]
			};

			VkFramebufferCreateInfo framebufferInfo = jsvk::init::framebufferCreateInfo();
			framebufferInfo.renderPass = m_renderPasses[0].renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = m_extent.width;
			framebufferInfo.height = m_extent.height;
			framebufferInfo.layers = 1;

			VK_CHECK(vkCreateFramebuffer(m_pVulkanDevice->Device(), &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]));
		}
	}

	int ResourcesVS::loadModel(std::vector<std::string> modelPaths) {

		for (std::string modelPath : modelPaths) {
			std::string texturePath;
			//bool imgSuccess = settings::SettingsManager::getString("texture", texturePath);
			//bool modelSuccess = settings::SettingsManager::getString("model", modelPath);
			bool modelSuccess = 1;
			bool imgSuccess = 0;


			if (modelSuccess && imgSuccess) {
				m_shaders[1].meshes.push_back(m_pMemManager->processMeshFromFile(modelPath, texturePath));
			}
			else {
				m_shaders[0].meshes.push_back(m_pMemManager->processMeshFromFile(modelPath));
			}
		}
		return 1;
	}

	void ResourcesVS::init(jsvk::VulkanDevice* pVulkanDevice, jsk::Presenter* presenter, VkSampleCountFlagBits msaaSamples, int width, int height)
	{
		m_pVulkanDevice = pVulkanDevice;
		m_renderPasses[0].width = width;
		m_renderPasses[0].height = height;
		m_msaaSamples = msaaSamples;
		m_pPresenter = presenter;
		m_pMemManager = new jsvk::MemoryManager(pVulkanDevice);
		m_extent = m_pPresenter->getExtent();
	}

	void ResourcesVS::hotReloadPipeline() {
		if (m_meshShaderPipelines.size() > 0) {
			for (auto pipeline : m_meshShaderPipelines) {
				vkDestroyPipeline(m_pVulkanDevice->Device(), pipeline, nullptr);
			}
		}

		createPipeline();
	}

	void ResourcesVS::createPipeline() 
	{
		VkFormat depthFormat = jsvk::findDepthFormat(m_pVulkanDevice->m_pPhysicalDevice);

		jsvk::createImageOnly(m_pVulkanDevice->Device(), m_extent.width, m_extent.height, 1, m_msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, m_renderPasses[0].depth.image);
		//m_renderPass.depth.view = jsvk::createImageView(m_pVulkanDevice->Device(), m_renderPass.depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		VkFormat colorFormat = m_pPresenter->getImageFormat();
		jsvk::createImageOnly(m_pVulkanDevice->Device(), m_extent.width, m_extent.height, 1, m_msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_renderPasses[0].color.image);
		//m_renderPass.color.view = jsvk::createImageView(m_pVulkanDevice->Device(), m_renderPass.color.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		// temp allocation of resources 
		m_pMemManager->allocateResources(m_shaders, m_renderPasses[0]);

		jsvk::transitionImageLayout(m_pVulkanDevice->Device(), m_pVulkanDevice->m_commandPool, m_pVulkanDevice->m_pGraphicsQueue, m_renderPasses[0].color.image, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

		auto vertShaderCode = readFile("jsvk/shaders/vert.spv");
		auto fragShaderCode = readFile("jsvk/shaders/frag.spv");

		VkShaderModule vertShaderModule = jsvk::createShaderModule(m_pVulkanDevice->Device(), vertShaderCode);
		VkShaderModule fragShaderModule = jsvk::createShaderModule(m_pVulkanDevice->Device(), fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = jsvk::init::pipelineShaderStageCreateinfo();
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = jsvk::init::pipelineShaderStageCreateinfo();
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDescrpition();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = jsvk::init::pipelineVertexinputStateCreateInfo();
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = jsvk::init::pipelineInputAssemblyStateCreateInfo();
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_extent.width;
		viewport.height = (float)m_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_extent;

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = jsvk::init::pipelineDynamicStateCreateInfo();
		dynamicState.dynamicStateCount = dynamicStateEnables.size();
		dynamicState.pDynamicStates = dynamicStateEnables.data();

		VkPipelineViewportStateCreateInfo viewportState = jsvk::init::pipelineViewportStateCreateInfo();
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = jsvk::init::pipelineRasterizationStateCreateInfo();
		rasterizer.depthClampEnable = VK_FALSE;

		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

		rasterizer.lineWidth = 1.0f;

		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		VkPipelineMultisampleStateCreateInfo multisampling = jsvk::init::pipelineMultisampleStateCreateInfo();
		multisampling.sampleShadingEnable = VK_TRUE;
		multisampling.rasterizationSamples = m_msaaSamples;
		multisampling.minSampleShading = 0.2f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		VkPipelineDepthStencilStateCreateInfo depthStencil = jsvk::init::pipelineDepthStencilStateCreateInfo();
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending = jsvk::init::pipelineColorBlendStateCreateInfo();
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = jsvk::init::pipelineLayoutCreateInfo();
		pipelineLayoutInfo.setLayoutCount =  m_pMemManager->descriptorLayouts.size(); // Optional
		pipelineLayoutInfo.pSetLayouts = m_pMemManager->descriptorLayouts.data();//&m_descriptorSetlayouts[0]; // Optional
		//pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		//pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		if (vkCreatePipelineLayout(m_pVulkanDevice->Device(), &pipelineLayoutInfo, nullptr, &m_meshShaderPipelineLayouts[0]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo = jsvk::init::graphicsPipelineCreateInfo();
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState; // Optional

		pipelineInfo.layout = m_meshShaderPipelineLayouts[0];

		pipelineInfo.renderPass = m_renderPasses[0].renderPass;
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(m_pVulkanDevice->Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_meshShaderPipelines[0]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(m_pVulkanDevice->Device(), vertShaderModule, nullptr);
		vkDestroyShaderModule(m_pVulkanDevice->Device(), fragShaderModule, nullptr);


		//temp add pipes to meshes
		//m_shaders[0].pipeline = &phong_pipeline;
		//m_shaders[0].pipelineLayout = &offscreenPipelineLayout;
		m_shaders[0].pipeline = &m_meshShaderPipelines[0];
		m_shaders[0].pipelineLayout = &m_meshShaderPipelineLayouts[0];
		m_shaders[0].num_meshes = m_shaders[0].meshes.size();




		//m_shaders[1].pipeline = &phong_texture_pipeline;
		//m_shaders[1].pipelineLayout = &offscreenPipelineLayout;
		m_shaders[1].pipeline = &m_meshShaderPipelines[1];
		m_shaders[1].pipelineLayout = &m_meshShaderPipelineLayouts[1];
		m_shaders[1].num_meshes = m_shaders[1].meshes.size();

	}

	void ResourcesVS::deinit() {
		std::cout << "Destroying Resources" << std::endl;
		// here we free a lot of shit

		// free descriptorpool
		//if (m_descriptorPools.size() > 0)
		//	for (auto descriptorPool : m_descriptorPools) {
		//		vkDestroyDescriptorPool(m_pVulkanDevice->Device(), descriptorPool, nullptr);
		//	}

		// free renderpasses
		if (m_renderPasses.size() > 0) {
			for (auto m_renderPass : m_renderPasses) {
				// free imageviews
				if (m_renderPass.color.view != VK_NULL_HANDLE) {
					vkDestroyImageView(m_pVulkanDevice->Device(), m_renderPass.color.view, nullptr);
				}
				if (m_renderPass.depth.view != VK_NULL_HANDLE) {
					vkDestroyImageView(m_pVulkanDevice->Device(), m_renderPass.depth.view, nullptr);
				}

				// free images
				if (m_renderPass.color.image != VK_NULL_HANDLE) {
					vkDestroyImage(m_pVulkanDevice->Device(), m_renderPass.color.image, nullptr);
				}
				if (m_renderPass.depth.image != VK_NULL_HANDLE) {
					vkDestroyImage(m_pVulkanDevice->Device(), m_renderPass.depth.image, nullptr);
				}
				// free image memory
				if (m_renderPass.color.memory != VK_NULL_HANDLE) {
					vkFreeMemory(m_pVulkanDevice->Device(), m_renderPass.color.memory, nullptr);
				}
				if (m_renderPass.depth.memory != VK_NULL_HANDLE) {
					vkFreeMemory(m_pVulkanDevice->Device(), m_renderPass.depth.memory, nullptr);
				}

				if (m_renderPass.renderPass != VK_NULL_HANDLE) {
					// Destroy renderPass
					vkDestroyRenderPass(m_pVulkanDevice->Device(), m_renderPass.renderPass, nullptr);
				}
			}
			// maybe just do this and add a destructor to the renderpass class
			m_renderPasses.clear();
		}
		//Destroy framebuffers
		if (m_swapchainFramebuffers.size() > 0) {
			for (auto framebuffer : m_swapchainFramebuffers) {
				vkDestroyFramebuffer(m_pVulkanDevice->Device(), framebuffer, nullptr);
			}
		}

		// free descriptorpool

		// free pipeline
		if (m_meshShaderPipelines.size() > 0) {
			for (auto pipeline : m_meshShaderPipelines) {
				vkDestroyPipeline(m_pVulkanDevice->Device(), pipeline, nullptr);
			}
		}

		// free pipelinelayout
		if (m_meshShaderPipelineLayouts.size() > 0) {
			for (auto pipelineLayout : m_meshShaderPipelineLayouts) {
				vkDestroyPipelineLayout(m_pVulkanDevice->Device(), pipelineLayout, nullptr);
			}
		}

		// free descriptorlayouts
		// TODO make layouts a vector for easy handeling
		//for (int i = 0; i < 4; ++i) {
		//	vkDestroyDescriptorSetLayout(m_pVulkanDevice->Device(), m_layouts[i], nullptr);
		//}

		//// lastly memorymanager is deleted
		//// temp mem deletion which needs to be moved inside memManager
		//if (m_uniformBuffer.m_pMemory != VK_NULL_HANDLE) {
		//	m_uniformBuffer.destroy();
		//}
		//if (m_mainBuffer.m_pMemory != VK_NULL_HANDLE) {
		//	m_mainBuffer.destroy();
		//}
		delete m_pMemManager;

	}

	void ResourcesVS::createRenderPass() {
		VkFormat swapchainImageFormat = m_pPresenter->getImageFormat();


		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapchainImageFormat;
		colorAttachment.samples = m_msaaSamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = jsvk::findDepthFormat(m_pVulkanDevice->m_pPhysicalDevice);
		depthAttachment.samples = m_msaaSamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve = {};
		colorAttachmentResolve.format = swapchainImageFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentResolveRef = {};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		VkRenderPassCreateInfo renderPassInfo = jsvk::init::renderPassCreateinfo();
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_pVulkanDevice->Device(), &renderPassInfo, nullptr, &m_renderPasses[0].renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}
}