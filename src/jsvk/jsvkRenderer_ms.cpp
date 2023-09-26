#include "jsvkRenderer.h"
#include "jsvkDevice.h"
#include "jsvkSwapchain.h"
#include "meshlet_builder.hpp"
#include "structures.h"
#include "jsvkResources.h"
#include "config.h"
#include "lodCamera.hpp"
#include "lodGeometry.hpp"

#include <vulkan/vulkan.h>
#include <iostream>
#include <chrono>

extern int CURRENTPIPE;
extern float ROTATION;

extern lod::Camera camera;
extern int MAX_LOD;

extern lod::World world;

namespace jsvk
{
	class MeshRenderer : public Renderer
	{
	public:
		class TypeCmd : Renderer::RenderType
		{
			bool isAvailable() const { return 1; }
			const char *name() const { return "Mesh Renderer"; }
			Renderer *create() const
			{
				MeshRenderer *renderer = new MeshRenderer();
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
		jsvk::ResourcesMS *getResources() { return m_pResources; }

		MeshRenderer()
		{
		}

	private:
		VkCommandPool m_commandPool;
		std::vector<VkCommandBuffer> m_commandBuffers;
		jsvk::VulkanDevice *m_pVulkanDevice;
		jsvk::ResourcesMS *m_pResources;
		bool first = true;

		void createCommmandBuffers()
		{

			m_commandBuffers.resize(m_pResources->m_swapchainFramebuffers.size() * 3);

			VkCommandBufferAllocateInfo allocInfo = jsvk::init::commandBufferAllocateInfo();
			allocInfo.commandPool = m_commandPool;
			// allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

			VK_CHECK(vkAllocateCommandBuffers(m_pVulkanDevice->Device(), &allocInfo, m_commandBuffers.data()));

			for (int p = 0; p < 3; ++p)
			{
				for (size_t k = 0; k < 3; ++k)
				{
					int i = k + (3 * p);
					VkCommandBufferInheritanceInfo inheritInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};

					inheritInfo.renderPass = m_pResources->m_renderPasses[0].renderPass;
					inheritInfo.framebuffer = VK_NULL_HANDLE; // m_pResources->m_swapchainFramebuffers[k];

					VkCommandBufferBeginInfo beginInfo = jsvk::init::commandBufferBeginInfo();
					// beginInfo.flags = 0; // Optional had to change to get rid of a warning I did not understand
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
					beginInfo.pInheritanceInfo = &inheritInfo; // Optional

					VK_CHECK(vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo));
					// state info
					int last_pipe = -1;
					int last_pipeLayout = -1;
					bool first = true;
					// int num_geos = meshletGeometry.size() + meshletGeometry32.size();
					int num_geos = m_pResources->m_geos.size();
					for (int j = 0; j < num_geos; ++j)
					{

						if (first || m_pResources->m_geos[j].pipelineID != last_pipe)
						{
							int pipelineID = m_pResources->m_geos[j].pipelineID;
							if (pipelineID == 0 && p == 0)
							{
								pipelineID = p;
							}
							else if (pipelineID == 0 && p != 0)
							{
								// add p and skip pipelineID 1
								pipelineID = p + 1;
							}
							vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pResources->m_meshShaderPipelines[pipelineID]);

							if (first)
							{
								// here I changed i to (i % 2) because i should be used as 0 or 1 but the swapchain has 3 images
								uint32_t view_offset = 0;

								vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pResources->m_meshShaderPipelineLayouts[1], 0,
														1, m_pResources->sceneSets.data(), 1, &view_offset);

								vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pResources->m_meshShaderPipelineLayouts[1], 2,
														1, m_pResources->geoSets.data(), 0, nullptr);
							}

							last_pipe = m_pResources->m_geos[j].pipelineID;
							first = false;
						}

						if (last_pipe)
						{
							vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pResources->m_meshShaderPipelineLayouts[1], 3,
													1, &m_pResources->imgSets[j], 0, nullptr);
						}
						// for each mesh draw!

						uint32_t obj_offset = j * sizeof(ObjectData);
						vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pResources->m_meshShaderPipelineLayouts[1], 1,
												1, m_pResources->objSets.data(), 1, &obj_offset);

						// we use the same vertex offset for both vbo and abo, our allocator should ensure this condition.
						// assert(uint32_t(geo.vbo.offset / vertexSize) == uint32_t(geo.abo.offset / vertexAttributeSize));

						uint32_t offsets[4] = {uint32_t(m_pResources->m_geos[j].desc_offset / sizeof(NVMeshlet::MeshletDesc)),
											   uint32_t(m_pResources->m_geos[j].prim_offset / (NVMeshlet::PRIMITIVE_INDICES_PER_FETCH)),
											   uint32_t(m_pResources->m_geos[j].vert_offset / (m_pResources->m_geos[j].shorts == 1 ? 2 : 4)),
											   uint32_t(m_pResources->m_geos[j].vbo_offset / (3 * sizeof(float)))};

						vkCmdPushConstants(m_commandBuffers[i], m_pResources->m_meshShaderPipelineLayouts[1],
										   VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV, 0, sizeof(offsets), offsets);

						if (useTask)
						{
							glm::uvec4 assigns;
							assigns.x = 0;																													 // uint32_t(geos[j].desc_offset) / sizeof(NVMeshlet::MeshletDesc);
							assigns.y = uint32_t(m_pResources->m_geos[j].desc_offset) / sizeof(NVMeshlet::MeshletDesc) + m_pResources->m_geos[j].desc_count; // di.meshlet.offset + di.meshlet.count - 1;
							assigns.z = 0;
							assigns.w = 0;
							vkCmdPushConstants(m_commandBuffers[i], m_pResources->m_meshShaderPipelineLayouts[1], VK_SHADER_STAGE_TASK_BIT_NV,
											   sizeof(uint32_t) * 4, sizeof(assigns), &assigns);
						}

						// update this when we go into task shaders
						uint32_t count = useTask == 1 ? NVMeshlet::computeTasksCount(m_pResources->m_geos[j].desc_count) : m_pResources->m_geos[j].desc_count; // ; // NVMeshlet::computeTasksCount(meshletGeometry.meshletDescriptors.size());

						vkCmdDrawMeshTasksNV(m_commandBuffers[i], count, 0);
					}
					VK_CHECK((vkEndCommandBuffer(m_commandBuffers[i])));
				}
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
	static MeshRenderer::TypeCmd s_type_cmdbuffer_vk;

	bool MeshRenderer::init(jsvk::VulkanDevice *pVulkanDevice, jsvk::Resources *pResources)
	{
		printf("Initializing Renderer\n________________________________________________\n\n");
		m_pVulkanDevice = pVulkanDevice;
		m_pResources = (jsvk::ResourcesMS *)pResources;
		createCommandPool();
		createCommmandBuffers();
		return 1;
	}

	void MeshRenderer::deinit()
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

	void MeshRenderer::updateUniforms()
	{
#if CONTINUOUSROTATION
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		glm::mat4 view = glm::lookAt(glm::vec3(glm::sin(glm::radians(25.0f) * time) * 5.0f, 0.0f, glm::cos(glm::radians(25.0f) * time) * (-5.0f)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
#else
		// glm::mat4 view = glm::lookAt(glm::vec3(glm::sin(glm::radians(ROTATION)) * 5, 0.0f, glm::cos(glm::radians(ROTATION)) * (-5.0f)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 view = camera.view;

#endif

		// glm::mat4 proj = glm::perspective(glm::radians(45.0f), m_pResources->m_extent.width / (float)m_pResources->m_extent.height, 0.1f, 10.0f);
		glm::mat4 proj = camera.projection;

		m_pResources->m_sceneData[0].viewProjMatrix = proj * view;
		m_pResources->m_sceneData[0].viewMatrix = view;
		glm::mat4 viewI = glm::inverse(view);
		m_pResources->m_sceneData[0].viewMatrixIT = glm::transpose(viewI);
		m_pResources->m_sceneData[0].viewPos = viewI[3]; // glm::vec4(0.0f, 0.0f, -5.0f, 1.0f); //  //glm::vec4(0.0f, 0.0f, -5.0f, 1.0f);
		m_pResources->m_sceneData[0].viewDir = -view[2];

		char *uniformChar = (char *)m_pResources->m_uniformBuffer.m_mapped;
		//}

		// check out vulkan memcpy for this
#if STATS
		memcpy(m_pResources->m_cullStats, &uniformChar[2 * m_pResources->m_dynamicAlignment], sizeof(CullStats));
		// print shit
		std::cout << "Number of meshlets actually drawn: " << m_pResources->m_cullStats->meshletsOutput << "."
				  << "\r";
		m_pResources->m_cullStats->meshletsOutput = 0;
		m_pResources->m_cullStats->tasksOutput = 0;
		memcpy(&uniformChar[2 * m_pResources->m_dynamicAlignment], m_pResources->m_cullStats, sizeof(CullStats));
#endif
		memcpy(&uniformChar[0], &m_pResources->m_sceneData[0], sizeof(SceneData));
		// first = -1;
	}

	void MeshRenderer::drawDynamic(GameObject *mesh, uint32_t imageIndex, VkCommandBuffer *primaryBuffer, VkCommandBufferInheritanceInfo &inheritInfo, GameObject *offsetMesh, int gameObjectOffset)
	{
		int desiredLOD = camera.offset;

		if (desiredLOD > MAX_LOD)
		{
			desiredLOD = MAX_LOD;
		}
		else if (desiredLOD < 0)
		{
			desiredLOD = 0;
		}

		VkCommandBufferBeginInfo beginInfo = jsvk::init::commandBufferBeginInfo();
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		beginInfo.pInheritanceInfo = &inheritInfo; // Optional

		VkCommandBuffer &secCmdBuffer = mesh->m_commandBuffers[imageIndex];

		vkBeginCommandBuffer(secCmdBuffer, &beginInfo);
		// VK_CHECK(vkBeginCommandBuffer(secCmdBuffer, &beginInfo));

		// I need to know the current pipelines
		int pipeID = mesh->pipelineID == 1 ? 1 : 0;
		vkCmdBindPipeline(secCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pResources->m_meshShaderPipelines[CURRENTPIPE]);

		// here I changed i to (i % 2) because i should be used as 0 or 1 but the swapchain has 3 images
		uint32_t view_offset = 0;

		vkCmdBindDescriptorSets(secCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pResources->m_meshShaderPipelineLayouts[1], 0,
								1, m_pResources->sceneSets.data(), 1, &view_offset);

		vkCmdBindDescriptorSets(secCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pResources->m_meshShaderPipelineLayouts[1], 2,
								1, m_pResources->geoSets.data(), 0, nullptr);

		// only add descriptor for texture when there is a texture for the object
		if (mesh->pipelineID == 1)
		{
			vkCmdBindDescriptorSets(secCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pResources->m_meshShaderPipelineLayouts[1], 3,
									1, &m_pResources->imgSets[mesh->ObjectOffset], 0, nullptr);
		}

		// for each mesh draw
		uint32_t obj_offset = mesh->ObjectOffset * sizeof(ObjectData);
		vkCmdBindDescriptorSets(secCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pResources->m_meshShaderPipelineLayouts[1], 1,
								1, m_pResources->objSets.data(), 1, &obj_offset);

		// we use the same vertex offset for both vbo and abo, our allocator should ensure this condition.
		// assert(uint32_t(geo.vbo.offset / vertexSize) == uint32_t(geo.abo.offset / vertexAttributeSize));

		uint32_t offsets[4];

		int offset = gameObjectOffset * (MAX_LOD + 1);

		// Camera.start being -1 means discrete LoD and therefore the entire model can be drawn in a single draw call
		if (camera.start == -1)
		{
			offsets[0] = uint32_t((((world.model.desc_offsets[desiredLOD + offset])) + (0 * sizeof(NVMeshlet::MeshletDesc))) / sizeof(NVMeshlet::MeshletDesc));
		}
		else
		{
			offsets[0] = uint32_t((((world.model.desc_offsets[desiredLOD + offset])) + (camera.start * sizeof(NVMeshlet::MeshletDesc))) / sizeof(NVMeshlet::MeshletDesc));
		}

		offsets[1] = uint32_t((((world.model.prim_offsets[desiredLOD + offset])) + (0 * sizeof(NVMeshlet::PrimitiveIndexType))) / (NVMeshlet::PRIMITIVE_INDICES_PER_FETCH));
		offsets[2] = uint32_t((((world.model.vert_offsets[desiredLOD + offset])) + (0 * sizeof(uint32_t))) / (mesh->shorts == 1 ? 2 : 4));
		offsets[3] = uint32_t((((world.model.vbo_offsets[desiredLOD + offset])) + (0 * (3 * sizeof(float))))) / (3 * sizeof(float));

		vkCmdPushConstants(secCmdBuffer, m_pResources->m_meshShaderPipelineLayouts[1],
						   VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV, 0, sizeof(offsets), offsets);

		if (useTask)
		{
			glm::uvec4 assigns;

			assigns.x = 0;
			assigns.y = uint32_t(camera.end - 1); // This is the thing that fucks not being able to send it in one draw call since it is telling it how many meshlets to draw at this offset
			assigns.z = desiredLOD;
			assigns.w = (camera.start == -1) ? 0 : camera.start;

			vkCmdPushConstants(secCmdBuffer, m_pResources->m_meshShaderPipelineLayouts[1], VK_SHADER_STAGE_TASK_BIT_NV,
							   sizeof(uint32_t) * 4, sizeof(assigns), &assigns);
		}

		// update this when we go into task shaders
		uint32_t count = useTask == 1 ? NVMeshlet::computeTasksCount(world.model.desc_counts[desiredLOD]) : world.model.desc_counts[desiredLOD]; // ; // NVMeshlet::computeTasksCount(meshletGeometry.meshletDescriptors.size());

		vkCmdDrawMeshTasksNV(secCmdBuffer, count, 0);

		vkEndCommandBuffer(secCmdBuffer);

		// VK_CHECK((vkEndCommandBuffer(secCmdBuffer)));
	}

	void MeshRenderer::draw(uint32_t imageIndex, VkCommandBuffer *primaryBuffer)
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

		vkCmdExecuteCommands(*primaryBuffer, 1, &m_commandBuffers[imageIndex + CURRENTPIPE]);

		vkCmdEndRenderPass(*primaryBuffer);

		VK_CHECK(vkEndCommandBuffer(*primaryBuffer));
	}
}