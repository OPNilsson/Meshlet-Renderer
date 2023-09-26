#include "jsvkTexture.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#include <stb_image.h>
#include <math.h>
#include <algorithm>





jsvk::Texture::Texture() {

}

jsvk::Texture::Texture(jsvk::VulkanDevice* device) {
	m_pLogicalDevice = device;
}


 void jsvk::Texture::Destroy() {
	vkDestroyImageView(m_pLogicalDevice->m_pLogicalDevice, view, nullptr);
	vkDestroyImage(m_pLogicalDevice->m_pLogicalDevice, image, nullptr);
	if (sampler) {
		vkDestroySampler(m_pLogicalDevice->m_pLogicalDevice, sampler, nullptr);
	}
}
 /*
void jsvk::Texture::CreateImageObjectFromOVR(const vr::RenderModel_TextureMap_t* vrDiffuseTexture, bool mipmap) {
	 diffuseTexture = vrDiffuseTexture;
	 image_type = 0;

	 width = vrDiffuseTexture->unWidth;
	 height = vrDiffuseTexture->unHeight;

	 imageSize = sizeof(uint8_t) * width * height;

	 if (mipmap) {

		 mipLevels = CalculateMipMapLevels(width, height);
	 }
	 else {
		 mipLevels = 1;
	 }

	 // not now 
	 //jsvk::Buffer imageBuffer;
	 //VK_CHECK(m_pLogicalDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageBuffer, imageSize, vrDiffuseTexture->rubTextureMapData));

	 //jsvk::createImage(m_pLogicalDevice->Device(), m_pLogicalDevice->m_pPhysicalDevice, width, height, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory, memRequirements);

	 VkImageCreateInfo imageInfo = {};
	 imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	 imageInfo.imageType = VK_IMAGE_TYPE_2D;
	 imageInfo.extent.width = static_cast<uint32_t>(width);
	 imageInfo.extent.height = static_cast<uint32_t>(height);
	 imageInfo.extent.depth = 1;
	 imageInfo.mipLevels = mipLevels;
	 imageInfo.arrayLayers = 1;
	 imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	 imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	 imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	 imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	 imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	 imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	 imageInfo.flags = 0; // Optional

	 if (vkCreateImage(m_pLogicalDevice->Device(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
		 throw std::runtime_error("failed to create image!");
	 }

	 vkGetImageMemoryRequirements(m_pLogicalDevice->Device(), image, &memRequirements);

 }
 */

uint32_t jsvk::Texture::CalculateMipMapLevels(uint32_t width, uint32_t height) {
	return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

void jsvk::Texture::CreateImageObjectFromFile(const std::string texture_path, bool mipmap) {
	int texChannels;
	image_type = 1;

	pixels = (char*)stbi_load(texture_path.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);

	imageSize = (VkDeviceSize)width * height * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	if (mipmap) {

		mipLevels = CalculateMipMapLevels(width, height);
	}
	else {
		mipLevels = 1;
	}

	// not now
	//jsvk::Buffer imageBuffer;
	//VK_CHECK(m_pLogicalDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageBuffer, imageSize, pixels));

	//stbi_image_free(pixels);

	//jsvk::createImage(m_pLogicalDevice->Device(), m_pLogicalDevice->m_pPhysicalDevice, width, height, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory, memRequirements);

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	if (vkCreateImage(m_pLogicalDevice->Device(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	vkGetImageMemoryRequirements(m_pLogicalDevice->Device(), image, &memRequirements);

}

void jsvk::Texture::createViewAndSampler() {
	// Image view
	view = jsvk::createImageView(m_pLogicalDevice->m_pLogicalDevice, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	// Image sampler
	VkSamplerCreateInfo samplerInfo = jsvk::init::samplerCreateInfo();
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	// can increase performance by deactivating this
	// did not see any performance problems from this
	samplerInfo.anisotropyEnable = VK_TRUE;
	// More than 16 samples is not used since the difference is neglible beyond 16.
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0;
	samplerInfo.minLod = 0; //static_cast<float>(mipLevels / 2);
	samplerInfo.maxLod = static_cast<float>(mipLevels);

	VK_CHECK(vkCreateSampler(m_pLogicalDevice->m_pLogicalDevice, &samplerInfo, nullptr, &sampler));
}

void jsvk::Texture::bindImage(const VkMemoryPropertyFlags properties, VkDeviceMemory memory, VkDeviceSize offset) {

	VkMemoryAllocateInfo allocInfo = jsvk::init::memoryAllocateInfo();
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = jsvk::findMemoryType(m_pLogicalDevice->m_pPhysicalDevice, memRequirements.memoryTypeBits, properties);

	//if (vkAllocateMemory(m_pLogicalDevice->Device(), &allocInfo, nullptr, &memory) != VK_SUCCESS) {
	//	throw std::runtime_error("failed to allocate image memory!");
	//}
	m_memory = memory;
	vkBindImageMemory(m_pLogicalDevice->Device(), image, m_memory, offset);

}

void  jsvk::Texture::copyImage() {

	jsvk::Buffer imageBuffer;
	VkDeviceSize imageSize;
	if (image_type) {
		imageSize = (VkDeviceSize)width * height * 4;
		VK_CHECK(m_pLogicalDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageBuffer, imageSize, pixels));
		stbi_image_free(pixels);
	}
	else {
		VkDeviceSize imageSize = sizeof(uint8_t) * width * height * 4;
		// TODO:: COMMENTED OUT UNTIL VR IS ADDED
		//VK_CHECK(m_pLogicalDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageBuffer, imageSize, diffuseTexture->rubTextureMapData));
	}

	jsvk::createImage(m_pLogicalDevice->Device(), m_pLogicalDevice->m_pPhysicalDevice, width, height, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, m_memory);

	jsvk::transitionImageLayout(m_pLogicalDevice->m_pLogicalDevice, m_pLogicalDevice->m_commandPool, m_pLogicalDevice->m_pGraphicsQueue, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	jsvk::copyBufferToImage(imageBuffer.m_pBuffer, image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), m_pLogicalDevice);

	imageBuffer.destroy();

	jsvk::generateMipmaps(image, VK_FORMAT_R8G8B8A8_UNORM, width, height, mipLevels, m_pLogicalDevice);

	createViewAndSampler();
}

//void bindImage(const VkDevice& device, const VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties, const VkMemoryPropertyFlags properties, const VkImage& image, VkDeviceMemory& imageMemory, const VkMemoryRequirements& memRequirements) {

//	VkMemoryAllocateInfo allocInfo = jsvk::init::memoryAllocateInfo();
//	allocInfo.allocationSize = memRequirements.size;
//	allocInfo.memoryTypeIndex;
//	jsvk::MemoryTypeFromProperties(physicalDeviceMemoryProperties, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocInfo.memoryTypeIndex);

//	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
//		throw std::runtime_error("failed to allocate image memory!");
//	}

//	vkBindImageMemory(device, image, imageMemory, 0);

//}


// TODO: SET UP METHOD SUCH THAT IT CAN HANDLE MIP MAPPING ON/OFF
// TODO: refactor mipmap into its own 
// TODO: refactor common part into own function

void jsvk::Texture::createTextureFromFile(const std::string texture_path) {
	int texChannels;
	stbi_uc* pixels = stbi_load(texture_path.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = (VkDeviceSize)width * height * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	jsvk::Buffer imageBuffer;
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

	VK_CHECK(m_pLogicalDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageBuffer, imageSize, pixels));

	stbi_image_free(pixels);

	jsvk::createImage(m_pLogicalDevice->Device(), m_pLogicalDevice->m_pPhysicalDevice, width, height, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, m_memory);

	jsvk::transitionImageLayout(m_pLogicalDevice->m_pLogicalDevice, m_pLogicalDevice->m_commandPool, m_pLogicalDevice->m_pGraphicsQueue, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	jsvk::copyBufferToImage(imageBuffer.m_pBuffer, image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), m_pLogicalDevice);

	imageBuffer.destroy();

	jsvk::generateMipmaps(image, VK_FORMAT_R8G8B8A8_UNORM, width, height, mipLevels, m_pLogicalDevice);
	// Image view
	view = jsvk::createImageView(m_pLogicalDevice->m_pLogicalDevice, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	// Image sampler
	VkSamplerCreateInfo samplerInfo = jsvk::init::samplerCreateInfo();
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	// can increase performance by deactivating this
	samplerInfo.anisotropyEnable = VK_TRUE;
	// More than 16 samples is not used since the difference is neglible beyond 16.
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0;
	samplerInfo.minLod = 0; //static_cast<float>(mipLevels / 2);
	samplerInfo.maxLod = static_cast<float>(mipLevels);

	VK_CHECK(vkCreateSampler(m_pLogicalDevice->m_pLogicalDevice, &samplerInfo, nullptr, &sampler));
}
/*
void jsvk::Texture::createTextureFromOVR(const vr::RenderModel_TextureMap_t* vrDiffuseTexture) {
	width = vrDiffuseTexture->unWidth;
	height = vrDiffuseTexture->unHeight;

	VkDeviceSize imageSize = sizeof(uint8_t) * width * height;

	jsvk::Buffer imageBuffer;
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

	VK_CHECK(m_pLogicalDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageBuffer, imageSize, vrDiffuseTexture->rubTextureMapData));

	jsvk::createImage(m_pLogicalDevice->Device(), m_pLogicalDevice->m_pPhysicalDevice, width, height, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, m_memory);

	jsvk::transitionImageLayout(m_pLogicalDevice->m_pLogicalDevice, m_pLogicalDevice->m_commandPool, m_pLogicalDevice->m_pGraphicsQueue, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	jsvk::copyBufferToImage(imageBuffer.m_pBuffer, image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), m_pLogicalDevice);

	imageBuffer.destroy();

	jsvk::generateMipmaps(image, VK_FORMAT_R8G8B8A8_UNORM, width, height, mipLevels, m_pLogicalDevice);
	// Image view
	view = jsvk::createImageView(m_pLogicalDevice->m_pLogicalDevice, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	// Image sampler
	VkSamplerCreateInfo samplerInfo = jsvk::init::samplerCreateInfo();
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	// can increase performance by deactivating this
	samplerInfo.anisotropyEnable = VK_TRUE;
	// More than 16 samples is not used since the difference is neglible beyond 16.
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0;
	samplerInfo.minLod = 0; //static_cast<float>(mipLevels / 2);
	samplerInfo.maxLod = static_cast<float>(mipLevels);

	VK_CHECK(vkCreateSampler(m_pLogicalDevice->m_pLogicalDevice, &samplerInfo, nullptr, &sampler));

}
*/

