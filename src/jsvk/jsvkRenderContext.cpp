// Internal includes
#include "jsvkRenderContext.h"
#include "jsvkHelpers.h"
#include "lodCamera.hpp"
#include "lodGeometry.hpp"
#include "jsvkThreadpool.hpp"
#include "lodThreadStealers.cpp"

// STL includes
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stack>
#include <array>
#include <chrono>

// External includes
#include "omp.h"
// #include "tbb/concurrent_vector.h"
// #include "tbb/concurrent_queue.h"
// #include "tbb/parallel_invoke.h"

using namespace std::chrono_literals;

extern int MAX_LOD;

extern int HOTRELOAD;
extern bool INITIALIZED; // Have the simplified models been created?

extern int MESHLETS_DRAWN;
extern int NO_TRIANGLES;

extern int DRAW_CALLS;

extern lod::Camera camera;

extern lod::World world;

inline bool DistanceThresholds = true;

namespace jsvk
{

	VKRenderContext::VKRenderContext(jsk::Presenter *presenter, jsvk::VulkanDevice *pDevice)
		: m_pPresenter(nullptr), m_pSwapchain(nullptr), m_pVulkanDevice(nullptr), m_currentFrame(0)
	{
		m_pPresenter = presenter;
		m_pSwapchain = (jsvk::VulkanSwapchain *)presenter->getPresentationEngine();
		m_pVulkanDevice = pDevice;

		for (int i = 0; i < m_MAX_FRAMES_IN_FLIGHT + 1; ++i)
		{
			jsvk::RenderFrame *frame = new jsvk::RenderFrame();
			frame->init(m_pVulkanDevice, m_pSwapchain, true);
			m_renderFrames.push_back(frame);
		}
	}

	void VKRenderContext::deinit()
	{
		std::cout << "Destroying Render Context" << std::endl;
		// Destroy renderframes
		if (m_renderFrames.size() > 0)
		{
			for (auto frame : m_renderFrames)
			{
				delete frame;
			}
		}

		if (m_pRenderer != nullptr)
		{
			m_pRenderer->deinit();
			m_pRenderer = nullptr;
		}
	}

	// Traverse the DAG in order to draw
	lod::ThreadPool threadPool(std::thread::hardware_concurrency());
	std::unordered_set<uint16_t> visited;

	// std::vector<std::vector<lod::Graph::Node>> threads_drawing(10);

	std::vector<lod::Graph::Node *> drawn{};

	std::mutex mtx;
	std::mutex mtx_drawn;
	std::mutex mtx_visited;

	// Returns true if the AABB is in the frustum
	bool isAABBInFrustum(const lod::BoundingBox &aabb, const lod::Frustum &frustum)
	{
		std::array<lod::Plane, 6> planes = {frustum.left, frustum.right, frustum.top, frustum.bottom, frustum.near, frustum.far};

		for (const auto &plane : planes)
		{
			glm::vec3 positiveVertex = aabb.minPoint;
			glm::vec3 negativeVertex = aabb.maxPoint;

			if (plane.normal.x >= 0)
			{
				positiveVertex.x = aabb.maxPoint.x;
				negativeVertex.x = aabb.minPoint.x;
			}
			if (plane.normal.y >= 0)
			{
				positiveVertex.y = aabb.maxPoint.y;
				negativeVertex.y = aabb.minPoint.y;
			}
			if (plane.normal.z >= 0)
			{
				positiveVertex.z = aabb.maxPoint.z;
				negativeVertex.z = aabb.minPoint.z;
			}

			if (glm::dot(plane.normal, positiveVertex) + plane.distance < 0)
				return false;
		}

		// If we haven't found a separating plane where the entire AABB is outside, then the AABB is in the frustum
		return true;
	}

	// This is the standard BFS search algorithm it chooses to draw based on screen space error of switching to the next LoD
	void processNode_SSE(lod::Graph::Node *node, lod::Camera *camera)
	{
		if (!(isAABBInFrustum(node->bb, camera->frustum)))
		{
			return;
		}

		// The bottom of the tree has been reached and now all meshlets need to be drawn
		if (node->children.size() == 0)
		{
			mtx.lock();
			drawn.push_back(node);
			mtx.unlock();
			return;
		}

		// Process node here.
		bool checkChildren = false;
		bool draw = false;

		float d = glm::distance(camera->position, node->center);

		float nodeSize = glm::length(node->bb.maxPoint - node->bb.minPoint); // Diagonal length of the node
		float angle = atan(nodeSize / 2.0f / d);
		float pixelWidth = (angle / camera->fov) * camera->res.x;

		// Don't check nodes that are too small
		if (pixelWidth < camera->pixelThreshold)
		{
			return;
		}

		float lod_error = world.simplification_errors[node->lod];

		angle = atan(lod_error / d) * 180 / 3.141592653589793238463;

		float SSE = (angle / camera->fov) * (camera->res.x * camera->res.y);

		float threshold = camera->thresholds[node->lod];

		if (d < threshold)
		{
			checkChildren = true;
		}
		else
		{
			if (SSE < camera->SSEThreshold)
			{

				draw = true;
			}
			else
			{
				checkChildren = true;
			}
		}

		// Add child nodes to the next LoD traversal
		if (checkChildren)
		{

			for (auto &child : node->children)
			{
				mtx.lock();
				if (visited.count(child.second->id) == 0)
				{
					visited.insert(child.second->id);
					threadPool.enqueue([=, &child]()
									   { processNode_SSE(child.second, camera); });
				}
				mtx.unlock();
			}
		}

		if (draw)
		{
			mtx.lock();
			drawn.push_back(node);
			mtx.unlock();
		}
	}

	// This is the standard BFS search algorithm it chooses to draw based on the distance to the camera
	void processNode_distance(lod::Graph::Node *node, lod::Camera *camera)
	{
		if (!(isAABBInFrustum(node->bb, camera->frustum)))
		{
			return;
		}

		// The bottom of the tree has been reached and now all meshlets need to be drawn
		if (node->lod == 0)
		{
			mtx.lock();
			drawn.push_back(node);
			mtx.unlock();
			return;
		}

		// Process node here.
		bool checkChildren = false;
		bool draw = false;

		float d = glm::distance(camera->position, node->center);
		float threshold = camera->thresholds[node->lod];

		if (d < threshold)
		{
			checkChildren = true;
		}
		else
		{
			draw = true;
		}

		// Add child nodes to the next LoD traversal
		if (checkChildren)
		{

			for (auto &child : node->children)
			{
				mtx.lock();
				if (visited.count(child.second->id) == 0)
				{
					visited.insert(child.second->id);
					threadPool.enqueue([=, &child]()
									   { processNode_distance(child.second, camera); });
				}
				mtx.unlock();
			}
		}

		if (draw)
		{
			mtx.lock();
			drawn.push_back(node);

			// The dream would be this done here
			// camera.offset = node->lod; // The start of the LoD indices and vertices
			// camera.end = 1;			   // How many meshlets to draw

			// GameObject &object = world.gameObjects[rp];

			// GameObject &objectOffset = resources->scene.gameObjects[node->meshIndex]; // This works for discrete but not for CLoD so we need to send it it as an offset object

			// m_pRenderer->drawDynamic(&object, imageIndex, &primaryBuffer, inheritInfo, &objectOffset, node->meshIndex);
			// secondaryBuffers.push_back((object.m_commandBuffers[imageIndex]));

			// DRAW_CALLS++;

			// rp++;

			// We cannot draw more meshlets than there are allocated command buffer pools
			// if (rp >= world.gameObjects.size())
			// {
			// 	break;
			// }

			mtx.unlock();
		}
	}

	std::atomic<bool> lock{false};
	// TODO: make synchronization better as this is definitely not production ready
	auto sleep = 0.005ns;

	void synchTs(std::vector<std::thread> &threads)
	{
		// wait for threads to finish
		for (auto &t : threads)
		{
			t.join();
		}

		threads.clear();
	}

	// This is for the DFS search algorithm but it performs worse as the overhead of closing the threads and waiting for thread unlocks piles on
	// void processNode(lod::Graph::Node *node, int desiredLOD, std::queue<lod::Graph::Node> &nextLevel)
	// {
	// 	std::queue<lod::Graph::Node*> nextLevel_private;

	// 	// Process node here.
	// 	bool checkChildren = false;
	// 	bool draw = false;

	// 	// The bottom of the tree has been reached and now all meshlets need to be drawn
	// 	// if ((node.lod == 0) || ((desiredLOD >= MAX_LOD) && (node.lod == MAX_LOD)))
	// 	if ((node.lod == 0))
	// 	{
	// 		draw = true;
	// 	}
	// 	else
	// 	{
	// 		float d = glm::distance(node.center, camera.position);

	// 		float threshold = camera.thresholds[node.lod];

	// 		if (d < threshold)
	// 		{
	// 			checkChildren = true;
	// 		}
	// 		else
	// 		{
	// 			draw = true;
	// 		}
	// 	}

	// 	// Add child nodes to the next LoD traversal
	// 	if (checkChildren)
	// 	{

	// 		for (auto &child : node.children)
	// 		{

	// 			if (visited.count(child.second->id) == 0)
	// 			{
	// 				mtx.lock();
	// 				nextLevel_private.push(child.second);
	// 				visited.insert(child.second.id);
	// 				mtx.unlock();
	// 			}
	// 		}
	// 	}

	// 	if (draw)
	// 	{
	// 		// Make sure the threads are not altering the same data at the same time
	// 		mtx.lock();
	// 		drawn.push_back(node);
	// 		mtx.unlock();
	// 	}

	// 	// This is a race condition so make sure to wait before
	// 	while (!nextLevel_private.empty())
	// 	{
	// 		mtx.lock();
	// 		nextLevel.push(nextLevel_private.front());
	// 		nextLevel_private.pop();
	// 		mtx.unlock();
	// 	}
	// }

	void VKRenderContext::Render()
	{

		/*
		if (FPSCOUNT) {
			// Measure speed
			double currentTime = glfwGetTime();
			nbFrames++;
			if (currentTime - lastTime >= 1.0) { // If last prinf() was more than 1 sec ago
				// printf and reset timer
				fps.push_back(1000.0 / double(nbFrames));
				//printf("%f ms/frame\n", 1000.0 / double(nbFrames));
				nbFrames = 0;
				lastTime += 1.0;
			}
		}
		*/
		jsvk::Resources *resources = m_pRenderer->getResources();

		if (HOTRELOAD)
		{
			HOTRELOAD = 0;
			std::cout << "reloaded pipelines" << std::endl;
			// big no no happening here
			vkDeviceWaitIdle(m_pVulkanDevice->Device());
			resources->hotReloadPipeline();
			m_pRenderer->deinit();
			m_pRenderer->init(m_pVulkanDevice, resources);
		}

		uint32_t imageIndex;
		acquireNextImage(&imageIndex);

		// VK_CHECK(acquireNextImage(&imageIndex));

		// VkCommandBuffer primaryBuffer = getTempBuffer();
		m_pRenderer->updateUniforms();

		VkCommandBuffer primaryBuffer = getPrimaryBuffer();

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = {1.0f, 1.0f, 1.0f, 1.0f};
		clearValues[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo renderPassInfo = jsvk::init::renderPassBeginInfo();
		renderPassInfo.renderPass = resources->m_renderPasses[0].renderPass;
		renderPassInfo.framebuffer = resources->m_swapchainFramebuffers[imageIndex];
		renderPassInfo.renderArea.extent.width = resources->m_renderPasses[0].width;
		renderPassInfo.renderArea.extent.height = resources->m_renderPasses[0].height;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(primaryBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		VkCommandBufferInheritanceInfo inheritInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};

		inheritInfo.renderPass = resources->m_renderPasses[0].renderPass;
		inheritInfo.framebuffer = resources->m_swapchainFramebuffers[imageIndex]; // m_pResources->m_swapchainFramebuffers[k];

		int desiredLOD = camera.lockedLOD;

		std::vector<VkCommandBuffer> secondaryBuffers;

		if (camera.lastPrintTime.time_since_epoch() > 500ms)
		{
			float distance = glm::distance(camera.position, world.center);

			// LOD Check
			desiredLOD = camera.thresholds.size() - 1;
			for (int lod = 0; lod < camera.thresholds.size(); lod++)
			{
				if (distance <= camera.thresholds[lod])
				{
					desiredLOD = lod;

					break;
				}
			}

			if (desiredLOD > MAX_LOD)
			{
				desiredLOD = MAX_LOD;
			}
			else if (desiredLOD < 0)
			{
				desiredLOD = 0;
			}

			printf("SPE: %s \t distance: %f \t lod %i \r", !DistanceThresholds ? "true" : "fals", distance, desiredLOD);
			camera.lastPrintTime = std::chrono::steady_clock::now();

			desiredLOD = camera.lockedLOD;
		}

		bool traversed = false;

		DRAW_CALLS = 0;
		MESHLETS_DRAWN = 0;
		NO_TRIANGLES = 0;

		// LoD is unlocked when lockedLOD is -1
		if (camera.lockedLOD == -1)
		{

			if (camera.discrete)
			{
				goto discrete;
			}

			if (((camera.velocity != glm::vec3(0.0f)) && (!camera.locked)) || (drawn.size() <= 0))
			{
				// Get a delay based on how fast the camera is moving so that the LOD doesn't change too quickly
				float delay = glm::length(camera.velocity) * 1.0f;

				if (camera.lastUpdateTime.time_since_epoch() > std::chrono::milliseconds((int)(delay * 1000.0f)))
				{
					mtx.lock();
					drawn = {};
					visited = {};
					mtx.unlock();

					// This is the optimal number of threads that should be competing for jobs
					threadPool.restart(12);

					// int startingLoD = (desiredLOD >= MAX_LOD - 1) ? MAX_LOD : desiredLOD + 1;

					// The root(s) of the DAG is(are) the MAX_LOD meshlets
					for (int i = 0; i < world.DAG.nodes[MAX_LOD].size(); i++)
					{
						lod::Graph::Node *root = &world.DAG.nodes[MAX_LOD][i];

						mtx.lock();
						visited.insert(root->id);
						mtx.unlock();

						if (DistanceThresholds)
						{
							threadPool.enqueue([=]()
											   { processNode_distance(root, &camera); });
						}
						else
						{
							threadPool.enqueue([=]()
											   { processNode_SSE(root, &camera); });
						}
					}

					threadPool.stopAndJoin();

					// Other possible concurrency Depth/Breadth First search
					// std::vector<std::thread> threads;
					// std::queue<lod::Graph::Node> currentLevel;
					// // This is the entire DAG
					// while (!currentLevel.empty())
					// {
					// 	std::queue<lod::Graph::Node> nextLevel;

					// 	// This is the local breadth of the DAG
					// 	while (!currentLevel.empty())
					// 	{
					// 		lod::Graph::Node node = currentLevel.front();
					// 		currentLevel.pop();

					// 		// processNode(node, desiredLOD, nextLevel);
					// 		// std::thread th = std::thread([node, desiredLOD, &nextLevel]()
					// 		// 							 { processNode(node, desiredLOD, nextLevel); });
					// 		// threads.push_back(std::move(th));
					// 	}

					// 	synchTs(threads);
					// 	currentLevel = nextLevel;
					// }

					camera.lastUpdateTime = std::chrono::steady_clock::now();
				}
			}

			int rp = 0; // This is to pick a render pool
			// Sort the drawn vector based on increasing meshletIndex so that more meshlet are packed into a single draw call?
			// std::sort(drawn.begin(), drawn.end(), [](lod::Graph::Node *a, lod::Graph::Node *b)
			// 		  { return a->meshletIndex < b->meshletIndex; });

			std::vector<std::unordered_map<int, std::queue<lod::Graph::Node *>>> mesh_node_heirarchy(world.mesh_paths.size()); // Maps the nodes that have been selected to be drawn into their respective LoD and render pool

			// loop through the selected meshlets to check which are part of the same LoD mesh so that they can be coalesced into one draw call
			int test = 0;
			int timesChanged = 0;
			for (auto &node : drawn)
			{
				mesh_node_heirarchy[node->meshIndex][node->lod].emplace(node);

				if (test != node->lod)
				{
					test = node->lod;
					timesChanged++;
				}
			}

			if (timesChanged == 1)
			{
				desiredLOD = test;
				traversed = true;
				goto discrete;
			}

			for (int i = 0; i < world.mesh_paths.size(); i++)
			{
				std::unordered_map<int, std::queue<lod::Graph::Node *>> &node_heirarchy = mesh_node_heirarchy[i]; // Maps the nodes that have been selected to be drawn into their respective LoD

				for (auto &[lod, nodes] : node_heirarchy)
				{
					int noMeshlets = 0;			   // This is the number of meshlets that will be drawn in a single draw call
					int previousMeshletIndex = -1; // This is to check if the meshlets are sequential
					bool drawCallFinalized = false;

					// Draw sequential meshlets in a single draw call
					do
					{
						if (previousMeshletIndex == -1)
						{
							previousMeshletIndex = nodes.front()->meshletIndex;
							camera.start = nodes.front()->meshletIndex; // Where the first meshlet in the draw call starts
							NO_TRIANGLES += nodes.front()->no_triangles;
							noMeshlets++;
							nodes.pop();
						}
						else if (previousMeshletIndex != (nodes.front()->meshletIndex - 1))
						{
							drawCallFinalized = true;
						}
						else if (previousMeshletIndex == nodes.front()->meshletIndex)
						{
							// This is a duplicate meshlet so just skip it
							nodes.pop();
						}
						else
						{
							// This is a sequential meshlet so just add it to the draw call
							previousMeshletIndex = nodes.front()->meshletIndex;
							noMeshlets++;
							NO_TRIANGLES += nodes.front()->no_triangles;
							nodes.pop();
						}

						if ((drawCallFinalized) || (nodes.empty()))
						{
							// This is a cheat so that the DAG doesn't need to be traversed for multiple copies of the same object, but is not the correct way of doing it.
							// for (int i = 0; i < world.mesh_paths.size(); i++)
							{
								camera.offset = lod;	 // The start of the LoD indices and vertices
								camera.end = noMeshlets; // How many meshlets to draw

								if (rp < world.gameObjects.size())
								{
									GameObject &object = world.gameObjects[rp];

									GameObject &objectOffset = resources->scene.gameObjects[i]; // This works for discrete but not for CLoD so we need to send it it as an offset object

									m_pRenderer->drawDynamic(&object, imageIndex, &primaryBuffer, inheritInfo, &objectOffset, i);
									secondaryBuffers.push_back((object.m_commandBuffers[imageIndex]));

									DRAW_CALLS++;

									rp += noMeshlets;

									// We cannot draw more meshlets than there are allocated command buffer pools
									if (rp >= world.gameObjects.size())
									{
										break;
									}
								}
							}

							noMeshlets = 0;
							drawCallFinalized = false;
							previousMeshletIndex = -1;
						}

						// We cannot draw more meshlets than there are allocated command buffer pools
						if (rp >= world.gameObjects.size())
						{
							break;
						}

					} while (!nodes.empty());
				}
			}

			MESHLETS_DRAWN = rp;
		}
		else
		{
			// We are using locked LoD which is just a discrete LoD so
			// we can just draw the meshlets that are at the desired LoD all in one command buffer
			// Discrete LoD does not blend LoDs into one model!
		discrete:

			for (int i = 0; i < world.mesh_paths.size(); i++)
			{
				GameObject &object = resources->scene.gameObjects[i];

				if ((camera.lockedLOD == -1) && (!traversed))
				{
					// Discrete LoD
					float distance = 0.0f;
					distance = glm::distance(camera.position, world.mesh_centers[i]);

					// LOD Check
					desiredLOD = camera.thresholds.size() - 1;
					for (int lod = 0; lod < camera.thresholds.size(); lod++)
					{
						if (distance <= camera.thresholds[lod])
						{
							desiredLOD = lod;

							if (camera.lod != desiredLOD)
							{
								camera.lod = desiredLOD;
							}

							break;
						}
					}
				}

				if (desiredLOD > MAX_LOD)
				{
					desiredLOD = MAX_LOD;
				}
				else if (desiredLOD < 0)
				{
					desiredLOD = 0;
				}

				int offset = i * (MAX_LOD + 1);
				int noMeshlets = world.model.desc_counts[desiredLOD + offset];

				camera.start = -1;			// Where to start drawing
				camera.offset = desiredLOD; // What offset to use in the model vertices
				camera.end = noMeshlets;	// How many meshlets to draw

				m_pRenderer->drawDynamic(&object, imageIndex, &primaryBuffer, inheritInfo, &object, i);
				secondaryBuffers.push_back((object.m_commandBuffers[imageIndex]));

				NO_TRIANGLES += world.model.no_triangles[desiredLOD + offset];
				MESHLETS_DRAWN += noMeshlets;
				DRAW_CALLS++;
			}
		}

		if (MESHLETS_DRAWN != 0)
		{
			vkCmdExecuteCommands(primaryBuffer, secondaryBuffers.size(), secondaryBuffers.data());
			vkCmdEndRenderPass(primaryBuffer);
		}
		else
		{
			vkCmdEndRenderPass(primaryBuffer);
		}

		vkEndCommandBuffer(primaryBuffer);
		presentImage(primaryBuffer, &imageIndex);
		// VK_CHECK(vkEndCommandBuffer(primaryBuffer));
		// VK_CHECK(presentImage(primaryBuffer, &imageIndex));
	}

	VkResult VKRenderContext::acquireNextImage(uint32_t *imageIndex)
	{
		VkResult res = VK_SUCCESS;

		jsvk::RenderFrame *frame = m_renderFrames[m_currentFrame];

		res = m_pSwapchain->acquireNextImage(frame->m_imageAvailableSemaphore, imageIndex);

		vkWaitForFences(m_pVulkanDevice->Device(), 1, &frame->m_inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_pVulkanDevice->Device(), 1, &frame->m_inFlightFence);

		// frame->reset();

		// reset cmdpool

		return res;
	}

	VkResult VKRenderContext::presentImage(VkCommandBuffer primaryBuffer, uint32_t *imageIndex)
	{
		VkResult res = VK_SUCCESS;

		jsvk::RenderFrame *frame = m_renderFrames[m_currentFrame];

		VkSubmitInfo submitInfo = jsvk::init::submitInfo();

		VkSemaphore waitSemaphores[] = {frame->m_imageAvailableSemaphore};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &primaryBuffer;

		VkSemaphore signalSemaphores[] = {frame->m_renderFinishedSemaphore};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VK_CHECK(vkQueueSubmit(m_pSwapchain->m_graphicsQueue, 1, &submitInfo, frame->m_inFlightFence));

		res = m_pSwapchain->queuePresent(m_pSwapchain->m_presentQueue, *imageIndex, frame->m_renderFinishedSemaphore);

		m_currentFrame = (m_currentFrame + 1) % m_MAX_FRAMES_IN_FLIGHT;

		return res;
	}

	VkCommandBuffer VKRenderContext::getTempBuffer()
	{
		return m_renderFrames[m_currentFrame]->createTempBuffer();
	}

	VkCommandBuffer VKRenderContext::getPrimaryBuffer()
	{
		return m_renderFrames[m_currentFrame]->getStaticBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	}
}