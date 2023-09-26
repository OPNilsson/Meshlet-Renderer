// Internal includes
#include "jsvkResources.h"
#include "lodGeometry.hpp"
#include "lodCamera.hpp"

// std library includes
#include <array>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>

// external includes
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

bool SIMPLIFIED = false;

extern bool SHOW_MESSAGES;

extern bool INITIALIZED; // Have the simplified models been created?

extern int MAX_LOD;

extern lod::Camera camera;

inline lod::World world;

extern int NO_TRIANGLES;

using namespace std::chrono_literals;

namespace jsvk
{

	ResourcesMS::ResourcesMS()
		: m_pVulkanDevice(nullptr), m_pPresenter(nullptr)
		  //, m_swapchainFramebuffers()
		  ,
		  m_msaaSamples(VK_SAMPLE_COUNT_1_BIT), m_geos(0), m_pMemManager(nullptr), m_uniformBuffer(), m_dynamicAlignment(0), DynamicObjectDataPointer(nullptr), m_cullStats(nullptr), m_layouts{VK_NULL_HANDLE}, sceneSets(), objSets(), imgSets(), geoSets(), textures(), useTask(true), m_descriptorPools(), m_mainBuffer()

	{
	}

	void ResourcesMS::init(jsvk::VulkanDevice *pVulkanDevice, jsk::Presenter *presenter, VkSampleCountFlagBits msaaSamples, int width, int height)
	{
		m_pVulkanDevice = pVulkanDevice;
		m_renderPasses[0].width = width;
		m_renderPasses[0].height = height;
		m_msaaSamples = msaaSamples;
		m_pPresenter = presenter;
		m_pMemManager = new jsvk::MemoryManager(pVulkanDevice);
		m_extent.width = width;
		m_extent.height = height;
	}

	// used to instantiate the class so the Registry can find it
	static ResourcesMS::TypeCmd s_type_resources_ms;

	void ResourcesMS::deinit()
	{
		std::cout << "Destroying Resources" << std::endl;
		// here we free a lot of shit
		for (GameObject &obj : scene.gameObjects)
		{
			if (obj.m_commandBuffers.size() > 0)
			{
				vkFreeCommandBuffers(m_pVulkanDevice->Device(), obj.m_commandPool, static_cast<uint32_t>(obj.m_commandBuffers.size()), obj.m_commandBuffers.data());
			}
			if (obj.m_commandPool != VK_NULL_HANDLE)
			{
				vkDestroyCommandPool(m_pVulkanDevice->Device(), obj.m_commandPool, nullptr);
			}
		}

		// free descriptorpool
		if (m_descriptorPools.size() > 0)
			for (auto descriptorPool : m_descriptorPools)
			{
				vkDestroyDescriptorPool(m_pVulkanDevice->Device(), descriptorPool, nullptr);
			}

		// free renderpasses
		if (m_renderPasses.size() > 0)
		{
			for (auto m_renderPass : m_renderPasses)
			{
				// free imageviews
				if (m_renderPass.color.view != VK_NULL_HANDLE)
				{
					vkDestroyImageView(m_pVulkanDevice->Device(), m_renderPass.color.view, nullptr);
				}
				if (m_renderPass.depth.view != VK_NULL_HANDLE)
				{
					vkDestroyImageView(m_pVulkanDevice->Device(), m_renderPass.depth.view, nullptr);
				}

				// free images
				if (m_renderPass.color.image != VK_NULL_HANDLE)
				{
					vkDestroyImage(m_pVulkanDevice->Device(), m_renderPass.color.image, nullptr);
				}
				if (m_renderPass.depth.image != VK_NULL_HANDLE)
				{
					vkDestroyImage(m_pVulkanDevice->Device(), m_renderPass.depth.image, nullptr);
				}
				// free image memory
				if (m_renderPass.color.memory != VK_NULL_HANDLE)
				{
					vkFreeMemory(m_pVulkanDevice->Device(), m_renderPass.color.memory, nullptr);
				}
				if (m_renderPass.depth.memory != VK_NULL_HANDLE)
				{
					vkFreeMemory(m_pVulkanDevice->Device(), m_renderPass.depth.memory, nullptr);
				}

				if (m_renderPass.renderPass != VK_NULL_HANDLE)
				{
					// Destroy renderPass
					vkDestroyRenderPass(m_pVulkanDevice->Device(), m_renderPass.renderPass, nullptr);
				}
			}
			// maybe just do this and add a destructor to the renderpass class
			m_renderPasses.clear();
		}

		// Destroy framebuffers
		if (m_swapchainFramebuffers.size() > 0)
		{
			for (auto framebuffer : m_swapchainFramebuffers)
			{
				vkDestroyFramebuffer(m_pVulkanDevice->Device(), framebuffer, nullptr);
			}
		}

		// free descriptorpool

		// free pipeline
		if (m_meshShaderPipelines.size() > 0)
		{
			for (auto pipeline : m_meshShaderPipelines)
			{
				vkDestroyPipeline(m_pVulkanDevice->Device(), pipeline, nullptr);
			}
		}

		// free pipelinelayout
		if (m_meshShaderPipelineLayouts.size() > 0)
		{
			for (auto pipelineLayout : m_meshShaderPipelineLayouts)
			{
				vkDestroyPipelineLayout(m_pVulkanDevice->Device(), pipelineLayout, nullptr);
			}
		}

		// free descriptorlayouts
		// TODO make layouts a vector for easy handeling
		for (int i = 0; i < 4; ++i)
		{
			vkDestroyDescriptorSetLayout(m_pVulkanDevice->Device(), m_layouts[i], nullptr);
		}

		// lastly memorymanager is deleted
		// temp mem deletion which needs to be moved inside memManager
		if (m_uniformBuffer.m_pMemory != VK_NULL_HANDLE)
		{
			m_uniformBuffer.destroy();
		}
		if (m_mainBuffer.m_pMemory != VK_NULL_HANDLE)
		{
			m_mainBuffer.destroy();
		}
		delete m_pMemManager;
	}

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

	double completion = 0.0;

	void printProgress(double percentage)
	{
		int val = (int)(percentage * 100);
		int lpad = (int)(percentage * PBWIDTH);
		int rpad = PBWIDTH - lpad;
		printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
		fflush(stdout);
	}

	std::atomic<bool> threadLock{false};
	// TODO: make synchronization better as this is definitely not production ready
	auto threadSleep = 1ms;

	void synchThreads(std::vector<std::thread> &threads, int progressTotal)
	{
		// wait for threads to finish
		for (auto &t : threads)
		{
			t.join();

			if (progressTotal != 0)
			{
				completion += 1.00f / progressTotal;
				printProgress(completion);
			}
		}

		threads.clear();
	}

	void simplification_task(int level, float reducePercentage, float maxError, float edgeThresh, lod::Mesh &originalMesh, std::string name)
	{
		assert(level >= 0);

		lod::VertexBufferBuilder simplifiedModel;

		simplifiedModel.replace(createSimplifiedModel(originalMesh.builder, reducePercentage, edgeThresh, maxError));
		originalMesh.builder.replace(simplifiedModel);

		// Update the vertices and indices lists
		glm::vec3 sum(0.0f, 0.0f, 0.0f);
		std::vector<mm::Vertex> vertices;
		for (auto vert : simplifiedModel.vertices)
		{
			mm::Vertex v{};
			v.pos = vert.pos;
			v.color = vert.color;
			v.texCoord = vert.texCoord;

			sum += vert.pos;

			vertices.push_back(v);
		}

		lod::Mesh mesh;
		mesh.file_path = originalMesh.file_path;
		mesh.lod = level;
		mesh.no_triangles = vertices.size() / 3;
		mesh.builder.replace(simplifiedModel);
		mesh.vertices = vertices;
		mesh.indices = simplifiedModel.indices;
		mesh.center = sum / static_cast<float>(vertices.size());
		mesh.name = name + "_lod_" + std::to_string(level);
		mesh.simplificationError = simplifiedModel.simplificationError;

		// Make sure the threads are not altering the same data at the same time
		while (threadLock)
		{
			std::this_thread::sleep_for(threadSleep);
		}

		// threadLock = true;

		world.meshes.push_back(mesh);

		// threadLock = false;
	}

	void serialization_task(lod::Mesh &mesh)
	{
		// Make sure the threads are not altering the same data at the same time
		while (threadLock)
		{
			std::this_thread::sleep_for(threadSleep);
		}

		threadLock = true;

		std::string path = "../models/binaries/" + mesh.name;

		// Serialize the model
		mesh.builder.serialize(path);

		// completion += 1.00f / world.meshes.size();

		// printProgress(completion);

		threadLock = false;
	}

	void deserialization_task(int level, std::string modelPath)
	{
		// Load the model
		lod::Mesh mesh;
		mesh.lod = level;
		mesh.file_path = modelPath;

		std::string mesh_name = modelPath.substr(modelPath.find_last_of("/") + 1);
		mesh_name = mesh_name.substr(0, mesh_name.find_last_of("."));
		mesh.name = mesh_name + "_lod_" + std::to_string(level);

		std::string path = "../models/binaries/" + mesh.name;

		try
		{
			mesh.builder.deserialize(path);
		}
		catch (const std::exception &e)
		{
			world.errors.push_back("Could NOT Deserialize model: " + mesh.name);
			return;
		}

		// Make sure the threads are not altering the same data at the same time
		while (threadLock)
		{
			std::this_thread::sleep_for(threadSleep);
		}

		std::vector<mm::Vertex> vertices;
		std::vector<uint32_t> indices_model;
		glm::vec3 sum(0.0f, 0.0f, 0.0f);
		for (auto i : mesh.builder.vertices)
		{
			mm::Vertex v{};

			v.pos = i.pos;
			v.color = i.color;
			v.texCoord = i.texCoord;

			sum += v.pos;

			vertices.push_back(v);
		}
		mesh.center = sum / static_cast<float>(vertices.size());

		mesh.vertices = vertices;
		mesh.indices = mesh.builder.indices;
		mesh.no_triangles = mesh.indices.size() / 3;

		threadLock = true;

		world.meshes.push_back(mesh);

		threadLock = false;
	}

	void createMeshlets_task(lod::Mesh &mesh)
	{
		std::vector<mm::Vertex> vertices;
		std::vector<uint32_t> indices_model;

		vertices = mesh.vertices;
		indices_model = mesh.indices;

		std::unordered_map<unsigned int, mm::Vert *> indexVertexMap;
		std::vector<mm::MeshletCache<uint32_t>> meshlets;
		std::vector<mm::Triangle *> triangles;
		std::vector<NVMeshlet::Stats> stats;
		std::vector<ObjectData> objectData;

		// Transform to meshlet
		mm::makeMesh(&indexVertexMap, &triangles, indices_model.size(), indices_model.data());
		mm::generateMeshlets(indexVertexMap, triangles, meshlets, vertices.data(), 1);
		NVMeshlet::Builder<uint32_t>::MeshletGeometry packedMeshlets = mm::packNVMeshlets(meshlets);

		mm::generateEarlyCulling(packedMeshlets, vertices, objectData);
		mm::collectStats(packedMeshlets, stats);

		mesh.indexVertexMap = indexVertexMap;
		mesh.meshletCache = meshlets;
		mesh.triangles = triangles;
		mesh.stats = stats;
		mesh.objectData = objectData;
		mesh.packedMeshlets = packedMeshlets;
		mesh.no_triangles = triangles.size();
	}

	void createMeshlets_subTask(lod::Mesh &mesh, int meshletIndex)
	{
		lod::Meshlet meshlet;
		meshlet.no_triangles = mesh.meshletCache[meshletIndex].numPrims;
		meshlet.lod = mesh.lod;
		meshlet.index = meshletIndex;
		// meshlet.indices = mesh.meshletCache[meshletIndex];

		// From the meshlet vertices get we can then re-generate the AABB for each meshlet
		glm::vec3 minPoint = glm::vec3(std::numeric_limits<float>::infinity());
		glm::vec3 maxPoint = glm::vec3(-std::numeric_limits<float>::infinity());

		glm::vec3 sum(0.0f, 0.0f, 0.0f); // Use the geometric center instead?

		std::vector<mm::Vertex> vertices;
		for (auto i : mesh.meshletCache[meshletIndex].actualVertices)
		{
			if (i.pos == glm::vec3(0.0))
			{
				break;
			}

			mm::Vertex v{};

			v.pos = i.pos;
			v.color = i.color;
			v.texCoord = i.texCoord;

			minPoint = glm::min(minPoint, v.pos);
			maxPoint = glm::max(maxPoint, v.pos);

			sum += i.pos;

			vertices.push_back(v);
		}

		meshlet.minPoint = minPoint;
		meshlet.maxPoint = maxPoint;

		meshlet.center = sum / static_cast<float>(vertices.size());
		// meshlet.center = 0.5f * (minPoint + maxPoint);
		// meshlet.center = vertices[vertices.size() / 2].pos;

		meshlet.vertices = vertices;

		mesh.meshlets[meshletIndex] = meshlet;
	}

	void
	modelBuilding_task(lod::Mesh &mesh, std::vector<NVMeshlet::Builder<uint32_t>::MeshletGeometry> &meshletGeometry32, std::vector<NVMeshlet::Builder<uint16_t>::MeshletGeometry> &meshletGeometry, std::vector<NVMeshlet::Stats> &stats, std::vector<uint32_t> &vertCount, std::vector<mm::Vertex> &vertices, std::vector<ObjectData> &objectData, std::vector<uint32_t> &indices_model, int modelIndex = 0)
	{
		std::vector<uint32_t> idxList_model;
		std::vector<mm::Vertex> verts_model;

		std::vector<mm::MeshletCache<uint32_t>> meshlets;
		std::unordered_map<unsigned int, mm::Vert *> indexVertexMap;
		std::vector<mm::Triangle *> triangles;

		NVMeshlet::Builder<uint32_t>::MeshletGeometry packedMeshlets;

		verts_model = mesh.vertices;
		idxList_model = mesh.indices;

		indexVertexMap = mesh.indexVertexMap;
		meshlets = mesh.meshletCache;
		triangles = mesh.triangles;
		packedMeshlets = mesh.packedMeshlets;

		world.model.no_triangles.push_back(mesh.no_triangles);

		vertices.insert(vertices.end(), verts_model.begin(), verts_model.end());
		indices_model.insert(indices_model.end(), idxList_model.begin(), idxList_model.end());

		// If each of the models sent in already have their own generated things
		stats.push_back(mesh.stats.front());
		objectData.push_back(mesh.objectData.front());
		vertCount.push_back(verts_model.size());

		meshletGeometry32.push_back(packedMeshlets);

		mesh.vertices.clear();
		mesh.indices.clear();
		mesh.indexVertexMap.clear();
		mesh.meshletCache.clear();
		mesh.triangles.clear();
		mesh.vertices.clear();
		mesh.indices.clear();
	}
	void createWorld(std::vector<std::string> file_paths)
	{
		file_paths.clear();

		MAX_LOD = 10; // How many times to simplify the models (so if MAX_LOD = 10 there will be 11 meshes loaded per model)

		bool calcMeshlets = true;  // if false only DLoD will be available
		bool copyMeshlets = false; // if true any of the same models will make the same LoD decisions

		for (int i = 0; i < 1; i++)
		{
			// TODO: Optimize the pre-processing since there are multiple duplications occurring during it
			// For example, the simplification stores vertices in the Manifold structure, then the mesh builder stores them, then they are stored again in the world, etc.
			// Which is terrible i know...

			// These are possible but require a TON of memory for the pre-processing
			// file_paths.push_back("../models/thai.obj");
			// file_paths.push_back("../models/dragon.obj");
			// file_paths.push_back("../models/Nefertiti.obj");
			file_paths.push_back("../models/happy.obj");

			// This works when MAX_LOD <= 3
			// file_paths.push_back("../models/armadillo.obj");

			// These are usually safe to use regardless of how many instances
			// file_paths.push_back("../models/bunny.obj");
			// file_paths.push_back("../models/kitten.obj");
			// file_paths.push_back("../models/bruce.obj");
			// file_paths.push_back("../models/teapot.obj");
		}

		for (int i = 0; i < 50; i++)
		{
			// file_paths.push_back("../models/kitten.obj");
		}

		// If a MAX_LOD that is higher than the predetermined thresholds then just add some more
		if (MAX_LOD > camera.thresholds.size())
		{
			for (int i = camera.thresholds.size(); i <= MAX_LOD; i++)
			{
				if (i == camera.thresholds.size())
				{
					camera.thresholds.resize(MAX_LOD + 1);
				}

				camera.thresholds[i] = camera.thresholds[i - 1] * 0.75;
			}
		}

		// If using distance thresholds scale up the thresholds for larger models
		for (auto &threshold : camera.thresholds)
		{
			// threshold *= 35.5f;
			// threshold *= 0.5f;
			threshold *= 15.5f; // For Armadillo and above models
								// threshold *= 35.5f;
		}

		// Multiple models currently only works for using the same exact mesh and LoD meshes

		assert(file_paths.size() > 0); // No point in initializing an empty world

		world.mesh_paths = file_paths;

		SHOW_MESSAGES = false; // Turn off update messages of the builder

		std::cout << "-----------------------------" << std::endl;
		std::cout << "Initializing Scene" << std::endl;
		std::cout << std::endl;

		std::vector<std::thread> threads;
		auto startTime = std::chrono::high_resolution_clock::now(); // Start the timer for how long it takes to initialize and simplify the scene

		// Create the model and serialize it
		if ((!SIMPLIFIED))
		{
			std::unordered_map<std::string, uint16_t> loaded_files{};

			// iterate over the different meshes
			int index = 1;
			for (auto path : file_paths)
			{
				// LOADING TASK
				// --------------------------------------------------------------------------------------------------------------------------------------
				if (loaded_files.count(path) != 0)
				{
					// The file has already been loaded so just copy the data from the previous model
					// This is done to avoid loading the same model multiple times and wasting time
					loaded_files[path]++;

					printf("Model previously loaded copying data: %s LoDs: %d Repeats: %d\r", path.c_str(), MAX_LOD, loaded_files[path]);

					for (int i = 0; i < world.meshes.size(); i++)
					{
						if (world.meshes[i].file_path == path)
						{
							// TODO: look for better approaches? This is log O(n)
							std::vector<lod::Mesh>::const_iterator first = world.meshes.begin() + i;
							std::vector<lod::Mesh>::const_iterator last = world.meshes.begin() + (i + MAX_LOD + 1);
							std::vector<lod::Mesh> meshes_copy(first, last);
							world.meshes.insert(world.meshes.end(), meshes_copy.begin(), meshes_copy.end());
							break;
						}
					}

					index++;

					continue;
				}

				printf("Loading Model: %s\n", path.c_str());

				loaded_files[path] = 1;

				lod::VertexBufferBuilder builder;

				if (path.find("thai") != std::string::npos)
				{
					float rotationAngle = glm::radians(90.0f);
					glm::vec3 rotationAxis = glm::vec3(0.0f, -1.0f, 1.0f);

					builder.scale = glm::vec3(0.5f, 0.5f, 0.5f);

					builder.rotationAxis = rotationAxis;
					builder.rotationAngle = rotationAngle;
				}
				else if (path.find("dragon") != std::string::npos)
				{
					builder.scale = glm::vec3(1.0f, 1.0f, 1.0f);
				}
				else if (path.find("Nefertiti") != std::string::npos)
				{
					builder.scale = glm::vec3(0.1f, 0.1f, 0.1f);
				}
				else if (path.find("happy") != std::string::npos)
				{
					builder.scale = glm::vec3(100.0f, 100.0f, 100.0f);
				}
				else if (path.find("armadillo") != std::string::npos)
				{
					builder.scale = glm::vec3(1.0f, 1.0f, 1.0f);
				}
				else if (path.find("bunny") != std::string::npos)
				{
					builder.scale = glm::vec3(50.0f, 50.0f, 50.0f);
				}
				else if (path.find("kitten") != std::string::npos)
				{
					builder.scale = glm::vec3(10.0f, 10.0f, 10.0f);
				}
				else if (((path.find("bruce")) != (std::string::npos)))
				{
					builder.scale = glm::vec3(0.1f, 0.1f, 0.1f);
				}
				else if (((path.find("teapot")) != (std::string::npos)))
				{
					builder.scale = glm::vec3(1.0f, 1.0f, 1.0f);
				}

				builder.replace(lod::createModelFromFile(path, builder)); // Get the original model from file

				completion = 0.0f;

				printProgress(completion);

				// Update the vertices and indices lists
				std::vector<mm::Vertex> vertices;
				for (auto vert : builder.vertices)
				{
					mm::Vertex v{};
					v.pos = vert.pos;
					v.color = vert.color;
					v.texCoord = vert.texCoord;

					vertices.push_back(v);

					completion += 1.0f / (builder.vertices.size() - 1);
					printProgress(completion);
				}

				lod::Mesh mesh;
				mesh.lod = 0;
				mesh.file_path = path;
				mesh.builder.replace(builder);
				mesh.vertices = vertices;
				mesh.indices = builder.indices;

				std::string mesh_name = path.substr(path.find_last_of("/") + 1);
				mesh_name = mesh_name.substr(0, mesh_name.find_last_of("."));
				mesh.name = mesh_name;

				world.meshes.push_back(mesh);

				// SIMPLIFICATION TASK
				// --------------------------------------------------------------------------------------------------------------------------------------
				// No point in simplifying models that are less than one meshlet
				int simplifications = 0;

				printf("\n\nSimplifying Model: %s\n", mesh.name.c_str());

				completion = 0.0f;

				// Create simplified meshes by removing 35% of the vertices each time
				for (int i = 1; i <= MAX_LOD; i++)
				{
					simplification_task(i, 0.55, 1.0, 1.0, world.meshes.back(), mesh.name); // This is faster if not multi-threaded as it uses the previous result as a starting point

					simplifications++;

					completion += 1.00f / (MAX_LOD);
					printProgress(completion);

					// The simplification failed if the number of vertices is the same as the previous iteration
					if (mesh.no_triangles == world.meshes.back().no_triangles)
					{
						std::string error = "Simplification failed: " + mesh.name;
						world.errors.push_back(error);
						world.meshes.pop_back();

						MAX_LOD = i - 1;
						break;
					}
				}

				// This makes sure the world space errors from the simplification compound and are not just the error from the last simplification
				for (int lod = 1; lod <= MAX_LOD; lod++)
				{
					if (lod == 1)
						world.simplification_errors.push_back(0.0f); // The first lod is always 0

					auto &mesh = world.meshes[lod];
					auto &prev_mesh = world.meshes[lod - 1];

					float error = mesh.simplificationError + prev_mesh.simplificationError;

					world.simplification_errors.push_back(error);
				}

				// Rename the original mesh to have a lod of 0
				world.meshes[world.meshes.size() - (simplifications + 1)].name += "_lod_" + std::to_string(0);

				if (simplifications <= 0)
				{
					printf("\n\n");
					printf("Model not simplified: %s\n", mesh.name.c_str());
				}

				// SERIALIZATION TASK
				// --------------------------------------------------------------------------------------------------------------------------------------
				// printf("\n\nSerializing Simplified Models: %s LoDs = %d \n", mesh.name.c_str(), simplifications + 1);
				// completion = 0.0f;

				// // Serialize the models to binary files
				// for (auto &mesh : world.meshes)
				// {
				// 	std::thread th = std::thread([&mesh]()
				// 								 { serialization_task(mesh); });
				// 	threads.push_back(std::move(th));
				// }

				// synchThreads(threads, world.meshes.size());

				// Free memory
				for (auto &mesh : world.meshes)
				{
					mesh.builder.clear();
				}

				printf("\n\n");

				index++;
			}

			SIMPLIFIED = true;
		}
		else
		{
			completion = 0.0f;

			for (auto path : file_paths)
			{
				for (int i = 0; i <= MAX_LOD; i++)
				{
					std::thread th = std::thread([i, path]()
												 { deserialization_task(i, path); });
					threads.push_back(std::move(th));

					// completion += 1.00f / (MAX_LOD * file_paths.size());

					printProgress(completion);
				}
			}

			synchThreads(threads, (MAX_LOD * file_paths.size()));
		}

		// Optional Rotation of repeating models Task
		// --------------------------------------------------------------------------------------------------------------------------------------
		// This is to make sure that there are multiple models being loaded/rendered since transform does not work

		printf("\n\n");
		printf("Placing models\n");

		completion = 0.0f;
		printProgress(completion);

		int grid_size = glm::ceil(glm::sqrt(static_cast<float>(world.meshes.size()))); // Compute the size of the grid
		float offsetX = 75.5f;														   // X offset (distance between meshes in the x direction)
		float offsetZ = 75.5f;														   // Z offset (distance between meshes in the z direction)

		// Calculate starting coordinates so that the first model is in the middle of the grid
		float startX = -((grid_size / 2) * offsetX);
		float startZ = -((grid_size / 2) * offsetZ);

		for (int i = 0; i < world.meshes.size(); i++)
		{
			// Compute the grid coordinates for this mesh
			int gridX = i % grid_size;
			int gridZ = i / grid_size;

			// Convert grid coordinates to world coordinates for the translation
			glm::vec3 translation = glm::vec3(startX + gridX * offsetX, 0.0f, startZ + gridZ * offsetZ);

			// Optional rotation of the models
			float rotationAngle = glm::radians(0.0f);
			// float rotationAngle = glm::radians(90.0f + (i * 5.0f)); // No matter if the meshlets are generated for each of the models separately they still get frustum culled
			glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);

			int temp = i;

			// Change all LODs of this mesh
			for (int j = 0; j <= MAX_LOD; j++)
			{
				auto &mesh = world.meshes[temp];
				temp++;

				glm::vec3 centroid = glm::vec3(0.0f);

				for (auto &vert : mesh.vertices)
				{
					vert.pos += translation;

					glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis);
					vert.pos = glm::vec3(rotationMatrix * glm::vec4(vert.pos, 1.0f));

					if (j == 0)
					{
						centroid += vert.pos;
					}
				}

				if (j == 0)
				{
					centroid /= static_cast<float>(mesh.vertices.size());

					mesh.center = centroid;

					world.mesh_centers.push_back(mesh.center);
				}
			}

			i += MAX_LOD;

			completion += 1.0f / (world.meshes.size() - i);
			if (completion > 1.0f)
			{
				completion = 1.0f;
			}
			printProgress(completion);
		}

		// Cluster Creation TASK
		// --------------------------------------------------------------------------------------------------------------------------------------
		// Generate Meshlets for each of the simplified models
		printf("\n\n");
		printf("Creating Meshlets\n");

		completion = 0.0f;
		printProgress(completion);

		// The following is faster to test but means that the meshlet positions will all be the same.
		std::unordered_map<std::string, uint16_t> generated_meshlets{};
		std::unordered_map<int, int> needs_copying{};

		for (int i = 0; i < world.meshes.size(); i++)
		{
			// No need to generate meshlets if they are already present
			if (world.meshes[i].meshletCache.size() == 0)
			{
				lod::Mesh &mesh = world.meshes[i];

				// Check if there is another mesh that has already generated meshlets
				if ((generated_meshlets.count(mesh.name) == 0))
				{
					// This is no longer faster on a single thread the endless loop has been fixed.
					std::thread th = std::thread([&mesh]()
												 { createMeshlets_task(mesh); });
					threads.push_back(std::move(th));

					if (threads.size() > std::thread::hardware_concurrency())
					{
						synchThreads(threads, 0.0f);
					}

					// This is too much memory usage for the multi-model scene
					if (copyMeshlets)
					{
						generated_meshlets[mesh.name] = i;
					}
				}
				else
				{
					// Mark the mesh to be copied from the other mesh once the threads have been synced
					int copyIndex = generated_meshlets[world.meshes[i].name];
					needs_copying[i] = copyIndex;
				}
			}
			else
			{
				generated_meshlets[world.meshes[i].name] = i;
			}

			completion += 1.0f / (world.meshes.size() - 1);
			printProgress(completion);
		}

		completion = 0.0f;

		synchThreads(threads, 0);

		if (needs_copying.size() > 0)
		{
			printf("\n\n");
			printf("Copying Meshlets\n");

			// TODO: Re-make so that it works with memory pointers and not copies

			completion = 0.0f;

			for (const auto &[key, value] : needs_copying)
			{
				// Copy meshlets from other mesh
				lod::Mesh &mesh = world.meshes[key];
				lod::Mesh &other = world.meshes[value];

				mesh.indexVertexMap = other.indexVertexMap;
				mesh.meshletCache = other.meshletCache;
				mesh.triangles = other.triangles;
				mesh.stats = other.stats;
				mesh.objectData = other.objectData;
				mesh.packedMeshlets = other.packedMeshlets;
				// mesh.center = other.center;
				mesh.no_triangles = other.triangles.size();

				completion += 1.00f / (needs_copying.size() - 1);
				printProgress(completion);
			}
		}

		// Get Individual Meshlet vertices TASK
		// This task is a waste of time and should just be taken from the meshlet creation task however I am too lazy to understand that code
		// --------------------------------------------------------------------------------------------------------------------------------------
		// Get the vertices for each meshlet
		// From the meshlet vertices get we can then re-generate the AABB for each meshlet done within the meshlet vertices subtask

		if (calcMeshlets)
		{
			printf("\n\n");
			printf("Getting Meshlet Vertices\n");

			completion = 0.0f;
			printProgress(completion);

			// This can take a while
			for (int i = 0; i < world.meshes.size(); i++)
			{
				auto &mesh = world.meshes[i];

				mesh.meshlets.resize(mesh.meshletCache.size());

				for (int j = 0; j < mesh.meshletCache.size(); j++)
				{
					std::thread th = std::thread([&mesh, j]()
												 { createMeshlets_subTask(mesh, j); });
					threads.push_back(std::move(th));

					if (threads.size() >= std::thread::hardware_concurrency())
					{
						synchThreads(threads, 0);
					}
				}

				synchThreads(threads, 0);

				completion += 1.00f / world.meshes.size();
				printProgress(completion);
			}
		}

		// Calculate Geometric Centroid TASK
		// --------------------------------------------------------------------------------------------------------------------------------------
		// This generates the geometric centroid for each mesh (and each LOD)
		printf("\n\n");
		printf("Calculating Geometric Centroids\n");

		completion = 0.0f;
		printProgress(completion);

		glm::vec3 centerOfAllMeshes = glm::vec3(0.0f);

		for (int i = 0; i < world.meshes.size(); i++)
		{
			glm::vec3 centroid = glm::vec3(0.0f);

			auto &mesh = world.meshes[i];

			if (calcMeshlets)
			{
				for (auto &meshlet : mesh.meshlets)
				{
					centroid += meshlet.center;
				}

				// This result is in model space
				centroid /= static_cast<float>(mesh.meshlets.size());

				mesh.center = centroid;
			}
			else
			{
				centroid = mesh.center;
			}

			centerOfAllMeshes += centroid;

			completion += 1.00f / world.meshes.size();
			printProgress(completion);
		}

		// Set camera position to be at the center of the first scene
		centerOfAllMeshes /= static_cast<float>(world.meshes.size());
		world.center = centerOfAllMeshes;
		glm::vec3 cameraPos = world.center;
		cameraPos.x -= camera.thresholds[MAX_LOD];
		cameraPos.x -= 50.0f;

		camera.position = cameraPos;

		camera.worldCenter = world.center;

		// Jinsoku coordinate system is -x - x | -y - y | -z - z )
		if (calcMeshlets)
		{
			camera.view = glm::lookAt(camera.position, world.meshes.front().meshlets.front().vertices.front().pos, glm::vec3(0.0f, -1.0f, 0.0f));
		}
		else
		{
			camera.view = glm::lookAt(camera.position, world.center, glm::vec3(0.0f, -1.0f, 0.0f));
		}

		// Graph creation TASK
		// We can now create the DAG using the vertices from the meshlets
		// With the meshlet AABBs we can now make a BVH for each meshlet
		// --------------------------------------------------------------------------------------------------------------------------------------
		if (calcMeshlets)
		{
			printf("\n\n");
			printf("Creating Graph: %s\n", "DAG");

			completion = 0.0f;
			// This map stores the vertices of each of the LoD meshlets then stores which meshlets have the same vertex
			std::unordered_map<glm::vec3, std::vector<lod::Graph::Node>, lod::vec3_hash> vertex_to_nodes;

			// A map of all the  nodes in the graph <lod, Node>
			// std::unordered_map<int, std::vector<lod::Graph::Node>> nodes;

			int i = 0;
			int id = 0;
			int meshIndex = 0;
			for (auto &mesh : world.meshes)
			{
				if (((i % (MAX_LOD + 1)) == 0) && (i != 0))
				{
					meshIndex++;
				}

				i++;

				// Create the DAG
				for (auto &meshlet : mesh.meshlets)
				{
					lod::Graph::Node node;
					node.id = id;
					node.meshIndex = meshIndex;
					node.meshletIndex = meshlet.index;
					node.lod = meshlet.lod;
					node.vertices = meshlet.vertices;
					node.no_triangles = meshlet.no_triangles;
					node.center = meshlet.center;

					node.bb.minPoint = meshlet.minPoint;
					node.bb.maxPoint = meshlet.maxPoint;

					// // A vertex map so that we can easily check the vertices
					// for (const auto &vert : node.vertices)
					// {
					// 	vertex_to_nodes[vert.pos].push_back(node);
					// }

					id++;

					world.DAG.nodes[node.lod].push_back(node);
				}
			}

			// Create parent child relationships based on which meshlets share vertices
			for (int lod = MAX_LOD; lod > 0; lod--)
			{
				for (int i = 0; i < world.DAG.nodes[lod].size(); i++)
				{
					lod::Graph::Node *parent = &world.DAG.nodes[lod][i];

					lod::BoundingBox parent_bb = parent->bb;

					// Check the lower level
					for (int j = 0; j < world.DAG.nodes[lod - 1].size(); j++)
					{
						lod::Graph::Node *child = &world.DAG.nodes[lod - 1][j];

						lod::BoundingBox child_bb = child->bb;

						if (parent_bb.isContained(parent_bb, child_bb) && (parent->children.count(child->id) == 0))
						{
							parent->children[child->id] = child;
						}
					}

					// The following is if we want one level of shared vertices
					// for (auto &vert : parent->vertices)
					// {
					// 	// Lookup nodes sharing the vertex
					// 	auto &potential_children = vertex_to_nodes[vert.pos];

					// 	for (auto &kid : potential_children)
					// 	{
					// 		// Check if potential_child is already a child of the parent
					// 		if ((kid.lod == lod - 1) && (parent->children.count(kid.id) == 0))
					// 		{
					// 			parent->children[kid.id] = kid;
					// 		}
					// 	}
					// }
				}

				completion += 1.00f / (MAX_LOD);
				printProgress(completion);
			}

			completion = 0;
			printProgress(completion);
		}

		// World Building TASK
		// --------------------------------------------------------------------------------------------------------------------------------------
		// Remove the simplified models from the world and the meshlets become their own game object
		printf("\n\n");
		printf("Building World\n");

		std::vector<NVMeshlet::Builder<uint32_t>::MeshletGeometry> meshletGeometry32;
		std::vector<NVMeshlet::Builder<uint16_t>::MeshletGeometry> meshletGeometry;
		std::vector<NVMeshlet::Stats> stats;
		std::vector<uint32_t> vertCount;
		std::vector<mm::Vertex> vertices{0};
		std::vector<ObjectData> objectData;
		std::vector<uint32_t> indices_model;

		completion = 0.0f;

		int lowestLoD = 0;

		// Coalesce the different models into one Vertex/Index Buffer
		for (auto &mesh : world.meshes)
		{
			completion += 1.0f / (world.meshes.size());
			printProgress(completion);
			// If each of the meshes (LoDs included) should be treated separately making multiple game objects
			modelBuilding_task(mesh, meshletGeometry32, meshletGeometry, stats, vertCount, vertices, objectData, indices_model);

			// What is the lowest LoD model that could be used for the scene
			if (mesh.lod > lowestLoD)
			{
				lowestLoD = mesh.lod;
			}
		}

		world.model.meshletGeometry32 = meshletGeometry32;
		world.model.meshletGeometry = meshletGeometry;
		world.model.stats = stats;
		world.model.vertCount = vertCount;
		world.model.vertices = vertices;
		world.model.objectData = objectData;
		world.model.indices = indices_model;
		world.lowestLod = lowestLoD;

		auto stopTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stopTime - startTime);

		long timestamp = duration.count();
		long milliseconds = (long)(timestamp / 1000) % 1000;
		long seconds = (((long)(timestamp / 1000) - milliseconds) / 1000) % 60;
		long minutes = (((((long)(timestamp / 1000) - milliseconds) / 1000) - seconds) / 60) % 60;
		long hours = ((((((long)(timestamp / 1000) - milliseconds) / 1000) - seconds) / 60) - minutes) / 60;

		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << "Meshes Initialized: " << world.meshes.size() << std::endl;
		std::cout << "Total time taken to initialize: " << hours << ":" << minutes << ":" << seconds << "." << milliseconds << std::endl;

		if (world.errors.size() > 0)
		{
			printf("\nThe following Errors occurred during initialization:\n");
			for (std::string error : world.errors)
			{
				std::cout << "\t" + error << std::endl;
			}

			world.errors.clear();
		}

		// No longer needed

		std::cout << "-----------------------------" << std::endl;

		SHOW_MESSAGES = true;
	}

	int ResourcesMS::loadModel(std::vector<std::string> modelPaths)
	{
		createWorld(modelPaths);

		scene.lowestLOD = world.lowestLod;

		std::vector<NVMeshlet::Builder<uint32_t>::MeshletGeometry> meshletGeometry32 = world.model.meshletGeometry32;
		std::vector<NVMeshlet::Builder<uint16_t>::MeshletGeometry> meshletGeometry = world.model.meshletGeometry;
		std::vector<NVMeshlet::Stats> stats = world.model.stats;
		std::vector<uint32_t> vertCount = world.model.vertCount;
		std::vector<mm::Vertex> vertices = world.model.vertices;
		std::vector<ObjectData> objectData = world.model.objectData;

		world.model.meshletGeometry32.clear();
		world.model.meshletGeometry.clear();
		world.model.stats.clear();
		world.model.vertCount.clear();
		world.model.vertices.clear();
		world.model.objectData.clear();
		world.model.indices.clear();

		// init cullstats with all zeros
		m_cullStats = new CullStats();
		m_cullStats->meshletsOutput = 0;

		// temp definitions to not break code!
		// reminde me to update this and get rid of them
		std::vector<float> verts;
		std::vector<float> attributes;
		std::vector<float> texCoords;

		// allocate and update descriptorsets
		size_t num_models16 = meshletGeometry.size();
		size_t num_models32 = meshletGeometry32.size();

		m_geos.resize(num_models16 + num_models32 + 1);

		// set pipeline for vr models
		if (num_models16 + num_models32 >= 2)
		{
			for (int i = 0; i < num_models16; ++i)
			{
				m_geos[i].pipelineID = 1;
				m_geos[i].shorts = 1;
			}
		}

		// std::cout << sizeof(NVMeshlet::MeshletDesc) << std::endl;

		// vr controllers are strange and require special care when building offsets
		for (int i = 1; i < m_geos.size(); ++i)
		{
			/*if (num_models16 == 0) {
				m_geos[i].vbo_offset = 0;
			}
			else {
				m_geos[i].vbo_offset = vertCount[i] + vertCount[i - 1];
			}*/
			m_geos[i].desc_offset = (stats[i - 1].meshletsTotal * sizeof(NVMeshlet::MeshletDesc)) + m_geos[i - 1].desc_offset;
			m_geos[i].prim_offset = (stats[i - 1].primIndices * sizeof(NVMeshlet::PrimitiveIndexType)) + m_geos[i - 1].prim_offset;
			m_geos[i].vert_offset = (stats[i - 1].vertexIndices * (m_geos[i - 1].shorts == 1 ? sizeof(uint16_t) : sizeof(uint32_t))) + m_geos[i - 1].vert_offset;
			m_geos[i].vbo_offset = (vertCount[i - 1] * (3 * sizeof(float))) + m_geos[i - 1].vbo_offset;

			m_geos[i - 1].desc_count = stats[i - 1].meshletsTotal;
		}
		// m_geos[1].vbo_offset = 0;
		//  if model is split into several meshlets we need this
		//  i should be 3 when controllers are present 1 when they are not
		// for (int i = (num_models16 == 0) ? 1 : 3;  i < num_models16 + num_models32 + 1; ++i) {
		//	m_geos[i].vbo_offset = m_geos[i-1].vbo_offset;
		// }
		//

		//  this is not needed anymore?!
		vertCount.clear();

		// This is the easiest way of implementing a single object per mesh and not per LoD  so that it still generates all the below stuff correctly.
		{
			// Save the offsets for each of the LoDs
			for (auto &object : m_geos)
			{
				// Assign offsets to each of the LoDs
				world.model.prim_offsets.push_back(object.prim_offset);
				world.model.desc_offsets.push_back(object.desc_offset);
				world.model.vbo_offsets.push_back(object.vbo_offset);
				world.model.vert_offsets.push_back(object.vert_offset);
				world.model.desc_counts.push_back(object.desc_count);
			}

			std::vector<GameObject> m_geos_temp{};

			std::string previous_path = "";
			for (int i = 0; i < world.mesh_paths.size(); i++)
			{
				// if (world.mesh_paths[i] != previous_path)
				// {
				GameObject original = m_geos[i * (MAX_LOD + 1)];
				original.center = world.meshes[i * (MAX_LOD + 1)].center;
				m_geos_temp.push_back(original);
				// }

				previous_path = world.mesh_paths[i];
			}

			GameObject offset_Obj = m_geos.back();
			m_geos.clear();

			for (auto &object : m_geos_temp)
			{
				m_geos.push_back(object);
			}

			m_geos.push_back(offset_Obj);

			world.meshes.clear();
		}

		// set up scene based on the meshes and init cmdpool and cmdbuffer
		int offset = 0;
		// should -1 here too.
		for (GameObject &object : m_geos)
		{
			jsvk::QueueFamilyIndices queueFamilyIndices;
			queueFamilyIndices = jsvk::findQueueFamilies(m_pVulkanDevice->m_pPhysicalDevice, m_pPresenter->getSurface());
			object.ObjectOffset = offset;
			++offset;
			VkCommandPoolCreateInfo poolInfo = jsvk::init::commandPoolCreateInfo();
			poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VK_CHECK(vkCreateCommandPool(m_pVulkanDevice->Device(), &poolInfo, nullptr, &object.m_commandPool));

			object.m_commandBuffers.resize(3);

			VkCommandBufferAllocateInfo allocInfo = jsvk::init::commandBufferAllocateInfo();
			allocInfo.commandPool = object.m_commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			allocInfo.commandBufferCount = (uint32_t)object.m_commandBuffers.size();

			VK_CHECK(vkAllocateCommandBuffers(m_pVulkanDevice->Device(), &allocInfo, object.m_commandBuffers.data()));

			scene.gameObjects.push_back(object);

			// Make enough room for all the meshlets in LoD 0 per mesh so that worst case we can draw each of the meshlets individually
			// TODO: Find a better solution? This is a lot of wasted space and resources
			// This will also not generate a command buffer for the last game object which is the offset object since it has no meshlets

			for (int i = 0; i < (object.desc_count * 2); ++i)
			{

				GameObject meshletObject;
				jsvk::QueueFamilyIndices queueFamilyIndices;
				queueFamilyIndices = jsvk::findQueueFamilies(m_pVulkanDevice->m_pPhysicalDevice, m_pPresenter->getSurface());
				meshletObject.ObjectOffset = offset;
				// ++offset;
				VkCommandPoolCreateInfo poolInfo = jsvk::init::commandPoolCreateInfo();
				poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

				vkCreateCommandPool(m_pVulkanDevice->Device(), &poolInfo, nullptr, &meshletObject.m_commandPool);

				meshletObject.m_commandBuffers.resize(3);

				VkCommandBufferAllocateInfo allocInfo = jsvk::init::commandBufferAllocateInfo();
				allocInfo.commandPool = meshletObject.m_commandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
				allocInfo.commandBufferCount = (uint32_t)meshletObject.m_commandBuffers.size();

				vkAllocateCommandBuffers(m_pVulkanDevice->Device(), &allocInfo, meshletObject.m_commandBuffers.data());

				world.gameObjects.push_back(meshletObject);
			}
		}

		size_t vertexSize = 0;

		size_t vert16Size = 0;
		for (int i = 0; i < num_models16; ++i)
		{
			vert16Size += sizeof(uint16_t) * meshletGeometry[i].vertexIndices.size();
			vertexSize += sizeof(uint16_t) * meshletGeometry[i].vertexIndices.size();
		}

		size_t vert32Size = 0;
		for (int i = 0; i < num_models32; ++i)
		{
			vert32Size += sizeof(uint32_t) * meshletGeometry32[i].vertexIndices.size();
			vertexSize += sizeof(uint32_t) * meshletGeometry32[i].vertexIndices.size();
		}

		uint16_t *vertexindices16 = (uint16_t *)malloc(vert16Size);
		uint32_t *vertexindices32 = (uint32_t *)malloc(vert32Size);

		// memcpy(vertexindices32, meshletGeometry[0].vertexIndices.data(), vertexSize);
		uint16_t *vertPointer = vertexindices16;
		for (int i = 0; i < num_models16; ++i)
		{
			size_t size = meshletGeometry[i].vertexIndices.size() * sizeof(uint16_t);
			// geos[i + 1].vert_offset = size;
			memcpy(vertPointer, meshletGeometry[i].vertexIndices.data(), size);
			vertPointer += meshletGeometry[i].vertexIndices.size();
		}

		uint32_t *vertPointer32 = vertexindices32;
		for (int i = 0; i < num_models32; ++i)
		{
			size_t size = meshletGeometry32[i].vertexIndices.size() * sizeof(uint32_t);
			// geos[i + 1].vert_offset = size;
			memcpy(vertPointer32, meshletGeometry32[i].vertexIndices.data(), size);
			vertPointer32 += meshletGeometry32[i].vertexIndices.size();
		}

		void *vertData16 = vertexindices16;
		void *vertData32 = vertexindices32;

		size_t descSize = 0;
		for (int i = 0; i < num_models16; ++i)
		{
			descSize += sizeof(NVMeshlet::MeshletDesc) * meshletGeometry[i].meshletDescriptors.size();
			// geos[i+1].desc_offset = sizeof(NVMeshlet::MeshletDesc) * meshletGeometry[i].meshletDescriptors.size();
		}
		for (int i = 0; i < num_models32; ++i)
		{
			descSize += sizeof(NVMeshlet::MeshletDesc) * meshletGeometry32[i].meshletDescriptors.size();
			// geos[i+1].desc_offset = sizeof(NVMeshlet::MeshletDesc) * meshletGeometry[i].meshletDescriptors.size();
		}

		size_t primSize = 0;
		for (int i = 0; i < num_models16; ++i)
		{
			primSize += sizeof(NVMeshlet::PrimitiveIndexType) * meshletGeometry[i].primitiveIndices.size();
			// geos[i + 1].prim_offset = sizeof(NVMeshlet::PrimitiveIndexType) * meshletGeometry[i].primitiveIndices.size();
		}
		for (int i = 0; i < num_models32; ++i)
		{
			primSize += sizeof(NVMeshlet::PrimitiveIndexType) * meshletGeometry32[i].primitiveIndices.size();
			// geos[i + 1].prim_offset = sizeof(NVMeshlet::PrimitiveIndexType) * meshletGeometry[i].primitiveIndices.size();
		}

		// TODO: This only works for the teapot at 11 LoDs for testing purposes
		// descSize = 4112;
		// primSize = 75264;

		NVMeshlet::MeshletDesc *descData = (NVMeshlet::MeshletDesc *)malloc(descSize);
		NVMeshlet::PrimitiveIndexType *primData = (NVMeshlet::PrimitiveIndexType *)malloc(primSize);

		// memcpy(descData, meshletGeometry[0].meshletDescriptors.data(), meshletGeometry[0].meshletDescriptors.size() * sizeof(NVMeshlet::MeshletDesc));
		NVMeshlet::MeshletDesc *descPointer = descData;
		for (int i = 0; i < num_models16; ++i)
		{
			uint32_t size = meshletGeometry[i].meshletDescriptors.size() * sizeof(NVMeshlet::MeshletDesc);
			memcpy(descPointer, meshletGeometry[i].meshletDescriptors.data(), size);
			descPointer += meshletGeometry[i].meshletDescriptors.size();
		}
		for (int i = 0; i < num_models32; ++i)
		{
			uint32_t size = meshletGeometry32[i].meshletDescriptors.size() * sizeof(NVMeshlet::MeshletDesc);
			memcpy(descPointer, meshletGeometry32[i].meshletDescriptors.data(), size);
			descPointer += meshletGeometry32[i].meshletDescriptors.size();
		}

		// memcpy(primData, meshletGeometry[0].primitiveIndices.data(), primSize);
		NVMeshlet::PrimitiveIndexType *primPointer = primData;
		for (int i = 0; i < num_models16; ++i)
		{
			uint32_t size = meshletGeometry[i].primitiveIndices.size() * sizeof(NVMeshlet::PrimitiveIndexType);
			// geos[i + 1].prim_offset = meshletGeometry[i].primitiveIndices.size();
			memcpy(primPointer, meshletGeometry[i].primitiveIndices.data(), size);
			primPointer += meshletGeometry[i].primitiveIndices.size();
		}
		for (int i = 0; i < num_models32; ++i)
		{
			uint32_t size = meshletGeometry32[i].primitiveIndices.size() * sizeof(NVMeshlet::PrimitiveIndexType);
			// geos[i + 1].prim_offset = meshletGeometry[i].primitiveIndices.size();
			memcpy(primPointer, meshletGeometry32[i].primitiveIndices.data(), size);
			primPointer += meshletGeometry32[i].primitiveIndices.size();
		}

		// rearrange into vbo and abo
		int controller_verts = 0;
		for (int i = 0; i < controller_verts; ++i)
		{
			verts.push_back(vertices.at(i).pos.x);
			verts.push_back(vertices.at(i).pos.y);
			verts.push_back(vertices.at(i).pos.z);

			attributes.push_back(vertices.at(i).color.x);
			attributes.push_back(vertices.at(i).color.y);
			attributes.push_back(vertices.at(i).color.z);

			texCoords.push_back(vertices.at(i).texCoord.x);
			texCoords.push_back(vertices.at(i).texCoord.y);
		}

		for (int i = controller_verts; i < vertices.size(); ++i)
		{
			verts.push_back(vertices.at(i).pos.x);
			verts.push_back(vertices.at(i).pos.y);
			verts.push_back(vertices.at(i).pos.z);

			attributes.push_back(vertices.at(i).color.x);
			attributes.push_back(vertices.at(i).color.y);
			attributes.push_back(vertices.at(i).color.z);
		}

		// Dont need these no more I think ?
		vertices.clear();

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_pVulkanDevice->m_pPhysicalDevice, &properties);
		VkPhysicalDeviceLimits &limits = properties.limits;

		// just storage buffer now!
		// VkDeviceSize m_alignment = std::max(limits.minTexelBufferOffsetAlignment, limits.minStorageBufferOffsetAlignment);
		VkDeviceSize m_alignment = limits.minStorageBufferOffsetAlignment;
		// to keep vbo/abo "parallel" to each other, we need to use a common multiple
		// that means every offset of vbo/abo of the same sub-allocation can be expressed as "nth vertex" offset from the buffer
		VkDeviceSize multiple = 1;
		VkDeviceSize vboStride = sizeof(float) * 3;
		VkDeviceSize aboStride = sizeof(float) * 3;
		while (true)
		{
			if (((multiple * vboStride) % m_alignment == 0) && ((multiple * aboStride) % m_alignment == 0))
			{
				break;
			}
			multiple++;
		}
		VkDeviceSize m_vboAlignment = multiple * vboStride;
		VkDeviceSize m_aboAlignment = multiple * aboStride;

		// buffer allocation
		// costs of entire model, provide offset into large buffers per geometry
		VkDeviceSize tboSize = limits.maxTexelBufferElements;
		VkDeviceSize maxChunk = 512 * 1024 * 1024;
		const VkDeviceSize vboMax = VkDeviceSize(tboSize) * sizeof(float) * 4;
		const VkDeviceSize iboMax = VkDeviceSize(tboSize) * sizeof(uint16_t);
		const VkDeviceSize meshMax = VkDeviceSize(tboSize) * sizeof(uint16_t);

		VkDeviceSize m_maxVboChunk = std::min(vboMax, maxChunk);
		VkDeviceSize m_maxIboChunk = std::min(iboMax, maxChunk);
		VkDeviceSize m_maxMeshChunk = std::min(meshMax, maxChunk);

		VkDeviceSize vboSize = verts.size() * sizeof(float);
		VkDeviceSize aboSize = attributes.size() * sizeof(float);
		VkDeviceSize texboSize = texCoords.size() * sizeof(float);
		VkDeviceSize meshSize = NVMeshlet::computeCommonAlignedSize(descSize) + NVMeshlet::computeCommonAlignedSize(primSize) + NVMeshlet::computeCommonAlignedSize(vertexSize);

		vboSize = alignedSize(vboSize, m_alignment);
		aboSize = alignedSize(aboSize, m_alignment);
		meshSize = alignedSize(meshSize, m_alignment);
		texboSize = alignedSize(texboSize, m_alignment);

		// lets make one big alloc TODO try adding textures to this alloc

		VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		m_pVulkanDevice->createBuffer(flags | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_mainBuffer, vboSize + aboSize + texboSize + meshSize);

		jsvk::Buffer stagingBuffer;
		m_pVulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, vboSize + aboSize + texboSize + meshSize);
		stagingBuffer.map();
		stagingBuffer.copyTo(verts.data(), vboSize);
		stagingBuffer.unmap();
		stagingBuffer.map(vboSize, aboSize);
		stagingBuffer.copyTo(attributes.data(), aboSize);
		if (texboSize > 0)
		{
			stagingBuffer.unmap();
			stagingBuffer.map(vboSize + aboSize, texboSize);
			stagingBuffer.copyTo(texCoords.data(), texboSize);
		}

		stagingBuffer.unmap();
		VK_CHECK(stagingBuffer.map(vboSize + aboSize + texboSize));
		stagingBuffer.copyTo(descData, descSize);
		stagingBuffer.unmap();
		stagingBuffer.map(vboSize + aboSize + texboSize + NVMeshlet::computeCommonAlignedSize(descSize));
		stagingBuffer.copyTo(primData, primSize);
		stagingBuffer.unmap();
		stagingBuffer.map(vboSize + aboSize + texboSize + NVMeshlet::computeCommonAlignedSize(descSize) + NVMeshlet::computeCommonAlignedSize(primSize));
		stagingBuffer.copyTo(vertData16, vert16Size);
		stagingBuffer.unmap();
		stagingBuffer.map(vboSize + aboSize + texboSize + NVMeshlet::computeCommonAlignedSize(descSize) + NVMeshlet::computeCommonAlignedSize(primSize) + vert16Size);
		stagingBuffer.copyTo(vertData32, vert32Size);
		stagingBuffer.unmap();

		jsvk::copyBuffer(m_pVulkanDevice->m_pLogicalDevice, m_pVulkanDevice->m_commandPool, m_pVulkanDevice->m_pGraphicsQueue, stagingBuffer.m_pBuffer, m_mainBuffer.m_pBuffer, vboSize + aboSize + texboSize + meshSize);

		stagingBuffer.destroy();

		// create pr scene and object resources
		// limit is size of 64 aka one glm::mat4
		VkDeviceSize minUboAlignment = m_pVulkanDevice->m_deviceProperties.limits.minUniformBufferOffsetAlignment;
		m_dynamicAlignment = sizeof(SceneData);

		if (minUboAlignment > 0)
		{
			m_dynamicAlignment = (m_dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}

		VkDeviceSize objectDynamicAlignment = sizeof(ObjectData);

		if (minUboAlignment > 0)
		{
			objectDynamicAlignment = (objectDynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}

		VkDeviceSize bufferSize = (VkDeviceSize)64 + 2 * m_dynamicAlignment + (num_models16 + num_models32) * objectDynamicAlignment + sizeof(CullStats);
		// we multiply by 2 because we have left and right eye view
		VkDeviceSize totalSize = bufferSize * 2;

		VK_CHECK(m_pVulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_uniformBuffer, bufferSize, nullptr, true));

		// initial fill of object data
		char *uniformChar = (char *)m_uniformBuffer.m_mapped;
		DynamicObjectDataPointer = m_uniformBuffer.m_mapped;

		for (int i = 0; i < (num_models16 + num_models32); ++i)
		{
			memcpy(&uniformChar[i * objectDynamicAlignment], &objectData[i], sizeof(ObjectData));
		}

		// segfaults the shit out of me!
		memcpy(&uniformChar[(VkDeviceSize)64 + 2 * m_dynamicAlignment + (num_models16 + num_models32) * objectDynamicAlignment], m_cullStats, sizeof(CullStats));

		m_uniformBuffer.unmap();
		m_uniformBuffer.map((num_models16 + num_models32) * objectDynamicAlignment, 2 * m_dynamicAlignment);

		VkPipelineStageFlags stageMesh = VK_SHADER_STAGE_MESH_BIT_NV;
		VkPipelineStageFlags stageTask = VK_SHADER_STAGE_TASK_BIT_NV;

		// bindings for pipeline
		VkDescriptorSetLayoutBinding sceneBindings[2];
		sceneBindings[0].binding = 0;
		sceneBindings[0].descriptorCount = 1;
		sceneBindings[0].stageFlags = stageTask | stageMesh | VK_SHADER_STAGE_FRAGMENT_BIT;
		sceneBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

		sceneBindings[1].binding = 1;
		sceneBindings[1].descriptorCount = 1;
		sceneBindings[1].stageFlags = stageTask | stageMesh;
		sceneBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		VkDescriptorSetLayoutCreateInfo sceneLayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		sceneLayoutInfo.bindingCount = 2;
		sceneLayoutInfo.pBindings = sceneBindings;
		sceneLayoutInfo.flags = 0;
		sceneLayoutInfo.pNext = nullptr;

		VkDescriptorSetLayout sceneDescriptorSetLayout;
		VkResult result = vkCreateDescriptorSetLayout(m_pVulkanDevice->Device(), &sceneLayoutInfo, nullptr, &m_layouts[0]);

		// bindings for pipeline
		VkDescriptorSetLayoutBinding objBindings[1];
		objBindings[0].binding = 0;
		objBindings[0].descriptorCount = 1;
		objBindings[0].stageFlags = stageTask | stageMesh | VK_SHADER_STAGE_FRAGMENT_BIT;
		objBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

		VkDescriptorSetLayoutCreateInfo objLayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		objLayoutInfo.bindingCount = 1;
		objLayoutInfo.pBindings = objBindings;
		objLayoutInfo.flags = 0;
		objLayoutInfo.pNext = nullptr;

		VkDescriptorSetLayout objDescriptorSetLayout;
		result = vkCreateDescriptorSetLayout(m_pVulkanDevice->Device(), &objLayoutInfo, nullptr, &m_layouts[1]);

		// image pipe
		VkDescriptorSetLayoutBinding ImgBindings[1];
		ImgBindings[0].binding = 0;
		ImgBindings[0].descriptorCount = 1;
		ImgBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		ImgBindings[0].pImmutableSamplers = nullptr;
		ImgBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo ImgLayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		ImgLayoutInfo.bindingCount = 1;
		ImgLayoutInfo.pBindings = ImgBindings;
		ImgLayoutInfo.flags = 0;
		ImgLayoutInfo.pNext = nullptr;

		VkDescriptorSetLayout objImageDescriptorSetLayout;
		result = vkCreateDescriptorSetLayout(m_pVulkanDevice->Device(), &ImgLayoutInfo, nullptr, &m_layouts[3]);

		// bindings for pipeline
		VkDescriptorSetLayoutBinding geoBindings[6];
		geoBindings[0].binding = 0;
		geoBindings[0].descriptorCount = 1;
		geoBindings[0].stageFlags = stageTask | stageMesh;
		geoBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		geoBindings[1].binding = 1;
		geoBindings[1].descriptorCount = 1;
		geoBindings[1].stageFlags = stageMesh;
		geoBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		geoBindings[2].binding = 2;
		geoBindings[2].descriptorCount = 1;
		geoBindings[2].stageFlags = stageMesh;
		geoBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		geoBindings[3].binding = 3;
		geoBindings[3].descriptorCount = 1;
		geoBindings[3].stageFlags = stageMesh;
		geoBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		geoBindings[4].binding = 4;
		geoBindings[4].descriptorCount = 1;
		geoBindings[4].stageFlags = stageMesh;
		geoBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		geoBindings[5].binding = 5;
		geoBindings[5].descriptorCount = 1;
		geoBindings[5].stageFlags = stageMesh;
		geoBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		VkDescriptorSetLayoutCreateInfo geoLayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		geoLayoutInfo.bindingCount = 6;
		geoLayoutInfo.pBindings = geoBindings;
		geoLayoutInfo.flags = 0;
		geoLayoutInfo.pNext = nullptr;

		VkDescriptorSetLayout geoDescriptorSetLayout;
		result = vkCreateDescriptorSetLayout(m_pVulkanDevice->Device(), &geoLayoutInfo, nullptr, &m_layouts[2]);

		VkPushConstantRange ranges[2];

		ranges[0].offset = 0;
		ranges[0].size = sizeof(uint32_t) * 8;
		ranges[0].stageFlags = VK_SHADER_STAGE_TASK_BIT_NV;
		ranges[1].offset = 0;
		ranges[1].size = sizeof(uint32_t) * 4;
		ranges[1].stageFlags = VK_SHADER_STAGE_MESH_BIT_NV;

		VkPipelineLayoutCreateInfo layoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		layoutCreateInfo.setLayoutCount = 3;
		layoutCreateInfo.pSetLayouts = m_layouts;
		layoutCreateInfo.pushConstantRangeCount = 2;
		layoutCreateInfo.pPushConstantRanges = ranges;
		layoutCreateInfo.flags = 0;

		result = vkCreatePipelineLayout(m_pVulkanDevice->Device(), &layoutCreateInfo, nullptr, &m_meshShaderPipelineLayouts[0]);

		// image pipeline
		layoutCreateInfo.setLayoutCount = 4;
		result = vkCreatePipelineLayout(m_pVulkanDevice->Device(), &layoutCreateInfo, nullptr, &m_meshShaderPipelineLayouts[1]);

		// setup poolsizes for each descriptorType
		std::array<VkDescriptorPoolSize, 2> scene_poolSizes = {};
		scene_poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		scene_poolSizes[0].descriptorCount = static_cast<uint32_t>(1);
		scene_poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		scene_poolSizes[1].descriptorCount = static_cast<uint32_t>(1);

		VkDescriptorPoolCreateInfo poolInfo = jsvk::init::descriptorPoolCreateInfo();
		poolInfo.poolSizeCount = static_cast<uint32_t>(scene_poolSizes.size());
		poolInfo.pPoolSizes = scene_poolSizes.data();

		poolInfo.maxSets = static_cast<uint32_t>(1);

		VkDescriptorPool sceneDescriptorPool;
		VK_CHECK(vkCreateDescriptorPool(m_pVulkanDevice->Device(), &poolInfo, nullptr, &sceneDescriptorPool));
		m_descriptorPools.push_back(sceneDescriptorPool);

		std::vector<VkDescriptorSetLayout> layouts(1, m_layouts[0]);
		sceneSets.resize(1);
		VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		allocInfo.descriptorPool = sceneDescriptorPool;
		allocInfo.descriptorSetCount = layouts.size();
		allocInfo.pSetLayouts = layouts.data();

		result = vkAllocateDescriptorSets(m_pVulkanDevice->Device(), &allocInfo, sceneSets.data());

		VkDescriptorBufferInfo offScreenBufferInfo = {};
		offScreenBufferInfo.buffer = m_uniformBuffer.m_pBuffer;
		offScreenBufferInfo.offset = (num_models16 + num_models32) * objectDynamicAlignment;
		offScreenBufferInfo.range = m_dynamicAlignment;

		VkWriteDescriptorSet uniformDescriptor = jsvk::init::writeDescriptorSet();
		uniformDescriptor.dstBinding = 0;
		uniformDescriptor.dstSet = sceneSets[0];
		uniformDescriptor.dstArrayElement = 0;
		uniformDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		uniformDescriptor.descriptorCount = 1;
		uniformDescriptor.pBufferInfo = &offScreenBufferInfo;

		vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &uniformDescriptor, 0, nullptr);

		offScreenBufferInfo = {};
		offScreenBufferInfo.buffer = m_uniformBuffer.m_pBuffer;
		offScreenBufferInfo.offset = (num_models16 + num_models32) * objectDynamicAlignment + 2 * m_dynamicAlignment;
		offScreenBufferInfo.range = sizeof(CullStats);

		uniformDescriptor = jsvk::init::writeDescriptorSet();
		uniformDescriptor.dstBinding = 1;
		uniformDescriptor.dstSet = sceneSets[0];
		uniformDescriptor.dstArrayElement = 0;
		uniformDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		uniformDescriptor.descriptorCount = 1;
		uniformDescriptor.pBufferInfo = &offScreenBufferInfo;

		vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &uniformDescriptor, 0, nullptr);

		// setup poolsizes for each descriptorType
		std::array<VkDescriptorPoolSize, 1> obj_poolSizes = {};
		obj_poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		obj_poolSizes[0].descriptorCount = static_cast<uint32_t>(1);

		poolInfo.poolSizeCount = static_cast<uint32_t>(obj_poolSizes.size());
		poolInfo.pPoolSizes = obj_poolSizes.data();

		poolInfo.maxSets = static_cast<uint32_t>(1);

		VkDescriptorPool objDescriptorPool;
		VK_CHECK(vkCreateDescriptorPool(m_pVulkanDevice->Device(), &poolInfo, nullptr, &objDescriptorPool));
		m_descriptorPools.push_back(objDescriptorPool);

		std::vector<VkDescriptorSetLayout> objlayouts(1, m_layouts[1]);
		objSets.resize(1);

		allocInfo.descriptorPool = objDescriptorPool;
		allocInfo.descriptorSetCount = objlayouts.size();
		allocInfo.pSetLayouts = objlayouts.data();

		result = vkAllocateDescriptorSets(m_pVulkanDevice->Device(), &allocInfo, objSets.data());

		offScreenBufferInfo = {};
		offScreenBufferInfo.buffer = m_uniformBuffer.m_pBuffer;
		offScreenBufferInfo.offset = 0;
		offScreenBufferInfo.range = sizeof(ObjectData);

		uniformDescriptor = jsvk::init::writeDescriptorSet();
		uniformDescriptor.dstBinding = 0;
		uniformDescriptor.dstSet = objSets[0];
		uniformDescriptor.dstArrayElement = 0;
		uniformDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		uniformDescriptor.descriptorCount = 1;
		uniformDescriptor.pBufferInfo = &offScreenBufferInfo;

		vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &uniformDescriptor, 0, nullptr);

		int num_tex = textures.size();
		if (num_tex > 0)
		{
			// setting up image sets
			// setup poolsizes for each descriptorType
			std::array<VkDescriptorPoolSize, 1> img_poolSizes = {};
			img_poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			img_poolSizes[0].descriptorCount = static_cast<uint32_t>(2);

			poolInfo.poolSizeCount = static_cast<uint32_t>(img_poolSizes.size());
			poolInfo.pPoolSizes = img_poolSizes.data();

			poolInfo.maxSets = static_cast<uint32_t>(2);

			VkDescriptorPool ImageDescriptorPool;
			VK_CHECK(vkCreateDescriptorPool(m_pVulkanDevice->Device(), &poolInfo, nullptr, &ImageDescriptorPool));
			m_descriptorPools.push_back(ImageDescriptorPool);

			std::vector<VkDescriptorSetLayout> imglayouts(num_tex, m_layouts[3]);
			imgSets.resize(num_tex);

			allocInfo.descriptorPool = ImageDescriptorPool;
			allocInfo.descriptorSetCount = imglayouts.size();
			allocInfo.pSetLayouts = imglayouts.data();

			result = vkAllocateDescriptorSets(m_pVulkanDevice->Device(), &allocInfo, imgSets.data());

			for (int i = 0; i < num_tex; ++i)
			{
				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = textures[i].view;
				imageInfo.sampler = textures[i].sampler;

				uniformDescriptor = jsvk::init::writeDescriptorSet();
				uniformDescriptor.dstBinding = 0;
				uniformDescriptor.dstSet = imgSets[i];
				uniformDescriptor.dstArrayElement = 0;
				uniformDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				uniformDescriptor.descriptorCount = 1;
				uniformDescriptor.pImageInfo = &imageInfo;

				vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &uniformDescriptor, 0, nullptr);
			}
		}

		// setup poolsizes for each descriptorType
		std::array<VkDescriptorPoolSize, 1> geo_poolSizes = {};
		geo_poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		geo_poolSizes[0].descriptorCount = static_cast<uint32_t>(6);

		poolInfo.poolSizeCount = static_cast<uint32_t>(geo_poolSizes.size());
		poolInfo.pPoolSizes = geo_poolSizes.data();

		poolInfo.maxSets = static_cast<uint32_t>(2);

		VkDescriptorPool geoDescriptorPool;
		VK_CHECK(vkCreateDescriptorPool(m_pVulkanDevice->Device(), &poolInfo, nullptr, &geoDescriptorPool));
		m_descriptorPools.push_back(geoDescriptorPool);

		std::vector<VkDescriptorSetLayout> geolayouts(2, m_layouts[2]);
		geoSets.resize(2);
		allocInfo.descriptorPool = geoDescriptorPool;
		allocInfo.descriptorSetCount = geolayouts.size();
		allocInfo.pSetLayouts = geolayouts.data();

		result = vkAllocateDescriptorSets(m_pVulkanDevice->Device(), &allocInfo, geoSets.data());

		// update descriptors
		VkDescriptorBufferInfo descBufferInfo = VkDescriptorBufferInfo();
		descBufferInfo.buffer = m_mainBuffer.m_pBuffer;
		descBufferInfo.offset = vboSize + aboSize + texboSize;
		descBufferInfo.range = descSize;

		VkWriteDescriptorSet descDescriptor = jsvk::init::writeDescriptorSet();
		descDescriptor.dstBinding = 0;
		descDescriptor.dstSet = geoSets[0];
		descDescriptor.dstArrayElement = 0;
		descDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descDescriptor.descriptorCount = 1;
		descDescriptor.pBufferInfo = &descBufferInfo;

		vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &descDescriptor, 0, nullptr);

		VkDescriptorBufferInfo primBufferInfo = VkDescriptorBufferInfo();
		primBufferInfo.buffer = m_mainBuffer.m_pBuffer;
		primBufferInfo.offset = vboSize + aboSize + texboSize + NVMeshlet::computeCommonAlignedSize(descSize);
		primBufferInfo.range = primSize;

		VkWriteDescriptorSet primDescriptor = jsvk::init::writeDescriptorSet();
		primDescriptor.dstBinding = 1;
		primDescriptor.dstSet = geoSets[0];
		primDescriptor.dstArrayElement = 0;
		primDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		primDescriptor.descriptorCount = 1;
		primDescriptor.pBufferInfo = &primBufferInfo;

		vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &primDescriptor, 0, nullptr);

		VkDescriptorBufferInfo vboBufferInfo = VkDescriptorBufferInfo();
		vboBufferInfo.buffer = m_mainBuffer.m_pBuffer;
		vboBufferInfo.offset = 0;
		vboBufferInfo.range = vboSize;

		VkWriteDescriptorSet vboDescriptor = jsvk::init::writeDescriptorSet();
		vboDescriptor.dstBinding = 2;
		vboDescriptor.dstSet = geoSets[0];
		vboDescriptor.dstArrayElement = 0;
		// vboDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		vboDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vboDescriptor.descriptorCount = 1;
		vboDescriptor.pBufferInfo = &vboBufferInfo;
		// vboDescriptor.pTexelBufferView = &vbobufferView;

		vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &vboDescriptor, 0, nullptr);

		VkDescriptorBufferInfo aboBufferInfo = VkDescriptorBufferInfo();
		aboBufferInfo.buffer = m_mainBuffer.m_pBuffer;
		aboBufferInfo.offset = vboSize;
		aboBufferInfo.range = aboSize;

		VkWriteDescriptorSet aboDescriptor = jsvk::init::writeDescriptorSet();
		aboDescriptor.dstBinding = 3;
		aboDescriptor.dstSet = geoSets[0];
		aboDescriptor.dstArrayElement = 0;
		// aboDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		aboDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		aboDescriptor.descriptorCount = 1;
		aboDescriptor.pBufferInfo = &aboBufferInfo;
		// aboDescriptor.pTexelBufferView = &abobufferView;

		vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &aboDescriptor, 0, nullptr);

		VkDescriptorBufferInfo iboBufferInfo = VkDescriptorBufferInfo();
		iboBufferInfo.buffer = m_mainBuffer.m_pBuffer;
		iboBufferInfo.offset = vboSize + aboSize + texboSize + NVMeshlet::computeCommonAlignedSize(descSize) + NVMeshlet::computeCommonAlignedSize(primSize);
		iboBufferInfo.range = vertexSize;

		VkWriteDescriptorSet iboDescriptor = jsvk::init::writeDescriptorSet();
		iboDescriptor.dstBinding = 4;
		iboDescriptor.dstSet = geoSets[0];
		iboDescriptor.dstArrayElement = 0;
		// iboDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		iboDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		iboDescriptor.descriptorCount = 1;
		// iboDescriptor.pTexelBufferView = &meshbufferView32;
		iboDescriptor.pBufferInfo = &iboBufferInfo;

		vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &iboDescriptor, 0, nullptr);

		if (texboSize > 0)
		{
			VkDescriptorBufferInfo texboBufferInfo = VkDescriptorBufferInfo();
			texboBufferInfo.buffer = m_mainBuffer.m_pBuffer;
			texboBufferInfo.offset = vboSize + aboSize;
			texboBufferInfo.range = texboSize;

			VkWriteDescriptorSet texboDescriptor = jsvk::init::writeDescriptorSet();
			texboDescriptor.dstBinding = 5;
			texboDescriptor.dstSet = geoSets[0];
			texboDescriptor.dstArrayElement = 0;
			texboDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			texboDescriptor.descriptorCount = 1;
			texboDescriptor.pBufferInfo = &texboBufferInfo;
			// aboDescriptor.pTexelBufferView = &abobufferView;

			vkUpdateDescriptorSets(m_pVulkanDevice->Device(), 1, &texboDescriptor, 0, nullptr);
		}

		// clean up
		free(vertexindices16);
		free(vertexindices32);
		free(descData);
		free(primData);

		return 1;
	}

	void ResourcesMS::createRenderPass()
	{

		VkFormat swapchainImageFormat = m_pPresenter->getImageFormat();

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapchainImageFormat;
		colorAttachment.samples = m_msaaSamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = jsvk::findDepthFormat(m_pVulkanDevice->m_pPhysicalDevice);
		depthAttachment.samples = m_msaaSamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve = {};
		colorAttachmentResolve.format = swapchainImageFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentResolveRef = {};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
		VkRenderPassCreateInfo renderPassInfo = jsvk::init::renderPassCreateinfo();
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VK_CHECK(vkCreateRenderPass(m_pVulkanDevice->Device(), &renderPassInfo, nullptr, &m_renderPasses[0].renderPass));
	}

	void ResourcesMS::createFramebuffer()
	{
		VkFormat depthFormat = jsvk::findDepthFormat(m_pVulkanDevice->m_pPhysicalDevice);

		jsvk::createImage(m_pVulkanDevice->Device(), m_pVulkanDevice->m_pPhysicalDevice, m_extent.width, m_extent.height, 1, m_msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_renderPasses[0].depth.image, m_renderPasses[0].depth.memory);
		m_renderPasses[0].depth.view = jsvk::createImageView(m_pVulkanDevice->Device(), m_renderPasses[0].depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		VkFormat colorFormat = m_pPresenter->getImageFormat();
		jsvk::createImage(m_pVulkanDevice->Device(), m_pVulkanDevice->m_pPhysicalDevice, m_extent.width, m_extent.height, 1, m_msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_renderPasses[0].color.image, m_renderPasses[0].color.memory);
		m_renderPasses[0].color.view = jsvk::createImageView(m_pVulkanDevice->Device(), m_renderPasses[0].color.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		jsvk::transitionImageLayout(m_pVulkanDevice->Device(), m_pVulkanDevice->m_commandPool, m_pVulkanDevice->m_pGraphicsQueue, m_renderPasses[0].color.image, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		uint32_t numImageViews = m_pPresenter->getImageViews().size();
		m_swapchainFramebuffers.resize(numImageViews);

		for (size_t i = 0; i < numImageViews; i++)
		{
			std::array<VkImageView, 3> attachments = {
				m_renderPasses[0].color.view,
				m_renderPasses[0].depth.view,
				m_pPresenter->getImageViews()[i]};

			VkFramebufferCreateInfo framebufferInfo = jsvk::init::framebufferCreateInfo();
			framebufferInfo.renderPass = m_renderPasses[0].renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = m_extent.width;
			framebufferInfo.height = m_extent.height;
			framebufferInfo.layers = 1;

			VK_CHECK(vkCreateFramebuffer(m_pVulkanDevice->Device(), &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]));
		}
	}

	void ResourcesMS::hotReloadPipeline()
	{
		if (m_meshShaderPipelines.size() > 0)
		{
			for (auto pipeline : m_meshShaderPipelines)
			{
				vkDestroyPipeline(m_pVulkanDevice->Device(), pipeline, nullptr);
			}
		}

		createPipeline();
	}

	void ResourcesMS::createPipeline()
	{

		VkPipelineShaderStageCreateInfo shaderStages[3];
		memset(shaderStages, 0, sizeof(shaderStages));

		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].pNext = 0;
		shaderStages[0].pName = "main";

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].pNext = 0;
		shaderStages[1].pName = "main";

		shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[2].pNext = 0;
		shaderStages[2].pName = "main";

		auto fragMeshletShaderCode = jsvk::readFile("jsvk/shaders/meshletFrag.spv");
		auto taskShaderCode = jsvk::readFile("jsvk/shaders/meshletTask.spv");
		auto meshShaderCode = jsvk::readFile("jsvk/shaders/meshletMesh.spv");

		auto fragMeshTextureShaderCode = jsvk::readFile("jsvk/shaders/meshletTextureFrag.spv");
		auto meshTextureShaderCode = jsvk::readFile("jsvk/shaders/meshletTextureMesh.spv");

		auto normalShaderCode = jsvk::readFile("jsvk/shaders/meshletNormalFrag.spv");
		auto meshletColorShaderCode = jsvk::readFile("jsvk/shaders/meshletMeshletColorWireFrag.spv");
		// auto meshletColorShaderCode = jsvk::readFile("jsvk/shaders/meshletMeshletColorFrag.spv");

		VkShaderModule taskShaderModule = jsvk::createShaderModule(m_pVulkanDevice->Device(), taskShaderCode);
		VkShaderModule meshShaderModule = jsvk::createShaderModule(m_pVulkanDevice->Device(), meshShaderCode);
		VkShaderModule meshFragShaderModule = jsvk::createShaderModule(m_pVulkanDevice->Device(), fragMeshletShaderCode);
		VkShaderModule meshTextureShaderModule = jsvk::createShaderModule(m_pVulkanDevice->Device(), meshTextureShaderCode);
		VkShaderModule meshTextureFragShaderModule = jsvk::createShaderModule(m_pVulkanDevice->Device(), fragMeshTextureShaderCode);
		VkShaderModule normalShaderCodeModule = jsvk::createShaderModule(m_pVulkanDevice->Device(), normalShaderCode);
		VkShaderModule meshletColorShaderCodeModule = jsvk::createShaderModule(m_pVulkanDevice->Device(), meshletColorShaderCode);

		VkGraphicsPipelineCreateInfo pipelineInfo = jsvk::init::graphicsPipelineCreateInfo();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = jsvk::init::pipelineVertexinputStateCreateInfo();

		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		auto bindingDescription_headless = Vertex::getBindingDescrpition();
		auto attributeDescriptions_headless = Vertex::getAttributeDescriptions();

		// task, mesh and fragment shaders are used
		if (useTask)
		{
			pipelineInfo.stageCount = 3;
			shaderStages[0].stage = VK_SHADER_STAGE_TASK_BIT_NV;
			shaderStages[0].module = taskShaderModule;

			shaderStages[1].stage = VK_SHADER_STAGE_MESH_BIT_NV;
			shaderStages[1].module = meshTextureShaderModule;

			shaderStages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStages[2].module = meshTextureFragShaderModule;
		}
		else
		{
			pipelineInfo.stageCount = 2;

			shaderStages[0].stage = VK_SHADER_STAGE_MESH_BIT_NV;
			shaderStages[0].module = meshTextureShaderModule;

			shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStages[1].module = meshTextureFragShaderModule;
		}

		// Create static state info for the pipeline.
		VkVertexInputBindingDescription bindingDescription[2];
		bindingDescription[0].stride = sizeof(float) * 3; // vertex stride
		bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDescription[0].binding = 0;
		bindingDescription[1].stride = sizeof(float) * 5;
		bindingDescription[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bindingDescription[1].binding = 1;

		int m_extraAttributes = 0;
		attributeDescriptions.resize(2 + m_extraAttributes); // add extra attributes
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].binding = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = 0;
		for (uint32_t i = 0; i < m_extraAttributes; i++)
		{
			attributeDescriptions[2 + i].location = 2 + i;
			attributeDescriptions[2 + i].binding = 1;
			attributeDescriptions[2 + i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[2 + i].offset = sizeof(float) * 3;
		}

		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = bindingDescription;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = jsvk::init::pipelineInputAssemblyStateCreateInfo();
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_extent.width;
		viewport.height = (float)m_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = {0, 0};
		scissor.extent.width = m_extent.width;
		scissor.extent.height = m_extent.height;

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR};

		VkPipelineDynamicStateCreateInfo dynamicState = jsvk::init::pipelineDynamicStateCreateInfo();
		dynamicState.dynamicStateCount = dynamicStateEnables.size();
		dynamicState.pDynamicStates = dynamicStateEnables.data();

		VkPipelineViewportStateCreateInfo viewportState = jsvk::init::pipelineViewportStateCreateInfo();
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = jsvk::init::pipelineRasterizationStateCreateInfo();
		rasterizer.depthClampEnable = VK_FALSE;

		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

		rasterizer.lineWidth = 1.0f;

		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		// rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f;		   // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f;	   // Optional

		VkPipelineMultisampleStateCreateInfo multisampling = jsvk::init::pipelineMultisampleStateCreateInfo();
		// feature is not enabled so sampleshadingenable must be false
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = m_msaaSamples;
		multisampling.minSampleShading = 0.2f;			// Optional
		multisampling.pSampleMask = nullptr;			// Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE;		// Optional

		VkPipelineDepthStencilStateCreateInfo depthStencil = jsvk::init::pipelineDepthStencilStateCreateInfo();
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;	 // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;			 // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;	 // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;			 // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending = jsvk::init::pipelineColorBlendStateCreateInfo();
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		// example of push constants
		// VkPushConstantRange pushRange = jsvk::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0);

		pipelineInfo.layout = m_meshShaderPipelineLayouts[1];

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		pipelineInfo.renderPass = m_renderPasses[0].renderPass;

		// TODO refactor these creations into one call
		VK_CHECK(vkCreateGraphicsPipelines(m_pVulkanDevice->Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_meshShaderPipelines[1]));

		if (useTask)
		{
			shaderStages[1].stage = VK_SHADER_STAGE_MESH_BIT_NV;
			shaderStages[1].module = meshShaderModule;

			shaderStages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStages[2].module = meshFragShaderModule;
		}
		else
		{
			shaderStages[0].stage = VK_SHADER_STAGE_MESH_BIT_NV;
			shaderStages[0].module = meshShaderModule;

			shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStages[1].module = meshFragShaderModule;
		}
		VK_CHECK(vkCreateGraphicsPipelines(m_pVulkanDevice->Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_meshShaderPipelines[0]));
		if (useTask)
		{
			shaderStages[1].stage = VK_SHADER_STAGE_MESH_BIT_NV;
			shaderStages[1].module = meshShaderModule;

			shaderStages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStages[2].module = normalShaderCodeModule;
		}
		else
		{
			shaderStages[0].stage = VK_SHADER_STAGE_MESH_BIT_NV;
			shaderStages[0].module = meshShaderModule;

			shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStages[1].module = normalShaderCodeModule;
		}
		VK_CHECK(vkCreateGraphicsPipelines(m_pVulkanDevice->Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_meshShaderPipelines[2]));
		if (useTask)
		{
			shaderStages[1].stage = VK_SHADER_STAGE_MESH_BIT_NV;
			shaderStages[1].module = meshShaderModule;

			shaderStages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStages[2].module = meshletColorShaderCodeModule;
		}
		else
		{
			shaderStages[0].stage = VK_SHADER_STAGE_MESH_BIT_NV;
			shaderStages[0].module = meshShaderModule;

			shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStages[1].module = meshletColorShaderCodeModule;
		}
		VK_CHECK(vkCreateGraphicsPipelines(m_pVulkanDevice->Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_meshShaderPipelines[3]));

		vkDestroyShaderModule(m_pVulkanDevice->Device(), meshFragShaderModule, nullptr);
		vkDestroyShaderModule(m_pVulkanDevice->Device(), meshShaderModule, nullptr);
		vkDestroyShaderModule(m_pVulkanDevice->Device(), taskShaderModule, nullptr);
		vkDestroyShaderModule(m_pVulkanDevice->Device(), meshTextureShaderModule, nullptr);
		vkDestroyShaderModule(m_pVulkanDevice->Device(), meshTextureFragShaderModule, nullptr);
		vkDestroyShaderModule(m_pVulkanDevice->Device(), normalShaderCodeModule, nullptr);
		vkDestroyShaderModule(m_pVulkanDevice->Device(), meshletColorShaderCodeModule, nullptr);
	}
}