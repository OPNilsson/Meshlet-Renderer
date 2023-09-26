#include "jsvkRenderFrame.h"
#include "jsvkHelpers.h"




namespace jsvk {

	RenderFrame::RenderFrame()
	: m_imageAvailableSemaphore(VK_NULL_HANDLE)
	, m_renderFinishedSemaphore(VK_NULL_HANDLE)
	, m_inFlightFence(VK_NULL_HANDLE)
	, m_pVulkanDevice(nullptr)
	, m_commandPool(VK_NULL_HANDLE)
	, m_pSwapchain(VK_NULL_HANDLE)
	, m_mainCmdBuffer(VK_NULL_HANDLE)
	{

	}

	RenderFrame::~RenderFrame()
	{
		if(m_imageAvailableSemaphore != VK_NULL_HANDLE){
			vkDestroySemaphore(m_pVulkanDevice->Device(), m_imageAvailableSemaphore, nullptr);
			vkDestroySemaphore(m_pVulkanDevice->Device(), m_renderFinishedSemaphore, nullptr);
			vkDestroyFence(m_pVulkanDevice->Device(), m_inFlightFence, nullptr);
		}
			
		if(m_commandPool != VK_NULL_HANDLE){
			reset();
			vkResetCommandPool(m_pVulkanDevice->Device(), m_commandPool, 0);
			vkDestroyCommandPool(m_pVulkanDevice->Device(), m_commandPool, nullptr);
		}
	}

	void RenderFrame::init(jsvk::VulkanDevice* device, jsvk::VulkanSwapchain* swapchain, bool staticPool /*= false*/)
	{
		m_pSwapchain = swapchain;
		m_pVulkanDevice = device;

		createSyncObjects();
		if (staticPool) {
			createCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
			createPrimaryBuffer();
		}
		else {
			createCommandPool();
		}
	}


	void RenderFrame::createSyncObjects() {
		VkSemaphoreCreateInfo semaphoreInfo = jsvk::init::semaphoreCreateInfo();

		VkFenceCreateInfo fenceInfo = jsvk::init::fenceCreateinfo();
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VK_CHECK(vkCreateSemaphore(m_pVulkanDevice->Device(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore));
		VK_CHECK(vkCreateSemaphore(m_pVulkanDevice->Device(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphore));
		VK_CHECK(vkCreateFence(m_pVulkanDevice->Device(), &fenceInfo, nullptr, &m_inFlightFence));
	

	}
	VkResult RenderFrame::begin(uint32_t* imageIndex){
		VkResult res = VK_SUCCESS;


		return res;
	}

	VkResult RenderFrame::end(uint32_t* imageIndex){
		VkResult res = VK_SUCCESS;


		return res;
	}

	void RenderFrame::createPrimaryBuffer(bool primary /*= true*/) {
		VkCommandBufferAllocateInfo allocInfo = jsvk::init::commandBufferAllocateInfo();
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = primary == true ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandBufferCount = 1;

		VK_CHECK(vkAllocateCommandBuffers(m_pVulkanDevice->Device(), &allocInfo, &m_mainCmdBuffer));
	}

	VkCommandBuffer RenderFrame::getStaticBuffer(VkCommandBufferUsageFlags flags /*= 0 */) {
		VkCommandBufferBeginInfo beginInfo = jsvk::init::commandBufferBeginInfo();
		beginInfo.flags = flags; // Optional had to change to get rid of a warning I did not understand
		beginInfo.pInheritanceInfo = nullptr; // Optional

		VK_CHECK(vkBeginCommandBuffer(m_mainCmdBuffer, &beginInfo));

		return m_mainCmdBuffer;
	}

	void RenderFrame::createCommandPool(VkCommandPoolCreateFlags flags /*= 0*/) {
		
		//jsvk::QueueFamilyIndices queueFamilyIndices;
		//queueFamilyIndices = jsvk::findQueueFamilies(m_pVulkanDevice->m_pPhysicalDevice, m_pSwapchain->m_pSurface);
		
		VkCommandPoolCreateInfo poolInfo = jsvk::init::commandPoolCreateInfo();
		poolInfo.queueFamilyIndex = m_pVulkanDevice->m_queueFamilyIndices.graphics;//queueFamilyIndices.graphicsFamily.value();// 
		poolInfo.flags = flags;

		VK_CHECK(vkCreateCommandPool(m_pVulkanDevice->Device(), &poolInfo, nullptr, &m_commandPool));
		
	}

	VkCommandBuffer RenderFrame::createTempBuffer(bool primary /*= true*/) {

		VkCommandBufferAllocateInfo allocInfo = jsvk::init::commandBufferAllocateInfo();
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = primary == true ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer tmpBuffer;
		VK_CHECK(vkAllocateCommandBuffers(m_pVulkanDevice->Device(), &allocInfo, &tmpBuffer));

		VkCommandBufferBeginInfo beginInfo = jsvk::init::commandBufferBeginInfo();
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Optional had to change to get rid of a warning I did not understand
		beginInfo.pInheritanceInfo = nullptr; // Optional

		VK_CHECK(vkBeginCommandBuffer(tmpBuffer, &beginInfo));

		return tmpBuffer;
	}

	void RenderFrame::reset() {
		vkResetCommandPool(m_pVulkanDevice->Device(), m_commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	}
}