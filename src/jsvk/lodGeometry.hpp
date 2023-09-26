#pragma once

// Internal includes
#include "structures.h"
#include "geometryProcessing.h"

// Std library includes
#include <vector>

// GEL includes
#include <GEL/HMesh/HMesh.h>

// External includes
#include <glm/glm.hpp>

namespace lod
{ // struct Scene
    // The Builder responsible for generating the vertex buffer and index buffer for the models in the scene
    struct VertexBufferBuilder
    {
        // lets get this from somewhere else so that we can set it in settings
        glm::vec3 rotation = glm::vec3{1.0f, 1.0f, 1.0f};

        float rotationAngle = 0.0f;
        glm::vec3 rotationAxis = {0.0f, 0.0f, 0.0f};

        glm::vec3 scale = glm::vec3{1.0f, 1.0f, 1.0f};
        glm::vec3 translation = glm::vec3{1.0f, 1.0f, 1.0f};

        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        HMesh::Manifold manifold;

        float simplificationError = -999.0f;

        std::unordered_map<Vertex, uint32_t>
            uniqueVertices{}; // Stores the unique vertices and their index
        // std::unordered_map<uint32_t, uint32_t> uniqueIndices{}; // Stores the indices and their unique vertex

        void loadObjFile(const std::string &modelPath); // This is the exact same as the loadTinyModel method of loading however it returns a builder object with the vertices and indices
        void loadSerializedModel(std::string name);
        void serialize(std::string name);
        void deserialize(std::string name);
        void synchMaps(); //

        void clear()
        {
            rotation = glm::vec3(1.0f);
            rotationAngle = 0.0f;
            scale = glm::vec3(1.0f);
            translation = glm::vec3(1.0f);

            vertices.clear();
            indices.clear();
            manifold.clear();
            uniqueVertices.clear();
        }

        void replace(VertexBufferBuilder other)
        {
            vertices = other.vertices;
            indices = other.indices;
            manifold = other.manifold;
            uniqueVertices = other.uniqueVertices;
            simplificationError = other.simplificationError;
        }

    }; // struct Builder

    VertexBufferBuilder createModelFromFile(const std::string &filePath, VertexBufferBuilder &builder);
    VertexBufferBuilder createSimplifiedModel(VertexBufferBuilder model, float reducePercentage, float edgeThreshold, float maxError);
    VertexBufferBuilder createUnifiedModel(VertexBufferBuilder completeModel, VertexBufferBuilder removeModel);
    std::vector<VertexBufferBuilder> createClusterModel(NVMeshlet::Builder<uint32_t>::MeshletGeometry &geometry, VertexBufferBuilder &modelBuilder, int INITIAL_CLUSTER = 0, int GROUP_NO_CLUSTERS = 1, bool MAKE_REMAINDER = false);

    // Helper Methods
    VertexBufferBuilder createModelFromVertexIndex(std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::string name = "No Name");

    // float thresholds[] = {1.0f, 2.0f, 3.0f, 4.0f}; // if distance to cam is in between thresholds[0] & thresholds [1] then the desired lod for the mesh should be 0

    struct Meshlet
    {
        int lod = 0;          // The LoD of the simplified model that the meshlet belongs to
        int index = 0;        // The index of the meshlet in the meshlet list of the mesh
        int no_triangles = 0; // The number of triangles in the meshlet

        // The AABB of the meshlet
        glm::vec3 minPoint = glm::vec3(0.0f);
        glm::vec3 maxPoint = glm::vec3(0.0f);

        // The center of the meshlet
        glm::vec3 center = glm::vec3(0.0f);

        // int id = 0;

        // TODO: this should be calculated using the bb or something else
        float distance = 0.0f; // The distance from one of the vertices of the meshlet to the camera

        bool visible = false;

        std::vector<mm::Vertex> vertices; // The list of unique vertices for this meshlet
        std::vector<uint32_t> indices;    // The list of indices for this meshlet

        lod::VertexBufferBuilder builder; // The builder used to create the meshlet

    }; // struct Meshlet

    struct Mesh
    {
        float simplificationError = 0.0f;   // The world space error that the mesh was simplified to
        std::vector<lod::Meshlet> meshlets; // The meshlets making up the mesh

        // int id = 0;
        int lod = 0;
        int no_triangles = 0;

        std::vector<mm::Vertex> vertices;
        std::vector<uint32_t> indices;

        glm::vec3 center;

        lod::VertexBufferBuilder builder; // The builder used to create the mesh

        std::string file_path;
        std::string name; // The name of the mesh without the file extension

        // Needed for the creation of the meshlets step
        std::unordered_map<unsigned int, mm::Vert *>
            indexVertexMap;
        std::vector<mm::MeshletCache<uint32_t>> meshletCache{};
        std::vector<mm::Triangle *> triangles;
        std::vector<NVMeshlet::Stats> stats;
        std::vector<ObjectData> objectData;
        NVMeshlet::Builder<uint32_t>::MeshletGeometry packedMeshlets;

    }; // struct Mesh

    struct Model
    {
        // This is the offsets into each of the LoDs
        std::vector<uint32_t> desc_offsets{};
        std::vector<uint32_t> prim_offsets{};
        std::vector<uint32_t> vert_offsets{};
        std::vector<uint32_t> vbo_offsets{};

        std::vector<uint32_t> desc_counts{};

        std::vector<int> no_triangles{};

        // All the code below is for the meshlet generation and culling but should be refactored into a separate class at some point in the future
        // lod::VertexBufferBuilder builder;
        std::vector<NVMeshlet::Builder<uint32_t>::MeshletGeometry> meshletGeometry32{};
        std::vector<NVMeshlet::Builder<uint16_t>::MeshletGeometry> meshletGeometry;

        std::vector<NVMeshlet::Stats> stats;
        std::vector<uint32_t> vertCount{0};
        std::vector<mm::Vertex> vertices;
        std::vector<ObjectData> objectData;
        std::vector<uint32_t> indices;

        // int no_triangles = 0;

    }; // struct Model

    // This hash is similar to the one found here:
    struct vec3_hash
    {
        std::size_t operator()(const glm::vec3 &v) const
        {
            std::hash<float> hash_fn;
            return ((hash_fn(v.x) ^ (hash_fn(v.y) << 1)) >> 1) ^ (hash_fn(v.z) << 1);
        }
    };

    struct BoundingBox
    {
        glm::vec3 minPoint = glm::vec3(0.0f);
        glm::vec3 maxPoint = glm::vec3(0.0f);

        bool isContained(const BoundingBox &parent, const BoundingBox &child)
        {
            // Check if any part of the child's bounding box is inside or touching the parent's bounding box.
            return !(child.maxPoint.x < parent.minPoint.x || child.minPoint.x > parent.maxPoint.x ||
                     child.maxPoint.y < parent.minPoint.y || child.minPoint.y > parent.maxPoint.y ||
                     child.maxPoint.z < parent.minPoint.z || child.minPoint.z > parent.maxPoint.z);

            // If the child's bounding box is completely inside the parent's bounding box then return true
            return (parent.minPoint.x <= child.minPoint.x && parent.minPoint.y <= child.minPoint.y && parent.minPoint.z <= child.minPoint.z) &&
                   (parent.maxPoint.x >= child.maxPoint.x && parent.maxPoint.y >= child.maxPoint.y && parent.maxPoint.z >= child.maxPoint.z);
        }
    }; // struct BoundingBox

    class Graph
    {
    public:
        struct Node
        {
            int lod; // The level of detail of the meshlet (0 is the highest level of detail)
            uint16_t id;
            int meshIndex = 0;    // This is the index of the game object from the list of meshes
            int meshletIndex = 0; // This is the index of the meshlet from the LoD offset

            std::unordered_map<uint16_t, Node *> children;

            // The AABB of the meshlet
            std::vector<mm::Vertex> vertices{};
            BoundingBox bb;

            glm::vec3 center = glm::vec3(0.0f); // Used to calculate distance

            int no_triangles = 0;
        }; // struct Node

        struct DAG
        {
            // The roots are at nodes[MAX_LOD] and have directions all the way to LoD 0 however this approach is memory intensive
            std::unordered_map<int, std::vector<lod::Graph::Node>> nodes; // This is the LOD, node mapping
        };                                                                // struct Graph

    }; // class GraphBuilder

    struct World // can't call this scene because Jinsoku's update now has a scene
    {
        std::vector<float> simplification_errors; // The list of simplification errors for each of the LoDs in world space
        std::vector<std::string> mesh_paths;      // The paths to the meshes in the scene

        std::vector<Mesh> meshes; // The list of meshes in the scene

        lod::Model model; // This is for some reason needed in Jinsoku

        std::vector<std::string> errors; // used to keep track of errors that happen in parallelized functions

        int lowestLod = 0; // The lowest lod that could be used in the scene

        std::vector<jsvk::GameObject> gameObjects; // The list of game objects in the scene

        lod::Graph::DAG DAG; // A DAG of a ll the meshlets going from LOD_MAX to LoD 0

        glm::vec3 center = glm::vec3(0.0f); // The center of the scene

        std::vector<glm::vec3> mesh_centers{};
    };

}