#pragma once
#ifndef VK_STRUCTURES_H
#define VK_STRUCTURES_H

#define GLM_ENABLE_EXPERIMENTAL
#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtx/hash.hpp>

#include <array>

#include "jsvkTexture.h"

namespace mm
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		bool operator==(const Vertex &other) const
		{
			return pos == other.pos && color == other.color && texCoord == other.texCoord;
		}

		glm::vec3 operator-(const Vertex &other) const
		{
			return glm::vec3(pos.x - other.pos.x, pos.y - other.pos.y, pos.z - other.pos.z);
		}

		float euclideanDistance(const Vertex &other) const
		{
			return std::sqrt(std::pow(other.pos.x - pos.x, 2) + std::pow(other.pos.y - pos.y, 2) + std::pow(other.pos.z - pos.z, 2));
		}
	};

	// must not change
	typedef uint8_t PrimitiveIndexType; // must store [0,MAX_VERTEX_COUNT_LIMIT-1]
	static const int MAX_VERTEX_COUNT_LIMIT = 256;
	static const int MAX_PRIMITIVE_COUNT_LIMIT = 256;

	template <class VertexIndexType>
	struct MeshletCache
	{
		PrimitiveIndexType primitives[MAX_PRIMITIVE_COUNT_LIMIT][3];
		uint32_t vertices[MAX_VERTEX_COUNT_LIMIT]; // this is the actual index buffer
		uint32_t numPrims;
		uint32_t numVertices;
		Vertex actualVertices[MAX_VERTEX_COUNT_LIMIT];

		// funky version!
		uint32_t numVertexDeltaBits;
		uint32_t numVertexAllBits;

		uint32_t primitiveBits = 1;
		uint32_t maxBlockBits = ~0;

		bool empty() const { return numVertices == 0; }

		void reset()
		{
			numPrims = 0;
			numVertices = 0;
			numVertexDeltaBits = 0;
			numVertexAllBits = 0;

			memset(vertices, 0xFFFFFFFF, sizeof(vertices));
			memset(actualVertices, 0x00000000, sizeof(actualVertices));
		}

		bool fitsBlock() const
		{
			uint32_t primBits = (numPrims - 1) * 3 * primitiveBits;
			uint32_t vertBits = (numVertices - 1) * numVertexDeltaBits;
			bool state = (primBits + vertBits) <= maxBlockBits;

			return state;
		}

		// check if cache can hold one more triangle
		bool cannotInsert(const VertexIndexType *indices, uint32_t maxVertexSize, uint32_t maxPrimitiveSize) const
		{
			// skip degenerate
			if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
			{
				return false;
			}

			uint32_t found = 0;

			// check if any of the incoming three indices are already in cache
			for (uint32_t v = 0; v < numVertices; ++v)
			{
				for (int i = 0; i < 3; ++i)
				{
					uint32_t idx = indices[i];
					if (vertices[v] == idx)
					{
						found++;
					}
				}
			}
			// out of bounds
			return (numVertices + 3 - found) > maxVertexSize || (numPrims + 1) > maxPrimitiveSize;
		}

		bool cannotInsertBlock(const VertexIndexType *indices, uint32_t maxVertexSize, uint32_t maxPrimitiveSize) const
		{
			// skip degenerate
			if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
			{
				return false;
			}

			uint32_t found = 0;

			// check if any of the incoming three indices are already in cache
			for (uint32_t v = 0; v < numVertices; ++v)
			{
				for (int i = 0; i < 3; ++i)
				{
					uint32_t idx = indices[i];
					if (vertices[v] == idx)
					{
						found++;
					}
				}
			}

			uint32_t firstVertex = numVertices ? vertices[0] : indices[0];
			uint32_t cmpBits = std::max(findMSB((firstVertex ^ indices[0]) | 1),
										std::max(findMSB((firstVertex ^ indices[1]) | 1), findMSB((firstVertex ^ indices[2]) | 1))) +
							   1;

			uint32_t deltaBits = std::max(cmpBits, numVertexDeltaBits);

			uint32_t newVertices = numVertices + 3 - found;
			uint32_t newPrims = numPrims + 1;

			uint32_t newBits;

			{
				uint32_t newVertBits = (newVertices - 1) * deltaBits;
				uint32_t newPrimBits = (newPrims - 1) * 3 * primitiveBits;
				newBits = newVertBits + newPrimBits;
			}

			// out of bounds
			return (numVertices + 3 - found) > maxVertexSize || (numPrims + 1) > maxPrimitiveSize;
		}

		// insert new triangle
		void insert(const VertexIndexType *indices, const Vertex *verts)
		{
			uint32_t triangle[3];

			// skip degenerate
			if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
			{
				return;
			}

			for (int i = 0; i < 3; ++i)
			{
				// take out an index
				uint32_t idx = indices[i];
				bool found = false;

				// check if idx is already in cache
				for (uint32_t v = 0; v < numVertices; ++v)
				{
					if (idx == vertices[v])
					{
						triangle[i] = v;
						found = true;
						break;
					}
				}
				// if idx is not in cache add it
				if (!found)
				{
					vertices[numVertices] = idx;
					actualVertices[numVertices] = verts[idx];
					triangle[i] = numVertices;

					if (numVertices)
					{
						numVertexDeltaBits = std::max(findMSB((idx ^ vertices[0]) | 1) + 1, numVertexDeltaBits);
					}
					numVertexAllBits = std::max(numVertexAllBits, findMSB(idx) + 1);

					numVertices++;
				}
			}

			primitives[numPrims][0] = triangle[0];
			primitives[numPrims][1] = triangle[1];
			primitives[numPrims][2] = triangle[2];
			numPrims++;

			assert(fitsBlock());
		}
	};

	struct Vert;

	struct Triangle
	{
		std::vector<Vert *> vertices;
		std::vector<Triangle *> neighbours;
		float centroid[3]{};
		uint32_t id;
		uint32_t flag = -1;
		uint32_t dist;
	};

	struct Vert
	{
		std::vector<Triangle *> neighbours;
		unsigned int index;
		unsigned int degree;
	};
}

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 texCoord;

	bool operator==(const Vertex &other) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}

	static VkVertexInputBindingDescription getBindingDescrpition()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
		// Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		// color
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);
		// texture coordinates
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
};

struct Vertex_noTex
{
	glm::vec3 pos;
	glm::vec3 color;

	bool operator==(const Vertex_noTex &other) const
	{
		return pos == other.pos && color == other.color;
	}

	static VkVertexInputBindingDescription getBindingDescrpition()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex_noTex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
		// Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex_noTex, pos);
		// color
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex_noTex, color);
		// texture coordinates

		return attributeDescriptions;
	}
};

struct CullStats
{
	glm::uint tasksInput;
	glm::uint tasksOutput;
	glm::uint meshletsInput;
	glm::uint meshletsOutput;
	glm::uint trisInput;
	glm::uint trisOutput;
	glm::uint attrInput;
	glm::uint attrOutput;
};

// must match cadscene!
struct ObjectData
{
	glm::mat4 worldMatrix;
	glm::mat4 worldMatrixIT;
	glm::mat4 objectMatrix;
	glm::vec4 bboxMin;
	glm::vec4 bboxMax;
	glm::vec3 _pad0;
	float winding;
	glm::vec4 color;
};

#define NUM_CLIPPING_PLANES 3

struct SceneData
{
	glm::mat4 viewProjMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 viewMatrixIT;

	glm::vec4 viewPos;
	glm::vec4 viewDir;

	glm::vec4 wLightPos;

	glm::ivec2 viewport;
	glm::vec2 viewportf;

	glm::vec2 viewportTaskCull;
	int colorize;
	int _pad0;

	glm::vec4 wClipPlanes[NUM_CLIPPING_PLANES];
};

namespace jsvk
{

	struct GameObject
	{
		// Added for LOD testing
		float distance = 0.0f;
		glm::vec3 center{0.0f};

		bool render = true;

		uint32_t desc_count;
		uint32_t desc_offset = 0;
		uint32_t prim_offset = 0;
		uint32_t vert_offset = 0;
		uint32_t vbo_offset = 0;
		VkCommandPool m_commandPool;
		std::vector<VkCommandBuffer> m_commandBuffers;
		int ObjectOffset;
		uint32_t shorts = 0;
		int pipelineID = 0;
	};

	struct Scene
	{
		std::vector<GameObject> gameObjects;
		SceneData sceneData;

		int lowestLOD = 0;
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct FrameBufferAttachment
	{
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
	}; // image for color and depth

	struct RenderPass
	{
		int32_t width = 0, height = 0;
		VkFramebuffer frameBuffer = VK_NULL_HANDLE;
		FrameBufferAttachment color, depth;
		VkRenderPass renderPass = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
		VkDescriptorImageInfo descriptor;
	};

	struct Mesh : public GameObject
	{
		std::vector<VkDescriptorSet> descriptorsets;
		uint32_t dynamicAlignment;
		uint32_t mesh_size;
		uint32_t mesh_offset;
		jsvk::Texture texture;

		void Draw(const VkCommandBuffer &pCommandBuffer, const VkPipelineLayout &pPipelineLayout, uint32_t dynamicAlignment)
		{
			vkCmdBindDescriptorSets(pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineLayout, 0, descriptorsets.size(), descriptorsets.data(), 1, &dynamicAlignment);
			vkCmdDrawIndexed(pCommandBuffer, mesh_size, 1, mesh_offset, 0, 0);
		}
	};

	// pr shader resources
	struct Shader
	{
		VkPipeline *pipeline;
		VkPipelineLayout *pipelineLayout;
		std::vector<Mesh> meshes;
		int num_meshes;
	};

	struct DynamicUniformBufferObject
	{
		glm::mat4 model;
	};

}

struct MeshInfo : public jsvk::GameObject
{
	uint32_t desc_offset = 0;
	uint32_t prim_offset = 0;
	uint32_t vert_offset = 0;
	uint32_t vbo_offset = 0;
	uint32_t desc_count;
	uint32_t shorts = 0;
	int pipelineID = 0;
};

// how about a struct that inherits
// so we have simple case of vertex
// and then pile on

/*
struct Vertex {
	glm::vec3 m_pos

	virtual bool operator==(const Vertex& other) const {
		return (m_pos == other.pos);
	}
}
*/

namespace std
{
	template <>
	struct hash<Vertex>
	{
		size_t operator()(Vertex const &vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
					 (hash<glm::vec3>()(vertex.color) << 1)) >>
					1) ^
				   (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
	template <>
	struct hash<Vertex_noTex>
	{
		size_t operator()(Vertex_noTex const &vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
					 (hash<glm::vec3>()(vertex.color) << 1)) >>
					1);
		}
	};
}

namespace std
{
	template <>
	struct hash<mm::Vertex>
	{
		size_t operator()(mm::Vertex const &vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
					 (hash<glm::vec3>()(vertex.color) << 1)) >>
					1) ^
				   (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

#endif // VK_STRUCTURES_H