// internal includes
#include "lodGeometry.hpp"
#include "structures.h"

// std library includes
#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <queue>

// GEL library includes
#include <GEL/HMesh/HMesh.h>
#include <GEL/Geometry/Load.h>

// external includes
#include <glm/glm.hpp>
#include "tiny_obj_loader.h"

extern bool debug;

bool SHOW_MESSAGES = false;

// The Builder Code
namespace lod
{

    VertexBufferBuilder createModelFromFile(const std::string &filePath, VertexBufferBuilder &builder)
    {
        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
            std::cout << "Loading Model: " << filePath << std::endl;
        }

        builder.loadObjFile(filePath);

        VertexBufferBuilder builtBuilder = createModelFromVertexIndex(builder.vertices, builder.indices, "Loaded");

        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
        }
        return builtBuilder;
    }

    std::vector<VertexBufferBuilder> createClusterModel(NVMeshlet::Builder<uint32_t>::MeshletGeometry &geometry, VertexBufferBuilder &modelBuilder, int INITIAL_CLUSTER, int GROUP_NO_CLUSTERS, bool MAKE_REMAINDER)
    {
        VertexBufferBuilder builder{};
        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
            std::cout << "Creating Builder Model from Meshlet Geometry" << std::endl;
        }

        std::vector<Vertex> vertices = modelBuilder.vertices;
        std::vector<uint32_t> indices = modelBuilder.indices;

        HMesh::Manifold tempMani = modelBuilder.manifold;

        if (SHOW_MESSAGES)
        {
            if (HMesh::valid(tempMani))
            {
                std::cout << "Original Mesh is a Valid Manifold " << std::endl;
            }
            else
            {
                std::cout << "Original Mesh is NOT a Valid Manifold! " << std::endl;
            }

            std::cout << "Original Mesh Manifold Vertex count: " << tempMani.no_vertices() << std::endl;
            std::cout << "Original Mesh Manifold Faces count: " << tempMani.no_faces() << std::endl;
            std::cout << "Original Mesh Manifold Edges count: " << tempMani.no_halfedges() << std::endl;
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertexes{};

        std::vector<Vertex> tempVertices{};

        if (debug && SHOW_MESSAGES)
        {
            std::cout << std::endl;
            std::cout << "Making Patch from index buffer" << std::endl;

            std::cout << "Indices "
                      << "total=" << indices.size() << " faces=" << indices.size() / 3 << " : {";
            for (auto i : indices)
            {
                std::cout << " " << i;
            }
            std::cout << " }" << std::endl;

            std::cout << "vertices "
                      << "total=" << vertices.size() << " : {";
            for (auto i : vertices)
            {
                std::cout << " " << i.pos.x << " " << i.pos.y << " " << i.pos.z << " | ";
            }
            std::cout << " }" << std::endl;
        }

        // Used for the left over model
        std::unordered_map<Vertex, uint32_t> uniqueVertices{}; // Stores the unique vertices and their index
        std::vector<Vertex> tVertices{};
        std::vector<Vertex> verts{};
        std::vector<uint32_t> indxes{};
        bool finished = false;
        bool started = false;
        bool found = false;
        int processedClusters = 0;
        int triangleCount = 0;
        int size = geometry.meshletDescriptors.size();
        uniqueVertexes.clear();
        tempVertices.clear();
        vertices.clear();
        indices.clear();
        builder.vertices.clear();
        builder.indices.clear();

        if (SHOW_MESSAGES)
        {
            std::cout << std::endl;
            std::cout << "Inital cluster " << INITIAL_CLUSTER << " of " << size << std::endl;
            std::cout << "Grouping " << GROUP_NO_CLUSTERS << " clusters" << std::endl;
        }

        if (GROUP_NO_CLUSTERS > geometry.meshletDescriptors.size() || GROUP_NO_CLUSTERS <= 0)
        {
            GROUP_NO_CLUSTERS = 1;
        }

        // The different clusters
        for (size_t i = 0; i < geometry.meshletDescriptors.size(); i++)
        {
            // This chooses different clusters as starting points
            if (i == INITIAL_CLUSTER)
            {
                started = true;
            }

            NVMeshlet::MeshletDesc &meshlet = geometry.meshletDescriptors[i];

            uint32_t primCount = meshlet.getNumPrims();
            uint32_t vertexCount = meshlet.getNumVertices();

            uint32_t primBegin = meshlet.getPrimBegin();
            uint32_t vertexBegin = meshlet.getVertexBegin();

            // skip unset
            if (vertexCount == 1)
                continue;

            if (SHOW_MESSAGES)
            {
                if (debug)
                {
                    std::cout << std::endl;
                    std::cout << "Triangle Information uniqueVertices[" << vertexCount << "]" << std::endl;
                }
            }

            for (uint32_t p = 0; p < primCount; p++)
            {
                const uint32_t primStride = (NVMeshlet::PRIMITIVE_PACKING == NVMeshlet::NVMESHLET_PACKING_TRIANGLE_UINT32) ? 4 : 3;

                uint32_t idxA = geometry.primitiveIndices[primBegin + p * primStride + 0];
                uint32_t idxB = geometry.primitiveIndices[primBegin + p * primStride + 1];
                uint32_t idxC = geometry.primitiveIndices[primBegin + p * primStride + 2];

                idxA = geometry.vertexIndices[vertexBegin + idxA];
                idxB = geometry.vertexIndices[vertexBegin + idxB];
                idxC = geometry.vertexIndices[vertexBegin + idxC];

                // Get the position of the vertices
                Vertex vertexA;
                Vertex vertexB;
                Vertex vertexC;

                for (auto &[key, value] : modelBuilder.uniqueVertices)
                {
                    if (value == (idxA))
                    {
                        vertexA = key;
                    }
                    else if (value == idxB)
                    {
                        vertexB = key;
                    }
                    else if (value == idxC)
                    {
                        vertexC = key;
                    }
                } // this should be replaced with the vale key map using the sync method of the builder

                if ((!finished) && started)
                {
                    // Make a new vertex index buffer for the cluster
                    if (uniqueVertexes.count(vertexA) == 0)
                    {
                        uniqueVertexes[vertexA] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertexA);
                    }
                    if (uniqueVertexes.count(vertexB) == 0)
                    {
                        uniqueVertexes[vertexB] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertexB);
                    }
                    if (uniqueVertexes.count(vertexC) == 0)
                    {
                        uniqueVertexes[vertexC] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertexC);
                    }

                    tempVertices.push_back(vertexA);
                    tempVertices.push_back(vertexB);
                    tempVertices.push_back(vertexC);

                    processedClusters++;
                }
                else
                {
                    if (MAKE_REMAINDER)
                    {
                        // Make the vertex/index buffer for the rest of the model without the cluster made above
                        if (uniqueVertices.count(vertexA) == 0)
                        {
                            uniqueVertices[vertexA] = static_cast<uint32_t>(verts.size());
                            verts.push_back(vertexA);
                        }
                        if (uniqueVertices.count(vertexB) == 0)
                        {
                            uniqueVertices[vertexB] = static_cast<uint32_t>(verts.size());
                            verts.push_back(vertexB);
                        }
                        if (uniqueVertices.count(vertexC) == 0)
                        {
                            uniqueVertices[vertexC] = static_cast<uint32_t>(verts.size());
                            verts.push_back(vertexC);
                        }

                        tVertices.push_back(vertexA);
                        tVertices.push_back(vertexB);
                        tVertices.push_back(vertexC);
                    }
                }

                if (SHOW_MESSAGES)
                {
                    if (debug)
                    {
                        std::cout << "Triangle " << p << " : {"
                                  << " index " << idxA << " posA: [" << vertexA.pos.x << " | " << vertexA.pos.y << " | " << vertexA.pos.z << "]"
                                  << " "
                                  << " index " << idxB << " posB: [" << vertexB.pos.x << " | " << vertexB.pos.y << " | " << vertexB.pos.z << "]"
                                  << " "
                                  << " index " << idxC << " posC: [" << vertexC.pos.x << " | " << vertexC.pos.y << " | " << vertexC.pos.z << "]"
                                  << " "
                                  << " }" << std::endl;
                    }
                }
            }

            if (processedClusters >= GROUP_NO_CLUSTERS && GROUP_NO_CLUSTERS != 0)
            {
                finished = true;
            }
        }

        // Cluster index buffer
        for (auto v : tempVertices)
        {
            indices.push_back(uniqueVertexes[v]);
        }

        // The rest of the model index buffer
        for (auto v : tVertices)
        {
            indxes.push_back(uniqueVertices[v]);
        }

        if (SHOW_MESSAGES)
        {
            std::cout << std::endl;
            std::cout << "Patch created from meshlet geometry " << std::endl;
            if (debug)
            {
                std::cout << "Indices "
                          << "total=" << indices.size() << " faces=" << indices.size() / 3 << " : {";
                for (auto i : indices)
                {
                    std::cout << " " << i;
                }
                std::cout << " }" << std::endl;

                std::cout << "Vertices "
                          << "total=" << vertices.size() << " : {";
                for (auto i : vertices)
                {
                    std::cout << " " << i.pos.x << " " << i.pos.y << " " << i.pos.z << " | ";
                }
                std::cout << " }" << std::endl;
            }
        }

        std::vector<VertexBufferBuilder> builders{};

        VertexBufferBuilder clusterBuilder = lod::createModelFromVertexIndex(vertices, indices, "Cluster");
        builders.push_back(clusterBuilder);

        if ((verts.size() > 0) && (MAKE_REMAINDER))
        {
            VertexBufferBuilder restBuilder = lod::createModelFromVertexIndex(verts, indxes, "Left-Over");
            builders.push_back(restBuilder);
        }

        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
        }

        return builders;
    }

    VertexBufferBuilder createSimplifiedModel(VertexBufferBuilder model, float reducePercentage, float edgeThreshold, float maxError)
    {
        // No need to simplify if the desired amount of vertices to keep is more than 100%
        if (reducePercentage >= 1.00f)
        {
            return model;
        }

        VertexBufferBuilder builder{};
        builder.scale = model.scale;
        builder.translation = model.translation;
        builder.rotation = model.rotation;
        builder.rotationAngle = model.rotationAngle;
        builder.rotationAxis = model.rotationAxis;

        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
            std::cout << "Creating Lower LOD Model " << std::endl;
        }

        // Create the manifold
        HMesh::Manifold manifold = model.manifold;

        if (SHOW_MESSAGES)
        {
            if (HMesh::valid(manifold))
            {
                std::cout << "Original is a Valid Manifold " << std::endl;
            }
            else
            {
                std::cout << "Original is NOT a Valid Manifold! " << std::endl;
            }

            int num_verts = manifold.no_vertices();
            std::cout << "Original Manifold Vertex count: " << num_verts << std::endl;
            std::cout << "Original Manifold Faces count: " << manifold.no_faces() << std::endl;
            std::cout << "Original Manifold Edges count: " << manifold.no_halfedges() << std::endl;

            int max_work = std::max(0, int(num_verts - reducePercentage * num_verts));
            std::cout << "Attempting to reduce the number of vertices by maximum of " << max_work << std::endl;
        }

        builder.simplificationError = HMesh::quadric_simplify(manifold, reducePercentage, edgeThreshold, maxError);

        // manifold.cleanup();

        // Get the vertices and corresponding idex buffer from the manifold
        HMesh::IteratorPair<HMesh::IDIterator<HMesh::Face>> faces = manifold.faces();
        std::unordered_map<Vertex, uint32_t> uniqueVertexes{};
        std::vector<Vertex> tempVertices{};
        for (auto f : faces)
        {
            HMesh::Walker w = manifold.walker(f);
            do
            {
                HMesh::VertexID vID = w.vertex();
                HMesh::Manifold::Vec vec = manifold.pos(vID);
                HMesh::Manifold::Vec vecNorm;
                vecNorm = HMesh::normal(manifold, vID);

                Vertex vertex{};
                vertex.pos = glm::vec3(vec[0], vec[1], vec[2]);
                vertex.normal = glm::vec3(vecNorm[0], vecNorm[1], vecNorm[2]);
                vertex.color = (builder.translation * builder.rotation * builder.scale * vertex.normal);

                if (uniqueVertexes.count(vertex) == 0)
                {
                    uniqueVertexes[vertex] = static_cast<uint32_t>(builder.vertices.size());
                    builder.vertices.push_back(vertex);
                }

                tempVertices.push_back(vertex);

                w = w.next(); // Traverse inside the face to the next edge and vertex
            } while (!w.full_circle());
        }

        for (auto v : tempVertices)
        {
            builder.indices.push_back(uniqueVertexes[v]);
        }

        VertexBufferBuilder simplifiedBuilder = lod::createModelFromVertexIndex(builder.vertices, builder.indices, "Simplified");

        simplifiedBuilder.simplificationError = builder.simplificationError;

        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
        }

        return simplifiedBuilder;
    }

    VertexBufferBuilder createUnifiedModel(VertexBufferBuilder first, VertexBufferBuilder second)
    {
        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
            std::cout << "Creating Unified Model " << std::endl;

            std::cout << "Cluster Model" << std::endl;
            std::cout << "Manifold Vert Count: " << first.manifold.no_vertices() << std::endl;
            std::cout << "Manifold Edge Count: " << first.manifold.no_halfedges() << std::endl;
            std::cout << "Manifold Face Count: " << first.manifold.no_faces() << std::endl;

            std::cout << "Remaining Model" << std::endl;
            std::cout << "Manifold Vert Count: " << second.manifold.no_vertices() << std::endl;
            std::cout << "Manifold Edge Count: " << second.manifold.no_halfedges() << std::endl;
            std::cout << "Manifold Face Count: " << second.manifold.no_faces() << std::endl;
        }

        std::vector<Vertex> vertices_first = first.vertices;
        std::vector<uint32_t> indices_first = first.indices;

        std::vector<Vertex> vertices_second = second.vertices;
        std::vector<uint32_t> indices_second = second.indices;

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        std::vector<uint32_t> indices_unified{};
        std::vector<Vertex> vertices_unified{};

        std::vector<Vertex> tempVerts{};

        for (auto i : indices_first)
        {
            Vertex vertex = vertices_first[i];

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices_unified.size());
                vertices_unified.push_back(vertex);
            }

            tempVerts.push_back(vertex);
        }

        for (auto i : indices_second)
        {
            Vertex vertex = vertices_second[i];

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices_unified.size());
                vertices_unified.push_back(vertex);
            }

            tempVerts.push_back(vertex);
        }

        for (auto v : tempVerts)
        {
            indices_unified.push_back(uniqueVertices[v]);
        }

        VertexBufferBuilder unifiedBuilder = lod::createModelFromVertexIndex(vertices_unified, indices_unified, "Unified");

        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
        }
        return unifiedBuilder;
    }

    VertexBufferBuilder createModelFromVertexIndex(std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::string name)
    {
        VertexBufferBuilder builder{};
        if (SHOW_MESSAGES)
        {
            std::cout << std::endl;
            std::cout << "Creating Model from Index Vertex Buffer" << std::endl;
        }
        builder.indices = indices;
        builder.vertices = vertices;

        // Create the manifold
        HMesh::Manifold manifold;
        Geometry::TriMesh mesh;

        std::unordered_map<Vertex, uint32_t> uniqueVertices{}; // Stores the unique vertices and their index

        int indx = 0;
        int processed = 0;
        for (auto i : builder.indices)
        {
            Vertex vertex = builder.vertices[i];
            HMesh::Manifold::Vec v = HMesh::Manifold::Vec(vertex.pos.x, vertex.pos.y, vertex.pos.z);
            CGLA::Vec3f v3f = CGLA::Vec3f(v[0], v[1], v[2]);

            processed++;

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(i);
                mesh.geometry.add_vertex(v3f);
            }

            if (processed % 3 == 0)
            {
                mesh.geometry.add_face(CGLA::Vec3i(builder.indices[indx], builder.indices[indx + 1], builder.indices[indx + 2]));
                indx += 3;
            }
        }
        // Get normals from computed mesh normals
        mesh.compute_normals();
        for (int i = 0; i < mesh.normals.no_vertices(); i++)
        {
            CGLA::Vec3f v3f = mesh.normals.vertex(i);
            glm::vec3 normal = glm::vec3(v3f[0], v3f[1], v3f[2]);
            builder.vertices[i].normal = normal;
            builder.vertices[i].color = (builder.translation * builder.rotation * builder.scale * normal);
        }

        builder.uniqueVertices = uniqueVertices;

        HMesh::build(manifold, mesh);
        builder.manifold = manifold;

        if (SHOW_MESSAGES)
        {
            std::cout << std::endl;
            std::cout << name + " Model" << std::endl;

            if (debug)
            {
                std::cout << "Indices "
                          << "total=" << builder.indices.size() << " faces=" << builder.indices.size() / 3 << " : {";
                for (auto i : builder.indices)
                {
                    std::cout << " " << i;
                }
                std::cout << " }" << std::endl;

                std::cout << "vertices "
                          << "total=" << builder.vertices.size() << " : {";
                for (auto i : builder.vertices)
                {
                    std::cout << " " << i.pos.x << " " << i.pos.y << " " << i.pos.z << " | ";
                }
                std::cout << " }" << std::endl;
            }

            if (HMesh::valid(manifold))
            {
                std::cout << name + " is a Valid Manifold " << std::endl;
            }
            else
            {
                std::cout << name + " is NOT a Valid Manifold! " << std::endl;
            }

            std::cout << name + " Manifold Vertex count: " << manifold.no_vertices() << std::endl;
            std::cout << name + " Manifold Faces count: " << manifold.no_faces() << std::endl;
            std::cout << name + " Manifold Edges count: " << manifold.no_halfedges() << std::endl;
        }

        return builder;
    }

    // ================================================================================================================
    // This is part of the builder code it is the exact same as the loadTinyModel but returns a builder object
    // ================================================================================================================
    void VertexBufferBuilder::loadSerializedModel(std::string name)
    {

        Util::Serialization serialization = Util::Serialization("model", std::ios_base::in);
        manifold.deserialize(serialization);

        // // Get the vertices and corresponding idex buffer from the manifold
        HMesh::IteratorPair<HMesh::IDIterator<HMesh::Face>> faces = manifold.faces();
        std::unordered_map<Vertex, uint32_t> uniqueVertexes{};
        std::vector<Vertex> tempVertices{};
        for (auto f : faces)
        {
            HMesh::Walker w = manifold.walker(f);
            do
            {
                HMesh::VertexID vID = w.vertex();
                HMesh::Manifold::Vec vec = manifold.pos(vID);
                HMesh::Manifold::Vec vecNorm;
                vecNorm = HMesh::normal(manifold, vID);

                Vertex vertex{};
                vertex.pos = glm::vec3(vec[0], vec[1], vec[2]);
                vertex.normal = glm::vec3(vecNorm[0], vecNorm[1], vecNorm[2]);
                // vertex.color = (translation * rotation * scale) * vertex.normal;

                if (uniqueVertexes.count(vertex) == 0)
                {
                    uniqueVertexes[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                tempVertices.push_back(vertex);

                w = w.next(); // Traverse inside the face to the next edge and vertex
            } while (!w.full_circle());
        }

        for (auto v : tempVertices)
        {
            indices.push_back(uniqueVertexes[v]);
        }
    }

    void VertexBufferBuilder::loadObjFile(const std::string &filePath)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str()))
        {
            throw std::runtime_error(warn + err);
        }

        vertices.clear();
        indices.clear();

        std::unordered_map<Vertex, uint32_t> uniqueVertices{}; // Stores the unique vertices and their index

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                Vertex vertex{};
                glm::vec4 normal{};
                if (index.vertex_index >= 0)
                {
                    glm::vec3 pos = glm::vec3(
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]);

                    pos += translation;

                    if (rotationAngle != 0.0f)
                    {
                        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis);
                        pos = glm::vec3(rotationMatrix * glm::vec4(pos, 1.0f));
                    }

                    pos *= scale;

                    vertex.pos = pos;
                }

                // if (attrib.texcoords.size() > 0)
                // {
                // 	vertex.texCoord = {
                // 		attrib.texcoords[2 * index.texcoord_index + 0],
                // 		1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
                // }

                // if (attrib.normals.size() > 0)
                // {
                // 	// vertex.color = {
                // 	//	attrib.normals[3 * index.normal_index + 0],
                // 	//	attrib.normals[3 * index.normal_index + 1],
                // 	//	attrib.normals[3 * index.normal_index + 2],
                // 	// };

                // 	normal = {
                // 		attrib.normals[3 * index.normal_index + 0],
                // 		attrib.normals[3 * index.normal_index + 1],
                // 		attrib.normals[3 * index.normal_index + 2],
                // 		0.0f};
                // }
                // else
                // {
                // 	normal = {1.0f, 1.0f, 1.0f, 0.0f};
                // }

                // // transform pos as a part of the preprocessing
                // vertex.pos = (translation * rotation * scale * pos).xyz();
                // vertex.color = (translation * rotation * scale * normal).xyz();

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void VertexBufferBuilder::serialize(std::string name)
    {
        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
            std::cout << "Serializing Model: " << std::endl;
        }

        std::string path = name + ".bin";

        Util::Serialization ser = Util::Serialization(path, std::ios_base::out);
        manifold.serialize(ser);

        if (SHOW_MESSAGES)
        {
            std::cout << "File Written: " << path << std::endl;

            std::cout
                << "-----------------------------" << std::endl;
        }
    }

    void VertexBufferBuilder::deserialize(std::string name)
    {
        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
            std::cout << "De-Serializing Model: " << std::endl;
        }
        std::string path = name + ".bin";

        Util::Serialization ser = Util::Serialization(path, std::ios_base::in);
        manifold.deserialize(ser);

        if (SHOW_MESSAGES)
        {
            std::cout << "File Read: " << path << std::endl;
        }

        // Get the vertices and corresponding idex buffer from the manifold
        VertexBufferBuilder builder{};
        HMesh::IteratorPair<HMesh::IDIterator<HMesh::Face>> faces = manifold.faces();
        std::unordered_map<Vertex, uint32_t> uniqueVertexes{};
        std::vector<Vertex> tempVertices{};
        for (auto f : faces)
        {
            HMesh::Walker w = manifold.walker(f);
            do
            {
                HMesh::VertexID vID = w.vertex();
                HMesh::Manifold::Vec vec = manifold.pos(vID);
                // HMesh::Manifold::Vec vecNorm;
                // vecNorm = HMesh::normal(manifold, vID);

                Vertex vertex{};
                vertex.pos = glm::vec3(vec[0], vec[1], vec[2]);
                // vertex.normal = glm::vec3(vecNorm[0], vecNorm[1], vecNorm[2]);
                // vertex.color = (builder.translation * builder.rotation * builder.scale * glm::vec4(vertex.normal, 0.0)).xyz();

                if (uniqueVertexes.count(vertex) == 0)
                {
                    uniqueVertexes[vertex] = static_cast<uint32_t>(builder.vertices.size());
                    builder.vertices.push_back(vertex);
                }

                tempVertices.push_back(vertex);

                w = w.next(); // Traverse inside the face to the next edge and vertex
            } while (!w.full_circle());
        }

        for (auto v : tempVertices)
        {
            builder.indices.push_back(uniqueVertexes[v]);
        }

        VertexBufferBuilder simplifiedBuilder = lod::createModelFromVertexIndex(builder.vertices, builder.indices, "Deserialized");

        this->replace(simplifiedBuilder);

        if (SHOW_MESSAGES)
        {
            std::cout << "-----------------------------" << std::endl;
        }
    }

    void VertexBufferBuilder::synchMaps()
    {
        for (auto &[key, value] : uniqueVertices)
        {
            std::cout << "Vertex [" << key.pos.x << key.pos.y << key.pos.z << "] indices: " << value << std::endl;
        }
    }
} // namespace LOD: The Builder Code for the Vertex Index Buffer

namespace lod
{

} // namespace LOD: The code for the Graphs
