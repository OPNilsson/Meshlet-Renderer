#pragma once
#ifndef HEADER_GUARD_GEOMETRYPROCESSING
#define HEADER_GUARD_GEOMETRYPROCESSING

#define GLM_FORCE_SWIZZLE

#include <string>
#include <vector>

#include "meshlet_builder.hpp"
#include "structures.h"

// #include <headers/openvr.h>

std::vector<NVMeshlet::Builder<uint16_t>::MeshletGeometry>
ConvertToMeshlet(std::vector<Vertex> *vertices, std::vector<uint16_t> *indices, std::vector<NVMeshlet::Stats> *stats);
// void loadVRObjAsMeshlet(std::vector<NVMeshlet::Builder<uint16_t>::MeshletGeometry>* meshletGeometry, std::vector<vr::RenderModel_t*> renderModels, std::vector<uint32_t>* vertCount, std::vector<Vertex>* vertices, std::vector<ObjectData>* objectData, std::vector<NVMeshlet::Stats>* stats);
void loadObjAsMeshlet(const std::string &modelPath, std::vector<NVMeshlet::Builder<uint32_t>::MeshletGeometry> *meshletGeometry, std::vector<uint32_t> *vertCount, std::vector<Vertex> *vertices, std::vector<ObjectData> *objectData, std::vector<NVMeshlet::Stats> *stats);
void loadTinyModel(const std::string &path, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices);
std::vector<NVMeshlet::Builder<uint32_t>::MeshletGeometry> ConvertToMeshlet(std::vector<Vertex> *vertices, std::vector<uint32_t> *indices, std::vector<NVMeshlet::Stats> *stats);

namespace mm
{
    struct AdjecencyInfo
    {
        std::vector<uint32_t> trianglesPerVertex;
        std::vector<uint32_t> indexBufferOffset;
        std::vector<uint32_t> triangleData;
    };
    void tipsifyIndexBuffer(const uint32_t *indicies, const uint32_t numIndices, const uint32_t numVerts, const int cacheSize, std::vector<uint32_t> &optimizedIdxBuffer);
    void buildAdjacency(const uint32_t numVerts, const uint32_t numIndices, const uint32_t *indices, AdjecencyInfo &info);
    void collectStats(const NVMeshlet::Builder<uint32_t>::MeshletGeometry &geometry, std::vector<NVMeshlet::Stats> &stats);
    void generateEarlyCulling(NVMeshlet::Builder<uint32_t>::MeshletGeometry &geometry, const std::vector<Vertex> &vertices, std::vector<ObjectData> &objectData);
    NVMeshlet::Builder<uint32_t>::MeshletGeometry packNVMeshlets(const std::vector<mm::MeshletCache<uint32_t>> &meshlets);
    void loadTinyModel(const std::string &path, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices);
    void generateMeshlets(std::unordered_map<unsigned int, Vert *> &indexVertexMap, std::vector<Triangle *> &triangles, std::vector<MeshletCache<uint32_t>> &mehslets, const Vertex *vertices, int strat = -1, uint32_t primitiveLimit = 125, uint32_t vertexLimit = 64);
    void makeMesh(std::unordered_map<unsigned int, Vert *> *indexVertexMap, std::vector<Triangle *> *triangles, const uint32_t numIndices, const uint32_t *indices);
}
#endif // HEADER_GUARD_GEOMETRYPROCESSING