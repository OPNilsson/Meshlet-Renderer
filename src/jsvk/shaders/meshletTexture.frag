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

#include "../common.h"

layout(location = 0, index = 0) out vec4 out_Color;

vec3 ligtColor = vec3(0.75);
float Ka = 0.1;

// figure out where this goes
layout(set = 3, binding = 0) uniform sampler2D texSampler;

//layout(location = 0) in flat uint meshletID;

layout(location = 0) in PerVertexData
{
    vec3 inNormal;
    vec3 inColor;
    vec3 inLightVec;
    vec3 inViewVec;
    vec2 inTexCoord;
} fragIn;

void main() {
    vec3 N = normalize(fragIn.inNormal);
    vec3 L = normalize(fragIn.inLightVec);
    vec3 V = normalize(fragIn.inViewVec);
    vec3 R = reflect(-L, N);

    vec3 ambient = fragIn.inColor * Ka; // strength of ambient
    vec3 diffuse = max(dot(N, L), 0.0) * texture(texSampler, fragIn.inTexCoord).xyz;

    vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * ligtColor;

    out_Color = vec4(ambient + diffuse + specular, 1.0);

}