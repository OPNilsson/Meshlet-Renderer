#pragma once
#ifndef HEADER_GUARD_VKDEVICE
#define HEADER_GUARD_VKDEVICE

#include "jsvkBuffer.h"
#include "jsvkHelpers.h"

namespace jsvk
{

	struct VulkanDevice
	{
		VkPhysicalDevice m_pPhysicalDevice;
		VkDevice m_pLogicalDevice;
		VkPhysicalDeviceProperties m_deviceProperties;
		VkPhysicalDeviceFeatures m_deviceFeatures;
		VkPhysicalDeviceFeatures m_pEnabledDeviceFeatures;
		VkPhysicalDeviceMemoryProperties m_memoryProperties;

		std::vector<VkQueueFamilyProperties> m_queueFamilyProperties;
		std::vector<std::string> m_supportedExtensions;

		VkCommandPool m_commandPool;
		VkQueue m_pGraphicsQueue;

		struct
		{
			uint32_t graphics;
			uint32_t compute;
			uint32_t transfer;
			uint32_t present;

		} m_queueFamilyIndices;

		VulkanDevice()
			: m_pPhysicalDevice(VK_NULL_HANDLE), m_pLogicalDevice(VK_NULL_HANDLE), m_deviceProperties(), m_deviceFeatures(), m_pEnabledDeviceFeatures(), m_memoryProperties(), m_queueFamilyProperties(), m_supportedExtensions(), m_commandPool(VK_NULL_HANDLE), m_queueFamilyIndices(), m_pGraphicsQueue(VK_NULL_HANDLE)
		{
		}

		void init(VkPhysicalDevice physicalDevice)
		{
			m_pPhysicalDevice = physicalDevice;
			// Get properties from the physical device
			vkGetPhysicalDeviceProperties(m_pPhysicalDevice, &m_deviceProperties);
			// Get features from the physical device
			vkGetPhysicalDeviceFeatures(m_pPhysicalDevice, &m_deviceFeatures);
			// Get memory properties for buffers
			vkGetPhysicalDeviceMemoryProperties(m_pPhysicalDevice, &m_memoryProperties);

			// set up queueFamilyIndicies
			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(m_pPhysicalDevice, &queueFamilyCount, nullptr);
			if (queueFamilyCount > 0)
			{
				m_queueFamilyProperties.resize(queueFamilyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(m_pPhysicalDevice, &queueFamilyCount, m_queueFamilyProperties.data());
			}

			// Get list of extensions
			uint32_t extCount = 0;
			vkEnumerateDeviceExtensionProperties(m_pPhysicalDevice, nullptr, &extCount, nullptr);
			if (extCount > 0)
			{
				std::vector<VkExtensionProperties> extensions(extCount);
				if (vkEnumerateDeviceExtensionProperties(m_pPhysicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
				{
					for (auto &ext : extensions)
					{
						m_supportedExtensions.push_back(ext.extensionName);
					}
				}
			}
		}

		~VulkanDevice()
		{
			if (m_commandPool)
			{
				std::cout << "Destroying commandpool" << std::endl;
				vkDestroyCommandPool(m_pLogicalDevice, m_commandPool, nullptr);
			}

			if (m_pLogicalDevice)
			{
				std::cout << "Destroying logical device" << std::endl;
				vkDestroyDevice(m_pLogicalDevice, nullptr);
			}
		}

		VkDevice Device() { return m_pLogicalDevice; };

		VkResult createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char *> enabledExtensions, void *pNextChain, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)
		{

			// First sort out the desired queue families
			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

			const float defaultQueuePriority = 1.0f;
			// graphics queue
			if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
			{
				m_queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
				VkDeviceQueueCreateInfo queueInfo = jsvk::init::deviceQueueCreateInfo();
				queueInfo.queueFamilyIndex = m_queueFamilyIndices.graphics;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
			else
			{
				m_queueFamilyIndices.graphics = 0;
			}

			// compute queue
			if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
			{
				m_queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
				// if compute queue is different from graphics request additional queue
				if (m_queueFamilyIndices.graphics != m_queueFamilyIndices.compute)
				{
					VkDeviceQueueCreateInfo queueInfo = jsvk::init::deviceQueueCreateInfo();
					queueInfo.queueFamilyIndex = m_queueFamilyIndices.compute;
					queueInfo.queueCount = 1;
					queueInfo.pQueuePriorities = &defaultQueuePriority;
					queueCreateInfos.push_back(queueInfo);
				}
			}
			else
			{
				m_queueFamilyIndices.compute = 0;
			}

			// transfer queue
			if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
			{
				m_queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
				// if compute queue is different from graphics request additional queue
				if ((m_queueFamilyIndices.transfer != m_queueFamilyIndices.graphics) && (m_queueFamilyIndices.transfer != m_queueFamilyIndices.compute))
				{
					VkDeviceQueueCreateInfo queueInfo = jsvk::init::deviceQueueCreateInfo();
					queueInfo.queueFamilyIndex = m_queueFamilyIndices.transfer;
					queueInfo.queueCount = 1;
					queueInfo.pQueuePriorities = &defaultQueuePriority;
					queueCreateInfos.push_back(queueInfo);
				}
			}
			else
			{
				m_queueFamilyIndices.transfer = 0;
			}

			// make list of extensions that are needed for logical device creation
			std::vector<const char *> deviceExtensions(enabledExtensions);
			if (useSwapChain)
			{
				deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
			}

			VkDeviceCreateInfo deviceInfo = jsvk::init::deviceCreateInfo();
			deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
			deviceInfo.pEnabledFeatures = &enabledFeatures;

			// check for device extensions
			VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV fragmentBarycenterDeviceFeature = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR};
			deviceExtensions.push_back(VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
			VkPhysicalDeviceMeshShaderFeaturesNV meshFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV};
			VkPhysicalDeviceFloat16Int8FeaturesKHR float16int8Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR};
			VkPhysicalDeviceMultiviewFeatures multiViewFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES};
			VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX multiviewProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX};
			deviceExtensions.push_back(VK_NVX_MULTIVIEW_PER_VIEW_ATTRIBUTES_EXTENSION_NAME);
			VkPhysicalDeviceShadingRateImageFeaturesNV variableShadingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV};
			VkPhysicalDevice16BitStorageFeatures sixteen_bit = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
			deviceExtensions.push_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
			VkPhysicalDeviceFeatures2 physicalDeviceFeatures = jsvk::init::physicalDeviceFeatures2(pNextChain);

			if (useSwapChain)
			{
				physicalDeviceFeatures.features = enabledFeatures;
				deviceInfo.pEnabledFeatures = nullptr;
				deviceInfo.pNext = &physicalDeviceFeatures;

				meshFeatures.meshShader = true;
				meshFeatures.taskShader = true;
				physicalDeviceFeatures.pNext = &meshFeatures;
				meshFeatures.pNext = &float16int8Features;
				float16int8Features.shaderInt8 = true;

				float16int8Features.pNext = &sixteen_bit;
				sixteen_bit.storageBuffer16BitAccess = true;

				fragmentBarycenterDeviceFeature.fragmentShaderBarycentric = true;
				sixteen_bit.pNext = &fragmentBarycenterDeviceFeature;
			}
			else
			{

				meshFeatures.meshShader = true;
				meshFeatures.taskShader = true;
				deviceInfo.pNext = &meshFeatures;
				meshFeatures.pNext = &float16int8Features;
				float16int8Features.shaderInt8 = true;

				float16int8Features.pNext = &sixteen_bit;
				sixteen_bit.storageBuffer16BitAccess = true;

				fragmentBarycenterDeviceFeature.fragmentShaderBarycentric = true;
				sixteen_bit.pNext = &fragmentBarycenterDeviceFeature;
			}

			// VkPhysicalDeviceFeatures2 physicalDeviceFeaturesMesh = jsvk::init::physicalDeviceFeatures2(pNextChain);
			// physicalDeviceFeaturesMesh.features = &meshFeatures;
			// deviceInfo.pEnabledFeatures = nullptr;
			// deviceInfo.pNext = &physicalDeviceFeatures;

			if (deviceExtensions.size() > 0)
			{
				deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
				deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
			}

			VK_CHECK(vkCreateDevice(m_pPhysicalDevice, &deviceInfo, nullptr, &m_pLogicalDevice))

			m_commandPool = createCommandPool(m_queueFamilyIndices.graphics);

			m_pEnabledDeviceFeatures = enabledFeatures;

			return VK_SUCCESS;
		}

		VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
		{
			// create graphicsqueue and pool

			vkGetDeviceQueue(m_pLogicalDevice, queueFamilyIndex, 0, &m_pGraphicsQueue);

			VkCommandPoolCreateInfo poolInfo = jsvk::init::commandPoolCreateInfo();
			poolInfo.queueFamilyIndex = queueFamilyIndex;
			poolInfo.flags = createFlags;
			VkCommandPool pool;
			VK_CHECK(vkCreateCommandPool(m_pLogicalDevice, &poolInfo, nullptr, &pool));
			return pool;
		}

		uint32_t getQueueFamilyIndex(VkQueueFlagBits queueFlags)
		{

			// First look for dedicated queues then look for combined queues
			// compute queue only
			if (queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				for (uint32_t i = 0; i < static_cast<uint32_t>(m_queueFamilyProperties.size()); ++i)
				{
					if ((m_queueFamilyProperties[i].queueFlags & queueFlags) && (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
					{
						return i;
						break;
					}
				}
			}

			// transfer queue only
			if (queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				for (uint32_t i = 0; i < static_cast<uint32_t>(m_queueFamilyProperties.size()); ++i)
				{
					if ((m_queueFamilyProperties[i].queueFlags & queueFlags) && (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
					{
						return i;
						break;
					}
				}
			}

			// combined queues
			for (uint32_t i = 0; i < static_cast<uint32_t>(m_queueFamilyProperties.size()); ++i)
			{
				if (m_queueFamilyProperties[i].queueFlags & queueFlags)
				{
					return i;

					break;
				}
			}

			throw std::runtime_error("could not find a matching queue family index");
		}

		VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, jsvk::Buffer *buffer, VkDeviceSize bufferSize, const void *data = nullptr, bool keepmapped = false)
		{

			VkResult result;

			buffer->m_pDevice = m_pLogicalDevice;

			// handle for buffer
			VkBufferCreateInfo bufferInfo = jsvk::init::bufferCreateInfo();
			bufferInfo.usage = usageFlags;
			bufferInfo.size = bufferSize;
			// sharing mode?
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			result = vkCreateBuffer(m_pLogicalDevice, &bufferInfo, nullptr, &buffer->m_pBuffer);

			if (result == VK_SUCCESS)
			{
				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements(m_pLogicalDevice, buffer->m_pBuffer, &memRequirements);

				VkMemoryAllocateInfo allocInfo = jsvk::init::memoryAllocateInfo();
				allocInfo.allocationSize = memRequirements.size;

				// allocInfo.memoryTypeIndex = findMemoryType(m_pPhysicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				allocInfo.memoryTypeIndex = findMemoryType(m_pPhysicalDevice, memRequirements.memoryTypeBits, memoryPropertyFlags);

				vkAllocateMemory(m_pLogicalDevice, &allocInfo, nullptr, &buffer->m_pMemory);

				buffer->m_allignment = memRequirements.alignment;
				buffer->m_size = bufferSize;
				buffer->m_usageFlags = usageFlags;
				buffer->m_memoryPropertyFlags = memoryPropertyFlags;

				// copy data to buffer
				if (data != nullptr)
				{
					VK_CHECK(buffer->map());
					memcpy(buffer->m_mapped, data, bufferSize);
					if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
					{
						VK_CHECK(buffer->flush());
					}
					buffer->unmap();
				}
				else if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && keepmapped)
				{
					VK_CHECK(buffer->map());
				}

				// Initialize descriptor
				buffer->setupDescriptor();

				// bind memory to buffer
				buffer->bind();
			}

			return result;
		}

		uint32_t findMemoryType(const VkPhysicalDevice &physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
			{
				if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}

			throw std::runtime_error("failed to find suitable memory type!");
		}
	};
}

#endif // HEADER_GUARD_VKDEVICE