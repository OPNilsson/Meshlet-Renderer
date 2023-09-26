
#include "jsvkContext.h"
#include "jsvkHelpers.h"
#include "jsvkUtilities.h"
#include "jsvkExtensions.h"

// #include "../jsvr/vrRenderContext.h"

#include <map>
#include <cstring>
#include <iostream>
#include <set>

namespace jsvk
{

	Context::Context()
		: m_instance(VK_NULL_HANDLE), m_msaaSamples(VK_SAMPLE_COUNT_1_BIT), m_pVulkanDevice(nullptr), m_pRenderContext(nullptr), m_pResources(nullptr), m_debugMessenger(nullptr), m_useVR(false), m_headless(false), m_useMeshShader(true), m_mode(""), m_deviceExtensions(), m_pRenderer(nullptr), m_requiredIstanceExtensions()

	{
	}

	void Context::init(jsk::Presenter *presenter, int width, int height)
	{
		const Resources::ResourcesRegistry registry = Resources::getRegistry();

		std::cout << "Number Supported Resources: " << registry.size() << std::endl;
		for (size_t i = 0; i < registry.size(); i++)
		{
			std::string rendererName = registry[i]->name();
			if (rendererName == m_mode + " Resources")
			{

				std::cout << registry[i]->name() << std::endl;
				m_pResources = registry[i]->create();
				m_pResources->init(m_pVulkanDevice, presenter, m_msaaSamples, width, height);
			}
		};
	}

	int Context::loadModel(std::vector<std::string> modelPaths)
	{
		// return m_pResources->loadModel(meshletGeometry, vertCount,  vertices,  objectData,  stats);
		return m_pResources->loadModel(modelPaths);
	}

	Context::~Context()
	{

		if (m_pResources != nullptr)
		{
			m_pResources->deinit();
			delete m_pResources;
		}

		// Destroy debug messenger
		if (m_debugMessenger != nullptr)
		{
			DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
		}

		if (m_pRenderContext != nullptr)
		{
			m_pRenderContext->deinit();
			delete m_pRenderContext;
		}

		if (m_pVulkanDevice != nullptr)
		{
			delete m_pVulkanDevice;
		}

		if (m_instance != VK_NULL_HANDLE)
		{
			std::cout << "destroying Instance" << std::endl;
			vkDestroyInstance(m_instance, nullptr);
		}
	}

	void Context::createRenderer(jsk::Presenter *presenter)
	{
		const Renderer::RenderRegistry registry = Renderer::getRegistry();

		std::cout << "Number Supported Renderers: " << registry.size() << std::endl;
		for (size_t i = 0; i < registry.size(); i++)
		{
			std::string rendererName = registry[i]->name();
			if (rendererName == m_mode + " Renderer")
			{

				std::cout << registry[i]->name() << std::endl;
				m_pRenderer = registry[i]->create();
				m_pRenderer->init(m_pVulkanDevice, m_pResources);
			}
		};

		// if (m_mode == "VR Mesh") {
		//	m_pRenderContext =  new jsvr::VRRenderContext(presenter, m_pVulkanDevice);//m_pRenderer->getRenderContext(m_pSwapchain);
		// }
		// else {
		//	m_pRenderContext =  new jsvk::VKRenderContext(presenter, m_pVulkanDevice);//m_pRenderer->getRenderContext(m_pSwapchain);
		// }
		m_pRenderContext = new jsvk::VKRenderContext(presenter, m_pVulkanDevice); // m_pRenderer->getRenderContext(m_pSwapchain);

		m_pRenderContext->m_pRenderer = m_pRenderer;
	}

	void Context::draw()
	{
		m_pRenderContext->Render();
	}

	void Context::loadMeshShaders()
	{
		int status = load_VK_NV_mesh_shader(m_instance, vkGetInstanceProcAddr, m_pVulkanDevice->Device(), vkGetDeviceProcAddr);
		if (status)
		{
			std::cout << "Loaded mesh shaders" << std::endl;
		}
	}

	void Context::waitForIdle()
	{
		vkDeviceWaitIdle(m_pVulkanDevice->Device());
	}

	void Context::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	VkInstance Context::getInstance()
	{
		return m_instance;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
														VkDebugUtilsMessageTypeFlagsEXT messageType,
														const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
														void *pUserData)
	{

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	std::vector<const char *> Context::getRequiredExtensions()
	{

		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (m_enableValidationLayers)
		{
			std::cout << "Enabling validation layers" << std::endl;
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	VkResult Context::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
												   const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	bool Context::checkValidationLayerSupport()
	{

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::cout << "Available Layers: " << layerCount << std::endl;
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char *layerName : m_validationLayers)
		{
			bool layerFound = false;
			for (const auto &layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	void Context::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		// message types: VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional
	}

	void Context::setupDebugMessenger()
	{
		if (!m_enableValidationLayers)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		VK_CHECK(CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger));
	}

	void Context::collectCandidates()
	{
		uint32_t physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

		// throw error if none of the GPUS support vulkan
		if (physicalDeviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::cout << "Number of GPUS with Vulkan support: " << physicalDeviceCount << "." << std::endl;

		// get info about all physical devices
		std::vector<VkPhysicalDevice> devices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, devices.data());

		candidates.resize(physicalDeviceCount);

		for (int i = 0; i < physicalDeviceCount; ++i)
		{
			candidates[i].deviceHandle = devices[i];
			candidates[i].score = jsvk::rateDeviceSuitability(candidates[i].deviceHandle);

			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(candidates[i].deviceHandle, nullptr, &extensionCount, nullptr);

			candidates[i].availableExtensions.resize(extensionCount);
			vkEnumerateDeviceExtensionProperties(candidates[i].deviceHandle, nullptr, &extensionCount, candidates[i].availableExtensions.data());
		}
	}

	void Context::addInstanceExtensions(std::vector<std::string> extensions)
	{
		for (std::string extension : extensions)
		{
			m_requiredIstanceExtensions.push_back(extension);
		}
	}

	bool Context::checkExtensionSupport(const std::vector<const char *> requestedExtensions, bool Optional)
	{

		for (const auto candidate : candidates)
		{
			std::set<std::string> requiredExtensions(requestedExtensions.begin(), requestedExtensions.end());

			for (const auto &extension : candidate.availableExtensions)
			{
				requiredExtensions.erase(extension.extensionName);
			}
			if (requiredExtensions.empty())
			{
				// add extensions to context if they are supported
				for (auto extension : requestedExtensions)
				{
					m_deviceExtensions.push_back(extension);
				}
				return true;
			}
		}

		if (Optional)
		{
			return false;
		}
		else
		{
			throw std::runtime_error("failed to find a GPU with support for requested Extensions");
		}
	}

	void Context::createDevice(VkSurfaceKHR surface)
	{
		// Only works with experimental driver that crashes blender smh
		// deviceExtensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
		// score found devices in decending order
		std::multimap<int, VkPhysicalDevice> approvedCandidates;

		int i = 0;
		for (const auto device : candidates)
		{
			// if (jsvk::isDeviceSuitable(device, vulkanSwapchain.m_pSurface, deviceExtensions)) {//&& vr::checkVRSupport(device, m_pHMD->m_phmdInstance, instance)) {
			//  so far we are only checking if device can present -- should add more
			if (jsvk::isDeviceSuitable(device.deviceHandle, surface, m_deviceExtensions))
			{
				int score = jsvk::rateDeviceSuitability(device.deviceHandle);
				approvedCandidates.insert(std::make_pair(score, device.deviceHandle));

				VkPhysicalDeviceProperties props;
				vkGetPhysicalDeviceProperties(device.deviceHandle, &props);

				std::cout << "Candidate nr " << i << ". Device name: " << props.deviceName << ". Score: " << score << "." << std::endl;
				++i;
			}
		}

		// If no device meet our requirements throw error
		if (approvedCandidates.size() == 0)
		{
			throw std::runtime_error("failed to find a suitable GPUs with Vulkan support!");
		}

		// Get physicaldevice with largest score
		VkPhysicalDevice physicalDevice = approvedCandidates.rbegin()->second;

		// can this be done more elegantly? ( save in vulkandevice maybe )
		m_msaaSamples = jsvk::getMaxUsableSampleCount(physicalDevice);

		// TODO: here we create two devices that can be used to devide processing
		// we need a main GPU which has the connection to the headset
		// and a secondary that renders one eye and copies the result back to main GPU

		m_pVulkanDevice = new jsvk::VulkanDevice();
		m_pVulkanDevice->init(physicalDevice);

		VkPhysicalDeviceFeatures deviceFeatures = {};
		// Enable anisotropy
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.sampleRateShading = VK_TRUE;

		// Get the required OpenVR Extensions for the device
		std::vector<std::string> requiredDeviceExtensions;

		for (int extension = 0; extension < requiredDeviceExtensions.size(); ++extension)
		{
			m_deviceExtensions.push_back(requiredDeviceExtensions[extension].c_str());
		}

		VK_CHECK(m_pVulkanDevice->createLogicalDevice(deviceFeatures, m_deviceExtensions, nullptr));
	}

	void Context::createInstance()
	{

		if (m_enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		// Create structure that holds information about application
		VkApplicationInfo appInfo = jsvk::init::applicationInfo();
		appInfo.pApplicationName = "Jinsoku";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Jinsoku";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_1;

		// Create structure that holds information about instance
		VkInstanceCreateInfo createInfo = jsvk::init::instanceCreateInfo();
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();

		if (m_useMeshShader)
		{
			m_requiredIstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}

		for (int extension = 0; extension < m_requiredIstanceExtensions.size(); ++extension)
		{
			extensions.push_back(m_requiredIstanceExtensions[extension].c_str());
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// Set up debug messenger for create and destroy instance
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

		// Add validation layers
		if (m_enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
			createInfo.ppEnabledLayerNames = m_validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));

		std::cout << "Instance created: " << m_instance << "\n";

		collectCandidates();
	}
}
