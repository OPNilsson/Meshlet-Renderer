/* Copyright (c) 2016-2018, NVIDIA CORPORATION. All rights reserved.
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

#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_NV_fragment_shader_barycentric: enable

#include "../common.h"

layout(location = 0, index = 0) out vec4 out_Color;

vec3 ligtColor = vec3(0.75);
float Ka = 0.1;

layout(location = 0) in PerVertexData {
  vec3 inNormal;
  vec3 inColor;
  vec3 inLightVec;
  vec3 inViewVec;
  flat uint meshletID;
} fragIn;

void main() {
  const vec3 bcCord = gl_BaryCoordNV;
  const vec3 pdbcX = dFdx(bcCord);
  const vec3 pdbcY = dFdy(bcCord);

  const vec3 eucDist = sqrt(pdbcX * pdbcX + pdbcY * pdbcY);

  const float edgeThickness = 0.0125f;
  const float fallOff = 0.025f;

  const vec3 remap = smoothstep(eucDist * edgeThickness, eucDist * (edgeThickness + fallOff), bcCord);

  const float closestColor = min(remap.x, min(remap.y, remap.z));

    //out_Color = vec4(closestColor.xxx, 1.0f);

  uint cp = murmurHash(fragIn.meshletID);
  out_Color = unpackUnorm4x8(cp);
  out_Color *= vec4(closestColor.xxx, 1.0f);

    /* Simple and fast wireframe
    const vec3 barycoord = gl_BaryCoordEXT; // ignore error in ide because it is avalaible via ext
    const float closestColor = min(barycoord.x, min(barycoord.y, barycoord.z));
    const float wireframe = smoothstep(0.0, 0.01, closestColor);
    if (wireframe == 0.0f){
        out_Color = vec4(wireframe.xxx, 1.0f);
    } else {
      uint cp = murmurHash(fragIn.meshletID);
      out_Color = unpackUnorm4x8(cp);  
    }
    */
}