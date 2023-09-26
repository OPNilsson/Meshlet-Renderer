%VK_SDK_PATH%/Bin/glslc.exe meshlet_phong.frag -o meshletFrag.spv


%VK_SDK_PATH%/Bin/glslc.exe meshlet_phong.frag -o meshletFrag.spv
%VK_SDK_PATH%/Bin/glslc.exe meshlet_normal.frag -o meshletNormalFrag.spv
%VK_SDK_PATH%/Bin/glslc.exe meshlet_meshlet_color.frag -o meshletMeshletColorFrag.spv
%VK_SDK_PATH%/Bin/glslc.exe meshlet_meshlet_wire_color.frag -o meshletMeshletColorWireFrag.spv

%VK_SDK_PATH%/Bin/glslc.exe --target-env=vulkan1.3 --target-spv=spv1.3 meshlet.task -o meshletTask.spv
%VK_SDK_PATH%/Bin/glslc.exe --target-env=vulkan1.3 --target-spv=spv1.3 meshlet.mesh -o meshletMesh.spv

%VK_SDK_PATH%/Bin/glslc.exe --target-env=vulkan1.3 --target-spv=spv1.3 meshletTexture.mesh -o meshletTextureMesh.spv
%VK_SDK_PATH%/Bin/glslc.exe meshletTexture.frag -o meshletTextureFrag.spv