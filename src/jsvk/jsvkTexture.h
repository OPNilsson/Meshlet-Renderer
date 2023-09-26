#pragma once
#ifndef HEADERGUARD_VKTEXTURE
#define HEADERGUARD_VKTEXTURE

#include "jsvkBuffer.h"
#include "jsvkDevice.h"
#include "jsvkUtilities.h"

//#include <headers/openvr.h>

namespace jsvk {
	class Texture {
	public:
		jsvk::VulkanDevice* m_pLogicalDevice;
		VkImage image = VK_NULL_HANDLE;
		VkImageView view;
		VkImageLayout layout;
		VkDeviceMemory m_memory;
		VkMemoryRequirements memRequirements;
		VkSampler sampler;
		int width, height;
		uint32_t mipLevels;
		VkDeviceSize imageSize;
		char* pixels;
		//const vr::RenderModel_TextureMap_t* diffuseTexture;
		bool image_type;

		Texture();
		Texture(jsvk::VulkanDevice* device);
		void Destroy();
		//void CreateImageObjectFromOVR(const vr::RenderModel_TextureMap_t* vrDiffuseTexture, bool mipmap=true);
		void CreateImageObjectFromFile(const std::string texture_path, bool mipmap = true);
		uint32_t CalculateMipMapLevels(uint32_t width, uint32_t height);
		void createViewAndSampler();
		void bindImage(const VkMemoryPropertyFlags properties, VkDeviceMemory memory, VkDeviceSize offset);
		void copyImage();
		void createTextureFromFile(const std::string texture_path);
		//void jsvk::Texture::CreateImageObjectFromOVR(const vr::RenderModel_TextureMap_t* vrDiffuseTexture, bool mipmap);
		//void createTextureFromOVR(const vr::RenderModel_TextureMap_t* vrDiffuseTexture);
	};
}

#endif // HEADERGUARD_VKTEXTURE