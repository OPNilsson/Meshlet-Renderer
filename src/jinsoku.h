#pragma once
#ifndef HEADERGUARD_JINSOKU_H
#define HEADERGUARD_JINSOKU_H

#include "jsvk/jsvkContext.h"
#include "jsvk/geometryprocessing.h"
#include "jsvk/meshlet_builder.hpp"
#include "jsvk/structures.h"
#include "jsvk/config.h"
#include "jsvk/lodCamera.hpp"

#include "presenter.h"
#include "inputHandler.h"

#include "GLFW/glfw3.h"

// #include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>

int CURRENTPIPE = 0;
int HOTRELOAD = 0;
float ROTATION = 0;

inline lod::Camera camera;
inline int NO_TRIANGLES = 0;
inline int MESHLETS_DRAWN = 0;
inline int DRAW_CALLS = 0;

namespace jsk
{

	class Jinsoku
	{

	private:
		jsvk::Context *m_pVkContext;
		// jsvr::HmdInstance* m_pHMD;
		jsk::Presenter *m_pPresenter;
		// std::string m_modelPath = "../models/bruceyboy.obj";
		std::string m_modelPath;
		std::chrono::time_point<std::chrono::high_resolution_clock> lastTimeStamp;
		// Frame counter to display fps
		uint32_t frameCounter = 0;
		uint32_t lastFPS = 0;
		/** @brief Last frame time measured using a high performance timer (if available) */
		float frameTimer = 1.0f;
		std::vector<float> averageRenderTimes;
		std::vector<int> tri_counts;
		std::vector<int> meshlet_counts;
		std::vector<int> drawCall_counts;
		std::string windowTitle = "Jinsoku";

		float timer = 500.0f;

		struct
		{
			uint32_t width = camera.res.x;
			uint32_t height = camera.res.y;
			std::vector<std::string> instanceExtensions;
			std::vector<const char *> optionalDeviceExtensions;
			std::vector<const char *> requiredDeviceExtensions;
		} m_settings;

	public:
		void setModel(std::string modelPath)
		{
			m_modelPath = modelPath;
			std::cout << "With model : " << modelPath << "." << std::endl;
		}

		Jinsoku()
			: m_pVkContext(nullptr)
			  //, m_pHMD(nullptr)
			  ,
			  m_pPresenter(nullptr)
		{
			m_pPresenter = new Presenter();
		}

		void initWindow()
		{
			// Initialize glfw
			m_pPresenter->initGLFW(m_settings.width, m_settings.height);

			// set up key callbacks
			glfwSetKeyCallback(m_pPresenter->getWindow(), key_callback);
		}

		void createContext()
		{
			// vulkan setup
			m_pVkContext = new jsvk::Context();
		}

		void initVulkan()
		{

			// create instance
			m_pVkContext->createInstance();

			m_pVkContext->setupDebugMessenger();

			// adjust settings
			bool success = m_pVkContext->checkExtensionSupport(std::vector<const char *>{VK_KHR_SWAPCHAIN_EXTENSION_NAME}, false);

			// check for mesh shader support
			// integrate this into the resources and gpu selection process
			success = m_pVkContext->checkExtensionSupport(std::vector<const char *>{VK_NV_MESH_SHADER_EXTENSION_NAME, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME}, true);
			if (success)
			{
				m_pVkContext->m_useMeshShader = true;
				m_pVkContext->m_mode = "Mesh";
			}
			else
			{
				m_pVkContext->m_useMeshShader = false;
				m_pVkContext->m_mode = "Vertex";
			}

			// create swapchain
			m_pPresenter->initSwapchainPresentation(m_pVkContext->getInstance());

			// create device
			m_pVkContext->createDevice(m_pPresenter->getSurface());

			m_pPresenter->connectToWindow(m_pVkContext->m_pVulkanDevice, m_settings.width, m_settings.height);

			// init resource manager and memory and render context
			m_pVkContext->init(m_pPresenter, m_settings.width, m_settings.height);

			m_pVkContext->m_pResources->createRenderPass();

			// load mesh shaders
			if (success)
				m_pVkContext->loadMeshShaders();
		}

		// should be load scene
		void loadModel()
		{
			std::vector<std::string> temp{"../models/bruceyboy.obj", "../models/bunny.obj", "../models/bruceyboy.obj", "../models/bunny.obj"};
			int success = m_pVkContext->loadModel({temp});

			if (!success)
			{
				std::cout << "Failed to load model" << std::endl;
			}
		}

		~Jinsoku()
		{
			delete m_pPresenter;
			delete m_pVkContext;
		}

		void createRenderer()
		{
			m_pVkContext->m_pResources->createPipeline();
			m_pVkContext->m_pResources->createFramebuffer();
			m_pVkContext->createRenderer(m_pPresenter);
		}

		void mainLoop(std::string fileName, int RUNTIME = 0)
		{
#if BENCHMARK
			std::ofstream outFile("benchmarks/" + fileName + ".txt");
#endif

			lastTimeStamp = std::chrono::high_resolution_clock::now();

			while (!glfwWindowShouldClose(m_pPresenter->getWindow()))
			{
				glfwPollEvents();

				auto tStart = std::chrono::high_resolution_clock::now();

				m_pVkContext->draw();

				auto tEnd = std::chrono::high_resolution_clock::now();

				auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
				frameTimer = (float)tDiff / 1000.0f;

				lod::updateCamera(m_pPresenter->getWindow(), tDiff, camera);

				frameCounter++;

				float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - lastTimeStamp).count());
				if (fpsTimer > timer)
				{
					lastFPS = static_cast<uint32_t>((float)frameCounter * (timer / fpsTimer));

					std::string tempTitle = windowTitle + " - " + std::to_string(timer / lastFPS) + " ms/frame " + " - " + std::to_string(lastFPS) + " fps" + " - " + std::to_string(NO_TRIANGLES) + " Triangles" + " - " + std::to_string(MESHLETS_DRAWN) + " Meshlets Drawn";

					if ((camera.locked) || (camera.lockedLOD != -1))
					{
						tempTitle += " - LOD locked";
					}
					else
					{
						tempTitle += " - LOD unlocked";
					}

					tempTitle += " - " + std::to_string(DRAW_CALLS) + " Draw Calls";

					glfwSetWindowTitle(m_pPresenter->getWindow(), tempTitle.c_str());
					frameCounter = 0;
					lastTimeStamp = tEnd;
#if BENCHMARK
					averageRenderTimes.push_back(timer / lastFPS);

					tri_counts.push_back(NO_TRIANGLES);
					meshlet_counts.push_back(MESHLETS_DRAWN);
					drawCall_counts.push_back(DRAW_CALLS);

					float tolerance = 1.0f;
					if (glm::length(camera.position - camera.worldCenter) < tolerance)
						break;

					if (averageRenderTimes.size() >= RUNTIME && RUNTIME != 0)
					{
						break;
					}
#endif
				}
			}
#if BENCHMARK
			int i = 0;
			for (const auto &e : averageRenderTimes)
			{
				outFile << std::setw(25) << "Time: " << std::setw(20) << e
						<< std::setw(25) << "Triangles: " << std::setw(20) << tri_counts[i]
						<< std::setw(25) << "Meshlets: " << std::setw(20) << meshlet_counts[i]
						<< std::setw(25) << "DrawCalls: " << std::setw(20) << tri_counts[i]
						<< "\n";
				i++;
			}
#endif
			std::cout << std::endl;
			m_pVkContext->waitForIdle();
		}
	};

}

#endif // HEADERGUARD_JINSOKU_H