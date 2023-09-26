/* Copyright (c) 2017-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//////////////////////////////////////////////////////////////////////////
// Affects both C++ and GLSL, must contain defines only

#ifndef _CONFIG_H_
#define _CONFIG_H_

/////////////////////////////////////////////////////

// keep in sync to nvmeshletbuilder.hpp PACKING configuration
// (values are explained there)
// if set we use a tight or fitted packing, stored in RG32UI texture
// otherwise R32UI
#define NVMESHLET_PACKING_FITTED_UINT8 1
#define NVMESHLET_PRIM_ALIGNMENT 32
#define NVMESHLET_VERTEX_ALIGNMENT 16

#ifdef VULKAN
#define IS_VULKAN 1
#endif

#ifndef IS_VULKAN
#define IS_VULKAN 0
#endif

#if IS_VULKAN
#define NVMESHLET_CLIP_Z_SIGNED 0
#else
#define NVMESHLET_CLIP_Z_SIGNED 1
#endif

///////////////////////////////////////////////////

// must not change
#define WARP_SIZE 32
#define WARP_STEPS 5

///////////////////////////////////////////////////

// set to zero for less resources being generated
// also reduces runtime binding costs
#define USE_PER_GEOMETRY_VIEWS 0

#define VERTEX_POS 0
#define VERTEX_NORMAL 1
#define VERTEX_XTRA 2 // must be NORMAL+1

// GL
#define UBO_SCENE_VIEW 0
#define UBO_OBJECT 1
#define UBO_GEOMETRY 2
#define SSBO_SCENE_STATS 0

// VK
#define DSET_SCENE 0
#define DSET_OBJECT 1
#define DSET_GEOMETRY 2

#define SCENE_UBO_VIEW 0
#define SCENE_SSBO_STATS 1

// changing order requires glsl changes in drawmesh_native.mesh.glsl
// geometryBuffer ubo
#define GEOMETRY_SSBO_MESHLETDESC 0
#define GEOMETRY_SSBO_PRIM 1
#define GEOMETRY_TEX_IBO 2
#define GEOMETRY_TEX_VBO 3
#define GEOMETRY_TEX_ABO 4
#define GEOMETRY_BINDINGS 5

////////////////////////////////////////////////////

#ifndef NVMESHLET_VERTEX_COUNT
// primitive count should be 40, 84 or 126
// vertex count should be 32 or 64
// 64 & 126 is the preferred size
#define NVMESHLET_VERTEX_COUNT 64
#define NVMESHLET_PRIMITIVE_COUNT 126
#endif

#ifndef EXTRA_ATTRIBUTES
// add how many extra fake attributes (vec4) you want to use
#define EXTRA_ATTRIBUTES 4
#endif

#ifndef USE_CLIPPING
#define USE_CLIPPING 0
#endif

#define NUM_CLIPPING_PLANES 3

#define NORMAL_STRIDE (1 + EXTRA_ATTRIBUTES)

#ifndef STATS
#define STATS 0
#endif

#ifndef BENCHMARK
#define BENCHMARK 0
#endif

#ifndef CONTINUOUSROTATION
#define CONTINUOUSROTATION 0
#endif

#endif
