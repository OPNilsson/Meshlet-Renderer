#pragma once
#ifndef HEADERGUARD_VKCONTEXT
#define HEADERGUARD_VKCONTEXT

#include <vulkan/vulkan.h>
#include <vector>

#include "jsvkDevice.h"
#include "jsvkSwapchain.h"
#include "jsvkResources.h"
#include "jsvkRenderer.h"
#include "jsvkRenderContext.h"
#include "../presenter.h"

namespace jsvk
{
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
														VkDebugUtilsMessageTypeFlagsEXT messageType,
														const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
														void *pUserData);

	class Context
	{
	private:
		struct vkCandidate
		{
			int score;
			std::vector<VkExtensionProperties> availableExtensions;
			VkPhysicalDevice deviceHandle;
		};

	public:
		VkInstance m_instance;
		VkSampleCountFlagBits m_msaaSamples;
		jsvk::VulkanDevice *m_pVulkanDevice;
		jsvk::RenderContext *m_pRenderContext;
		jsvk::Resources *m_pResources;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		bool m_useVR;
		bool m_headless;
		bool m_useMeshShader;
		std::string m_mode;
		std::vector<vkCandidate> candidates;

		std::vector<const char *> m_deviceExtensions;
		jsvk::Renderer *m_pRenderer;
		std::vector<std::string> m_requiredIstanceExtensions;
#ifdef NDEBUG
		const static bool m_enableValidationLayers = false;
#else
		const static bool m_enableValidationLayers = true;
#endif // DEBUG

		std::vector<const char *> m_validationLayers = {
			"VK_LAYER_KHRONOS_validation"};

		// constructor
		Context();

		// destructor
		~Context();

		void createInstance();
		void createDevice(VkSurfaceKHR surface);
		VkInstance getInstance();
		void createRenderer(jsk::Presenter *presenter);
		void init(jsk::Presenter *presenter, int width, int height);
		void draw();
		void waitForIdle();

		// aux functions
		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);
		std::vector<const char *> getRequiredExtensions();
		bool checkValidationLayerSupport();
		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
											  const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
		void setupDebugMessenger();
		void loadMeshShaders();
		void collectCandidates();
		bool checkExtensionSupport(const std::vector<const char *> availableExtensions, bool Optional = true);
		void addInstanceExtensions(std::vector<std::string> extensions);
		int loadModel(std::vector<std::string> modelPaths);
	};

} // namespace jsvk

#endif // HEADERGUARD_VKCONTEXT