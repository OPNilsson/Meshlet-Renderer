#pragma once
#ifndef HEADER_GUARD_VKUTILITIES
#define HEADER_GUARD_VKUTILITIES
#include <optional>
#include <vector>

#include "jsvkDevice.h"




namespace jsvk {


	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilites;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	std::vector<char> readFile(const std::string& filename);

	VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDevice & physicalDevice);

	VkFormat findDepthFormat(const VkPhysicalDevice & physicalDevice);
	VkFormat findSupportedFormat(const VkPhysicalDevice & physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	VkShaderModule createShaderModule(const VkDevice & device, const std::vector<char>& code);

	VkImageView createImageView(const VkDevice & device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, jsvk::VulkanDevice* device);

	uint32_t findMemoryType(const VkPhysicalDevice & physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	QueueFamilyIndices findQueueFamiliesWithoutPresentation(const VkPhysicalDevice& device);
	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice & device, const VkSurfaceKHR & surface);
	//VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow * window);
	//SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice & device, const VkSurfaceKHR & surface);
	void createImageOnly(const VkDevice& device, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage& image);
	void createImage(const VkDevice & device, const VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void createImage(const VkDevice& device, const VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	int rateDeviceSuitability(const VkPhysicalDevice & device);
	bool isDeviceSuitableHeadless(const VkPhysicalDevice& device, const std::vector<const char*> deviceExtensions);
	bool isDeviceSuitable(const VkPhysicalDevice & device, const VkSurfaceKHR & surface, const std::vector<const char*> deviceExtensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions);

	VkCommandBuffer beginSingleTimeCommands(const VkDevice & device, const VkCommandPool & commandPool);
	void endSingleTimeCommands(const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & graphicsQueue, const VkCommandBuffer & commandBuffer);

	void copyBuffer(const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, jsvk::VulkanDevice* device);

	void transitionImageLayout(const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	bool MemoryTypeFromProperties(const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t nMemoryTypeBits, VkMemoryPropertyFlags nMemoryProperties, uint32_t* pTypeIndexOut);
}

#endif