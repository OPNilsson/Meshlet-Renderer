#pragma once
#ifndef HEADER_GUARD_VKHELPERS
#define HEADER_GUARD_VKHELPERS

#include <vulkan/vulkan.h>
#include <iostream>
#include <cassert>




#define VK_CHECK(f) \
{                    \
	VkResult res = (f); \
	if (res != VK_SUCCESS) \
	{ \
		std::cout << "Fatal : VkResult is \"" << jsvk::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
		assert(res == VK_SUCCESS); \
	} \
 \
} \

#define CONVERT_ERROR_TO_MESSAGE(r) case VK_ ##r: return #r

namespace jsvk {

	std::string errorString(VkResult errorCode);

	namespace init{
	
		inline VkMemoryAllocateInfo memoryAllocateInfo()
		{
			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.pNext = 0;
			return allocInfo;
		}

		inline VkImageCreateInfo imageCreateInfo()
		{
			VkImageCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			createInfo.pNext = 0;
			return createInfo;
		}

		inline VkImageViewCreateInfo imageViewCreateInfo()
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.pNext = 0;
			return viewInfo;
		}

		inline VkSamplerCreateInfo samplerCreateInfo()
		{
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.pNext = 0;
			return samplerInfo;
		}

		inline VkRenderPassCreateInfo renderPassCreateinfo()
		{
			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pNext = 0;
			return renderPassInfo;
		}

		inline VkFramebufferCreateInfo framebufferCreateInfo()
		{
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.pNext = 0;
			return framebufferInfo;
		}

		inline VkPipelineShaderStageCreateInfo pipelineShaderStageCreateinfo()
		{
			VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
			shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCreateInfo.pNext = 0;
			return shaderStageCreateInfo;
		}

		inline VkPipelineVertexInputStateCreateInfo pipelineVertexinputStateCreateInfo()
		{
			VkPipelineVertexInputStateCreateInfo vertInputInfo{};
			vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertInputInfo.pNext = 0;
			return vertInputInfo;
		}

		inline VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo()
		{
			VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
			assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			assemblyInfo.pNext = 0;
			return assemblyInfo;
		}

		inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo()
		{
			VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
			dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateInfo.pNext = 0;
			return dynamicStateInfo;
		}

		inline VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo()
		{
			VkPipelineViewportStateCreateInfo viewportStateInfo{};
			viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateInfo.pNext = 0;
			return viewportStateInfo;
		}

		inline VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo()
		{
			VkPipelineRasterizationStateCreateInfo rasterizationStateInfo{};
			rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationStateInfo.pNext = 0;
			return rasterizationStateInfo;
		}

		inline VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo()
		{
			VkPipelineMultisampleStateCreateInfo multisampleStateInfo{};
			multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleStateInfo.pNext = 0;
			return multisampleStateInfo;
		}

		inline VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo()
		{
			VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{};
			depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilStateInfo.pNext = 0;
			return depthStencilStateInfo;
		}

		inline VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo()
		{
			VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
			colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendStateCreateInfo.pNext = 0;
			return colorBlendStateCreateInfo;
		}

		inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo()
		{
			VkPipelineLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutInfo.pNext = 0;
			return layoutInfo;
		}

		inline VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo()
		{
			VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
			graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineInfo.pNext = 0;
			return graphicsPipelineInfo;
		}

		inline VkSemaphoreCreateInfo semaphoreCreateInfo()
		{
			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreInfo.pNext = 0;
			return semaphoreInfo;
		}

		inline VkFenceCreateInfo fenceCreateinfo()
		{
			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.pNext = 0;
			return fenceInfo;
		}

		inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo()
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.pNext = 0;
			return descriptorPoolInfo;
		}

		inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo()
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
			descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutInfo.pNext = 0;
			return descriptorSetLayoutInfo;
		}

		inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo()
		{
			VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
			descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorAllocateInfo.pNext = 0;
			return descriptorAllocateInfo;
		}

		inline VkBufferCreateInfo bufferCreateInfo() 
		{
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.pNext = 0;
			return bufferInfo;
		}

		inline VkCommandPoolCreateInfo commandPoolCreateInfo()
		{
			VkCommandPoolCreateInfo commandPoolInfo{};
			commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolInfo.pNext = 0;
			return commandPoolInfo;
		}

		inline VkSwapchainCreateInfoKHR swapchainCreateInfoHKR()
		{
			VkSwapchainCreateInfoKHR swapchainInfo{};
			swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchainInfo.pNext = 0;
			return swapchainInfo;
		}

		inline VkDeviceQueueCreateInfo deviceQueueCreateInfo() 
		{
			VkDeviceQueueCreateInfo deviceQueueInfo{};
			deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueInfo.pNext = 0;
			return deviceQueueInfo;
		}

		inline VkDeviceCreateInfo deviceCreateInfo()
		{
			VkDeviceCreateInfo deviceInfo{};
			deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceInfo.pNext = 0;
			return deviceInfo;
		}

		inline VkInstanceCreateInfo instanceCreateInfo()
		{
			VkInstanceCreateInfo instanceInfo{};
			instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instanceInfo.pNext = 0;
			return instanceInfo;
		}

		inline VkCommandBufferAllocateInfo commandBufferAllocateInfo()
		{
			VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.pNext = 0;
			return commandBufferAllocateInfo;
		}

		inline VkCommandBufferBeginInfo commandBufferBeginInfo()
		{
			VkCommandBufferBeginInfo bufferBeginInfo{};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			bufferBeginInfo.pNext = 0;
			return bufferBeginInfo;
		}

		inline VkImageMemoryBarrier imageMemoryBarrier()
		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.pNext = 0;
			return imageMemoryBarrier;

		}

		inline VkWriteDescriptorSet writeDescriptorSet()
		{
			VkWriteDescriptorSet descriptorSet{};
			descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorSet.pNext = 0;
			return descriptorSet;
		}

		inline VkPhysicalDeviceFeatures2KHR physicalDeviceFeatures2KHR()
		{
			VkPhysicalDeviceFeatures2KHR physicalDevice2HKR{};
			physicalDevice2HKR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
			physicalDevice2HKR.pNext = 0;
			return physicalDevice2HKR;
		}

		inline VkPhysicalDeviceMultiviewFeaturesKHR physicalDeviceMultiviewFeaturesKHR()
		{
			VkPhysicalDeviceMultiviewFeaturesKHR physicalDeviceMultiviewFeaturesHKR{};
			physicalDeviceMultiviewFeaturesHKR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
			physicalDeviceMultiviewFeaturesHKR.pNext = 0;
			return physicalDeviceMultiviewFeaturesHKR;
		}

		inline VkRenderPassBeginInfo renderPassBeginInfo()
		{
			VkRenderPassBeginInfo renderPassBeginInfo{};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = 0;
			return renderPassBeginInfo;
		}

		inline VkSubmitInfo submitInfo()
		{
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = 0;
			return submitInfo;
		}

		inline VkPresentInfoKHR presentInfoKHR()
		{
			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = 0;
			return presentInfo;
		}

		inline VkApplicationInfo applicationInfo()
		{
			VkApplicationInfo applicationInfo{};
			applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			applicationInfo.pNext = 0;
			return applicationInfo;
		}

		inline VkPhysicalDeviceFeatures2 physicalDeviceFeatures2(void * pNextChain) {
			VkPhysicalDeviceFeatures2 deviceFeatures{};
			deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			deviceFeatures.pNext = pNextChain;
			return deviceFeatures;
		}

		inline VkPushConstantRange pushConstantRange(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset){
			VkPushConstantRange pushRange{};
			pushRange.stageFlags = stageFlags;
			pushRange.offset = offset;
			pushRange.size = size;
			return pushRange;
		}
	}
}

#endif //HEADER_GUARD_VKHELPERS




