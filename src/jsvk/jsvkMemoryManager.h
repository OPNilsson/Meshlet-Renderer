#pragma once
#ifndef HEADER_GUARD_VKMEMORYMANAGER
#define HEADER_GUARD_VKMEMORYMANAGER

#include "jsvkDevice.h"
#include "jsvkUtilities.h"
#include "structures.h"

// Should this really be in the memory manager ?
#include "geometryProcessing.h"

#include <vulkan/vulkan.h>
#include <vector>

static const uint32_t INVALID_ID_INDEX = ~0;

static inline VkDeviceSize alignedSize(VkDeviceSize sz, VkDeviceSize align)
{
	return ((sz + align - 1) / (align)) * align;
}

namespace jsvk
{

#define MAX_MEMORY_ALLOC_SIZE (VkDeviceSize(2 * 1024) * 1024 * 1024)
#define DEFAULT_MEM_BLOCKSIZE (VkDeviceSize(128) * 1024 * 1024)
#define DEFAULT_STAGING_BLOCKSIZE (VkDeviceSize(64) * 1024 * 1024)

	// General class that can be derived from for type specific memory management.
	class MemoryManager
	{

	public:
		jsvk::VulkanDevice *m_pVulkanDevice;
		uint32_t indexBuffer_offset;
		VkDeviceSize dynamicAlignment;
		DynamicUniformBufferObject *uniformData;
		VkDeviceMemory memory;
		std::vector<VkDescriptorSetLayout> descriptorLayouts;
		VkDescriptorSet uniformDescriptor;
		VkDescriptorPool descriptorPool;
		jsvk::Buffer m_uniformBuffer;
		jsvk::Buffer m_vertexBuffer;
		std::vector<VkDeviceSize> imageOffset;

		// object of interest
		std::vector<Vertex> vertices_texture;
		std::vector<Vertex> vertices_no_texture;
		std::vector<uint32_t> indices;
		uint32_t offset;
		int num_mesh = 0;

		// process vertex data
		std::vector<uint32_t> mesh_offsets;
		std::vector<uint32_t> mesh_sizes;
		uint32_t total_verts = 0;
		uint32_t total_indices = 0;
		VkDeviceSize totalImageSize = 0;
		uint32_t vertexOffset = 0;

	public:
		// Wrapper functions for aligned memory allocation
		// From Sascha Willems https://github.com/SaschaWillems/Vulkan/blob/master/examples/dynamicuniformbuffer/dynamicuniformbuffer.cpp
		// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
		void *alignedAlloc(size_t size, size_t alignment)
		{
			void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
			data = _aligned_malloc(size, alignment);
#else
			int res = posix_memalign(&data, alignment, size);
			if (res != 0)
				data = nullptr;
#endif
			return data;
		}

		void alignedFree(void *data)
		{
#if defined(_MSC_VER) || defined(__MINGW32__)
			_aligned_free(data);
#else
			free(data);
#endif
		}

		Mesh processMeshFromFile(std::string modelPath, std::string texturePath = "none")
		{
			// Should this really be in the memory manager ?
			loadTinyModel(modelPath, &vertices_no_texture, &indices);
			total_verts += vertices_no_texture.size();
			total_indices += indices.size();
			mesh_offsets.push_back(0);
			mesh_sizes.push_back(indices.size());

			Mesh mesh;

			mesh.mesh_size = indices.size();
			mesh.mesh_offset = 0;

			// if (texturePath != "none") {
			//	jsvk::Texture texture = jsvk::Texture(m_pVulkanDevice);
			//	texture.CreateImageObjectFromFile(texturePath);
			//	mesh.texture = texture;
			//	totalImageSize += texture.memRequirements.size;
			//	imageOffset.push_back(totalImageSize);
			// }

			offset = indices.size();

			++num_mesh;
			vertexOffset = static_cast<uint32_t>(vertices_no_texture.size());
			return mesh;
		}

		void UpdateUBO(std::vector<glm::mat4> neededMatrices)
		{
			// I do not need to pack the data because the min offset fits perfectly with a single matrix!
			memcpy(m_uniformBuffer.m_mapped, neededMatrices.data(), neededMatrices.size() * sizeof(DynamicUniformBufferObject));
		}

		VkDescriptorSet createDescriptorSet(const VkDescriptorPool &descriptorPool, const VkDescriptorSetLayout &layout)
		{
			VkDescriptorSetAllocateInfo offscreenAllocInfo = jsvk::init::descriptorSetAllocateInfo();
			offscreenAllocInfo.descriptorPool = descriptorPool;
			offscreenAllocInfo.descriptorSetCount = 1;
			offscreenAllocInfo.pSetLayouts = &layout;

			VkDescriptorSet descriptorSet;
			VK_CHECK(vkAllocateDescriptorSets(m_pVulkanDevice->Device(), &offscreenAllocInfo, &descriptorSet));

			return descriptorSet;
		}

		void createDescriptorPool()
		{
			// TODO: this +4 business is not good for business

			std::array<VkDescriptorPoolSize, 2> poolSizes = {};
			// poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			// poolSizes[0].descriptorCount = static_cast<uint32_t>(vulkanSwapchain.m_swapchainImages.size()+100);
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			poolSizes[0].descriptorCount = static_cast<uint32_t>(1);
			// poolSizes[0].descriptorCount = static_cast<uint32_t>(vulkanSwapchain.m_swapchainImages.size() + 100);
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			// poolSizes[1].descriptorCount = static_cast<uint32_t>(vulkanSwapchain.m_swapchainImages.size() + 100);
			poolSizes[1].descriptorCount = static_cast<uint32_t>(3);

			VkDescriptorPoolCreateInfo poolInfo = jsvk::init::descriptorPoolCreateInfo();
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();

			// poolInfo.maxSets = static_cast<uint32_t>(vulkanSwapchain.m_swapchainImages.size()+100);
			poolInfo.maxSets = static_cast<uint32_t>(7);

			if (vkCreateDescriptorPool(m_pVulkanDevice->Device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create descriptor pool!");
			}
		}

		void createUniformBuffer()
		{

			// limit is size of 64 aka one glm::mat4
			VkDeviceSize minUboAlignment = m_pVulkanDevice->m_deviceProperties.limits.minUniformBufferOffsetAlignment;
			dynamicAlignment = sizeof(DynamicUniformBufferObject);

			if (minUboAlignment > 0)
			{
				dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
			}

			VkDeviceSize bufferSize = num_mesh * dynamicAlignment;
			// we multiply by 2 because we have left and right eye view
			VkDeviceSize totalSize = bufferSize * 2;

			uniformData = (DynamicUniformBufferObject *)alignedAlloc(bufferSize, dynamicAlignment);

			VK_CHECK(m_pVulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_uniformBuffer, totalSize, nullptr, true));

			// for (size_t i = 0; i < 2; ++i) {
			//	VK_CHECK(m_pVulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_uniformBuffers[i], bufferSize, nullptr, true));
			// }
		}

		VkMemoryRequirements createBufferObject(VkBufferUsageFlags usageFlags, VkDeviceSize bufferSize)
		{
			VkResult result;

			m_vertexBuffer.m_pDevice = m_pVulkanDevice->Device();

			// handle for buffer
			VkBufferCreateInfo bufferInfo = jsvk::init::bufferCreateInfo();
			bufferInfo.usage = usageFlags;
			bufferInfo.size = bufferSize;
			// sharing mode?
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			result = vkCreateBuffer(m_vertexBuffer.m_pDevice, &bufferInfo, nullptr, &m_vertexBuffer.m_pBuffer);

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(m_vertexBuffer.m_pDevice, m_vertexBuffer.m_pBuffer, &memRequirements);
			return memRequirements;
		}

		void updateDynamicUniformBufferDescriptorSet(const VkBuffer &buffer, VkDescriptorSet &descriptor)
		{
			VkDescriptorBufferInfo offScreenBufferInfo = {};
			offScreenBufferInfo.buffer = buffer;
			offScreenBufferInfo.offset = 0;
			offScreenBufferInfo.range = sizeof(DynamicUniformBufferObject);

			VkWriteDescriptorSet uniformDescriptor = jsvk::init::writeDescriptorSet();
			uniformDescriptor.dstBinding = 0;
			uniformDescriptor.dstSet = descriptor;
			uniformDescriptor.dstArrayElement = 0;
			uniformDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			uniformDescriptor.descriptorCount = 1;
			uniformDescriptor.pBufferInfo = &offScreenBufferInfo;

			vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &uniformDescriptor, 0, nullptr);
		}

		VkDescriptorSetLayoutBinding layoutBinding = {};
		VkDescriptorSetLayout createDescriptorLayout(VkShaderStageFlags shaderStage, VkDescriptorType type, uint32_t binding = 0, uint32_t count = 1)
		{
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.binding = binding;
			layoutBinding.descriptorCount = count;
			layoutBinding.descriptorType = type;
			layoutBinding.pImmutableSamplers = nullptr;
			layoutBinding.stageFlags = shaderStage;

			VkDescriptorSetLayoutCreateInfo layoutSamplerInfo = jsvk::init::descriptorSetLayoutCreateInfo();
			layoutSamplerInfo.bindingCount = 1;
			layoutSamplerInfo.pBindings = &layoutBinding;

			VkDescriptorSetLayout descriptorLayout;
			VK_CHECK(vkCreateDescriptorSetLayout(m_pVulkanDevice->Device(), &layoutSamplerInfo, nullptr, &descriptorLayout));

			return descriptorLayout;
		}

		void allocateResources(std::array<Shader, 2> &shaders, RenderPass &renderpass)
		{
			// uniform buffer
			createUniformBuffer();

			// vertex and index buffer object
			VkDeviceSize totalVertexDataSize = vertices_texture.size() * sizeof(Vertex) + vertices_no_texture.size() * sizeof(Vertex) + indices.size() * sizeof(uint32_t);
			VkMemoryRequirements memRequirements = createBufferObject(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, totalVertexDataSize);

			// get image requirements for remaining images
			VkDeviceSize imagesizes = 0;
			std::vector<VkDeviceSize> image_offsets;
			image_offsets.push_back(0);
			for (int i = 0; i < 1; ++i)
			{
				VkMemoryRequirements memReq;

				vkGetImageMemoryRequirements(m_pVulkanDevice->Device(), renderpass.color.image, &memReq);
				imagesizes += memReq.size;
				image_offsets.push_back(imagesizes);

				VkMemoryAllocateInfo allocInfo = jsvk::init::memoryAllocateInfo();

				allocInfo.allocationSize = memReq.size;
				allocInfo.memoryTypeIndex = jsvk::findMemoryType(m_pVulkanDevice->m_pPhysicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				// if (!jsvk::MemoryTypeFromProperties(m_pVulkanDevice->m_physicalDeviceMemoryProperties, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocInfo.memoryTypeIndex)) {
				//	throw std::runtime_error("Failed to find memory type matching requirements for multiview.");
				// }

				vkGetImageMemoryRequirements(m_pVulkanDevice->Device(), renderpass.depth.image, &memReq);
				imagesizes += memReq.size;
				image_offsets.push_back(imagesizes);

				allocInfo.allocationSize = memReq.size;
				allocInfo.memoryTypeIndex = jsvk::findMemoryType(m_pVulkanDevice->m_pPhysicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				// if (!jsvk::MemoryTypeFromProperties(physicalDeviceMemoryProperties, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocInfo.memoryTypeIndex)) {
				//	throw std::runtime_error("Failed to find memory type matching requirements for multiview.");

				//}
			}

			// the we allocate total size
			VkMemoryAllocateInfo allocInfo = jsvk::init::memoryAllocateInfo();
			allocInfo.allocationSize = totalImageSize + memRequirements.size + imagesizes;
			if (shaders[1].num_meshes > 0)
			{
				allocInfo.memoryTypeIndex = jsvk::findMemoryType(m_pVulkanDevice->m_pPhysicalDevice, shaders[0].meshes[0].texture.memRequirements.memoryTypeBits | memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			}
			else
			{
				allocInfo.memoryTypeIndex = jsvk::findMemoryType(m_pVulkanDevice->m_pPhysicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			}

			if (vkAllocateMemory(m_pVulkanDevice->Device(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate image memory!");
			}

			createDescriptorPool();

			VkDescriptorSetLayout dynamicUniformBufferLayout = createDescriptorLayout(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
			VkDescriptorSetLayout imageSamplerLayout = createDescriptorLayout(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			descriptorLayouts.push_back(dynamicUniformBufferLayout);
			descriptorLayouts.push_back(imageSamplerLayout);

			// push back offset of the first image
			imageOffset.push_back(0);

			// allocate and update descriptorsets
			uniformDescriptor = createDescriptorSet(descriptorPool, descriptorLayouts[0]);
			updateDynamicUniformBufferDescriptorSet(m_uniformBuffer.m_pBuffer, uniformDescriptor);
			int mesh_idx = 0;
			for (int i = 0; i < shaders.size(); ++i)
			{
				for (int j = 0; j < shaders[i].meshes.size(); ++j)
				{
					shaders[i].meshes[j].descriptorsets.push_back(uniformDescriptor);
					if (shaders[i].meshes[j].texture.image != VK_NULL_HANDLE)
					{
						shaders[i].meshes[j].texture.bindImage(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory, imagesizes + imageOffset[mesh_idx]);
						shaders[i].meshes[j].texture.copyImage();
						VkDescriptorSet imageDescriptor = createDescriptorSet(descriptorPool, descriptorLayouts[1]);
						updateImageSamplerDescriptorSet(shaders[i].meshes[j].texture.view, shaders[i].meshes[j].texture.sampler, imageDescriptor);
						shaders[i].meshes[j].descriptorsets.push_back(imageDescriptor);
						++mesh_idx;
					}
				}
			}

			// Then we allocate all other image resources
			int offset = 0;
			for (int i = 0; i < 1; ++i)
			{
				// if (vkAllocateMemory(m_pVulkanDevice->Device(), &allocInfo, nullptr, &offscreenpasses[i].color.memory) != VK_SUCCESS) {
				//	throw std::runtime_error("Failed to allocate memory for depth image on multiview.");
				// }

				if (vkBindImageMemory(m_pVulkanDevice->Device(), renderpass.color.image, memory, image_offsets[offset]) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to bind memory for depth image on multiview.");
				}
				++offset;

				VkImageViewCreateInfo colorView = jsvk::init::imageViewCreateInfo();
				colorView.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				colorView.format = VK_FORMAT_B8G8R8A8_UNORM;
				colorView.flags = 0;
				colorView.subresourceRange = {};
				colorView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				colorView.subresourceRange.baseMipLevel = 0;
				colorView.subresourceRange.levelCount = 1;
				colorView.subresourceRange.baseArrayLayer = 0;
				colorView.subresourceRange.layerCount = 1;
				colorView.image = renderpass.color.image;

				if (vkCreateImageView(m_pVulkanDevice->Device(), &colorView, nullptr, &renderpass.color.view) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to create image view for depth image on multiview.");
				}

				if (vkBindImageMemory(m_pVulkanDevice->Device(), renderpass.depth.image, memory, image_offsets[offset]) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to bind memory for depth image on multiview.");
				}
				++offset;

				VkImageViewCreateInfo depthView = jsvk::init::imageViewCreateInfo();
				depthView.viewType = VK_IMAGE_VIEW_TYPE_2D;
				depthView.format = jsvk::findDepthFormat(m_pVulkanDevice->m_pPhysicalDevice);
				;
				depthView.flags = 0;
				depthView.subresourceRange = {};
				depthView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				depthView.subresourceRange.baseMipLevel = 0;
				depthView.subresourceRange.levelCount = 1;
				depthView.subresourceRange.baseArrayLayer = 0;
				depthView.subresourceRange.layerCount = 1;
				depthView.image = renderpass.depth.image;

				if (vkCreateImageView(m_pVulkanDevice->Device(), &depthView, nullptr, &renderpass.depth.view) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to create image view for depth image on multiview.");
				}
			}

			m_vertexBuffer.m_pMemory = memory;
			m_vertexBuffer.bind(totalImageSize + imagesizes);

			indexBuffer_offset = vertices_texture.size() * sizeof(Vertex) + vertices_no_texture.size() * sizeof(Vertex);
			VkDeviceSize geoSize = vertices_no_texture.size() * sizeof(Vertex);
			VkDeviceSize ovrVertSize = vertices_texture.size() * sizeof(Vertex);
			VkDeviceSize bufferSize = vertices_texture.size() * sizeof(Vertex) + indices.size() * sizeof(uint32_t) + vertices_no_texture.size() * sizeof(Vertex);
			VkDeviceSize bufferSizeIndex = indices.size() * sizeof(uint32_t);

			jsvk::Buffer stagingBuffer;
			VK_CHECK(m_pVulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, bufferSize));
			VK_CHECK(stagingBuffer.map(0, geoSize));
			stagingBuffer.copyTo((void *)vertices_no_texture.data(), geoSize);
			stagingBuffer.unmap();
			if (ovrVertSize > 0)
			{
				VK_CHECK(stagingBuffer.map(geoSize, ovrVertSize));
				stagingBuffer.copyTo((void *)vertices_texture.data(), ovrVertSize);
				stagingBuffer.unmap();
			}
			VK_CHECK(stagingBuffer.map(indexBuffer_offset, bufferSizeIndex));
			stagingBuffer.copyTo((void *)indices.data(), bufferSizeIndex);
			stagingBuffer.unmap();

			// new staging buffer for every fucking thing?!
			jsvk::copyBuffer(m_pVulkanDevice->m_pLogicalDevice, m_pVulkanDevice->m_commandPool, m_pVulkanDevice->m_pGraphicsQueue, stagingBuffer.m_pBuffer, m_vertexBuffer.m_pBuffer, bufferSize);

			stagingBuffer.destroy();
			vertices_texture.empty();
			vertices_no_texture.empty();
			indices.empty();
		}

		void updateImageSamplerDescriptorSet(const VkImageView &imageView, const VkSampler &sampler, VkDescriptorSet &descriptor)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = imageView;
			imageInfo.sampler = sampler;

			VkWriteDescriptorSet samplerDescriptor = jsvk::init::writeDescriptorSet();
			samplerDescriptor.dstSet = descriptor;
			samplerDescriptor.dstBinding = 0;
			samplerDescriptor.dstArrayElement = 0;
			samplerDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerDescriptor.descriptorCount = 1;
			samplerDescriptor.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &samplerDescriptor, 0, nullptr);
		}

		// should initialize the memory
		MemoryManager(jsvk::VulkanDevice *device)
			: m_pVulkanDevice(nullptr), indexBuffer_offset(0), dynamicAlignment(0), uniformData(nullptr), memory(VK_NULL_HANDLE), descriptorLayouts(), uniformDescriptor(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE), m_uniformBuffer(), m_vertexBuffer(), imageOffset(0), vertices_texture(0), vertices_no_texture(0), indices(0), offset(0), num_mesh(0), mesh_offsets(0), mesh_sizes(0), total_verts(0), total_indices(0), totalImageSize(0), vertexOffset(0)
		{
			m_pVulkanDevice = device;
			// VkMemoryRequirements memReq;

			// jsvk::findMemoryType(m_pVulkanDevice->m_pPhysicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// m_pVulkanDevice = vulkanDevice;

			// VkMemoryAllocateInfo allocInfo = jsvk::init::memoryAllocateInfo();

			// allocInfo.allocationSize = size;
			// allocInfo.memoryTypeIndex =

			// VK_CHECK(vkAllocateMemory(m_pVulkanDevice->Device(), &allocInfo, nullptr, &memory));
		}

		~MemoryManager()
		{
			std::cout << "Destroying memory manager" << std::endl;
			// here we free a lot of shit

			// free descriptorpool
			if (descriptorPool != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorPool(m_pVulkanDevice->Device(), descriptorPool, nullptr);
			}

			// free descriptorlayouts
			// TODO make layouts a vector for easy handeling
			if (descriptorLayouts.size() > 0)
			{
				for (auto descriptorLayout : descriptorLayouts)
				{
					vkDestroyDescriptorSetLayout(m_pVulkanDevice->Device(), descriptorLayout, nullptr);
				}
			}

			// lastly memorymanager is deleted
			// temp mem deletion which needs to be moved inside memManager
			if (m_uniformBuffer.m_pMemory != VK_NULL_HANDLE)
			{
				m_uniformBuffer.destroy();
			}
			if (m_vertexBuffer.m_pMemory != VK_NULL_HANDLE)
			{
				m_vertexBuffer.destroy();
			}
		}

		// createdescriptorset

		// createdescriptorlayout

		// updateDynamicUniformBufferDescriptorSet

		// updateImageSamplerDescriptorSet

		// createDescriptorPool

		// createUniformBuffer

		// processMeshFromFile

		// processMeshFromOVR

		// UpdateUBO

		// allocateResources
	};
}

#endif // HEADER_GUARD_VKMEMORYMANAGER
