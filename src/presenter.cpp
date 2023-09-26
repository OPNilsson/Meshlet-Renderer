#include "presenter.h"
#include "inputhandler.h"

namespace jsk {

	Presenter::Presenter()
		: m_usingVR(false)
	{

	}

	Presenter::~Presenter() {
		// order is important!
		if (m_pSwapchain != nullptr) {
			delete m_pSwapchain;
		}

		//if (m_pHmd != nullptr) {
		//	delete m_pHmd;
		//}
	}

	void Presenter::initGLFW(int width, int height) {
		// Initialize glfw
		glfwInit();

		// Tell glfw to not create an opengl context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// Disable resize
		// for now this is best option size swapchain is fucked
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		// Create window
		m_pWindow = glfwCreateWindow(width, height, "Jinsoku", nullptr, nullptr);
		glfwSetWindowUserPointer(m_pWindow, this);
		glfwSetFramebufferSizeCallback(m_pWindow, framebufferResizeCallback);
	}

	//void Presenter::initVRPresentation(jsvr::HmdInstance* pHmd) {
	//	m_pHmd = pHmd;
	//	m_usingVR = true;

	//}


	//void Presenter::setUpVulkanData(jsvk::VulkanDevice* device, int width, int height,VkSampleCountFlagBits msaaSamples, VkInstance* instance) {

	//	m_vulkanData.m_pDevice = (VkDevice_T*)device->Device();
	//	m_vulkanData.m_pPhysicalDevice = (VkPhysicalDevice_T*)device->m_pPhysicalDevice;
	//	m_vulkanData.m_pInstance = (VkInstance_T*)instance;
	//	m_vulkanData.m_pQueue = (VkQueue_T*)m_pSwapchain->m_presentQueue;//device->m_queueFamilyIndices.present;
	//	m_vulkanData.m_nQueueFamilyIndex = 0;

	//	m_vulkanData.m_nWidth = width;
	//	m_vulkanData.m_nHeight = height;
	//	m_vulkanData.m_nFormat = VK_FORMAT_R8G8B8A8_SRGB;
	//	m_vulkanData.m_nSampleCount = msaaSamples;
	//}

	void* Presenter::getPresentationEngine() {
		//if (false) {
		//	return (void*)m_pHmd;
		//}
		//else {
		//	return (void*)m_pSwapchain;
		//}
		return (void*)m_pSwapchain;
	}

	void Presenter::initSwapchainPresentation(VkInstance instance) {
		m_pSwapchain = new jsvk::VulkanSwapchain();
		m_pSwapchain->createSurface(instance, m_pWindow);
		m_usingSwapchain = true;
	}

	VkSurfaceKHR Presenter::getSurface() {
		return m_pSwapchain->m_pSurface;
	}

	void Presenter::connectToWindow(jsvk::VulkanDevice* device, int width, int height) {
		m_pSwapchain->connect(device, m_pWindow);
		m_pSwapchain->createSwapchain(width, height);
	}

	VkFormat Presenter::getImageFormat() {
		return m_pSwapchain->m_swapchainImageFormat;
	}

	VkExtent2D Presenter::getExtent() {
		return m_pSwapchain->m_extent;
	}

	std::vector<VkImageView> Presenter::getImageViews() {
		return m_pSwapchain->m_swapchainImageViews;
	}
}