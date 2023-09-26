#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include <vector>

#include "jsvkDevice.h"
#include "jsvkResources.h"

namespace jsvk
{
	class Renderer
	{
	public:
		class RenderType
		{
		public:
			RenderType() { getRegistry().push_back(this); }

		public:
			virtual bool isAvailable() const = 0;
			virtual const char *name() const = 0;
			virtual Renderer *create() const = 0;
			virtual unsigned int priority() const { return 0xFF; }
		};

		typedef std::vector<RenderType *> RenderRegistry;

		static RenderRegistry &getRegistry()
		{
			static RenderRegistry s_registry;
			return s_registry;
		}

	public:
		virtual jsvk::Resources *getResources() = 0;
		virtual bool init(jsvk::VulkanDevice *pVulkanDevice, jsvk::Resources *pResources) = 0;
		virtual void deinit() = 0;
		virtual void draw(uint32_t imageIndex, VkCommandBuffer *primaryBuffer) = 0;
		virtual void updateUniforms() = 0;
		virtual void drawDynamic(GameObject *mesh, uint32_t imageIndex, VkCommandBuffer *primaryBuffer, VkCommandBufferInheritanceInfo &inheritInfo, GameObject *offsetMesh, int gameObjectOffset) = 0;

		virtual ~Renderer() {}
	};

}

#endif // RENDERER_H