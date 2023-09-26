#pragma once
#ifndef HEADERGUARD_VKEXTENSIONS
#define HEADERGUARD_VKEXTENSIONS


#define LOADER_VK_NV_mesh_shader 1

static PFN_vkCmdDrawMeshTasksNV pfn_vkCmdDrawMeshTasksNV = 0;
static PFN_vkCmdDrawMeshTasksIndirectNV pfn_vkCmdDrawMeshTasksIndirectNV = 0;
static PFN_vkCmdDrawMeshTasksIndirectCountNV pfn_vkCmdDrawMeshTasksIndirectCountNV = 0;

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksNV(
	VkCommandBuffer commandBuffer,
	uint32_t taskCount,
	uint32_t firstTask)
{
	assert(pfn_vkCmdDrawMeshTasksNV);
	pfn_vkCmdDrawMeshTasksNV(commandBuffer, taskCount, firstTask);
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectNV(
	VkCommandBuffer commandBuffer,
	VkBuffer buffer,
	VkDeviceSize offset,
	uint32_t drawCount,
	uint32_t stride)
{
	assert(pfn_vkCmdDrawMeshTasksIndirectNV);
	pfn_vkCmdDrawMeshTasksIndirectNV(commandBuffer, buffer, offset, drawCount, stride);
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectCountNV(
	VkCommandBuffer commandBuffer,
	VkBuffer buffer,
	VkDeviceSize offset,
	VkBuffer countBuffer,
	VkDeviceSize countBufferOffset,
	uint32_t maxDrawCount,
	uint32_t stride)
{
	assert(pfn_vkCmdDrawMeshTasksIndirectCountNV);
	pfn_vkCmdDrawMeshTasksIndirectCountNV(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

int has_VK_NV_mesh_shader = 0;
int load_VK_NV_mesh_shader(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr)
{
	pfn_vkCmdDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV)getDeviceProcAddr(device, "vkCmdDrawMeshTasksNV");

	pfn_vkCmdDrawMeshTasksIndirectNV = (PFN_vkCmdDrawMeshTasksIndirectNV)getDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectNV");
	pfn_vkCmdDrawMeshTasksIndirectCountNV = (PFN_vkCmdDrawMeshTasksIndirectCountNV)getDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectCountNV");
	int success = 1;
	success = success && (pfn_vkCmdDrawMeshTasksNV != 0);
	success = success && (pfn_vkCmdDrawMeshTasksIndirectNV != 0);
	success = success && (pfn_vkCmdDrawMeshTasksIndirectCountNV != 0);
	has_VK_NV_mesh_shader = success;
	return success;
}

#endif // HEADERGUARD_VKEXTENSIONS