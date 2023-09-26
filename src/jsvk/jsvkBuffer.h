#pragma once
#ifndef VK_BUFFER_H
#define VK_BUFFER_H

#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace jsvk {

	struct Buffer {
		VkDevice m_pDevice;
		VkBuffer m_pBuffer;
		VkDeviceMemory m_pMemory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo m_descriptor;
		VkDeviceSize m_size = 0;
		VkDeviceSize m_allignment = 0;
		void* m_mapped = nullptr;


		VkBufferUsageFlags m_usageFlags;
		VkMemoryPropertyFlags m_memoryPropertyFlags;

		VkResult map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE)
		{
			return vkMapMemory(m_pDevice, m_pMemory, offset, size, 0, &m_mapped);
		}

		VkResult mapPart(VkDeviceSize offset = 0)
		{
			return vkMapMemory(m_pDevice, m_pMemory, offset, m_size, 0, &m_mapped);
		}


		void unmap() 
		{
			if (m_mapped) 
			{
				vkUnmapMemory(m_pDevice, m_pMemory);
				m_mapped = nullptr;
			}
		}
		VkDeviceSize m_offset = 0;
		VkResult bind(VkDeviceSize offset = 0)
		{
			m_offset += offset;
			return vkBindBufferMemory(m_pDevice, m_pBuffer, m_pMemory, m_offset);
		}

		void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
		{
			m_descriptor.offset = 0;
			m_descriptor.buffer = m_pBuffer;
			m_descriptor.range = size;
		}

		void copyTo(void* data, VkDeviceSize size)
		{
			assert(m_mapped);
			memcpy(m_mapped, data, size);
		}

		VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
		{
			VkMappedMemoryRange mappedRange{};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = m_pMemory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			return vkFlushMappedMemoryRanges(m_pDevice, 1, &mappedRange);
		}

		VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
		{
			VkMappedMemoryRange mappedRange{};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = m_pMemory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			return vkInvalidateMappedMemoryRanges(m_pDevice, 1, &mappedRange);
		}

		void destroy() 
		{
			if (m_pBuffer)
			{
				vkDestroyBuffer(m_pDevice, m_pBuffer, nullptr);
			}
			if (m_pMemory) {
				vkFreeMemory(m_pDevice, m_pMemory, nullptr);
			}
		}
	};
}
#endif //VK_BUFFER_H
